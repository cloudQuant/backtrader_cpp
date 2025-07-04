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
#include "broker/Position.h"
#include <memory>
#include <cassert>
#include <iostream>

using namespace backtrader::tests::original;

// 基本仓位操作测试
TEST(OriginalTests, Position_BasicOperations) {
    double size = 10.0;
    double price = 10.0;
    
    // 创建初始仓位
    Position pos(size, price);
    EXPECT_DOUBLE_EQ(pos.getSize(), size) << "Initial position size should match";
    EXPECT_DOUBLE_EQ(pos.getPrice(), price) << "Initial position price should match";
    
    // 增加仓位
    double upsize = 5.0;
    double upprice = 12.5;
    auto result = pos.update(upsize, upprice);
    
    double nsize = result.size;
    double nprice = result.price;
    double opened = result.opened;
    double closed = result.closed;
    
    // 验证仓位增加后的状态
    EXPECT_DOUBLE_EQ(pos.getSize(), size + upsize) << "Position size should be updated";
    EXPECT_DOUBLE_EQ(pos.getSize(), nsize) << "Returned size should match position size";
    
    // 计算期望的加权平均价格
    double expected_price = ((size * price) + (upsize * upprice)) / pos.getSize();
    EXPECT_DOUBLE_EQ(pos.getPrice(), expected_price) << "Position price should be weighted average";
    EXPECT_DOUBLE_EQ(pos.getPrice(), nprice) << "Returned price should match position price";
    
    // 验证opened和closed数量
    EXPECT_DOUBLE_EQ(opened, upsize) << "Opened amount should equal update size";
    EXPECT_DOUBLE_EQ(closed, 0.0) << "Closed amount should be 0 for position increase";
}

// 减少仓位测试
TEST(OriginalTests, Position_ReducePosition) {
    // 从上一个测试的状态开始
    Position pos(15.0, ((10.0 * 10.0) + (5.0 * 12.5)) / 15.0);  // 加权平均价格
    
    double size = pos.getSize();
    double price = pos.getPrice();
    double upsize = -7.0;
    double upprice = 14.5;
    
    auto result = pos.update(upsize, upprice);
    
    double nsize = result.size;
    double nprice = result.price;
    double opened = result.opened;
    double closed = result.closed;
    
    // 验证仓位减少后的状态
    EXPECT_DOUBLE_EQ(pos.getSize(), size + upsize) << "Position size should be reduced";
    EXPECT_DOUBLE_EQ(pos.getSize(), nsize) << "Returned size should match position size";
    
    // 减少仓位时价格不变
    EXPECT_DOUBLE_EQ(pos.getPrice(), price) << "Position price should remain unchanged when reducing";
    EXPECT_DOUBLE_EQ(pos.getPrice(), nprice) << "Returned price should match position price";
    
    // 验证opened和closed数量
    EXPECT_DOUBLE_EQ(opened, 0.0) << "Opened amount should be 0 for position reduction";
    EXPECT_DOUBLE_EQ(closed, upsize) << "Closed amount should equal update size (with sign)";
}

// 反向仓位测试
TEST(OriginalTests, Position_ReversePosition) {
    // 从8.0的多头仓位开始
    double initial_size = 8.0;
    double initial_price = ((10.0 * 10.0) + (5.0 * 12.5)) / 15.0;  // 从前面计算的加权平均价格
    Position pos(initial_size, initial_price);
    
    double size = pos.getSize();
    double price = pos.getPrice();
    double upsize = -15.0;  // 超过当前仓位的卖出
    double upprice = 17.5;
    
    auto result = pos.update(upsize, upprice);
    
    double nsize = result.size;
    double nprice = result.price;
    double opened = result.opened;
    double closed = result.closed;
    
    // 验证反向仓位后的状态
    EXPECT_DOUBLE_EQ(pos.getSize(), size + upsize) << "Position size should be negative (short)";
    EXPECT_DOUBLE_EQ(pos.getSize(), nsize) << "Returned size should match position size";
    
    // 反向仓位时价格变为新的价格
    EXPECT_DOUBLE_EQ(pos.getPrice(), upprice) << "Position price should be new price for reverse";
    EXPECT_DOUBLE_EQ(pos.getPrice(), nprice) << "Returned price should match position price";
    
    // 验证opened和closed数量
    EXPECT_DOUBLE_EQ(opened, size + upsize) << "Opened amount should be the new position size";
    EXPECT_DOUBLE_EQ(closed, -size) << "Closed amount should be negative of original size";
}

