/**
 * @file test_trade.cpp
 * @brief 交易测试 - 对应Python test_trade.py
 * 
 * 原始Python测试:
 * - 测试交易对象的创建和更新
 * - 验证交易的开仓、加仓、减仓和平仓逻辑
 * - 测试交易价格和手续费的计算
 * - 验证交易状态的正确转换
 */

#include "test_common.h"
#include "broker/Trade.h"
#include "broker/Order.h"
#include "broker/CommInfo.h"
#include "data/LineRoot.h"
#include <memory>
#include <cassert>

using namespace backtrader::tests::original;

// 模拟手续费信息类
class FakeCommInfo : public CommInfo {
public:
    FakeCommInfo() : CommInfo() {}

    double getValueSize(double size, double price) override {
        (void)size; (void)price;
        return 0.0;
    }

    double profitAndLoss(double size, double price, double newPrice) override {
        (void)size; (void)price; (void)newPrice;
        return 0.0;
    }

    double getOperationCost(double size, double price) override {
        (void)size; (void)price;
        return 0.0;
    }

    double getCommission(double size, double price) override {
        (void)size; (void)price;
        return 0.0;
    }
};

// 模拟数据类
class FakeData : public LineRoot {
public:
    FakeData() : LineRoot(1, "fake_data") {
        forward(0.0);  // datetime
        addLine("close");
        getLine("close")->forward(0.0);
    }

    size_t size() const override { return 0; }

    double datetime(int ago = 0) const override {
        (void)ago;
        return 0.0;
    }

    double close(int ago = 0) const override {
        (void)ago;
        return 0.0;
    }
};

// 完整的Python测试用例复现
TEST(OriginalTests, Trade_PythonTestReplication) {
    // 创建交易对象
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    
    // 创建订单
    Order order(fakeData, OrderType::Buy, 0, 1.0, ExecutionType::Market, true);
    
    // 第一次更新：开仓
    double commrate = 0.025;
    double size = 10.0;
    double price = 10.0;
    double value = size * price;
    double commission = value * commrate;
    
    auto commInfo = std::make_shared<FakeCommInfo>();
    tr.update(order, size, price, value, commission, 0.0, commInfo);
    
    // 验证第一次更新后的状态
    EXPECT_FALSE(tr.isClosed()) << "Trade should not be closed after opening";
    EXPECT_DOUBLE_EQ(tr.getSize(), size) << "Trade size should match";
    EXPECT_DOUBLE_EQ(tr.getPrice(), price) << "Trade price should match";
    EXPECT_DOUBLE_EQ(tr.getCommission(), commission) << "Trade commission should match";
    EXPECT_DOUBLE_EQ(tr.getPnl(), 0.0) << "Trade PnL should be 0";
    EXPECT_DOUBLE_EQ(tr.getPnlComm(), tr.getPnl() - tr.getCommission()) 
        << "PnL with commission should be correct";
    
    // 第二次更新：减仓
    double upsize = -5.0;
    double upprice = 12.5;
    double upvalue = upsize * upprice;
    double upcomm = abs(value) * commrate;
    
    tr.update(order, upsize, upprice, upvalue, upcomm, 0.0, commInfo);
    
    // 验证减仓后的状态
    EXPECT_FALSE(tr.isClosed()) << "Trade should not be closed after partial close";
    EXPECT_DOUBLE_EQ(tr.getSize(), size + upsize) << "Trade size should be reduced";
    EXPECT_DOUBLE_EQ(tr.getPrice(), price) << "Trade price should not change when reducing";
    EXPECT_DOUBLE_EQ(tr.getCommission(), commission + upcomm) << "Commission should be accumulated";
    
    // 第三次更新：加仓
    size = tr.getSize();
    price = tr.getPrice();
    commission = tr.getCommission();
    
    upsize = 7.0;
    upprice = 14.5;
    upvalue = upsize * upprice;
    upcomm = abs(value) * commrate;
    
    tr.update(order, upsize, upprice, upvalue, upcomm, 0.0, commInfo);
    
    // 验证加仓后的状态
    EXPECT_FALSE(tr.isClosed()) << "Trade should not be closed after adding position";
    EXPECT_DOUBLE_EQ(tr.getSize(), size + upsize) << "Trade size should be increased";
    
    // 计算期望的加权平均价格
    double expected_price = ((size * price) + (upsize * upprice)) / (size + upsize);
    EXPECT_DOUBLE_EQ(tr.getPrice(), expected_price) << "Trade price should be weighted average";
    EXPECT_DOUBLE_EQ(tr.getCommission(), commission + upcomm) << "Commission should be accumulated";
    
    // 第四次更新：完全平仓
    size = tr.getSize();
    price = tr.getPrice();
    commission = tr.getCommission();
    
    upsize = -size;
    upprice = 12.5;
    upvalue = upsize * upprice;
    upcomm = abs(value) * commrate;
    
    tr.update(order, upsize, upprice, upvalue, upcomm, 0.0, commInfo);
    
    // 验证平仓后的状态
    EXPECT_TRUE(tr.isClosed()) << "Trade should be closed after full close";
    EXPECT_DOUBLE_EQ(tr.getSize(), size + upsize) << "Trade size should be 0";
    EXPECT_DOUBLE_EQ(tr.getPrice(), price) << "Trade price should not change when closing";
    EXPECT_DOUBLE_EQ(tr.getCommission(), commission + upcomm) << "Commission should be accumulated";
}

