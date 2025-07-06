#pragma once

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <chrono>

// Include backtrader headers
#include "lineroot.h"
#include "lineseries.h"
#include "strategy.h"
#include "dataseries.h"
#include "feed.h"
#include "timeframe.h"
#include "order.h"
#include "trade.h"
#include "indicators/crossover.h"
#include "broker.h"

// Add using declarations for common types
using Order = backtrader::Order;
using Trade = backtrader::Trade;
using DataReplay = backtrader::DataReplay;
using DataResample = backtrader::DataResample;
using TimeFrame = backtrader::TimeFrame;
using OrderStatus = backtrader::OrderStatus;
using OrderType = backtrader::OrderType;

// Add indicators namespace
namespace backtrader {
namespace indicators {
    using CrossOver = backtrader::CrossOver;
    // SMA is already defined in backtrader::indicators namespace
}
}

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
 * @brief Create a data feed from CSV data for use with tests
 */
class TestDataFeed : public backtrader::CSVDataBase {
private:
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    size_t current_index_;
    
public:
    TestDataFeed(const std::vector<CSVDataReader::OHLCVData>& data) 
        : csv_data_(data), current_index_(0) {
        // Set up basic parameters
        params.dataname = "test_data";
        params.name = "TestData";
        
        // Initialize lines for OHLCV data
        lines = std::make_shared<backtrader::Lines>();
        for (int i = 0; i < 7; ++i) {
            lines->add_line(std::make_shared<backtrader::LineBuffer>());
        }
    }
    
protected:
    bool _loadline(const std::vector<std::string>& linetokens) override {
        // This won't be called since we override _load directly
        return false;
    }
    
    bool _load() override {
        if (current_index_ >= csv_data_.size()) {
            return false; // End of data
        }
        
        const auto& bar = csv_data_[current_index_++];
        
        // Set the line values (OHLCV + datetime + openinterest)
        if (lines->size() > backtrader::DataSeries::DateTime) {
            lines->getline(backtrader::DataSeries::DateTime)->set(0, current_index_); // Simple numeric date
        }
        if (lines->size() > backtrader::DataSeries::Open) {
            lines->getline(backtrader::DataSeries::Open)->set(0, bar.open);
        }
        if (lines->size() > backtrader::DataSeries::High) {
            lines->getline(backtrader::DataSeries::High)->set(0, bar.high);
        }
        if (lines->size() > backtrader::DataSeries::Low) {
            lines->getline(backtrader::DataSeries::Low)->set(0, bar.low);
        }
        if (lines->size() > backtrader::DataSeries::Close) {
            lines->getline(backtrader::DataSeries::Close)->set(0, bar.close);
        }
        if (lines->size() > backtrader::DataSeries::Volume) {
            lines->getline(backtrader::DataSeries::Volume)->set(0, bar.volume);
        }
        if (lines->size() > backtrader::DataSeries::OpenInterest) {
            lines->getline(backtrader::DataSeries::OpenInterest)->set(0, bar.openinterest);
        }
        
        return true;
    }
};

/**
 * @brief Create a shared_ptr to AbstractDataBase from CSV data
 */
inline std::shared_ptr<backtrader::AbstractDataBase> getdata_feed(int index = 0) {
    auto csv_data = getdata(index);
    return std::make_shared<TestDataFeed>(csv_data);
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
    
    // Add missing methods for Strategy compatibility
    size_t len() const {
        if (!datas.empty() && datas[0]) {
            return datas[0]->size();
        }
        return 0;
    }
    
    std::shared_ptr<LineSeries> data(int idx) {
        if (idx < datas.size()) {
            return datas[idx];
        }
        return nullptr;
    }
    
    void addIndicator(std::shared_ptr<LineIterator> indicator) {
        addindicator(indicator);
    }
    
    void init() override {
        auto dataPtr = this->data(0);
        if (dataPtr) {
            // Get the close price line from the data series
            auto closeLine = dataPtr->getline(DataSeries::Close);
            if (closeLine) {
                indicator_ = std::make_shared<IndicatorType>(std::dynamic_pointer_cast<LineRoot>(closeLine));
                addIndicator(indicator_);
            }
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
        int l = indicator_->size();
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

/**
 * @brief Utility function to check if a value is NaN
 */
inline bool isNaN(double value) {
    return std::isnan(value);
}

/**
 * @brief Convert numeric date to string representation
 * @param datetime Numeric datetime value
 * @return String representation of the date
 */
inline std::string num2date(double datetime) {
    // Simple conversion for now - in a full implementation this would
    // convert from numeric format to human-readable date string
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << datetime;
    return oss.str();
}

/**
 * @brief Convert chrono time_point to double for compatibility
 * @param tp time_point to convert
 * @return double representation of time
 */
inline double timepoint_to_double(const std::chrono::system_clock::time_point& tp) {
    auto duration = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return static_cast<double>(seconds.count());
}

} // namespace original
} // namespace tests
} // namespace backtrader