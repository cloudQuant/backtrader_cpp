/**
 * @file test_fractal.cpp
 * @brief Fractal指标测试 - 对应Python test_fractal.py
 * 
 * 原始Python测试:
 * - 测试Fractal分形指标
 * - 期望值: [["nan", "nan", "nan"], ["nan", "nan", "3553.692850"]]
 * - 最小周期5，包含2条线（向上和向下分形）
 */

#include "test_common.h"
#include "indicators/Fractal.h"
#include "cerebro/Cerebro.h"
#include "strategy/Strategy.h"
#include <memory>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace backtrader;
using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> FRACTAL_EXPECTED_VALUES = {
    {"nan", "nan", "nan"},           // line 0 (向上分形)
    {"nan", "nan", "3553.692850"}   // line 1 (向下分形)
};

const int FRACTAL_MIN_PERIOD = 5;

} // anonymous namespace

// 使用默认参数的Fractal测试
DEFINE_INDICATOR_TEST(Fractal_Default, Fractal, FRACTAL_EXPECTED_VALUES, FRACTAL_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Fractal_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建Fractal指标
    auto fractal = std::make_shared<indicators::Fractal>(high_line, low_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        fractal->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 5;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证2条线的值
    for (int line = 0; line < 2; ++line) {
        auto expected = FRACTAL_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = fractal->getLine(line)->get(check_points[i]);
            std::string actual_str;
            
            if (std::isnan(actual)) {
                actual_str = "nan";
            } else {
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(6) << actual;
                actual_str = ss.str();
            }
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "Fractal line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(fractal->getMinPeriod(), 5) << "Fractal minimum period should be 5";
}

// 测试分形检测逻辑
TEST(OriginalTests, Fractal_DetectionLogic) {
    // 创建特定的测试数据来验证分形检测
    std::vector<double> highs = {10, 15, 20, 15, 10, 12, 18, 22, 18, 14, 16, 25, 30, 25, 20};
    std::vector<double> lows = {8, 12, 17, 12, 8, 10, 15, 19, 15, 11, 13, 22, 27, 22, 17};
    
    auto high_line = std::make_shared<LineRoot>(highs.size(), "high");
    auto low_line = std::make_shared<LineRoot>(lows.size(), "low");
    
    for (size_t i = 0; i < highs.size(); ++i) {
        high_line->forward(highs[i]);
        low_line->forward(lows[i]);
    }
    
    auto fractal = std::make_shared<indicators::Fractal>(high_line, low_line);
    
    std::vector<double> up_fractals, down_fractals;
    
    for (size_t i = 0; i < highs.size(); ++i) {
        fractal->calculate();
        
        double up_fractal = fractal->getLine(0)->get(0);   // 向上分形
        double down_fractal = fractal->getLine(1)->get(0); // 向下分形
        
        up_fractals.push_back(up_fractal);
        down_fractals.push_back(down_fractal);
        
        if (i < highs.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 验证分形检测
    int up_fractal_count = 0;
    int down_fractal_count = 0;
    
    for (size_t i = 0; i < up_fractals.size(); ++i) {
        if (!std::isnan(up_fractals[i])) {
            up_fractal_count++;
            std::cout << "Up fractal at index " << i << ": " << up_fractals[i] << std::endl;
        }
        if (!std::isnan(down_fractals[i])) {
            down_fractal_count++;
            std::cout << "Down fractal at index " << i << ": " << down_fractals[i] << std::endl;
        }
    }
    
    // 应该检测到一些分形
    EXPECT_GT(up_fractal_count + down_fractal_count, 0) 
        << "Should detect some fractals";
}

// 测试分形参数
TEST(OriginalTests, Fractal_DifferentPeriods) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 测试不同的周期参数
    std::vector<int> periods = {3, 5, 7, 9};
    
    for (int period : periods) {
        auto fractal = std::make_shared<indicators::Fractal>(high_line, low_line, period);
        
        for (size_t i = 0; i < csv_data.size(); ++i) {
            fractal->calculate();
            if (i < csv_data.size() - 1) {
                high_line->forward();
                low_line->forward();
            }
        }
        
        // 验证最小周期
        EXPECT_EQ(fractal->getMinPeriod(), period) 
            << "Fractal minimum period should equal period parameter";
        
        // 统计分形数量
        int fractal_count = 0;
        for (int i = -(static_cast<int>(csv_data.size())); i <= 0; ++i) {
            if (!std::isnan(fractal->getLine(0)->get(i)) || 
                !std::isnan(fractal->getLine(1)->get(i))) {
                fractal_count++;
            }
        }
        
        std::cout << "Period " << period << " detected " << fractal_count 
                  << " fractals" << std::endl;
        
        // 重置数据线位置
        high_line->reset();
        low_line->reset();
        for (const auto& bar : csv_data) {
            high_line->forward(bar.high);
            low_line->forward(bar.low);
        }
    }
}

// 测试分形的对称性
TEST(OriginalTests, Fractal_Symmetry) {
    // 创建对称的测试数据
    std::vector<double> symmetric_highs = {10, 15, 20, 25, 20, 15, 10, 15, 20, 15, 10};
    std::vector<double> symmetric_lows = {8, 12, 17, 22, 17, 12, 8, 12, 17, 12, 8};
    
    auto high_line = std::make_shared<LineRoot>(symmetric_highs.size(), "high");
    auto low_line = std::make_shared<LineRoot>(symmetric_lows.size(), "low");
    
    for (size_t i = 0; i < symmetric_highs.size(); ++i) {
        high_line->forward(symmetric_highs[i]);
        low_line->forward(symmetric_lows[i]);
    }
    
    auto fractal = std::make_shared<indicators::Fractal>(high_line, low_line, 5);
    
    for (size_t i = 0; i < symmetric_highs.size(); ++i) {
        fractal->calculate();
        if (i < symmetric_highs.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 验证分形检测的对称性
    std::vector<double> up_fractals, down_fractals;
    for (int i = -(static_cast<int>(symmetric_highs.size())); i <= 0; ++i) {
        double up = fractal->getLine(0)->get(i);
        double down = fractal->getLine(1)->get(i);
        
        if (!std::isnan(up)) {
            up_fractals.push_back(up);
        }
        if (!std::isnan(down)) {
            down_fractals.push_back(down);
        }
    }
    
    std::cout << "Symmetric test: " << up_fractals.size() 
              << " up fractals, " << down_fractals.size() 
              << " down fractals" << std::endl;
}

// 测试分形的时间滞后
TEST(OriginalTests, Fractal_TimeLag) {
    // 创建明显的峰值和谷值数据
    std::vector<double> highs = {10, 20, 10, 5, 15, 25, 15, 8, 18, 30, 18, 12};
    std::vector<double> lows = {8, 18, 8, 3, 13, 23, 13, 6, 16, 28, 16, 10};
    
    auto high_line = std::make_shared<LineRoot>(highs.size(), "high");
    auto low_line = std::make_shared<LineRoot>(lows.size(), "low");
    
    for (size_t i = 0; i < highs.size(); ++i) {
        high_line->forward(highs[i]);
        low_line->forward(lows[i]);
    }
    
    auto fractal = std::make_shared<indicators::Fractal>(high_line, low_line, 3);
    
    struct FractalEvent {
        int index;
        double value;
        bool is_up;
    };
    
    std::vector<FractalEvent> fractal_events;
    
    for (size_t i = 0; i < highs.size(); ++i) {
        fractal->calculate();
        
        double up_fractal = fractal->getLine(0)->get(0);
        double down_fractal = fractal->getLine(1)->get(0);
        
        if (!std::isnan(up_fractal)) {
            fractal_events.push_back({static_cast<int>(i), up_fractal, true});
        }
        if (!std::isnan(down_fractal)) {
            fractal_events.push_back({static_cast<int>(i), down_fractal, false});
        }
        
        if (i < highs.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 验证分形事件的时间滞后特性
    for (const auto& event : fractal_events) {
        std::cout << (event.is_up ? "Up" : "Down") 
                  << " fractal at index " << event.index 
                  << " with value " << event.value << std::endl;
        
        // 分形应该在实际峰值/谷值之后被识别
        EXPECT_GE(event.index, 1) 
            << "Fractal should be detected with some lag";
    }
}

// 测试分形指标的边界条件
TEST(OriginalTests, Fractal_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_highs(20, 100.0);
    std::vector<double> flat_lows(20, 95.0);
    
    auto high_line = std::make_shared<LineRoot>(flat_highs.size(), "high");
    auto low_line = std::make_shared<LineRoot>(flat_lows.size(), "low");
    
    for (size_t i = 0; i < flat_highs.size(); ++i) {
        high_line->forward(flat_highs[i]);
        low_line->forward(flat_lows[i]);
    }
    
    auto flat_fractal = std::make_shared<indicators::Fractal>(high_line, low_line, 5);
    
    for (size_t i = 0; i < flat_highs.size(); ++i) {
        flat_fractal->calculate();
        if (i < flat_highs.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 相同价格时应该没有分形
    int fractal_count = 0;
    for (int i = -(static_cast<int>(flat_highs.size())); i <= 0; ++i) {
        if (!std::isnan(flat_fractal->getLine(0)->get(i)) || 
            !std::isnan(flat_fractal->getLine(1)->get(i))) {
            fractal_count++;
        }
    }
    
    EXPECT_EQ(fractal_count, 0) 
        << "Flat prices should not generate fractals";
    
    // 测试数据不足的情况
    std::vector<double> insufficient_highs = {10, 20, 15};
    std::vector<double> insufficient_lows = {8, 18, 13};
    
    auto insufficient_high_line = std::make_shared<LineRoot>(insufficient_highs.size(), "high");
    auto insufficient_low_line = std::make_shared<LineRoot>(insufficient_lows.size(), "low");
    
    for (size_t i = 0; i < insufficient_highs.size(); ++i) {
        insufficient_high_line->forward(insufficient_highs[i]);
        insufficient_low_line->forward(insufficient_lows[i]);
    }
    
    auto insufficient_fractal = std::make_shared<indicators::Fractal>(
        insufficient_high_line, insufficient_low_line, 5);
    
    for (size_t i = 0; i < insufficient_highs.size(); ++i) {
        insufficient_fractal->calculate();
        if (i < insufficient_highs.size() - 1) {
            insufficient_high_line->forward();
            insufficient_low_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result_up = insufficient_fractal->getLine(0)->get(0);
    double result_down = insufficient_fractal->getLine(1)->get(0);
    EXPECT_TRUE(std::isnan(result_up)) 
        << "Fractal should return NaN when insufficient data (up)";
    EXPECT_TRUE(std::isnan(result_down)) 
        << "Fractal should return NaN when insufficient data (down)";
}

// 性能测试
TEST(OriginalTests, Fractal_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_highs, large_lows;
    large_highs.reserve(data_size);
    large_lows.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        double base = dist(rng);
        large_highs.push_back(base + 2.0);
        large_lows.push_back(base - 2.0);
    }
    
    auto large_high_line = std::make_shared<LineRoot>(large_highs.size(), "high");
    auto large_low_line = std::make_shared<LineRoot>(large_lows.size(), "low");
    
    for (size_t i = 0; i < large_highs.size(); ++i) {
        large_high_line->forward(large_highs[i]);
        large_low_line->forward(large_lows[i]);
    }
    
    auto large_fractal = std::make_shared<indicators::Fractal>(
        large_high_line, large_low_line, 5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_highs.size(); ++i) {
        large_fractal->calculate();
        if (i < large_highs.size() - 1) {
            large_high_line->forward();
            large_low_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Fractal calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_up = large_fractal->getLine(0)->get(0);
    double final_down = large_fractal->getLine(1)->get(0);
    
    // 结果可能是NaN或有限值
    if (!std::isnan(final_up)) {
        EXPECT_TRUE(std::isfinite(final_up)) << "Final up fractal should be finite";
    }
    if (!std::isnan(final_down)) {
        EXPECT_TRUE(std::isfinite(final_down)) << "Final down fractal should be finite";
    }
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}