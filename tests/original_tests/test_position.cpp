/**
 * @file test_position.cpp
 * @brief 仓位测试 - 对应Python test_position.py
 * 
 * 原始Python测试:
 * - 测试仓位的基本创建和更新功能
 * - 验证增加仓位时的价格计算
 * - 测试减少仓位和反向仓位的处理
 * - 验证opened和closed数量的正确计算
 */

#include "test_common.h"
#include "position.h"
#include <memory>
#include <cassert>
#include <iostream>

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

// 基本仓位操作测试
TEST(OriginalTests, Position_BasicOperations) {
    double size = 10.0;
    double price = 10.0;
    
    // 创建初始仓位
    backtrader::Position pos(size, price);
    EXPECT_DOUBLE_EQ(pos.get_size(), size) << "Initial position size should match";
    EXPECT_DOUBLE_EQ(pos.get_price(), price) << "Initial position price should match";
    
    // 增加仓位
    double upsize = 5.0;
    double upprice = 12.5;
    pos.update(upsize, upprice);
    
    // 验证仓位增加后的状态
    EXPECT_DOUBLE_EQ(pos.get_size(), size + upsize) << "Position size should be updated";
    
    // 计算期望的加权平均价格
    double expected_price = ((size * price) + (upsize * upprice)) / pos.get_size();
    EXPECT_DOUBLE_EQ(pos.get_price(), expected_price) << "Position price should be weighted average";
}

// 减少仓位测试
TEST(OriginalTests, Position_ReducePosition) {
    // 从上一个测试的状态开始
    backtrader::Position pos(15.0, ((10.0 * 10.0) + (5.0 * 12.5)) / 15.0);  // 加权平均价格
    
    double size = pos.get_size();
    double price = pos.get_price();
    double upsize = -7.0;
    double upprice = 14.5;
    
    pos.update(upsize, upprice);
    
    // 验证仓位减少后的状态
    EXPECT_DOUBLE_EQ(pos.get_size(), size + upsize) << "Position size should be reduced";
    
    // 减少仓位时价格不变
    EXPECT_DOUBLE_EQ(pos.get_price(), price) << "Position price should remain unchanged when reducing";
}

// 反向仓位测试
TEST(OriginalTests, Position_ReversePosition) {
    // 从8.0的多头仓位开始
    double initial_size = 8.0;
    double initial_price = ((10.0 * 10.0) + (5.0 * 12.5)) / 15.0;  // 从前面计算的加权平均价格
    backtrader::Position pos(initial_size, initial_price);
    
    double size = pos.get_size();
    double upsize = -15.0;  // 超过当前仓位的卖出
    double upprice = 17.5;
    
    pos.update(upsize, upprice);
    
    // 验证反向仓位后的状态
    EXPECT_DOUBLE_EQ(pos.get_size(), size + upsize) << "Position size should be negative (short)";
    
    // 反向仓位时价格变为新的价格
    EXPECT_DOUBLE_EQ(pos.get_price(), upprice) << "Position price should be new price for reverse";
}

// 完整的Python测试用例复现
TEST(OriginalTests, Position_PythonTestReplication) {
    // 第一阶段：创建初始仓位
    double size = 10.0;
    double price = 10.0;
    
    backtrader::Position pos(size, price);
    ASSERT_DOUBLE_EQ(pos.get_size(), size);
    ASSERT_DOUBLE_EQ(pos.get_price(), price);
    
    // 第二阶段：增加仓位
    double upsize = 5.0;
    double upprice = 12.5;
    pos.update(upsize, upprice);
    
    ASSERT_DOUBLE_EQ(pos.get_size(), size + upsize);
    ASSERT_DOUBLE_EQ(pos.get_price(), ((size * price) + (upsize * upprice)) / pos.get_size());
    
    // 第三阶段：减少仓位
    size = pos.get_size();
    price = pos.get_price();
    upsize = -7.0;
    upprice = 14.5;
    
    pos.update(upsize, upprice);
    
    ASSERT_DOUBLE_EQ(pos.get_size(), size + upsize);
    ASSERT_DOUBLE_EQ(pos.get_price(), price);
    
    // 第四阶段：反向仓位
    size = pos.get_size();
    upsize = -15.0;
    upprice = 17.5;
    
    pos.update(upsize, upprice);
    
    ASSERT_DOUBLE_EQ(pos.get_size(), size + upsize);
    ASSERT_DOUBLE_EQ(pos.get_price(), upprice);
}

