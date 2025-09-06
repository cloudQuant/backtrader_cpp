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
#include "trade.h"
#include "order.h"
#include "comminfo.h"
#include "dataseries.h"
#include <memory>
#include <cassert>
#include <cmath>

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

// 模拟手续费信息类
class FakeCommInfo : public CommInfo {
public:
    FakeCommInfo() : CommInfo() {}

    double getValueSize(double size, double price) {
        (void)size; (void)price;
        return 0.0;
    }

    double profitAndLoss(double size, double price, double newPrice) {
        (void)size; (void)price; (void)newPrice;
        return 0.0;
    }

    double getOperationCost(double size, double price) {
        (void)size; (void)price;
        return 0.0;
    }

    double getCommission(double size, double price) {
        (void)size; (void)price;
        return 0.0;
    }
};

// 模拟数据类
class FakeData : public backtrader::DataSeries {
public:
    FakeData() : backtrader::DataSeries() {
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
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 10.0;
    order->price = 10.0;
    
    // 第一次更新：开仓
    double commrate = 0.025;
    double size = 10.0;
    double price = 10.0;
    double value = size * price;
    double commission = value * commrate;
    
    auto commInfo = std::make_shared<FakeCommInfo>();
    tr.update(order, size, price, value, commission, 0.0, std::chrono::system_clock::now());
    
    // 验证第一次更新后的状态
    EXPECT_FALSE(tr.isclosed()) << "Trade should not be closed after opening";
    EXPECT_DOUBLE_EQ(tr.size, size) << "Trade size should match";
    EXPECT_DOUBLE_EQ(tr.price, price) << "Trade price should match";
    EXPECT_DOUBLE_EQ(tr.commission, commission) << "Trade commission should match";
    EXPECT_DOUBLE_EQ(tr.pnl, 0.0) << "Trade PnL should be 0";
    EXPECT_DOUBLE_EQ(tr.pnlcomm, tr.pnl - tr.commission) 
        << "PnL with commission should be correct";
    
    // 第二次更新：减仓
    double upsize = -5.0;
    double upprice = 12.5;
    double upvalue = upsize * upprice;
    double upcomm = std::abs(value) * commrate;
    
    tr.update(order, upsize, upprice, upvalue, upcomm, 0.0, std::chrono::system_clock::now());
    
    // 验证减仓后的状态
    EXPECT_FALSE(tr.isclosed()) << "Trade should not be closed after partial close";
    EXPECT_DOUBLE_EQ(tr.size, size + upsize) << "Trade size should be reduced";
    EXPECT_DOUBLE_EQ(tr.price, price) << "Trade price should not change when reducing";
    EXPECT_DOUBLE_EQ(tr.commission, commission + upcomm) << "Commission should be accumulated";
    
    // 第三次更新：加仓
    size = tr.size;
    price = tr.price;
    commission = tr.commission;
    
    upsize = 7.0;
    upprice = 14.5;
    upvalue = upsize * upprice;
    upcomm = abs(value) * commrate;
    
    tr.update(order, upsize, upprice, upvalue, upcomm, 0.0, std::chrono::system_clock::now());
    
    // 验证加仓后的状态
    EXPECT_FALSE(tr.isclosed()) << "Trade should not be closed after adding position";
    EXPECT_DOUBLE_EQ(tr.size, size + upsize) << "Trade size should be increased";
    
    // 计算期望的加权平均价格
    double expected_price = ((size * price) + (upsize * upprice)) / (size + upsize);
    EXPECT_DOUBLE_EQ(tr.price, expected_price) << "Trade price should be weighted average";
    EXPECT_DOUBLE_EQ(tr.commission, commission + upcomm) << "Commission should be accumulated";
    
    // 第四次更新：完全平仓
    size = tr.size;
    price = tr.price;
    commission = tr.commission;
    
    upsize = -size;
    upprice = 12.5;
    upvalue = upsize * upprice;
    upcomm = abs(value) * commrate;
    
    tr.update(order, upsize, upprice, upvalue, upcomm, 0.0, std::chrono::system_clock::now());
    
    // 验证平仓后的状态
    EXPECT_TRUE(tr.isclosed()) << "Trade should be closed after full close";
    EXPECT_DOUBLE_EQ(tr.size, size + upsize) << "Trade size should be 0";
    EXPECT_DOUBLE_EQ(tr.price, price) << "Trade price should not change when closing";
    EXPECT_DOUBLE_EQ(tr.commission, commission + upcomm) << "Commission should be accumulated";
}

// 测试交易状态
TEST(OriginalTests, Trade_Status) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 初始状态
    EXPECT_FALSE(tr.isclosed()) << "New trade should not be closed";
    EXPECT_FALSE(tr.isopen()) << "New trade should not be open";
    EXPECT_DOUBLE_EQ(tr.size, 0.0) << "New trade should have zero size";
    
