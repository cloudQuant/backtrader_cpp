#include <gtest/gtest.h>
#include "strategy/StrategyBase.h"
#include "data/DataFeed.h"
#include "indicators/SMA.h"
#include "test_utils/TestDataProvider.h"

using namespace backtrader;
using namespace backtrader::strategy;
using namespace backtrader::data;

// 简单的测试策略
class TestStrategy : public StrategyBase {
private:
    std::shared_ptr<SMA> sma_fast_;
    std::shared_ptr<SMA> sma_slow_;
    
public:
    TestStrategy() : StrategyBase("TestStrategy") {}
    
    void init() override {
        auto data = getData();
        if (data) {
            sma_fast_ = std::make_shared<SMA>(data->close(), 5);
            sma_slow_ = std::make_shared<SMA>(data->close(), 10);
            
            addIndicator(sma_fast_);
            addIndicator(sma_slow_);
        }
    }
    
    void next() override {
        if (!sma_fast_ || !sma_slow_) return;
        
        double fast_value = sma_fast_->get(0);
        double slow_value = sma_slow_->get(0);
        
        if (isNaN(fast_value) || isNaN(slow_value)) return;
        
        // 简单的金叉死叉策略
        if (isEmpty()) {
            if (fast_value > slow_value) {
                buy(1.0);  // 金叉买入
            }
        } else if (isLong()) {
            if (fast_value < slow_value) {
                sell(getPositionSize());  // 死叉卖出
            }
        }
    }
};

class StrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试数据
        auto test_data = createTestOHLCVData(100, 100.0, 0.02);
        data_feed_ = std::make_unique<StaticDataFeed>(test_data, "TestData");
        
        strategy_ = std::make_unique<TestStrategy>();
        strategy_->addDataFeed(data_feed_.get());
    }
    
    std::unique_ptr<StaticDataFeed> data_feed_;
    std::unique_ptr<TestStrategy> strategy_;
};

TEST_F(StrategyTest, BasicSetup) {
    EXPECT_EQ(strategy_->getName(), "TestStrategy");
    EXPECT_NE(strategy_->getData(), nullptr);
    EXPECT_TRUE(strategy_->isEmpty());
    EXPECT_EQ(strategy_->getTotalPnL(), 0.0);
    EXPECT_EQ(strategy_->getTradeCount(), 0);
}

TEST_F(StrategyTest, ParameterManagement) {
    strategy_->setParam("test_param", 123.45);
    EXPECT_DOUBLE_EQ(strategy_->getParam("test_param"), 123.45);
    EXPECT_DOUBLE_EQ(strategy_->getParam("non_existent", 99.9), 99.9);
}

TEST_F(StrategyTest, OrderCreation) {
    // 加载一些数据
    data_feed_->loadBatch(20);
    
    // 创建买入订单
    size_t order_id = strategy_->buy(10.0);
    EXPECT_GT(order_id, 0);
    
    // 检查订单
    const Order* order = strategy_->getOrder(order_id);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->type, OrderType::MARKET);
    EXPECT_EQ(order->side, OrderSide::BUY);
    EXPECT_EQ(order->status, OrderStatus::COMPLETED);
    EXPECT_DOUBLE_EQ(order->size, 10.0);
    EXPECT_DOUBLE_EQ(order->executed_size, 10.0);
    
    // 检查持仓
    EXPECT_TRUE(strategy_->isLong());
    EXPECT_DOUBLE_EQ(strategy_->getPositionSize(), 10.0);
}

TEST_F(StrategyTest, OrderCancellation) {
    // 创建限价单（不会立即执行）
    size_t order_id = strategy_->buyLimit(50.0, 5.0);  // 远低于市价的限价单
    
    const Order* order = strategy_->getOrder(order_id);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->status, OrderStatus::CREATED);
    
    // 取消订单
    EXPECT_TRUE(strategy_->cancelOrder(order_id));
    
    order = strategy_->getOrder(order_id);
    EXPECT_EQ(order->status, OrderStatus::CANCELED);
}