// 测试交易状态
TEST(OriginalTests, Trade_Status) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 初始状态
    EXPECT_FALSE(tr.isClosed()) << "New trade should not be closed";
    EXPECT_FALSE(tr.isOpen()) << "New trade should not be open";
    EXPECT_DOUBLE_EQ(tr.getSize(), 0.0) << "New trade should have zero size";
    
    // 开仓
    tr.update(order, 100.0, 10.0, 1000.0, 25.0, 0.0, commInfo);
    EXPECT_TRUE(tr.isOpen()) << "Trade should be open after initial update";
    EXPECT_FALSE(tr.isClosed()) << "Trade should not be closed after opening";
    
    // 完全平仓
    tr.update(order, -100.0, 12.0, -1200.0, 30.0, 200.0, commInfo);
    EXPECT_TRUE(tr.isClosed()) << "Trade should be closed after full close";
    EXPECT_FALSE(tr.isOpen()) << "Trade should not be open after closing";
}

// 测试交易盈亏计算
TEST(OriginalTests, Trade_PnLCalculation) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 开仓：买入100股，每股10元
    tr.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, commInfo);
    
    // 平仓：卖出100股，每股12元，盈利200元
    double pnl = 200.0;
    tr.update(order, -100.0, 12.0, -1200.0, 12.0, pnl, commInfo);
    
    EXPECT_DOUBLE_EQ(tr.getPnl(), pnl) << "PnL should match";
    EXPECT_DOUBLE_EQ(tr.getPnlComm(), pnl - tr.getCommission()) 
        << "PnL with commission should be correct";
    EXPECT_TRUE(tr.isClosed()) << "Trade should be closed";
}

// 测试多次部分交易
TEST(OriginalTests, Trade_MultiplePartialTrades) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 分批建仓
    std::vector<std::tuple<double, double, double>> positions = {
        {30.0, 10.0, 3.0},   // 30股，10元，3元手续费
        {40.0, 11.0, 4.4},   // 40股，11元，4.4元手续费
        {30.0, 9.0, 2.7}     // 30股，9元，2.7元手续费
    };
    
    double total_size = 0.0;
    double total_value = 0.0;
    double total_commission = 0.0;
    
    for (const auto& pos : positions) {
        double size = std::get<0>(pos);
        double price = std::get<1>(pos);
        double comm = std::get<2>(pos);
        
        tr.update(order, size, price, size * price, comm, 0.0, commInfo);
        
        total_size += size;
        total_value += size * price;
        total_commission += comm;
        
        EXPECT_DOUBLE_EQ(tr.getSize(), total_size) << "Trade size should accumulate";
        EXPECT_DOUBLE_EQ(tr.getCommission(), total_commission) << "Commission should accumulate";
        EXPECT_FALSE(tr.isClosed()) << "Trade should remain open";
    }
    
    // 验证加权平均价格
    double expected_avg_price = total_value / total_size;
    EXPECT_NEAR(tr.getPrice(), expected_avg_price, 1e-10) << "Average price should be correct";
}

// 测试交易方向
TEST(OriginalTests, Trade_Direction) {
    auto fakeData = std::make_shared<FakeData>();
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 测试多头交易
    Trade longTrade(fakeData);
    Order buyOrder(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    longTrade.update(buyOrder, 100.0, 10.0, 1000.0, 10.0, 0.0, commInfo);
    
    EXPECT_TRUE(longTrade.isLong()) << "Should be long trade";
    EXPECT_FALSE(longTrade.isShort()) << "Should not be short trade";
    
    // 测试空头交易
    Trade shortTrade(fakeData);
    Order sellOrder(fakeData, OrderType::Sell, -100, 1.0, ExecutionType::Market, true);
    shortTrade.update(sellOrder, -100.0, 10.0, -1000.0, 10.0, 0.0, commInfo);
    
    EXPECT_TRUE(shortTrade.isShort()) << "Should be short trade";
    EXPECT_FALSE(shortTrade.isLong()) << "Should not be long trade";
}

// 测试交易时间
TEST(OriginalTests, Trade_Timing) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 开仓时间
    double open_time = 20230101.0;
    tr.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, commInfo);
    tr.setOpenTime(open_time);
    
    EXPECT_DOUBLE_EQ(tr.getOpenTime(), open_time) << "Open time should be set";
    EXPECT_DOUBLE_EQ(tr.getCloseTime(), 0.0) << "Close time should be 0 for open trade";
    
    // 平仓时间
    double close_time = 20230102.0;
    tr.update(order, -100.0, 12.0, -1200.0, 12.0, 200.0, commInfo);
    tr.setCloseTime(close_time);
    
    EXPECT_DOUBLE_EQ(tr.getCloseTime(), close_time) << "Close time should be set";
    EXPECT_GT(tr.getCloseTime(), tr.getOpenTime()) << "Close time should be after open time";
}