// 测试零仓位
TEST(OriginalTests, Position_ZeroPosition) {
    backtrader::Position pos;  // 默认构造函数应该创建零仓位
    
    EXPECT_DOUBLE_EQ(pos.get_size(), 0.0) << "Default position size should be 0";
    EXPECT_DOUBLE_EQ(pos.get_price(), 0.0) << "Default position price should be 0";
    
    // 从零仓位开始建仓
    pos.update(100.0, 50.0);
    
    EXPECT_DOUBLE_EQ(pos.get_size(), 100.0) << "Position size should be updated from zero";
    EXPECT_DOUBLE_EQ(pos.get_price(), 50.0) << "Position price should be set";
}

// 测试仓位清空
TEST(OriginalTests, Position_ClosePosition) {
    backtrader::Position pos(50.0, 25.0);
    
    // 完全清仓
    pos.update(-50.0, 30.0);
    
    EXPECT_DOUBLE_EQ(pos.get_size(), 0.0) << "Position should be flat after closing";
    EXPECT_DOUBLE_EQ(pos.get_price(), 0.0) << "Position price should be reset to 0";
}

// 测试仓位符号变化
TEST(OriginalTests, Position_SignChanges) {
    backtrader::Position pos;
    
    // 建立多头仓位
    pos.update(100.0, 10.0);
    EXPECT_GT(pos.get_size(), 0.0) << "Should have long position";
    
    // 转为空头仓位
    pos.update(-200.0, 15.0);
    EXPECT_LT(pos.get_size(), 0.0) << "Should have short position";
    EXPECT_DOUBLE_EQ(pos.get_size(), -100.0) << "Short position size should be correct";
    EXPECT_DOUBLE_EQ(pos.get_price(), 15.0) << "Short position price should be correct";
    
    // 转回多头仓位
    pos.update(150.0, 20.0);
    EXPECT_GT(pos.get_size(), 0.0) << "Should have long position again";
    EXPECT_DOUBLE_EQ(pos.get_size(), 50.0) << "Long position size should be correct";
    EXPECT_DOUBLE_EQ(pos.get_price(), 20.0) << "Long position price should be correct";
}

// 测试累积平均价格计算
TEST(OriginalTests, Position_AveragePriceCalculation) {
    backtrader::Position pos;
    
    // 多次建仓，测试平均价格计算
    std::vector<std::pair<double, double>> trades = {
        {100.0, 10.0},  // 100股，10元
        {50.0, 12.0},   // 50股，12元
        {25.0, 8.0},    // 25股，8元
        {25.0, 16.0}    // 25股，16元
    };
    
    double total_value = 0.0;
    double total_size = 0.0;
    
    for (const auto& trade : trades) {
        pos.update(trade.first, trade.second);
        total_value += trade.first * trade.second;
        total_size += trade.first;
        
        double expected_avg_price = total_value / total_size;
        EXPECT_NEAR(pos.get_price(), expected_avg_price, 1e-10) 
            << "Average price calculation should be accurate";
    }
    
    EXPECT_DOUBLE_EQ(pos.get_size(), total_size) << "Total size should match";
}

// 测试部分平仓
TEST(OriginalTests, Position_PartialClose) {
    backtrader::Position pos(100.0, 15.0);
    
    // 部分平仓不改变价格
    pos.update(-30.0, 20.0);
    
    EXPECT_DOUBLE_EQ(pos.get_size(), 70.0) << "Remaining position should be correct";
    EXPECT_DOUBLE_EQ(pos.get_price(), 15.0) << "Price should remain unchanged";
    
    // 再次部分平仓
    pos.update(-20.0, 18.0);
    
    EXPECT_DOUBLE_EQ(pos.get_size(), 50.0) << "Remaining position should be correct";
    EXPECT_DOUBLE_EQ(pos.get_price(), 15.0) << "Price should still remain unchanged";
}

// 简化的边界条件测试
TEST(OriginalTests, Position_EdgeCases) {
    backtrader::Position pos;
    
    // 测试零价格（某些特殊情况下可能出现）
    pos.update(100.0, 0.0);
    EXPECT_DOUBLE_EQ(pos.get_price(), 0.0) << "Should handle zero price";
    EXPECT_DOUBLE_EQ(pos.get_size(), 100.0) << "Should handle zero price with correct size";
    
    // 从零价格仓位更新
    pos.update(50.0, 10.0);
    double expected_price = (100.0 * 0.0 + 50.0 * 10.0) / 150.0;
    EXPECT_DOUBLE_EQ(pos.get_price(), expected_price) << "Should calculate average with zero price";
}