TEST_F(StrategyTest, PositionTracking) {
    data_feed_->loadBatch(20);
    
    // 建立多头持仓
    strategy_->buy(10.0);
    EXPECT_TRUE(strategy_->isLong());
    EXPECT_FALSE(strategy_->isShort());
    EXPECT_FALSE(strategy_->isEmpty());
    EXPECT_DOUBLE_EQ(strategy_->getPositionSize(), 10.0);
    
    // 增加持仓
    strategy_->buy(5.0);
    EXPECT_DOUBLE_EQ(strategy_->getPositionSize(), 15.0);
    
    // 部分平仓
    strategy_->sell(8.0);
    EXPECT_DOUBLE_EQ(strategy_->getPositionSize(), 7.0);
    EXPECT_TRUE(strategy_->isLong());
    
    // 反向到空头
    strategy_->sell(12.0);
    EXPECT_DOUBLE_EQ(strategy_->getPositionSize(), -5.0);
    EXPECT_TRUE(strategy_->isShort());
    EXPECT_FALSE(strategy_->isEmpty());
    
    // 平仓
    strategy_->buy(5.0);
    EXPECT_TRUE(strategy_->isEmpty());
    EXPECT_DOUBLE_EQ(strategy_->getPositionSize(), 0.0);
}

TEST_F(StrategyTest, StrategyExecution) {
    strategy_->initialize();
    
    // 模拟运行策略
    size_t initial_trades = strategy_->getTradeCount();
    
    while (data_feed_->hasNext()) {
        data_feed_->next();
        strategy_->processNext();
    }
    
    strategy_->finalize();
    
    // 应该产生一些交易
    EXPECT_GT(strategy_->getTradeCount(), initial_trades);
    
    // 检查是否有指标
    EXPECT_GE(strategy_->getIndicator(0) != nullptr, true);
    EXPECT_GE(strategy_->getIndicator(1) != nullptr, true);
}

TEST_F(StrategyTest, PnLCalculation) {
    data_feed_->loadBatch(20);
    
    double initial_price = data_feed_->close()->get(0);
    
    // 买入
    strategy_->buy(10.0);
    double entry_price = strategy_->getPosition().price;
    
    // 模拟价格变化（前进几个数据点）
    for (int i = 0; i < 5; ++i) {
        if (data_feed_->hasNext()) {
            data_feed_->next();
            strategy_->processNext();
        }
    }
    
    // 检查未实现盈亏
    const Position& position = strategy_->getPosition();
    double current_price = data_feed_->close()->get(0);
    double expected_unrealized = position.size * (current_price - position.price);
    
    EXPECT_NEAR(position.unrealized_pnl, expected_unrealized, 1e-6);
    
    // 卖出平仓
    strategy_->sell(10.0);
    
    // 检查已实现盈亏
    EXPECT_NE(strategy_->getTotalPnL(), 0.0);  // 应该有盈亏（包括手续费）
}

TEST_F(StrategyTest, CommissionCalculation) {
    data_feed_->loadBatch(10);
    
    strategy_->setCommission(0.001);  // 0.1% 手续费
    EXPECT_DOUBLE_EQ(strategy_->getCommission(), 0.001);
    
    double initial_pnl = strategy_->getTotalPnL();
    
    // 执行一笔交易
    strategy_->buy(10.0);
    
    // 手续费应该从PnL中扣除
    double price = data_feed_->close()->get(0);
    double expected_commission = 10.0 * price * 0.001;
    EXPECT_NEAR(strategy_->getTotalPnL(), initial_pnl - expected_commission, 1e-6);
}

TEST_F(StrategyTest, OrderHistory) {
    data_feed_->loadBatch(10);
    
    // 创建多个订单
    strategy_->buy(5.0);
    strategy_->sell(3.0);
    strategy_->buyLimit(50.0, 2.0);  // 限价单
    
    const auto& orders = strategy_->getOrders();
    EXPECT_EQ(orders.size(), 3);
    
    // 检查订单状态
    EXPECT_EQ(orders[0].status, OrderStatus::COMPLETED);  // 市价买入
    EXPECT_EQ(orders[1].status, OrderStatus::COMPLETED);  // 市价卖出
    EXPECT_EQ(orders[2].status, OrderStatus::CREATED);    // 限价单未执行
}