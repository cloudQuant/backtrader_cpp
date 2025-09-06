/**
 * @file test_ind_minperiod_simple.cpp
 * @brief Simplified MinPeriod test that works with current indicator design
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>

#include "indicators/sma.h"
#include "indicators/stochastic.h"
#include "indicators/macd.h"
#include "indicators/highest.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

// Test that indicators report correct minimum periods
TEST(OriginalTests, MinPeriod_BasicChecks) {
    // Load test data
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // Create data lines with all data
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    
    auto high_line = std::make_shared<LineSeries>();
    high_line->lines->add_line(std::make_shared<LineBuffer>());
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line->lines->getline(0));
    
    auto low_line = std::make_shared<LineSeries>();
    low_line->lines->add_line(std::make_shared<LineBuffer>());
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line->lines->getline(0));
    
    // Fill buffers with all data
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
    }
    
    // Create indicators
    auto sma = std::make_shared<SMA>(close_line);  // Default 30 period
    auto stochastic = std::make_shared<Stochastic>(high_line, low_line, close_line);
    auto macd = std::make_shared<MACD>(close_line);
    auto highest = std::make_shared<Highest>(high_line, 30);  // Explicitly specify 30 period
    
    // Test minimum periods
    EXPECT_EQ(sma->getMinPeriod(), 30) << "SMA default minimum period should be 30";
    EXPECT_GE(stochastic->getMinPeriod(), 14) << "Stochastic minimum period should be at least 14";
    EXPECT_EQ(macd->getMinPeriod(), 34) << "MACD minimum period should be 34";
    EXPECT_EQ(highest->getMinPeriod(), 30) << "Highest default minimum period should be 30";
    
    // The combined minimum period should be the maximum
    int combined = std::max({
        sma->getMinPeriod(),
        stochastic->getMinPeriod(), 
        macd->getMinPeriod(),
        highest->getMinPeriod()
    });
    EXPECT_EQ(combined, 34) << "Combined minimum period should be 34 (from MACD)";
    
    // Calculate all indicators once
    sma->calculate();
    stochastic->calculate();
    macd->calculate();
    highest->calculate();
    
    // After calculation, the indicators should have valid values at the end
    EXPECT_FALSE(std::isnan(sma->get(0))) << "SMA should have valid current value";
    EXPECT_FALSE(std::isnan(stochastic->getLine(0)->get(0))) << "Stochastic should have valid current value";
    EXPECT_FALSE(std::isnan(macd->getLine(0)->get(0))) << "MACD should have valid current value";
    EXPECT_FALSE(std::isnan(highest->get(0))) << "Highest should have valid current value";
}

// Test minimum period with different parameters
TEST(OriginalTests, MinPeriod_ParameterVariations) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    // Test SMA with different periods
    std::vector<int> periods = {1, 10, 20, 50};
    for (int period : periods) {
        auto sma = std::make_shared<SMA>(close_line, period);
        EXPECT_EQ(sma->getMinPeriod(), period) 
            << "SMA minimum period should equal its period parameter";
    }
    
    // Test MACD with different parameters
    auto macd1 = std::make_shared<MACD>(close_line, 12, 26, 9);
    EXPECT_EQ(macd1->getMinPeriod(), 34);  // 26 + 9 - 1
    
    auto macd2 = std::make_shared<MACD>(close_line, 8, 17, 9);
    EXPECT_EQ(macd2->getMinPeriod(), 25);  // 17 + 9 - 1
}

// Test edge cases
TEST(OriginalTests, MinPeriod_EdgeCases) {
    // Single data point
    auto single_line = std::make_shared<LineSeries>();
    single_line->lines->add_line(std::make_shared<LineBuffer>());
    auto single_buffer = std::dynamic_pointer_cast<LineBuffer>(single_line->lines->getline(0));
    single_buffer->append(100.0);
    
    // SMA with period 1 should work with single data point
    auto sma1 = std::make_shared<SMA>(single_line, 1);
    EXPECT_EQ(sma1->getMinPeriod(), 1);
    sma1->calculate();
    EXPECT_FALSE(std::isnan(sma1->get(0)));
    EXPECT_NEAR(sma1->get(0), 100.0, 1e-10);
    
    // Test period 0 handling
    auto sma0 = std::make_shared<SMA>(single_line, 0);
    EXPECT_GE(sma0->getMinPeriod(), 1) << "Minimum period should be at least 1";
}