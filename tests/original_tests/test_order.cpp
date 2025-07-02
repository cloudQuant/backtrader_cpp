/**
 * @file test_order.cpp
 * @brief 订单测试 - 对应Python test_order.py
 * 
 * 原始Python测试:
 * - 测试订单的部分执行功能
 * - 验证订单克隆和待处理状态的正确性
 * - 测试订单执行后的状态更新
 */

#include "test_common.h"
#include "broker/Order.h"
#include "broker/Position.h"
#include "broker/CommInfo.h"
#include "data/LineRoot.h"
#include <memory>
#include <vector>
#include <cassert>

using namespace backtrader;
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

// 执行订单的辅助函数
void executeOrder(Position& position, Order& order, double size, double price, bool partial) {
    // 保存原始仓位价格
    double originalPrice = position.getPrice();
    
    // 更新仓位
    auto result = position.update(size, price);
    double newSize = result.size;
    double newPrice = result.price;
    double opened = result.opened;
    double closed = result.closed;
    
    // 获取手续费信息
    auto commInfo = order.getCommInfo();
    
    // 计算各种费用
    double closedValue = commInfo->getOperationCost(closed, originalPrice);
    double closedComm = commInfo->getCommission(closed, price);
    double openedValue = commInfo->getOperationCost(opened, price);
    double openedComm = commInfo->getCommission(opened, price);
    double pnl = commInfo->profitAndLoss(-closed, originalPrice, price);
    double margin = commInfo->getValueSize(size, price);
    
    // 执行订单
    order.execute(0.0,  // datetime
                  size, price,
                  closed, closedValue, closedComm,
                  opened, openedValue, openedComm,
                  margin, pnl,
                  newSize, newPrice);
    
    // 设置订单状态
    if (partial) {
        order.partial();
    } else {
        order.completed();
    }
}

// 测试订单的部分执行和克隆功能
TEST(OriginalTests, Order_PartialExecutionAndClone) {
    // 创建仓位和手续费信息
    Position position;
    auto commInfo = std::make_shared<FakeCommInfo>();
    
    // 创建模拟数据
    auto fakeData = std::make_shared<FakeData>();
    
    // 创建买入订单
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    order.addCommInfo(commInfo);
    
    // 第一次部分执行：10股，价格1.0
    executeOrder(position, order, 10, 1.0, true);
    
    // 第二次部分执行：20股，价格1.1
    executeOrder(position, order, 20, 1.1, true);
    
    // 克隆订单并检查待处理状态
    auto clone1 = order.clone();
    auto pending1 = clone1->getExecuted().getPending();
    
    EXPECT_EQ(pending1.size(), 2) << "Should have 2 pending executions";
    EXPECT_DOUBLE_EQ(pending1[0].size, 10) << "First pending execution size should be 10";
    EXPECT_DOUBLE_EQ(pending1[0].price, 1.0) << "First pending execution price should be 1.0";
    EXPECT_DOUBLE_EQ(pending1[1].size, 20) << "Second pending execution size should be 20";
    EXPECT_DOUBLE_EQ(pending1[1].price, 1.1) << "Second pending execution price should be 1.1";
    
    // 第三次部分执行：30股，价格1.2
    executeOrder(position, order, 30, 1.2, true);
    
    // 第四次完整执行：40股，价格1.3
    executeOrder(position, order, 40, 1.3, false);
    
    // 再次克隆订单并检查待处理状态
    auto clone2 = order.clone();
    auto pending2 = clone2->getExecuted().getPending();
    
    EXPECT_EQ(pending2.size(), 2) << "Should still have 2 pending executions after clone";
    EXPECT_DOUBLE_EQ(pending2[0].size, 30) << "New first pending execution size should be 30";
    EXPECT_DOUBLE_EQ(pending2[0].price, 1.2) << "New first pending execution price should be 1.2";
    EXPECT_DOUBLE_EQ(pending2[1].size, 40) << "New second pending execution size should be 40";
    EXPECT_DOUBLE_EQ(pending2[1].price, 1.3) << "New second pending execution price should be 1.3";
}