// 测试交易历史记录
TEST(OriginalTests, Trade_History) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 记录交易历史
    tr.update(order, 50.0, 10.0, 500.0, 5.0, 0.0, commInfo);
    tr.update(order, 50.0, 11.0, 550.0, 5.5, 0.0, commInfo);
    tr.update(order, -30.0, 12.0, -360.0, 3.6, 60.0, commInfo);
    tr.update(order, -70.0, 13.0, -910.0, 9.1, 210.0, commInfo);
    
    // 验证历史统计
    const auto& history = tr.getHistory();
    EXPECT_GE(history.size(), 4) << "Should have at least 4 history entries";
    
    // 验证最终状态
    EXPECT_TRUE(tr.isClosed()) << "Trade should be closed";
    EXPECT_DOUBLE_EQ(tr.getSize(), 0.0) << "Final size should be 0";
}

// 测试交易复制
TEST(OriginalTests, Trade_Copy) {
    auto fakeData = std::make_shared<FakeData>();
    Trade originalTrade(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 设置原始交易
    originalTrade.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, commInfo);
    originalTrade.setOpenTime(20230101.0);
    
    // 复制交易
    Trade copiedTrade = originalTrade;
    
    // 验证复制的正确性
    EXPECT_DOUBLE_EQ(copiedTrade.getSize(), originalTrade.getSize()) << "Size should match";
    EXPECT_DOUBLE_EQ(copiedTrade.getPrice(), originalTrade.getPrice()) << "Price should match";
    EXPECT_DOUBLE_EQ(copiedTrade.getCommission(), originalTrade.getCommission()) << "Commission should match";
    EXPECT_EQ(copiedTrade.isClosed(), originalTrade.isClosed()) << "Status should match";
    EXPECT_DOUBLE_EQ(copiedTrade.getOpenTime(), originalTrade.getOpenTime()) << "Open time should match";
}

// 测试交易的边界条件
TEST(OriginalTests, Trade_EdgeCases) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 零价格交易
    tr.update(order, 100.0, 0.0, 0.0, 0.0, 0.0, commInfo);
    EXPECT_DOUBLE_EQ(tr.getPrice(), 0.0) << "Should handle zero price";
    EXPECT_DOUBLE_EQ(tr.getSize(), 100.0) << "Size should be correct with zero price";
    
    // 重置交易
    tr = Trade(fakeData);
    
    // 负价格交易（理论测试）
    tr.update(order, 100.0, -5.0, -500.0, 5.0, 0.0, commInfo);
    EXPECT_DOUBLE_EQ(tr.getPrice(), -5.0) << "Should handle negative price";
    EXPECT_DOUBLE_EQ(tr.getSize(), 100.0) << "Size should be correct with negative price";
}

// 测试交易统计
TEST(OriginalTests, Trade_Statistics) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 完整的交易周期
    tr.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, commInfo);  // 开仓
    tr.update(order, -100.0, 15.0, -1500.0, 15.0, 500.0, commInfo);  // 平仓
    
    // 统计信息
    EXPECT_DOUBLE_EQ(tr.getGrossProfit(), 500.0) << "Gross profit should be correct";
    EXPECT_DOUBLE_EQ(tr.getNetProfit(), 500.0 - 25.0) << "Net profit should account for commission";
    EXPECT_DOUBLE_EQ(tr.getReturnOnInvestment(), (500.0 - 25.0) / 1000.0) 
        << "ROI should be correct";
}

// 性能测试
TEST(OriginalTests, Trade_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto fakeData = std::make_shared<FakeData>();
    auto commInfo = std::make_shared<FakeCommInfo>();
    const int num_trades = 10000;
    
    std::vector<Trade> trades;
    trades.reserve(num_trades);
    
    // 创建大量交易
    for (int i = 0; i < num_trades; ++i) {
        Trade tr(fakeData);
        Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
        
        tr.update(order, 100.0, 10.0 + i * 0.01, 1000.0, 10.0, 0.0, commInfo);
        tr.update(order, -100.0, 12.0 + i * 0.01, -1200.0, 12.0, 200.0, commInfo);
        
        trades.push_back(std::move(tr));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Trade performance test: created and processed " << num_trades 
              << " trades in " << duration.count() << " ms" << std::endl;
    
    // 验证所有交易都正确创建
    EXPECT_EQ(trades.size(), num_trades) << "Should create all trades";
    
    // 验证所有交易都已关闭
    for (const auto& trade : trades) {
        EXPECT_TRUE(trade.isClosed()) << "All trades should be closed";
    }
    
    // 性能要求
    EXPECT_LT(duration.count(), 1000) 
        << "Performance test should complete within 1 second";
}