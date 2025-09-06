/**
 * @file test_comminfo.cpp
 * @brief backtrader::CommissionInfo测试 - 对应Python test_comminfo.py
 * 
 * 原始Python测试:
 * - 测试股票类型的佣金计算
 * - 测试期货类型的佣金计算
 * - 验证各种佣金相关计算
 */

#include "test_common.h"
#include "comminfo.h"
#include "position.h"
#include <gtest/gtest.h>



// 测试股票类型的佣金计算
TEST(OriginalTests, CommInfo_Stocks) {
    double commission = 0.5;
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(commission);
    
    double price = 10.0;
    double cash = 10000.0;
    double size = 100.0;
    
    // 测试操作成本计算
    double opcost = comm->getoperationcost(size, price);
    EXPECT_DOUBLE_EQ(opcost, size * price) 
        << "Operation cost should equal size * price for stocks";
    
    // 测试持仓价值计算
    auto pos = std::make_shared<backtrader::Position>();
    pos->size = size;
    pos->price = price;
    double value = comm->getvalue(pos, price);
    EXPECT_DOUBLE_EQ(value, size * price) 
        << "backtrader::Position value should equal size * price for stocks";
    
    // 测试佣金计算
    double commcost = comm->getcommission(size, price);
    EXPECT_DOUBLE_EQ(commcost, size * price * commission) 
        << "Commission should equal size * price * commission rate";
    
    // 测试盈亏计算
    double newprice = 5.0;
    double pnl = comm->profitandloss(pos->size, pos->price, newprice);
    EXPECT_DOUBLE_EQ(pnl, pos->size * (newprice - price)) 
        << "P&L should equal size * price difference for stocks";
    
    // 测试现金调整
    double ca = comm->cashadjust(size, price, newprice);
    EXPECT_DOUBLE_EQ(ca, 0.0) 
        << "Cash adjust should be 0 for stocks";
}

// 测试期货类型的佣金计算
TEST(OriginalTests, CommInfo_Futures) {
    double commission = 0.5;
    double margin = 10.0;
    double mult = 10.0;
    
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(commission);
    comm->setMult(mult);
    comm->setMargin(margin);
    comm->setStocklike(false);  // 设置为期货类型
    
    double price = 10.0;
    double cash = 10000.0;
    double size = 100.0;
    
    // 测试操作成本计算
    double opcost = comm->getoperationcost(size, price);
    EXPECT_DOUBLE_EQ(opcost, size * margin) 
        << "Operation cost should equal size * margin for futures";
    
    // 测试持仓价值计算
    auto pos = std::make_shared<backtrader::Position>();
    pos->size = size;
    pos->price = price;
    double value = comm->getvalue(pos, price);
    EXPECT_DOUBLE_EQ(value, size * margin) 
        << "backtrader::Position value should equal size * margin for futures";
    
    // 测试佣金计算
    double commcost = comm->getcommission(size, price);
    EXPECT_DOUBLE_EQ(commcost, size * commission) 
        << "Commission should equal size * commission for futures";
    
    // 测试盈亏计算
    double newprice = 5.0;
    double pnl = comm->profitandloss(pos->size, pos->price, newprice);
    EXPECT_DOUBLE_EQ(pnl, pos->size * (newprice - price) * mult) 
        << "P&L should equal size * price difference * multiplier for futures";
    
    // 测试现金调整
    double ca = comm->cashadjust(size, price, newprice);
    EXPECT_DOUBLE_EQ(ca, size * (newprice - price) * mult) 
        << "Cash adjust should equal size * price difference * multiplier for futures";
}

// 测试百分比佣金
TEST(OriginalTests, CommInfo_PercentageCommission) {
    double commission = 0.001;  // 0.1%
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(commission);
    comm->setPercent(true);
    
    double price = 100.0;
    double size = 50.0;
    
    // 测试佣金计算
    double commcost = comm->getcommission(size, price);
    EXPECT_DOUBLE_EQ(commcost, size * price * commission) 
        << "Percentage commission should be calculated correctly";
}

// 测试固定佣金
TEST(OriginalTests, CommInfo_FixedCommission) {
    double commission = 5.0;  // 每笔交易5元
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(commission);
    comm->setPercent(false);
    comm->setCommtype(backtrader::CommissionInfo::CommType::Fixed);
    
    double price = 100.0;
    double size = 50.0;
    
    // 测试佣金计算
    double commcost = comm->getcommission(size, price);
    EXPECT_DOUBLE_EQ(commcost, commission) 
        << "Fixed commission should be constant regardless of size/price";
}

