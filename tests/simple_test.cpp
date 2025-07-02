/**
 * @file simple_test.cpp
 * @brief Simple test runner without external dependencies
 * 
 * This file provides a basic test framework to verify core functionality
 * without requiring GoogleTest or other external test frameworks.
 */

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <memory>

// Include core headers
#include "LineRoot.h"
#include "indicators/SMA.h"
#include "Trade.h"
#include "Position.h"

using namespace backtrader;

// Simple test framework
class SimpleTest {
private:
    static int tests_run;
    static int tests_passed;
    static int tests_failed;
    
public:
    static void assert_true(bool condition, const std::string& message) {
        tests_run++;
        if (condition) {
            tests_passed++;
            std::cout << "[PASS] " << message << std::endl;
        } else {
            tests_failed++;
            std::cout << "[FAIL] " << message << std::endl;
        }
    }
    
    static void assert_near(double actual, double expected, double tolerance, const std::string& message) {
        tests_run++;
        bool condition = std::abs(actual - expected) <= tolerance;
        if (condition) {
            tests_passed++;
            std::cout << "[PASS] " << message << " (actual: " << actual << ", expected: " << expected << ")" << std::endl;
        } else {
            tests_failed++;
            std::cout << "[FAIL] " << message << " (actual: " << actual << ", expected: " << expected << ", diff: " << std::abs(actual - expected) << ")" << std::endl;
        }
    }
    
    static void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << tests_run << std::endl;
        std::cout << "Passed: " << tests_passed << std::endl;
        std::cout << "Failed: " << tests_failed << std::endl;
        if (tests_failed == 0) {
            std::cout << "All tests PASSED!" << std::endl;
        } else {
            std::cout << "Some tests FAILED!" << std::endl;
        }
    }
    
    static bool all_passed() {
        return tests_failed == 0;
    }
};

int SimpleTest::tests_run = 0;
int SimpleTest::tests_passed = 0;
int SimpleTest::tests_failed = 0;

// Test functions
void test_lineroot_basic() {
    std::cout << "\n=== Testing LineRoot Basic Functionality ===" << std::endl;
    
    auto line = std::make_shared<LineRoot>(10, "test_line");
    
    // Test basic operations
    line->forward(1.0);
    line->forward(2.0);
    line->forward(3.0);
    
    SimpleTest::assert_near(line->get(0), 3.0, 1e-10, "Current value should be 3.0");
    SimpleTest::assert_near(line->get(-1), 2.0, 1e-10, "Previous value should be 2.0");
    SimpleTest::assert_near(line->get(-2), 1.0, 1e-10, "Value 2 periods ago should be 1.0");
    
    SimpleTest::assert_true(line->len() == 3, "LineRoot size should be 3");
}

void test_sma_indicator() {
    std::cout << "\n=== Testing SMA Indicator ===" << std::endl;
    
    // Create test data
    auto close_line = std::make_shared<LineRoot>(100, "close");
    
    // Add some test prices
    std::vector<double> prices = {10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0};
    
    // Create SMA(5) indicator
    auto sma5 = std::make_shared<SMA>(close_line, 5);
    
    // Add data and calculate step by step
    for (size_t i = 0; i < prices.size(); ++i) {
        close_line->forward(prices[i]);
        sma5->calculate();
    }
    
    // Test SMA calculation
    // Last 5 values: 16, 17, 18, 19, 20 -> average = 18.0
    SimpleTest::assert_near(sma5->get(0), 18.0, 1e-10, "SMA(5) last value should be 18.0");
    
    // Check minimum period
    SimpleTest::assert_true(sma5->getMinPeriod() == 5, "SMA minimum period should be 5");
}

void test_order_basic() {
    std::cout << "\n=== Testing Order Basic Functionality ===" << std::endl;
    
    Order order;
    order.id = 1;
    order.type = OrderType::MARKET;
    order.side = OrderSide::BUY;
    order.size = 100.0;
    order.price = 50.0;
    
    SimpleTest::assert_true(order.isBuyOrder(), "Order should be a buy order");
    SimpleTest::assert_true(order.isMarketOrder(), "Order should be a market order");
    SimpleTest::assert_near(order.getRemainingSize(), 100.0, 1e-10, "Remaining size should be 100.0");
    
    // Test partial execution
    order.addExecution(30.0, 50.5);
    SimpleTest::assert_near(order.executed_size, 30.0, 1e-10, "Executed size should be 30.0");
    SimpleTest::assert_near(order.executed_price, 50.5, 1e-10, "Executed price should be 50.5");
    SimpleTest::assert_true(order.isPartial(), "Order should be partially filled");
    
    // Test complete execution
    order.addExecution(70.0, 49.5);
    SimpleTest::assert_true(order.isCompleted(), "Order should be completed");
    SimpleTest::assert_near(order.getRemainingSize(), 0.0, 1e-10, "Remaining size should be 0.0");
}

void test_trade_basic() {
    std::cout << "\n=== Testing Trade Basic Functionality ===" << std::endl;
    
    Trade trade(true, 100.0, 50.0, 1, "TEST");
    
    SimpleTest::assert_true(trade.isLong(), "Trade should be long");
    SimpleTest::assert_true(trade.isOpen(), "Trade should be open");
    SimpleTest::assert_near(trade.getSize(), 100.0, 1e-10, "Trade size should be 100.0");
    SimpleTest::assert_near(trade.getEntryPrice(), 50.0, 1e-10, "Entry price should be 50.0");
    
    // Close the trade
    trade.close(55.0, 10, 2.0);
    
    SimpleTest::assert_true(trade.isClosed(), "Trade should be closed");
    SimpleTest::assert_near(trade.getExitPrice(), 55.0, 1e-10, "Exit price should be 55.0");
    SimpleTest::assert_near(trade.getPnL(), 500.0, 1e-10, "PnL should be 500.0 (100 * (55-50))");
    SimpleTest::assert_near(trade.getPnLComm(), 498.0, 1e-10, "PnL after commission should be 498.0");
    SimpleTest::assert_true(trade.isProfitable(), "Trade should be profitable");
}

void test_position_basic() {
    std::cout << "\n=== Testing Position Basic Functionality ===" << std::endl;
    
    Position position("TEST");
    
    SimpleTest::assert_true(position.isEmpty(), "Position should be empty initially");
    
    // Open long position
    position.update(100.0, 50.0);
    SimpleTest::assert_true(position.isLong(), "Position should be long");
    SimpleTest::assert_near(position.size, 100.0, 1e-10, "Position size should be 100.0");
    SimpleTest::assert_near(position.price, 50.0, 1e-10, "Position price should be 50.0");
    
    // Update unrealized PnL
    position.updateUnrealizedPnL(55.0);
    SimpleTest::assert_near(position.unrealized_pnl, 500.0, 1e-10, "Unrealized PnL should be 500.0");
    
    // Partial close
    position.update(-30.0, 60.0);
    SimpleTest::assert_near(position.size, 70.0, 1e-10, "Position size should be 70.0 after partial close");
    SimpleTest::assert_near(position.realized_pnl, 300.0, 1e-10, "Realized PnL should be 300.0 (30 * (60-50))");
}

int main() {
    std::cout << "Starting simple tests for backtrader C++ core functionality..." << std::endl;
    
    try {
        test_lineroot_basic();
        test_sma_indicator();
        test_order_basic();
        test_trade_basic();
        test_position_basic();
        
        SimpleTest::print_summary();
        
        return SimpleTest::all_passed() ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}