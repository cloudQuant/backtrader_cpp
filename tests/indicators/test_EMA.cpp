#include <gtest/gtest.h>
#include "indicators/EMA.h"
#include "../utils/TestDataProvider.h"
#include <cmath>

namespace backtrader {
namespace test {

class EMATest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试数据
        test_data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
        data_line = TestDataProvider::createLineRootFromData(test_data, "test_data");
    }
    
    std::vector<double> test_data;
    std::shared_ptr<LineRoot> data_line;
};

TEST_F(EMATest, BasicConstruction) {
    EMA ema(data_line, 5);
    
    EXPECT_EQ(ema.getPeriod(), 5);
    EXPECT_EQ(ema.getMinPeriod(), 1);
    EXPECT_EQ(ema.getName(), "EMA");
    EXPECT_EQ(ema.getInputCount(), 1);
    EXPECT_EQ(ema.getInput(0), data_line);
    
    // 检查alpha值
    double expected_alpha = 2.0 / (5 + 1);
    EXPECT_DOUBLE_EQ(ema.getAlpha(), expected_alpha);
}

TEST_F(EMATest, ParameterValidation) {
    // 测试无效周期
    EXPECT_THROW(EMA(data_line, 0), std::invalid_argument);
    
    // 测试有效周期
    EXPECT_NO_THROW(EMA(data_line, 1));
    EXPECT_NO_THROW(EMA(data_line, 100));
}

TEST_F(EMATest, AlphaCalculation) {
    EMA ema5(data_line, 5);
    EMA ema10(data_line, 10);
    EMA ema20(data_line, 20);
    
    EXPECT_DOUBLE_EQ(ema5.getAlpha(), 2.0 / 6.0);
    EXPECT_DOUBLE_EQ(ema10.getAlpha(), 2.0 / 11.0);
    EXPECT_DOUBLE_EQ(ema20.getAlpha(), 2.0 / 21.0);
}

TEST_F(EMATest, FirstValueHandling) {
    EMA ema(data_line, 5);
    
    // 第一个值应该等于输入值
    ema.calculate();
    EXPECT_DOUBLE_EQ(ema.get(0), 1.0);
    EXPECT_TRUE(ema.hasPreviousValue());
    EXPECT_DOUBLE_EQ(ema.getPreviousEMA(), 1.0);
}

TEST_F(EMATest, SingleStepCalculation) {
    EMA ema(data_line, 3);  // alpha = 2/4 = 0.5
    
    // 第一步：EMA = 输入值
    ema.calculate();
    EXPECT_DOUBLE_EQ(ema.get(0), 1.0);
    
    // 第二步：EMA = 0.5 * 2 + 0.5 * 1 = 1.5
    data_line->forward();
    ema.calculate();
    EXPECT_DOUBLE_EQ(ema.get(0), 1.5);
    
    // 第三步：EMA = 0.5 * 3 + 0.5 * 1.5 = 2.25
    data_line->forward();
    ema.calculate();
    EXPECT_DOUBLE_EQ(ema.get(0), 2.25);
    
    // 第四步：EMA = 0.5 * 4 + 0.5 * 2.25 = 3.125
    data_line->forward();
    ema.calculate();
    EXPECT_DOUBLE_EQ(ema.get(0), 3.125);
}

TEST_F(EMATest, ExpectedValuesWithKnownAlpha) {
    // 使用简单的alpha值进行验证
    EMA ema(data_line, 1);  // alpha = 2/2 = 1.0 (pure current value)
    
    std::vector<double> actual_results;
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        ema.calculate();
        actual_results.push_back(ema.get(0));
        
        if (i < test_data.size() - 1) {
            data_line->forward();
        }
    }
    
    // 当alpha=1时，EMA应该等于当前输入值
    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_DOUBLE_EQ(actual_results[i], test_data[i]);
    }
}