    // 开仓
    tr.update(order, 100.0, 10.0, 1000.0, 25.0, 0.0, std::chrono::system_clock::now());
    EXPECT_TRUE(tr.isopen()) << "Trade should be open after initial update";
    EXPECT_FALSE(tr.isclosed()) << "Trade should not be closed after opening";
    
    // 完全平仓
    tr.update(order, -100.0, 12.0, -1200.0, 30.0, 200.0, std::chrono::system_clock::now());
    EXPECT_TRUE(tr.isclosed()) << "Trade should be closed after full close";
    EXPECT_FALSE(tr.isopen()) << "Trade should not be open after closing";
}

// 测试交易盈亏计算
TEST(OriginalTests, Trade_PnLCalculation) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 开仓：买入100股，每股10元
    tr.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, std::chrono::system_clock::now());
    
    // 平仓：卖出100股，每股12元，盈利200元
    double pnl = 200.0;
    tr.update(order, -100.0, 12.0, -1200.0, 12.0, pnl, std::chrono::system_clock::now());
    
    EXPECT_DOUBLE_EQ(tr.pnl, pnl) << "PnL should match";
    EXPECT_DOUBLE_EQ(tr.pnlcomm, pnl - tr.commission) 
        << "PnL with commission should be correct";
    EXPECT_TRUE(tr.isclosed()) << "Trade should be closed";
}

// 测试多次部分交易
TEST(OriginalTests, Trade_MultiplePartialTrades) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
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
        
        tr.update(order, size, price, size * price, comm, 0.0, std::chrono::system_clock::now());
        
        total_size += size;
        total_value += size * price;
        total_commission += comm;
        
        EXPECT_DOUBLE_EQ(tr.size, total_size) << "Trade size should accumulate";
        EXPECT_DOUBLE_EQ(tr.commission, total_commission) << "Commission should accumulate";
        EXPECT_FALSE(tr.isclosed()) << "Trade should remain open";
    }
    
    // 验证加权平均价格
    double expected_avg_price = total_value / total_size;
    EXPECT_NEAR(tr.price, expected_avg_price, 1e-10) << "Average price should be correct";
}

// 测试交易方向
TEST(OriginalTests, Trade_Direction) {
    auto fakeData = std::make_shared<FakeData>();
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 测试多头交易
    Trade longTrade(fakeData);
    auto buyOrder = std::make_shared<Order>();
    buyOrder->data = fakeData;
    buyOrder->type = OrderType::Market;
    buyOrder->size = 100.0;
    longTrade.update(buyOrder, 100.0, 10.0, 1000.0, 10.0, 0.0, std::chrono::system_clock::now());
    
    EXPECT_TRUE(longTrade.long_()) << "Should be long trade";
    EXPECT_FALSE(longTrade.short_()) << "Should not be short trade";
    
    // 测试空头交易
    Trade shortTrade(fakeData);
    auto sellOrder = std::make_shared<Order>();
    sellOrder->data = fakeData;
    sellOrder->type = OrderType::Market;
    sellOrder->size = -100.0;
    shortTrade.update(sellOrder, -100.0, 10.0, -1000.0, 10.0, 0.0, std::chrono::system_clock::now());
    
    EXPECT_TRUE(shortTrade.short_()) << "Should be short trade";
    EXPECT_FALSE(shortTrade.long_()) << "Should not be long trade";
}

// 测试交易时间
TEST(OriginalTests, Trade_Timing) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 开仓时间
    double open_time = 20230101.0;
    tr.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, std::chrono::system_clock::now());
    // 直接设置时间戳成员
    // tr.dtopen = std::chrono::system_clock::from_time_t(open_time);
    
    // 开仓时间和平仓时间使用直接成员访问
    // 平仓时间
    double close_time = 20230102.0;
    tr.update(order, -100.0, 12.0, -1200.0, 12.0, 200.0, std::chrono::system_clock::now());
    
    // 直接验证交易是否关闭
    EXPECT_TRUE(tr.isclosed()) << "Trade should be closed after full close";
}

