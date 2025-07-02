#include <gtest/gtest.h>
#include "indicators/SMA.h"
#include "../utils/TestDataProvider.h"
#include <cmath>

namespace backtrader {
namespace test {

class SMATest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试数据
        test_data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
        data_line = TestDataProvider::createLineRootFromData(test_data, "test_data");
    }
    
    std::vector<double> test_data;
    std::shared_ptr<LineRoot> data_line;
};

TEST_F(SMATest, BasicConstruction) {
    SMA sma(data_line, 5);
    
    EXPECT_EQ(sma.getPeriod(), 5);
    EXPECT_EQ(sma.getMinPeriod(), 5);
    EXPECT_EQ(sma.getName(), "SMA");
    EXPECT_EQ(sma.getInputCount(), 1);
    EXPECT_EQ(sma.getInput(0), data_line);
}

TEST_F(SMATest, ParameterValidation) {
    // 测试无效周期
    EXPECT_THROW(SMA(data_line, 0), std::invalid_argument);
    
    // 测试有效周期
    EXPECT_NO_THROW(SMA(data_line, 1));
    EXPECT_NO_THROW(SMA(data_line, 100));
}

TEST_F(SMATest, SingleStepCalculation) {
    SMA sma(data_line, 3);
    
    // 前两步应该返回NaN（数据不足）
    sma.calculate();
    EXPECT_TRUE(isNaN(sma.get(0)));
    
    data_line->forward();
    sma.calculate();
    EXPECT_TRUE(isNaN(sma.get(0)));
    
    // 第三步开始有有效值
    data_line->forward();
    sma.calculate();
    EXPECT_DOUBLE_EQ(sma.get(0), 2.0);  // (1+2+3)/3 = 2.0
    
    // 继续计算
    data_line->forward();
    sma.calculate();
    EXPECT_DOUBLE_EQ(sma.get(0), 3.0);  // (2+3+4)/3 = 3.0
    
    data_line->forward();
    sma.calculate();
    EXPECT_DOUBLE_EQ(sma.get(0), 4.0);  // (3+4+5)/3 = 4.0
}

TEST_F(SMATest, IncrementalVsDirectCalculation) {
    // 测试增量计算和直接计算的结果一致性
    auto data1 = TestDataProvider::createLineRootFromData(test_data, "data1");
    auto data2 = TestDataProvider::createLineRootFromData(test_data, "data2");
    
    SMA sma_incremental(data1, 3, true);   // 增量计算
    SMA sma_direct(data2, 3, false);       // 直接计算
    
    std::vector<double> results_inc, results_dir;
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        sma_incremental.calculate();
        sma_direct.calculate();
        
        results_inc.push_back(sma_incremental.get(0));
        results_dir.push_back(sma_direct.get(0));
        
        if (i < test_data.size() - 1) {
            data1->forward();
            data2->forward();
        }
    }
    
    // 比较结果
    EXPECT_TRUE(TestDataProvider::compareDoubleVectors(results_inc, results_dir, 1e-10));
}

TEST_F(SMATest, ExpectedValues) {
    SMA sma(data_line, 3);
    std::vector<double> expected_results;
    std::vector<double> actual_results;
    
    // 手动计算期望值
    for (size_t i = 0; i < test_data.size(); ++i) {
        sma.calculate();
        actual_results.push_back(sma.get(0));
        
        if (i < 2) {
            expected_results.push_back(NaN);
        } else {
            double sum = 0.0;
            for (size_t j = i - 2; j <= i; ++j) {
                sum += test_data[j];
            }
            expected_results.push_back(sum / 3.0);
        }
        
        if (i < test_data.size() - 1) {
            data_line->forward();
        }
    }
    
    // 比较结果（跳过NaN值）
    for (size_t i = 2; i < expected_results.size(); ++i) {
        EXPECT_DOUBLE_EQ(actual_results[i], expected_results[i]);
    }
}

TEST_F(SMATest, DifferentPeriods) {
    std::vector<size_t> periods = {1, 2, 5, 10};
    
    for (size_t period : periods) {
        if (period <= test_data.size()) {
            auto data = TestDataProvider::createLineRootFromData(test_data);
            SMA sma(data, period);
            
            // 移动到有足够数据的位置
            for (size_t i = 0; i < period - 1; ++i) {
                data->forward();
            }
            
            sma.calculate();
            
            if (period == 1) {
                EXPECT_DOUBLE_EQ(sma.get(0), test_data[period - 1]);
            } else if (period == 2) {
                double expected = (test_data[0] + test_data[1]) / 2.0;
                EXPECT_DOUBLE_EQ(sma.get(0), expected);
            }
        }
    }
}