// 完整的Python测试用例复现
TEST(OriginalTests, Position_PythonTestReplication) {
    // 第一阶段：创建初始仓位
    double size = 10.0;
    double price = 10.0;
    
    Position pos(size, price);
    ASSERT_DOUBLE_EQ(pos.getSize(), size);
    ASSERT_DOUBLE_EQ(pos.getPrice(), price);
    
    // 第二阶段：增加仓位
    double upsize = 5.0;
    double upprice = 12.5;
    auto result1 = pos.update(upsize, upprice);
    
    double nsize1 = result1.size;
    double nprice1 = result1.price;
    double opened1 = result1.opened;
    double closed1 = result1.closed;
    
    ASSERT_DOUBLE_EQ(pos.getSize(), size + upsize);
    ASSERT_DOUBLE_EQ(pos.getSize(), nsize1);
    ASSERT_DOUBLE_EQ(pos.getPrice(), ((size * price) + (upsize * upprice)) / pos.getSize());
    ASSERT_DOUBLE_EQ(pos.getPrice(), nprice1);
    ASSERT_DOUBLE_EQ(opened1, upsize);
    ASSERT_DOUBLE_EQ(closed1, 0.0);
    
    // 第三阶段：减少仓位
    size = pos.getSize();
    price = pos.getPrice();
    upsize = -7.0;
    upprice = 14.5;
    
    auto result2 = pos.update(upsize, upprice);
    
    double nsize2 = result2.size;
    double nprice2 = result2.price;
    double opened2 = result2.opened;
    double closed2 = result2.closed;
    
    ASSERT_DOUBLE_EQ(pos.getSize(), size + upsize);
    ASSERT_DOUBLE_EQ(pos.getSize(), nsize2);
    ASSERT_DOUBLE_EQ(pos.getPrice(), price);
    ASSERT_DOUBLE_EQ(pos.getPrice(), nprice2);
    ASSERT_DOUBLE_EQ(opened2, 0.0);
    ASSERT_DOUBLE_EQ(closed2, upsize);  // closed必须有update size的符号
    
    // 第四阶段：反向仓位
    size = pos.getSize();
    price = pos.getPrice();
    upsize = -15.0;
    upprice = 17.5;
    
    auto result3 = pos.update(upsize, upprice);
    
    double nsize3 = result3.size;
    double nprice3 = result3.price;
    double opened3 = result3.opened;
    double closed3 = result3.closed;
    
    ASSERT_DOUBLE_EQ(pos.getSize(), size + upsize);
    ASSERT_DOUBLE_EQ(pos.getSize(), nsize3);
    ASSERT_DOUBLE_EQ(pos.getPrice(), upprice);
    ASSERT_DOUBLE_EQ(pos.getPrice(), nprice3);
    ASSERT_DOUBLE_EQ(opened3, size + upsize);
    ASSERT_DOUBLE_EQ(closed3, -size);
}

// 测试零仓位
TEST(OriginalTests, Position_ZeroPosition) {
    Position pos;  // 默认构造函数应该创建零仓位
    
    EXPECT_DOUBLE_EQ(pos.getSize(), 0.0) << "Default position size should be 0";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 0.0) << "Default position price should be 0";
    
    // 从零仓位开始建仓
    auto result = pos.update(100.0, 50.0);
    
    EXPECT_DOUBLE_EQ(pos.getSize(), 100.0) << "Position size should be updated from zero";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 50.0) << "Position price should be set";
    EXPECT_DOUBLE_EQ(result.opened, 100.0) << "All should be opened";
    EXPECT_DOUBLE_EQ(result.closed, 0.0) << "Nothing should be closed";
}

// 测试仓位清空
TEST(OriginalTests, Position_ClosePosition) {
    Position pos(50.0, 25.0);
    
    // 完全清仓
    auto result = pos.update(-50.0, 30.0);
    
    EXPECT_DOUBLE_EQ(pos.getSize(), 0.0) << "Position should be flat after closing";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 0.0) << "Position price should be reset to 0";
    EXPECT_DOUBLE_EQ(result.opened, 0.0) << "Nothing should be opened";
    EXPECT_DOUBLE_EQ(result.closed, -50.0) << "All should be closed";
}

// 测试仓位符号变化
TEST(OriginalTests, Position_SignChanges) {
    Position pos;
    
    // 建立多头仓位
    pos.update(100.0, 10.0);
    EXPECT_GT(pos.getSize(), 0.0) << "Should have long position";
    
    // 转为空头仓位
    pos.update(-200.0, 15.0);
    EXPECT_LT(pos.getSize(), 0.0) << "Should have short position";
    EXPECT_DOUBLE_EQ(pos.getSize(), -100.0) << "Short position size should be correct";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 15.0) << "Short position price should be correct";
    
    // 转回多头仓位
    pos.update(150.0, 20.0);
    EXPECT_GT(pos.getSize(), 0.0) << "Should have long position again";
    EXPECT_DOUBLE_EQ(pos.getSize(), 50.0) << "Long position size should be correct";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 20.0) << "Long position price should be correct";
}

// 测试累积平均价格计算
TEST(OriginalTests, Position_AveragePriceCalculation) {
    Position pos;
    
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
        EXPECT_NEAR(pos.getPrice(), expected_avg_price, 1e-10) 
            << "Average price calculation should be accurate";
    }
    
    EXPECT_DOUBLE_EQ(pos.getSize(), total_size) << "Total size should match";
}

