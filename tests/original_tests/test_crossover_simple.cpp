/**
 * @file test_crossover_simple.cpp
 * @brief CrossOver指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/CrossOver.h"
#include "indicators/SMA.h"

using namespace backtrader::tests::original;
using namespace backtrader;

// 测试CrossOver指标
TEST(OriginalTests, CrossOver_Manual) {
    // 创建测试数据
    std::vector<double> prices = {100, 101, 102, 103, 102, 101, 100, 101, 102, 103, 104, 105};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "close");
    
    // 创建两个SMA指标作为CrossOver的输入
    auto sma_fast = std::make_shared<SMA>(close_line, 3);  // 快线
    auto sma_slow = std::make_shared<SMA>(close_line, 5);  // 慢线
    
    // 创建CrossOver指标
    auto crossover = std::make_shared<CrossOver>(sma_fast, sma_slow);
    
    std::cout << "CrossOver Test Results:" << std::endl;
    std::cout << "Price | SMA3  | SMA5  | Cross" << std::endl;
    std::cout << "------|-------|-------|------" << std::endl;
    
    // 计算所有值
    for (size_t i = 0; i < prices.size(); ++i) {
        close_line->forward(prices[i]);
        sma_fast->calculate();
        sma_slow->calculate();
        crossover->calculate();
        
        double fast_val = sma_fast->get(0);
        double slow_val = sma_slow->get(0);
        double cross_val = crossover->get(0);
        
        std::cout << std::setw(5) << prices[i] 
                  << " | " << std::setw(5) << std::fixed << std::setprecision(2) 
                  << (std::isnan(fast_val) ? 0.0 : fast_val)
                  << " | " << std::setw(5) << std::fixed << std::setprecision(2) 
                  << (std::isnan(slow_val) ? 0.0 : slow_val)
                  << " | " << std::setw(5) << std::fixed << std::setprecision(1) 
                  << cross_val << std::endl;
    }
    
    // 验证基本属性
    EXPECT_EQ(crossover->getMinPeriod(), 2) << "CrossOver minimum period should be 2";
    
    // 验证最后的值不是NaN
    double last_value = crossover->get(0);
    EXPECT_FALSE(std::isnan(last_value)) << "Last CrossOver value should not be NaN";
}

// 测试交叉检测
TEST(OriginalTests, CrossOver_CrossDetection) {
    // 创建简单的交叉数据
    std::vector<double> line1_data = {1, 2, 3, 4, 3, 2, 1, 2, 3, 4};
    std::vector<double> line2_data = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
    
    auto line1 = std::make_shared<LineRoot>(line1_data.size(), "line1");
    auto line2 = std::make_shared<LineRoot>(line2_data.size(), "line2");
    
    auto crossover = std::make_shared<CrossOver>(line1, line2);
    
    bool found_golden_cross = false;
    bool found_death_cross = false;
    
    for (size_t i = 0; i < line1_data.size(); ++i) {
        line1->forward(line1_data[i]);
        line2->forward(line2_data[i]);
        crossover->calculate();
        
        double cross_val = crossover->get(0);
        
        if (cross_val > 0.0) {
            found_golden_cross = true;
        } else if (cross_val < 0.0) {
            found_death_cross = true;
        }
    }
    
    EXPECT_TRUE(found_golden_cross) << "Should detect golden cross";
    EXPECT_TRUE(found_death_cross) << "Should detect death cross";
}