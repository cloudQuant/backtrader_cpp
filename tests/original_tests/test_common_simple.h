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
#include "LineRoot.h"
#include "Common.h"

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
        
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return data;
        }
        
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
    
    std::string filepath = "../../datas/" + datafiles[index];
    return CSVDataReader::loadCSV(filepath);
}

/**
 * @brief 简化版本的测试函数，直接测试指标
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
    
    // 创建指标
    auto indicator = std::make_shared<IndicatorType>(close_line);
    
    // 逐步添加数据并计算指标（模拟Python的逐步处理）
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        indicator->calculate();
    }
    
    // 验证最小周期
    ASSERT_EQ(indicator->getMinPeriod(), expected_min_period) << "Minimum period mismatch";
    
    // 计算检查点，对应Python的chkpts
    int l = static_cast<int>(csv_data.size());
    int mp = expected_min_period;
    // Python uses floor division (//) which for negative numbers gives different results than C++ /
    // Python: (-225) // 2 = -113, C++: (-225) / 2 = -112.5 -> -112
    // We need to use floor division for negative numbers to match Python behavior
    int middle_checkpoint = static_cast<int>(std::floor(static_cast<double>(-(l - mp)) / 2.0));
    std::vector<int> chkpts = {0, -(l - mp), middle_checkpoint};
    
    if (main) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Data length: " << l << std::endl;
        std::cout << "Minimum period: " << mp << std::endl;
        std::cout << "Check points: ";
        for (int chkpt : chkpts) {
            std::cout << chkpt << " ";
        }
        std::cout << std::endl;
        
        // 输出实际值
        std::cout << "Actual values:" << std::endl;
        for (size_t i = 0; i < chkpts.size(); ++i) {
            double val = indicator->get(chkpts[i]);
            std::cout << "  [" << i << "] = " << std::fixed << std::setprecision(6) << val << std::endl;
        }
        
        std::cout << "Expected values:" << std::endl;
        if (!expected_vals.empty()) {
            for (size_t i = 0; i < expected_vals[0].size() && i < chkpts.size(); ++i) {
                std::cout << "  [" << i << "] = " << expected_vals[0][i] << std::endl;
            }
        }
    } else {
        // 验证指标值
        if (!expected_vals.empty()) {
            const auto& line_vals = expected_vals[0];
            for (size_t i = 0; i < chkpts.size() && i < line_vals.size(); ++i) {
                double actual_val = indicator->get(chkpts[i]);
                
                // 处理NaN值的特殊情况
                if (line_vals[i] == "nan" || line_vals[i] == "'nan'") {
                    EXPECT_TRUE(std::isnan(actual_val)) 
                        << "Expected NaN at point " << i;
                } else {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(6) << actual_val;
                    std::string actual_str = ss.str();
                    
                    EXPECT_EQ(actual_str, line_vals[i]) 
                        << "Value mismatch at point " << i 
                        << " (actual: " << actual_str << ", expected: " << line_vals[i] << ")";
                }
            }
        }
    }
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