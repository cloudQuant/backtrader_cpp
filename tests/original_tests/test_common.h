#pragma once

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

// Include backtrader headers
#include "lineroot.h"
#include "lineseries.h"
#include "strategy.h"

namespace backtrader {
namespace tests {
namespace original {

/**
 * @brief CSV数据读取器，兼容原始Python测试数据格式
 */
class CSVDataReader {
public:
    struct OHLCVData {
        std::string date;
        double open;
        double high;
        double low;
        double close;
        double volume;
        double openinterest;
    };
    
    static std::vector<OHLCVData> loadCSV(const std::string& filename) {
        std::vector<OHLCVData> data;
        std::ifstream file(filename);
        std::string line;
        
        // 跳过标题行
        if (std::getline(file, line)) {
            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string item;
                OHLCVData bar;
                
                // Date
                if (std::getline(ss, item, ',')) {
                    bar.date = item;
                }
                // Open
                if (std::getline(ss, item, ',')) {
                    bar.open = std::stod(item);
                }
                // High
                if (std::getline(ss, item, ',')) {
                    bar.high = std::stod(item);
                }
                // Low
                if (std::getline(ss, item, ',')) {
                    bar.low = std::stod(item);
                }
                // Close
                if (std::getline(ss, item, ',')) {
                    bar.close = std::stod(item);
                }
                // Volume
                if (std::getline(ss, item, ',')) {
                    bar.volume = std::stod(item);
                }
                // OpenInterest
                if (std::getline(ss, item, ',')) {
                    bar.openinterest = std::stod(item);
                }
                
                data.push_back(bar);
            }
        }
        
        return data;
    }
};

/**
 * @brief 测试数据获取函数，对应Python的getdata()
 */
inline std::vector<CSVDataReader::OHLCVData> getdata(int index = 0) {
    // 原始测试使用的数据文件
    std::vector<std::string> datafiles = {
        "2006-day-001.txt",
        "2006-week-001.txt"
    };
    
    std::string filepath = "../../../tests/datas/" + datafiles[index];
    return CSVDataReader::loadCSV(filepath);
}

/**
 * @brief 测试策略基类，对应Python的TestStrategy
 */
template<typename IndicatorType>
class TestStrategy : public Strategy {
private:
    std::shared_ptr<IndicatorType> indicator_;
    std::vector<std::vector<std::string>> expected_values_;
    int expected_min_period_;
    int actual_min_period_;
    int next_calls_;
    bool main_debug_;
    
public:
    TestStrategy(const std::vector<std::vector<std::string>>& expected_vals,
                 int expected_min,
                 bool main = false)
        : Strategy(),
          expected_values_(expected_vals),
          expected_min_period_(expected_min),
          actual_min_period_(0),
          next_calls_(0),
          main_debug_(main) {}
    
    void init() override {
        auto data = this->data(0);
        if (data && data->close()) {
            indicator_ = std::make_shared<IndicatorType>(data->close());
            addIndicator(indicator_);
        }
    }
    
    void nextstart() override {
        actual_min_period_ = len();
        Strategy::nextstart();
    }
    
    void next() override {
        next_calls_++;
        
        if (main_debug_) {
            // Debug output similar to Python version
            std::cout << "Length: " << len() << ", Indicator value: " << indicator_->get(0) << std::endl;
        }
    }
    
    void start() override {
        next_calls_ = 0;
    }
    