TEST_F(EMATest, ManualAlphaSetting) {
    EMA ema(data_line, 10);  // 初始周期
    
    // 设置自定义alpha
    ema.setAlpha(0.1);
    EXPECT_DOUBLE_EQ(ema.getAlpha(), 0.1);
    
    // 验证无效alpha值
    EXPECT_THROW(ema.setAlpha(0.0), std::invalid_argument);
    EXPECT_THROW(ema.setAlpha(1.1), std::invalid_argument);
    EXPECT_THROW(ema.setAlpha(-0.1), std::invalid_argument);
}

TEST_F(EMATest, InitialValueSetting) {
    EMA ema(data_line, 5);
    
    // 设置初始值
    ema.setInitialValue(10.0);
    EXPECT_TRUE(ema.hasPreviousValue());
    EXPECT_DOUBLE_EQ(ema.getPreviousEMA(), 10.0);
    
    // 计算应该基于设置的初始值
    ema.calculate();  // alpha * 1.0 + (1-alpha) * 10.0
    double expected = ema.getAlpha() * 1.0 + (1.0 - ema.getAlpha()) * 10.0;
    EXPECT_DOUBLE_EQ(ema.get(0), expected);
    
    // 测试无效初始值
    EXPECT_THROW(ema.setInitialValue(NaN), std::invalid_argument);
}

TEST_F(EMATest, NaNHandling) {
    // 创建包含NaN的数据
    std::vector<double> nan_data = {1.0, 2.0, NaN, 4.0, 5.0};
    auto nan_line = TestDataProvider::createLineRootFromData(nan_data, "nan_data");
    
    EMA ema(nan_line, 3);
    
    // 第一个值
    ema.calculate();
    EXPECT_DOUBLE_EQ(ema.get(0), 1.0);
    
    // 第二个值
    nan_line->forward();
    ema.calculate();
    EXPECT_FALSE(isNaN(ema.get(0)));
    
    // NaN值应该导致NaN输出
    nan_line->forward();
    ema.calculate();
    EXPECT_TRUE(isNaN(ema.get(0)));
    
    // 后续有效值
    nan_line->forward();
    ema.calculate();
    EXPECT_FALSE(isNaN(ema.get(0)));
}

TEST_F(EMATest, BatchCalculation) {
    auto large_data = TestDataProvider::generateTrendingData(1000, 100.0, 0.1, 1.0);
    auto large_line = TestDataProvider::createLineRootFromData(large_data);
    
    EMA ema(large_line, 20);
    
    // 重置到开始
    large_line->home();
    for (const auto& value : large_data) {
        large_line->forward(value);
    }
    
    // 批量计算
    ema.calculateBatch(0, 100);
    
    // 验证结果不是NaN
    EXPECT_FALSE(isNaN(ema.get(0)));
}

TEST_F(EMATest, WeightCalculation) {
    EMA ema(data_line, 5);
    
    // 测试权重计算
    auto weights = ema.getWeights(10);
    EXPECT_EQ(weights.size(), 10);
    
    // 第一个权重应该是alpha
    EXPECT_DOUBLE_EQ(weights[0], ema.getAlpha());
    
    // 验证权重递减
    for (size_t i = 1; i < weights.size(); ++i) {
        EXPECT_LT(weights[i], weights[i-1]);
    }
    
    // 测试总权重
    double total_weight = ema.getTotalWeight(10);
    EXPECT_GT(total_weight, 0.0);
    EXPECT_LT(total_weight, 1.0);
    
    // 更长的回溯期应该有更高的总权重
    double longer_weight = ema.getTotalWeight(20);
    EXPECT_GT(longer_weight, total_weight);
}

TEST_F(EMATest, StabilizationPeriod) {
    EMA ema5(data_line, 5);
    EMA ema20(data_line, 20);
    
    // 更长周期的EMA需要更长时间稳定
    EXPECT_LT(ema5.getStabilizationPeriod(), ema20.getStabilizationPeriod());
    EXPECT_EQ(ema5.getStabilizationPeriod(), 15);   // 5 * 3
    EXPECT_EQ(ema20.getStabilizationPeriod(), 60);  // 20 * 3
}

