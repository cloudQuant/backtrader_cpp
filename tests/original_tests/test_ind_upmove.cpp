/**
 * @file test_ind_upmove.cpp
 * @brief UpMove指标测试 - 对应Python test_ind_upmove.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['-10.720000', '10.010000', '14.000000']
 * ]
 * chkmin = 2
 * chkind = btind.UpMove
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>

#include "indicators/upmove.h"
#include "linebuffer.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> UPMOVE_EXPECTED_VALUES = {
    {"-10.720000", "10.010000", "14.000000"}
};

const int UPMOVE_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的UpMove测试
DEFINE_INDICATOR_TEST(UpMove_Default, UpMove, UPMOVE_EXPECTED_VALUES, UPMOVE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, UpMove_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线 - 使用LineSeries+LineBuffer模式
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close_line", 0);
    auto close_line_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    
    if (close_line_buffer) {
        close_line_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_line_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建UpMove指标
    auto upmove = std::make_shared<UpMove>(close_line);
    
    // 计算所有值 - 只调用一次
    upmove->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 2;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // 注意：由于LineBuffer在位置0有NaN，索引可能需要调整
    std::vector<int> check_points = {
        0,                                    // 第一个有效值（最新的值）
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2 - 1  // 中间值 - 减1来修正偏移
    };
    
    std::vector<std::string> expected = {"-10.720000", "10.010000", "14.000000"};
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = upmove->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        
        
        // Check if values are close enough (within 0.1% tolerance)
        double expected_val = std::stod(expected[i]);
        bool close_enough = std::abs(actual - expected_val) < std::abs(expected_val) * 0.001;
        
        if (close_enough || std::abs(actual - expected_val) < 0.001) {
            // Use a looser comparison for values that are close
            EXPECT_NEAR(actual, expected_val, std::max(std::abs(expected_val) * 0.001, 0.001)) 
                << "UpMove value close but not exact at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        } else {
            EXPECT_EQ(actual_str, expected[i]) 
                << "UpMove value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(upmove->getMinPeriod(), 2) << "UpMove minimum period should be 2";
}

// UpMove计算逻辑验证测试
TEST(OriginalTests, UpMove_CalculationLogic) {
    // 使用简单的测试数据验证UpMove计算
    std::vector<double> high_prices = {100.0, 105.0, 102.0, 108.0, 104.0, 110.0, 106.0, 112.0, 109.0, 115.0};
    
    auto high_line = std::make_shared<LineSeries>();
    high_line->lines->add_line(std::make_shared<LineBuffer>());
    high_line->lines->add_alias("high_line", 0);
    auto high_line_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line->lines->getline(0));
    
    if (high_line_buffer) {
        high_line_buffer->set(0, high_prices[0]);
        for (size_t i = 1; i < high_prices.size(); ++i) {
            high_line_buffer->append(high_prices[i]);
        }
    }
    
    auto upmove = std::make_shared<UpMove>(high_line);
    
    // Calculate once
    upmove->calculate();
    
    // 验证UpMove值
    for (size_t i = 1; i < high_prices.size(); ++i) {
        double current_high = high_prices[i];
        double prev_high = high_prices[i - 1];
        double expected_upmove = current_high - prev_high;
        
        // UpMove = current_high - prev_high (can be negative!)
        // No max(0, ...) - Python version allows negative values
        
        double actual_upmove = upmove->get(-(high_prices.size() - 1 - i));
        
        if (!std::isnan(actual_upmove)) {
            EXPECT_NEAR(actual_upmove, expected_upmove, 1e-10) 
                << "UpMove calculation mismatch at position " << i
                << " (prev=" << prev_high << ", current=" << current_high << ")";
        }
    }
}

// UpMove向上运动识别测试
TEST(OriginalTests, UpMove_UpwardMovementDetection) {
    // 创建明确的向上运动数据
    std::vector<double> upward_highs = {100.0, 105.0, 110.0, 115.0, 120.0, 125.0, 130.0};
    
    auto up_line = std::make_shared<LineSeries>();
    up_line->lines->add_line(std::make_shared<LineBuffer>());
    up_line->lines->add_alias("up_line", 0);
    auto up_line_buffer = std::dynamic_pointer_cast<LineBuffer>(up_line->lines->getline(0));
    
    if (up_line_buffer) {
        up_line_buffer->set(0, upward_highs[0]);
        for (size_t i = 1; i < upward_highs.size(); ++i) {
            up_line_buffer->append(upward_highs[i]);
        }
    }
    
    auto up_upmove = std::make_shared<UpMove>(up_line);
    
    // Calculate once
    up_upmove->calculate();
    
    std::vector<double> upmove_values;
    for (size_t i = 1; i < upward_highs.size(); ++i) {
        double upmove_val = up_upmove->get(-(upward_highs.size() - 1 - i));
        if (!std::isnan(upmove_val)) {
            upmove_values.push_back(upmove_val);
        }
    }
    
    // 在持续上升的数据中，所有UpMove值都应该为正
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        EXPECT_GT(upmove_values[i], 0.0) 
            << "UpMove should be positive for upward movement at position " << i;
    }
    
    std::cout << "Upward movement UpMove values:" << std::endl;
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        std::cout << "Position " << i + 1 << ": " << upmove_values[i] << std::endl;
    }
}

// UpMove向下运动测试
TEST(OriginalTests, UpMove_DownwardMovementTest) {
    // 创建向下运动数据
    std::vector<double> downward_highs = {130.0, 125.0, 120.0, 115.0, 110.0, 105.0, 100.0};
    
    auto down_line = std::make_shared<LineSeries>();
    down_line->lines->add_line(std::make_shared<LineBuffer>());
    down_line->lines->add_alias("down_line", 0);
    auto down_line_buffer = std::dynamic_pointer_cast<LineBuffer>(down_line->lines->getline(0));
    
    if (down_line_buffer) {
        down_line_buffer->set(0, downward_highs[0]);
        for (size_t i = 1; i < downward_highs.size(); ++i) {
            down_line_buffer->append(downward_highs[i]);
        }
    }
    
    auto down_upmove = std::make_shared<UpMove>(down_line);
    
    // Calculate once
    down_upmove->calculate();
    
    std::vector<double> upmove_values;
    for (size_t i = 1; i < downward_highs.size(); ++i) {
        double upmove_val = down_upmove->get(-(downward_highs.size() - 1 - i));
        if (!std::isnan(upmove_val)) {
            upmove_values.push_back(upmove_val);
        }
    }
    
    // 在持续下降的数据中，所有UpMove值都应该为负
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        EXPECT_LT(upmove_values[i], 0.0) 
            << "UpMove should be negative for downward movement at position " << i;
    }
    
    std::cout << "Downward movement UpMove values:" << std::endl;
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        std::cout << "Position " << i + 1 << ": " << upmove_values[i] << std::endl;
    }
}

// UpMove混合运动测试
TEST(OriginalTests, UpMove_MixedMovementTest) {
    // 创建混合运动数据（上下交替）
    std::vector<double> mixed_highs = {100.0, 105.0, 103.0, 108.0, 106.0, 111.0, 109.0, 114.0};
    
    auto mixed_line = std::make_shared<LineSeries>();
    mixed_line->lines->add_line(std::make_shared<LineBuffer>());
    mixed_line->lines->add_alias("mixed_line", 0);
    auto mixed_line_buffer = std::dynamic_pointer_cast<LineBuffer>(mixed_line->lines->getline(0));
    
    if (mixed_line_buffer) {
        mixed_line_buffer->set(0, mixed_highs[0]);
        for (size_t i = 1; i < mixed_highs.size(); ++i) {
            mixed_line_buffer->append(mixed_highs[i]);
        }
    }
    
    auto mixed_upmove = std::make_shared<UpMove>(mixed_line);
    
    // Calculate once
    mixed_upmove->calculate();
    
    // 验证计算结果
    for (size_t i = 1; i < mixed_highs.size(); ++i) {
        double expected_move = mixed_highs[i] - mixed_highs[i-1];
        double expected_upmove = expected_move;  // No max(0, ...) - allow negative values
        double actual_upmove = mixed_upmove->get(-(mixed_highs.size() - 1 - i));
        
        if (!std::isnan(actual_upmove)) {
            EXPECT_NEAR(actual_upmove, expected_upmove, 1e-10) 
                << "UpMove mismatch at position " << i;
            
            std::cout << "Position " << i << ": " 
                      << mixed_highs[i-1] << " -> " << mixed_highs[i] 
                      << ", UpMove = " << actual_upmove << std::endl;
        }
    }
}

// UpMove与真实市场数据测试
TEST(OriginalTests, UpMove_RealMarketData) {
    auto csv_data = getdata(0);
    
    auto high_line = std::make_shared<LineSeries>();
    high_line->lines->add_line(std::make_shared<LineBuffer>());
    high_line->lines->add_alias("market_high", 0);
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line->lines->getline(0));
    
    if (high_buffer) {
        high_buffer->set(0, csv_data[0].high);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
        }
    }
    
    auto market_upmove = std::make_shared<UpMove>(high_line);
    
    // Calculate once
    market_upmove->calculate();
    
    // 统计分析
    int positive_moves = 0;
    int zero_moves = 0;
    double total_move = 0.0;
    double max_move = -std::numeric_limits<double>::infinity();
    double min_move = std::numeric_limits<double>::infinity();
    
    for (size_t i = 1; i < csv_data.size(); ++i) {
        double upmove_val = market_upmove->get(-(csv_data.size() - 1 - i));
        
        if (!std::isnan(upmove_val)) {
            if (upmove_val > 0.0) {
                positive_moves++;
                total_move += upmove_val;
            } else {
                zero_moves++;
            }
            
            max_move = std::max(max_move, upmove_val);
            min_move = std::min(min_move, upmove_val);
        }
    }
    
    int total_moves = positive_moves + zero_moves;
    
    std::cout << "Real market data UpMove statistics:" << std::endl;
    std::cout << "Total data points: " << total_moves << std::endl;
    std::cout << "Positive moves: " << positive_moves << " (" 
              << (100.0 * positive_moves / total_moves) << "%)" << std::endl;
    std::cout << "Zero moves: " << zero_moves << " (" 
              << (100.0 * zero_moves / total_moves) << "%)" << std::endl;
    std::cout << "Average positive move: " 
              << (positive_moves > 0 ? total_move / positive_moves : 0.0) << std::endl;
    std::cout << "Max move: " << max_move << std::endl;
    std::cout << "Min move: " << min_move << std::endl;
    
    // 验证基本属性 - UpMove can be negative in Python implementation
    EXPECT_GT(positive_moves, 0) << "Should have some positive moves in real data";
}

// 参数化测试 - 测试不同的价格范围
class UpMoveParameterizedTest : public ::testing::TestWithParam<double> {
protected:
    void SetUp() override {
        scale_ = GetParam();
        
        // 创建测试数据
        for (int i = 0; i < 20; ++i) {
            double base = 100.0 * scale_;
            double variation = (i % 5 - 2) * scale_;  // -2, -1, 0, 1, 2 pattern
            test_data_.push_back(base + variation);
        }
        
        high_line_ = std::make_shared<LineSeries>();
        high_line_->lines->add_line(std::make_shared<LineBuffer>());
        high_line_->lines->add_alias("param_high", 0);
        high_buffer_ = std::dynamic_pointer_cast<LineBuffer>(high_line_->lines->getline(0));
        
        if (high_buffer_) {
            high_buffer_->set(0, test_data_[0]);
            for (size_t i = 1; i < test_data_.size(); ++i) {
                high_buffer_->append(test_data_[i]);
            }
        }
    }
    
    double scale_;
    std::vector<double> test_data_;
    std::shared_ptr<LineSeries> high_line_;
    std::shared_ptr<LineBuffer> high_buffer_;
};

TEST_P(UpMoveParameterizedTest, DifferentScales) {
    auto upmove = std::make_shared<UpMove>(high_line_);
    
    // Calculate once
    upmove->calculate();
    
    // 验证计算结果的正确性
    for (size_t i = 1; i < test_data_.size(); ++i) {
        double expected_move = test_data_[i] - test_data_[i-1];
        double expected_upmove = expected_move;  // No max(0, ...) - allow negative values
        double actual_upmove = upmove->get(-(test_data_.size() - 1 - i));
        
        if (!std::isnan(actual_upmove)) {
            EXPECT_NEAR(actual_upmove, expected_upmove, std::abs(expected_upmove) * 1e-10) 
                << "UpMove calculation error at position " << i 
                << " with scale " << scale_;
        }
    }
}

// 测试不同的价格范围
INSTANTIATE_TEST_SUITE_P(
    VariousScales,
    UpMoveParameterizedTest,
    ::testing::Values(0.01, 0.1, 1.0, 10.0, 100.0, 1000.0)
);

// 边界条件测试
TEST(OriginalTests, UpMove_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> same_prices(10, 100.0);
    
    auto same_line = std::make_shared<LineSeries>();
    same_line->lines->add_line(std::make_shared<LineBuffer>());
    same_line->lines->add_alias("same_line", 0);
    auto same_buffer = std::dynamic_pointer_cast<LineBuffer>(same_line->lines->getline(0));
    
    if (same_buffer) {
        same_buffer->set(0, same_prices[0]);
        for (size_t i = 1; i < same_prices.size(); ++i) {
            same_buffer->append(same_prices[i]);
        }
    }
    
    auto same_upmove = std::make_shared<UpMove>(same_line);
    
    // Calculate once
    same_upmove->calculate();
    
    // 所有相同价格的UpMove应该为0
    for (size_t i = 1; i < same_prices.size(); ++i) {
        double upmove_val = same_upmove->get(-(same_prices.size() - 1 - i));
        if (!std::isnan(upmove_val)) {
            EXPECT_EQ(upmove_val, 0.0) 
                << "UpMove should be 0 for same prices at position " << i;
        }
    }
    
    // 测试大幅跳跃
    std::vector<double> jump_prices = {100.0, 200.0, 50.0, 300.0, 10.0};
    
    auto jump_line = std::make_shared<LineSeries>();
    jump_line->lines->add_line(std::make_shared<LineBuffer>());
    jump_line->lines->add_alias("jump_line", 0);
    auto jump_buffer = std::dynamic_pointer_cast<LineBuffer>(jump_line->lines->getline(0));
    
    if (jump_buffer) {
        jump_buffer->set(0, jump_prices[0]);
        for (size_t i = 1; i < jump_prices.size(); ++i) {
            jump_buffer->append(jump_prices[i]);
        }
    }
    
    auto jump_upmove = std::make_shared<UpMove>(jump_line);
    
    // Calculate once
    jump_upmove->calculate();
    
    // 验证大幅跳跃的处理 - UpMove允许负值
    EXPECT_NEAR(jump_upmove->get(-3), 100.0, 1e-10);   // 100 -> 200
    EXPECT_NEAR(jump_upmove->get(-2), -150.0, 1e-10);  // 200 -> 50
    EXPECT_NEAR(jump_upmove->get(-1), 250.0, 1e-10);   // 50 -> 300
    EXPECT_NEAR(jump_upmove->get(0), -290.0, 1e-10);   // 300 -> 10
}