    void stop() override {
        validateResults();
    }
    
private:
    void validateResults() {
        int l = indicator_->len();
        int mp = actual_min_period_;
        
        // 计算检查点，对应Python的chkpts
        std::vector<int> chkpts = {0, -l + mp, (-l + mp) / 2};
        
        if (main_debug_) {
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "len ind " << l << " == " << len() << " len self" << std::endl;
            std::cout << "minperiod " << actual_min_period_ << std::endl;
            std::cout << "expected minperiod " << expected_min_period_ << std::endl;
            std::cout << "nextcalls " << next_calls_ << std::endl;
            
            std::cout << "chkpts are ";
            for (int chkpt : chkpts) {
                std::cout << chkpt << " ";
            }
            std::cout << std::endl;
            
            // 输出实际值
            for (size_t lidx = 0; lidx < indicator_->size(); ++lidx) {
                std::cout << "    [";
                for (size_t i = 0; i < chkpts.size(); ++i) {
                    double val = indicator_->getLine(lidx)->get(chkpts[i]);
                    std::cout << "'" << std::fixed << std::setprecision(6) << val << "'";
                    if (i < chkpts.size() - 1) std::cout << ", ";
                }
                std::cout << "]," << std::endl;
            }
            
            std::cout << "vs expected" << std::endl;
            for (const auto& chkval : expected_values_) {
                std::cout << "    [";
                for (size_t i = 0; i < chkval.size(); ++i) {
                    std::cout << "'" << chkval[i] << "'";
                    if (i < chkval.size() - 1) std::cout << ", ";
                }
                std::cout << "]," << std::endl;
            }
        } else {
            // 执行断言验证
            ASSERT_EQ(l, len()) << "Indicator length should match strategy length";
            ASSERT_EQ(actual_min_period_, expected_min_period_) << "Minimum period mismatch";
            
            // 验证指标值
            for (size_t lidx = 0; lidx < expected_values_.size() && lidx < indicator_->size(); ++lidx) {
                const auto& line_vals = expected_values_[lidx];
                for (size_t i = 0; i < chkpts.size() && i < line_vals.size(); ++i) {
                    double actual_val = indicator_->getLine(lidx)->get(chkpts[i]);
                    std::string actual_str = formatValue(actual_val);
                    
                    // 处理NaN值的特殊情况
                    if (line_vals[i] == "nan" || line_vals[i] == "'nan'") {
                        EXPECT_TRUE(std::isnan(actual_val)) 
                            << "Expected NaN at line " << lidx << ", point " << i;
                    } else {
                        EXPECT_EQ(actual_str, line_vals[i]) 
                            << "Value mismatch at line " << lidx << ", point " << i 
                            << " (actual: " << actual_str << ", expected: " << line_vals[i] << ")";
                    }
                }
            }
        }
    }
    
    std::string formatValue(double value) const {
        if (std::isnan(value)) {
            return "nan";
        }
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << value;
        return ss.str();
    }
};

/**
 * @brief 执行测试的主函数，对应Python的runtest()
 */
template<typename IndicatorType>
void runtest(const std::vector<std::vector<std::string>>& expected_vals,
             int expected_min_period,
             bool main = false,
             int data_index = 0) {
    
    // 加载测试数据
    auto csv_data = getdata(data_index);
    ASSERT_FALSE(csv_data.empty()) << "Failed to load test data";
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建测试策略
    auto strategy = std::make_shared<TestStrategy<IndicatorType>>(
        expected_vals, expected_min_period, main);
    
    // 这里需要实现一个简化版的回测执行
    // 由于我们主要关注指标计算的正确性，可以直接模拟策略执行
    
    // 模拟策略初始化
    // strategy->setData(data_feed); // 需要实现数据设置
    strategy->init();
    
    // 模拟策略执行
    strategy->start();
    
    // 逐步执行策略
    for (size_t i = 0; i < csv_data.size(); ++i) {
        if (i == 0) {
            strategy->nextstart();
        } else {
            strategy->next();
        }
        close_line->forward();
    }
    
    strategy->stop();
}

/**
 * @brief 测试宏，简化测试用例定义
 */
#define DEFINE_INDICATOR_TEST(TestName, IndicatorClass, ExpectedVals, MinPeriod) \
    TEST(OriginalTests, TestName) { \
        std::vector<std::vector<std::string>> expected_vals = ExpectedVals; \
        runtest<IndicatorClass>(expected_vals, MinPeriod, false); \
    } \
    \
    TEST(OriginalTests, TestName##_Debug) { \
        std::vector<std::vector<std::string>> expected_vals = ExpectedVals; \
        runtest<IndicatorClass>(expected_vals, MinPeriod, true); \
    }

} // namespace original
} // namespace tests
} // namespace backtrader