// 测试订单状态转换
TEST(OriginalTests, Order_StatusTransitions) {
    Position position;
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    // 创建订单
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    order.addCommInfo(commInfo);
    
    // 初始状态应该是Created
    EXPECT_EQ(order.getStatus(), OrderStatus::Created) << "Initial order status should be Created";
    
    // 提交订单
    order.submit();
    EXPECT_EQ(order.getStatus(), OrderStatus::Submitted) << "Order status should be Submitted";
    
    // 接受订单
    order.accept();
    EXPECT_EQ(order.getStatus(), OrderStatus::Accepted) << "Order status should be Accepted";
    
    // 部分执行
    executeOrder(position, order, 50, 1.0, true);
    EXPECT_EQ(order.getStatus(), OrderStatus::Partial) << "Order status should be Partial";
    
    // 完全执行
    executeOrder(position, order, 50, 1.0, false);
    EXPECT_EQ(order.getStatus(), OrderStatus::Completed) << "Order status should be Completed";
}

// 测试不同类型的订单
TEST(OriginalTests, Order_DifferentOrderTypes) {
    Position position;
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    // 测试买入订单
    Order buyOrder(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    buyOrder.addCommInfo(commInfo);
    EXPECT_EQ(buyOrder.getType(), OrderType::Buy) << "Order type should be Buy";
    EXPECT_EQ(buyOrder.getSize(), 100) << "Order size should be 100";
    EXPECT_DOUBLE_EQ(buyOrder.getPrice(), 1.0) << "Order price should be 1.0";
    
    // 测试卖出订单
    Order sellOrder(fakeData, OrderType::Sell, 50, 2.0, ExecutionType::Limit, false);
    sellOrder.addCommInfo(commInfo);
    EXPECT_EQ(sellOrder.getType(), OrderType::Sell) << "Order type should be Sell";
    EXPECT_EQ(sellOrder.getSize(), 50) << "Order size should be 50";
    EXPECT_DOUBLE_EQ(sellOrder.getPrice(), 2.0) << "Order price should be 2.0";
    EXPECT_EQ(sellOrder.getExecutionType(), ExecutionType::Limit) << "Execution type should be Limit";
    EXPECT_FALSE(sellOrder.isSimulated()) << "Order should not be simulated";
}

// 测试订单执行历史
TEST(OriginalTests, Order_ExecutionHistory) {
    Position position;
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    order.addCommInfo(commInfo);
    
    // 执行多次部分交易
    std::vector<std::pair<double, double>> executions = {
        {25, 1.0}, {30, 1.1}, {20, 1.2}, {25, 1.3}
    };
    
    for (size_t i = 0; i < executions.size(); ++i) {
        bool isLast = (i == executions.size() - 1);
        executeOrder(position, order, executions[i].first, executions[i].second, !isLast);
    }
    
    // 验证总执行量
    double totalExecuted = order.getExecuted().getSize();
    EXPECT_DOUBLE_EQ(totalExecuted, 100) << "Total executed size should be 100";
    
    // 验证平均执行价格
    double avgPrice = order.getExecuted().getPrice();
    double expectedAvgPrice = (25*1.0 + 30*1.1 + 20*1.2 + 25*1.3) / 100.0;
    EXPECT_NEAR(avgPrice, expectedAvgPrice, 1e-6) << "Average execution price should match expected";
}

// 测试订单克隆的独立性
TEST(OriginalTests, Order_CloneIndependence) {
    Position position;
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    Order originalOrder(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    originalOrder.addCommInfo(commInfo);
    
    // 执行一次交易
    executeOrder(position, originalOrder, 50, 1.0, true);
    
    // 克隆订单
    auto clonedOrder = originalOrder.clone();
    
    // 验证克隆的独立性
    EXPECT_NE(&originalOrder, clonedOrder.get()) << "Cloned order should be different object";
    EXPECT_EQ(originalOrder.getType(), clonedOrder->getType()) << "Order types should match";
    EXPECT_EQ(originalOrder.getSize(), clonedOrder->getSize()) << "Order sizes should match";
    EXPECT_DOUBLE_EQ(originalOrder.getPrice(), clonedOrder->getPrice()) << "Order prices should match";
    
    // 修改原始订单状态
    originalOrder.cancel();
    
    // 克隆的订单状态不应该改变
    EXPECT_EQ(originalOrder.getStatus(), OrderStatus::Cancelled) << "Original order should be cancelled";
    EXPECT_NE(clonedOrder->getStatus(), OrderStatus::Cancelled) << "Cloned order should not be cancelled";
}

// 测试订单取消功能
TEST(OriginalTests, Order_Cancellation) {
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Limit, false);
    order.addCommInfo(commInfo);
    
    // 提交并接受订单
    order.submit();
    order.accept();
    EXPECT_EQ(order.getStatus(), OrderStatus::Accepted) << "Order should be accepted";
    
    // 取消订单
    order.cancel();
    EXPECT_EQ(order.getStatus(), OrderStatus::Cancelled) << "Order should be cancelled";
    
    // 已取消的订单不应该能够执行
    EXPECT_FALSE(order.canExecute()) << "Cancelled order should not be executable";
}

// 测试订单拒绝功能
TEST(OriginalTests, Order_Rejection) {
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    order.addCommInfo(commInfo);
    
    // 提交订单
    order.submit();
    EXPECT_EQ(order.getStatus(), OrderStatus::Submitted) << "Order should be submitted";
    
    // 拒绝订单
    order.reject();
    EXPECT_EQ(order.getStatus(), OrderStatus::Rejected) << "Order should be rejected";
    
    // 已拒绝的订单不应该能够执行
    EXPECT_FALSE(order.canExecute()) << "Rejected order should not be executable";
}

// 测试订单参数验证
TEST(OriginalTests, Order_ParameterValidation) {
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    // 正常订单
    EXPECT_NO_THROW({
        Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
        order.addCommInfo(commInfo);
    }) << "Normal order creation should not throw";
    
    // 零数量订单应该被处理（在某些情况下可能有效）
    EXPECT_NO_THROW({
        Order order(fakeData, OrderType::Buy, 0, 1.0, ExecutionType::Market, true);
        order.addCommInfo(commInfo);
    }) << "Zero size order creation should not throw";
    
    // 负数量订单应该被处理（可能表示卖出）
    EXPECT_NO_THROW({
        Order order(fakeData, OrderType::Sell, -100, 1.0, ExecutionType::Market, true);
        order.addCommInfo(commInfo);
    }) << "Negative size order creation should not throw";
}

// 测试订单优先级
TEST(OriginalTests, Order_Priority) {
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    // 创建不同优先级的订单
    Order highPriorityOrder(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    highPriorityOrder.addCommInfo(commInfo);
    highPriorityOrder.setPriority(1);
    
    Order lowPriorityOrder(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    lowPriorityOrder.addCommInfo(commInfo);
    lowPriorityOrder.setPriority(10);
    
    // 验证优先级设置
    EXPECT_EQ(highPriorityOrder.getPriority(), 1) << "High priority order should have priority 1";
    EXPECT_EQ(lowPriorityOrder.getPriority(), 10) << "Low priority order should have priority 10";
    
    // 高优先级订单应该排在前面
    EXPECT_LT(highPriorityOrder.getPriority(), lowPriorityOrder.getPriority()) 
        << "High priority should be less than low priority";
}

// 测试订单时间戳
TEST(OriginalTests, Order_Timestamps) {
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    Order order(fakeData, OrderType::Buy, 100, 1.0, ExecutionType::Market, true);
    order.addCommInfo(commInfo);
    
    // 创建时间应该被设置
    double createTime = order.getCreateTime();
    EXPECT_GE(createTime, 0.0) << "Create time should be set";
    
    // 提交订单并检查提交时间
    order.submit();
    double submitTime = order.getSubmitTime();
    EXPECT_GE(submitTime, createTime) << "Submit time should be >= create time";
    
    // 接受订单并检查接受时间
    order.accept();
    double acceptTime = order.getAcceptTime();
    EXPECT_GE(acceptTime, submitTime) << "Accept time should be >= submit time";
}

// 性能测试
TEST(OriginalTests, Order_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto commInfo = std::make_shared<FakeCommInfo>();
    auto fakeData = std::make_shared<FakeData>();
    
    // 创建大量订单
    const int num_orders = 10000;
    std::vector<std::unique_ptr<Order>> orders;
    orders.reserve(num_orders);
    
    for (int i = 0; i < num_orders; ++i) {
        auto order = std::make_unique<Order>(
            fakeData, OrderType::Buy, 100, 1.0 + i * 0.01, ExecutionType::Market, true);
        order->addCommInfo(commInfo);
        orders.push_back(std::move(order));
    }
    
    // 执行状态转换
    for (auto& order : orders) {
        order->submit();
        order->accept();
    }
    
    // 克隆所有订单
    std::vector<std::unique_ptr<Order>> cloned_orders;
    cloned_orders.reserve(num_orders);
    
    for (const auto& order : orders) {
        cloned_orders.push_back(order->clone());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Order performance test: created and processed " << num_orders 
              << " orders in " << duration.count() << " ms" << std::endl;
    
    // 验证所有订单都正确创建
    EXPECT_EQ(orders.size(), num_orders) << "Should create all orders";
    EXPECT_EQ(cloned_orders.size(), num_orders) << "Should clone all orders";
    
    // 性能要求
    EXPECT_LT(duration.count(), 1000) 
        << "Performance test should complete within 1 second";
}