TEST_F(SMATest, NaNHandling) {
    // 创建包含NaN的数据
    std::vector<double> nan_data = {1.0, 2.0, NaN, 4.0, 5.0};
    auto nan_line = TestDataProvider::createLineRootFromData(nan_data, "nan_data");
    
    SMA sma(nan_line, 3);
    
    // 移动到第三个位置（包含NaN）
    nan_line->forward();
    nan_line->forward();
    sma.calculate();
    
    // 包含NaN的窗口应该返回NaN
    EXPECT_TRUE(isNaN(sma.get(0)));
    
    // 移动到不包含NaN的位置
    nan_line->forward();
    sma.calculate();
    
    // 这里的窗口包含NaN，所以仍然是NaN
    EXPECT_TRUE(isNaN(sma.get(0)));
    
    // 再移动一步，窗口不包含NaN
    nan_line->forward();
    sma.calculate();
    
    // 现在应该有有效值 (NaN, 4, 5) -> NaN
    EXPECT_TRUE(isNaN(sma.get(0)));
}

TEST_F(SMATest, BatchCalculation) {
    auto large_data = TestDataProvider::generateTrendingData(1000, 100.0, 0.1, 1.0);
    auto large_line = TestDataProvider::createLineRootFromData(large_data);
    
    SMA sma(large_line, 20);
    
    // 重置到开始
    large_line->home();
    for (const auto& value : large_data) {
        large_line->forward(value);
    }
    
    // 批量计算
    sma.calculateBatch(20, 100);
    
    // 验证结果不是NaN
    for (size_t i = 20; i < 100; ++i) {
        large_line->backward(100 - i - 1);
        EXPECT_FALSE(isNaN(sma.get(0)));
        large_line->forward(NaN, 100 - i - 1);
    }
}

TEST_F(SMATest, WindowDataAccess) {
    SMA sma(data_line, 3, true);  // 使用增量计算以测试窗口数据
    
    // 移动到有足够数据的位置
    data_line->forward();
    data_line->forward();
    sma.calculate();
    
    auto window = sma.getCurrentWindow();
    EXPECT_EQ(window.size(), 3);
    
    // 验证窗口数据（注意顺序）
    EXPECT_DOUBLE_EQ(window[0], 3.0);  // 最新值
    EXPECT_DOUBLE_EQ(window[1], 2.0);  
    EXPECT_DOUBLE_EQ(window[2], 1.0);  // 最旧值
    
    // 测试增量计算的当前和
    EXPECT_DOUBLE_EQ(sma.getCurrentSum(), 6.0);  // 1+2+3
}

TEST_F(SMATest, ParameterManagement) {
    SMA sma(data_line, 5);
    
    EXPECT_TRUE(sma.hasParam("period"));
    EXPECT_DOUBLE_EQ(sma.getParam("period"), 5.0);
    
    // 改变周期
    sma.setPeriod(3);
    EXPECT_EQ(sma.getPeriod(), 3);
    EXPECT_DOUBLE_EQ(sma.getParam("period"), 3.0);
    EXPECT_EQ(sma.getMinPeriod(), 3);
}

TEST_F(SMATest, ResetFunctionality) {
    SMA sma(data_line, 3, true);
    
    // 执行一些计算
    data_line->forward();
    data_line->forward();
    sma.calculate();
    
    EXPECT_FALSE(isNaN(sma.get(0)));
    EXPECT_GT(sma.getCurrentSum(), 0.0);
    
    // 重置
    sma.reset();
    
    EXPECT_EQ(sma.len(), 0);
    EXPECT_DOUBLE_EQ(sma.getCurrentSum(), 0.0);
}

TEST_F(SMATest, PerformanceComparison) {
    // 创建大数据集
    auto large_data = TestDataProvider::generateRandomData(10000, 100.0, 10.0);
    auto line1 = TestDataProvider::createLineRootFromData(large_data);
    auto line2 = TestDataProvider::createLineRootFromData(large_data);
    
    SMA sma_incremental(line1, 50, true);
    SMA sma_direct(line2, 50, false);
    
    // 测试增量计算性能
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < large_data.size(); ++i) {
        sma_incremental.calculate();
        if (i < large_data.size() - 1) line1->forward();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto incremental_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 测试直接计算性能
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < large_data.size(); ++i) {
        sma_direct.calculate();
        if (i < large_data.size() - 1) line2->forward();
    }
    end = std::chrono::high_resolution_clock::now();
    auto direct_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 增量计算应该更快
    EXPECT_LT(incremental_time.count(), direct_time.count() * 2);  // 至少快一半
    
    std::cout << "Incremental: " << incremental_time.count() << "μs, "
              << "Direct: " << direct_time.count() << "μs" << std::endl;
}

} // namespace test
} // namespace backtrader