// 测试部分平仓
TEST(OriginalTests, Position_PartialClose) {
    Position pos(100.0, 15.0);
    
    // 部分平仓不改变价格
    auto result = pos.update(-30.0, 20.0);
    
    EXPECT_DOUBLE_EQ(pos.getSize(), 70.0) << "Remaining position should be correct";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 15.0) << "Price should remain unchanged";
    EXPECT_DOUBLE_EQ(result.opened, 0.0) << "Nothing opened in partial close";
    EXPECT_DOUBLE_EQ(result.closed, -30.0) << "Closed amount should match";
    
    // 再次部分平仓
    result = pos.update(-20.0, 18.0);
    
    EXPECT_DOUBLE_EQ(pos.getSize(), 50.0) << "Remaining position should be correct";
    EXPECT_DOUBLE_EQ(pos.getPrice(), 15.0) << "Price should still remain unchanged";
    EXPECT_DOUBLE_EQ(result.opened, 0.0) << "Nothing opened in second partial close";
    EXPECT_DOUBLE_EQ(result.closed, -20.0) << "Closed amount should match";
}

// 测试精度处理
TEST(OriginalTests, Position_PrecisionHandling) {
    Position pos;
    
    // 使用需要精确计算的价格
    pos.update(1.0/3.0, 1.0/7.0);
    pos.update(2.0/3.0, 2.0/7.0);
    
    // 验证计算精度
    double expected_size = 1.0;
    double expected_price = (1.0/3.0 * 1.0/7.0 + 2.0/3.0 * 2.0/7.0) / expected_size;
    
    EXPECT_NEAR(pos.getSize(), expected_size, 1e-15) << "Size calculation should be precise";
    EXPECT_NEAR(pos.getPrice(), expected_price, 1e-15) << "Price calculation should be precise";
}

// 测试极大数值
TEST(OriginalTests, Position_LargeNumbers) {
    Position pos;
    
    // 测试大数值
    double large_size = 1e10;
    double large_price = 1e6;
    
    pos.update(large_size, large_price);
    EXPECT_DOUBLE_EQ(pos.getSize(), large_size) << "Should handle large sizes";
    EXPECT_DOUBLE_EQ(pos.getPrice(), large_price) << "Should handle large prices";
    
    // 测试非常小的数值
    double small_size = 1e-10;
    double small_price = 1e-6;
    
    pos.update(small_size, small_price);
    
    // 验证加权平均计算
    double expected_total_value = large_size * large_price + small_size * small_price;
    double expected_total_size = large_size + small_size;
    double expected_avg_price = expected_total_value / expected_total_size;
    
    EXPECT_NEAR(pos.getPrice(), expected_avg_price, 1e-15) 
        << "Should handle mixed large and small numbers";
}

// 性能测试
TEST(OriginalTests, Position_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    Position pos;
    const int num_updates = 100000;
    
    // 执行大量仓位更新操作
    for (int i = 0; i < num_updates; ++i) {
        double size = (i % 2 == 0) ? 10.0 : -5.0;  // 交替买卖
        double price = 100.0 + i * 0.01;
        pos.update(size, price);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Position performance test: " << num_updates 
              << " updates in " << duration.count() << " ms" << std::endl;
    
    // 验证最终状态合理
    EXPECT_TRUE(std::isfinite(pos.getSize())) << "Final position size should be finite";
    EXPECT_TRUE(std::isfinite(pos.getPrice())) << "Final position price should be finite";
    
    // 性能要求
    EXPECT_LT(duration.count(), 1000) 
        << "Performance test should complete within 1 second";
}

// 边界条件测试
TEST(OriginalTests, Position_EdgeCases) {
    Position pos;
    
    // 测试零价格（某些特殊情况下可能出现）
    auto result = pos.update(100.0, 0.0);
    EXPECT_DOUBLE_EQ(pos.getPrice(), 0.0) << "Should handle zero price";
    EXPECT_DOUBLE_EQ(pos.getSize(), 100.0) << "Should handle zero price with correct size";
    
    // 从零价格仓位更新
    result = pos.update(50.0, 10.0);
    double expected_price = (100.0 * 0.0 + 50.0 * 10.0) / 150.0;
    EXPECT_DOUBLE_EQ(pos.getPrice(), expected_price) << "Should calculate average with zero price";
    
    // 重置仓位
    pos = Position();
    
    // 测试负价格（理论上不应该出现，但测试鲁棒性）
    result = pos.update(100.0, -5.0);
    EXPECT_DOUBLE_EQ(pos.getPrice(), -5.0) << "Should handle negative price";
    EXPECT_DOUBLE_EQ(pos.getSize(), 100.0) << "Should handle negative price with correct size";
}