TEST_F(EMATest, PeriodChanges) {
    EMA ema(data_line, 5);
    
    double original_alpha = ema.getAlpha();
    
    // 改变周期
    ema.setPeriod(10);
    EXPECT_EQ(ema.getPeriod(), 10);
    EXPECT_LT(ema.getAlpha(), original_alpha);  // 更长周期 = 更小alpha
    
    // 验证参数更新
    EXPECT_DOUBLE_EQ(ema.getParam("period"), 10.0);
    EXPECT_DOUBLE_EQ(ema.getParam("alpha"), ema.getAlpha());
}

TEST_F(EMATest, ResetFunctionality) {
    EMA ema(data_line, 5);
    
    // 执行一些计算
    ema.calculate();
    data_line->forward();
    ema.calculate();
    
    EXPECT_TRUE(ema.hasPreviousValue());
    EXPECT_GT(ema.len(), 0);
    
    // 重置
    ema.reset();
    
    EXPECT_FALSE(ema.hasPreviousValue());
    EXPECT_EQ(ema.len(), 0);
    EXPECT_TRUE(isNaN(ema.getPreviousEMA()));
}

TEST_F(EMATest, ResponseToStepFunction) {
    // 测试EMA对阶跃函数的响应
    std::vector<double> step_levels = {1.0, 10.0};
    auto step_data = TestDataProvider::generateStepFunction(100, step_levels, 50);
    auto step_line = TestDataProvider::createLineRootFromData(step_data);
    
    EMA ema(step_line, 10);
    
    std::vector<double> ema_values;
    
    // 计算所有EMA值
    for (size_t i = 0; i < step_data.size(); ++i) {
        ema.calculate();
        ema_values.push_back(ema.get(0));
        
        if (i < step_data.size() - 1) {
            step_line->forward();
        }
    }
    
    // 验证EMA对阶跃的响应
    // 在阶跃发生后，EMA应该逐渐趋向新值
    EXPECT_LT(ema_values[51], step_data[51]);  // 应该还没完全到达新水平
    EXPECT_GT(ema_values[51], ema_values[49]); // 但应该在增长
    
    // 最终值应该接近阶跃后的水平
    EXPECT_GT(ema_values[99], 8.0);  // 应该接近10.0
}

TEST_F(EMATest, PerformanceTest) {
    // 创建大数据集
    auto large_data = TestDataProvider::generateRandomData(100000, 100.0, 10.0);
    auto large_line = TestDataProvider::createLineRootFromData(large_data);
    
    EMA ema(large_line, 20);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 执行大量计算
    for (size_t i = 0; i < large_data.size(); ++i) {
        ema.calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 性能要求：应该在合理时间内完成
    EXPECT_LT(duration.count(), 100000); // 小于100ms
    
    std::cout << "EMA Performance: " << duration.count() << "μs for " 
              << large_data.size() << " calculations" << std::endl;
}

TEST_F(EMATest, ComparisonWithSMA) {
    // 比较EMA和SMA的响应特性
    auto trend_data = TestDataProvider::generateTrendingData(100, 100.0, 1.0, 0.5);
    auto ema_line = TestDataProvider::createLineRootFromData(trend_data);
    
    EMA ema(ema_line, 10);
    
    std::vector<double> ema_values;
    
    for (size_t i = 0; i < trend_data.size(); ++i) {
        ema.calculate();
        ema_values.push_back(ema.get(0));
        
        if (i < trend_data.size() - 1) {
            ema_line->forward();
        }
    }
    
    // EMA应该比SMA更快响应趋势变化
    // 验证EMA能跟上趋势
    double first_third = ema_values[33];
    double last_third = ema_values[99];
    
    // 上升趋势中，后面的值应该大于前面的值
    EXPECT_GT(last_third, first_third);
}

} // namespace test
} // namespace backtrader