// 测试混合参数情况
TEST(OriginalTests, CommInfo_MixedParameters) {
    // 测试期货类型但有不同的参数组合
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    
    // 情况1：高保证金低乘数
    comm->setCommission(0.2);
    comm->setMargin(100.0);
    comm->setMult(1.0);
    comm->setStocklike(false);
    
    double price = 50.0;
    double size = 10.0;
    
    double opcost = comm->getoperationcost(size, price);
    EXPECT_DOUBLE_EQ(opcost, size * 100.0) 
        << "High margin operation cost calculation";
    
    // 情况2：低保证金高乘数
    comm->setMargin(5.0);
    comm->setMult(100.0);
    
    opcost = comm->getoperationcost(size, price);
    EXPECT_DOUBLE_EQ(opcost, size * 5.0) 
        << "Low margin operation cost calculation";
    
    double newprice = 55.0;
    double pnl = comm->profitandloss(size, price, newprice);
    EXPECT_DOUBLE_EQ(pnl, size * (newprice - price) * 100.0) 
        << "High multiplier P&L calculation";
}

// 测试边界条件
TEST(OriginalTests, CommInfo_EdgeCases) {
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(0.5);
    
    // 测试零大小
    double price = 100.0;
    double size = 0.0;
    
    double opcost = comm->getoperationcost(size, price);
    EXPECT_DOUBLE_EQ(opcost, 0.0) 
        << "Zero size should result in zero operation cost";
    
    double commcost = comm->getcommission(size, price);
    EXPECT_DOUBLE_EQ(commcost, 0.0) 
        << "Zero size should result in zero commission";
    
    // 测试零价格
    size = 100.0;
    price = 0.0;
    
    opcost = comm->getoperationcost(size, price);
    EXPECT_DOUBLE_EQ(opcost, 0.0) 
        << "Zero price should result in zero operation cost for stocks";
    
    // 测试负数大小（卖出）
    size = -100.0;
    price = 50.0;
    
    commcost = comm->getcommission(size, price);
    EXPECT_GT(commcost, 0.0) 
        << "Commission should be positive for negative size (sell)";
    EXPECT_DOUBLE_EQ(commcost, std::abs(size) * price * 0.5) 
        << "Commission calculation should use absolute size";
}

// 测试利息计算
TEST(OriginalTests, CommInfo_Interest) {
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setInterest(0.05);  // 5%年利率
    comm->setInterest_long(true);
    
    double price = 100.0;
    double size = 100.0;
    int days = 30;
    
    auto pos = std::make_shared<backtrader::Position>();
    pos->size = size;
    pos->price = price;
    
    double interest = comm->get_credit_interest(pos, price, days);
    double expected_interest = size * price * 0.05 * days / 365.0;
    EXPECT_NEAR(interest, expected_interest, 0.01) 
        << "Interest calculation should be correct";
}

// 测试杠杆计算
TEST(OriginalTests, CommInfo_Leverage) {
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setLeverage(2.0);  // 2倍杠杆
    
    double price = 100.0;
    double size = 100.0;
    
    double opcost = comm->getoperationcost(size, price);
    double expected_cost = size * price / 2.0;  // 2倍杠杆，只需要50%的资金
    EXPECT_DOUBLE_EQ(opcost, expected_cost) 
        << "Leverage should reduce operation cost";
}

// 综合测试
TEST(OriginalTests, CommInfo_Comprehensive) {
    // 创建一个完整的交易场景
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(0.002);  // 0.2%佣金
    comm->setPercent(true);
    comm->setMinimum(5.0);  // 最小佣金5元
    
    // 小额交易测试最小佣金
    double small_size = 10.0;
    double small_price = 10.0;
    double small_comm = comm->getcommission(small_size, small_price);
    EXPECT_DOUBLE_EQ(small_comm, 5.0) 
        << "Small trade should trigger minimum commission";
    
    // 大额交易测试百分比佣金
    double large_size = 1000.0;
    double large_price = 100.0;
    double large_comm = comm->getcommission(large_size, large_price);
    double expected_comm = large_size * large_price * 0.002;
    EXPECT_DOUBLE_EQ(large_comm, expected_comm) 
        << "Large trade should use percentage commission";
}

// 性能测试
TEST(OriginalTests, CommInfo_Performance) {
    auto comm = std::make_shared<backtrader::CommissionInfo>();
    comm->setCommission(0.5);
    comm->setMult(10.0);
    comm->setMargin(10.0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 执行大量计算
    const int iterations = 100000;
    double total = 0.0;
    for (int i = 0; i < iterations; ++i) {
        double price = 50.0 + (i % 100) * 0.1;
        double size = 100.0 + (i % 50);
        
        total += comm->getoperationcost(size, price);
        total += comm->getcommission(size, price);
        total += comm->profitandloss(size, price, price * 1.1);
        total += comm->cashadjust(size, price, price * 0.9);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "backtrader::CommissionInfo performance test: " << iterations 
              << " iterations took " << duration.count() << " ms" << std::endl;
    std::cout << "Average time per calculation: " 
              << (duration.count() * 1000.0 / (iterations * 4)) << " microseconds" << std::endl;
    
    // 确保计算被使用，避免编译器优化
    EXPECT_GT(total, 0.0) << "Total should be positive";
    
    // 性能要求
    EXPECT_LT(duration.count(), 1000) 
        << "Performance test should complete within 1 second";
}