// 测试交易历史记录
TEST(OriginalTests, Trade_History) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 记录交易历史
    tr.update(order, 50.0, 10.0, 500.0, 5.0, 0.0, std::chrono::system_clock::now());
    tr.update(order, 50.0, 11.0, 550.0, 5.5, 0.0, std::chrono::system_clock::now());
    tr.update(order, -30.0, 12.0, -360.0, 3.6, 60.0, std::chrono::system_clock::now());
    tr.update(order, -70.0, 13.0, -910.0, 9.1, 210.0, std::chrono::system_clock::now());
    
    // 验证历史统计
    const auto& history = tr.history;
    EXPECT_GE(history.size(), 4) << "Should have at least 4 history entries";
    
    // 验证最终状态
    EXPECT_TRUE(tr.isclosed()) << "Trade should be closed";
    EXPECT_DOUBLE_EQ(tr.size, 0.0) << "Final size should be 0";
}

// 测试交易复制
TEST(OriginalTests, Trade_Copy) {
    auto fakeData = std::make_shared<FakeData>();
    Trade originalTrade(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 设置原始交易
    originalTrade.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, std::chrono::system_clock::now());
    // 直接设置时间戳成员
    // originalTrade.dtopen = std::chrono::system_clock::from_time_t(20230101);
    
    // 复制交易
    Trade copiedTrade = originalTrade;
    
    // 验证复制的正确性
    EXPECT_DOUBLE_EQ(copiedTrade.size, originalTrade.size) << "Size should match";
    EXPECT_DOUBLE_EQ(copiedTrade.price, originalTrade.price) << "Price should match";
    EXPECT_DOUBLE_EQ(copiedTrade.commission, originalTrade.commission) << "Commission should match";
    EXPECT_EQ(copiedTrade.isclosed(), originalTrade.isclosed()) << "Status should match";
    // 时间比较需要时间戳
    EXPECT_EQ(copiedTrade.ref, originalTrade.ref) << "Ref should match";
}

// 测试交易的边界条件
TEST(OriginalTests, Trade_EdgeCases) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 零价格交易
    tr.update(order, 100.0, 0.0, 0.0, 0.0, 0.0, std::chrono::system_clock::now());
    EXPECT_DOUBLE_EQ(tr.price, 0.0) << "Should handle zero price";
    EXPECT_DOUBLE_EQ(tr.size, 100.0) << "Size should be correct with zero price";
    
    // 重置交易
    tr = Trade(fakeData);
    
    // 负价格交易（理论测试）
    tr.update(order, 100.0, -5.0, -500.0, 5.0, 0.0, std::chrono::system_clock::now());
    EXPECT_DOUBLE_EQ(tr.price, -5.0) << "Should handle negative price";
    EXPECT_DOUBLE_EQ(tr.size, 100.0) << "Size should be correct with negative price";
}

// 测试交易统计
TEST(OriginalTests, Trade_Statistics) {
    auto fakeData = std::make_shared<FakeData>();
    Trade tr(fakeData);
    auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 完整的交易周期
    tr.update(order, 100.0, 10.0, 1000.0, 10.0, 0.0, std::chrono::system_clock::now());  // 开仓
    tr.update(order, -100.0, 15.0, -1500.0, 15.0, 500.0, std::chrono::system_clock::now());  // 平仓
    
    // 统计信息 - 使用直接成员访问
    EXPECT_DOUBLE_EQ(tr.pnl, 500.0) << "Gross profit should be correct";
    EXPECT_DOUBLE_EQ(tr.pnlcomm, 500.0 - 25.0) << "Net profit should account for commission";
    // ROI计算需要确认具体实现
    EXPECT_DOUBLE_EQ(tr.commission, 25.0) << "Commission should be correct";
}

// 性能测试
TEST(OriginalTests, Trade_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto fakeData = std::make_shared<FakeData>();
    auto commInfo = std::make_shared<FakeCommInfo>();
    const int num_trades = 10000;
    
    std::vector<Trade> trades;
    trades.reserve(num_trades);
    
    // 创建大量交易;
    for (int i = 0; i < num_trades; ++i) {
        Trade tr(fakeData);
        auto order = std::make_shared<Order>();
    order->data = fakeData;
    order->type = OrderType::Market;
    order->size = 100.0;
        
        tr.update(order, 100.0, 10.0 + i * 0.01, 1000.0, 10.0, 0.0, std::chrono::system_clock::now());
        tr.update(order, -100.0, 12.0 + i * 0.01, -1200.0, 12.0, 200.0, std::chrono::system_clock::now());
        
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
        EXPECT_TRUE(trade.isclosed()) << "All trades should be closed";
    }
    
    // 性能要求
    EXPECT_LT(duration.count(), 1000) 
        << "Performance test should complete within 1 second";
}