/**
 * @file test_ind_zlind_simple.cpp
 * @brief Simplified ZeroLagIndicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['4110.282052', '3644.444667', '3564.906194']]
 * chkmin = 30
 * chkargs = dict()
 */

#include "test_common_simple.h"

#include "indicators/zlind.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> ZEROLAGINDICATOR_EXPECTED_VALUES = {
    {"4110.282052", "3644.444667", "3564.906194"}
};

const int ZEROLAGINDICATOR_MIN_PERIOD = 30;

} // anonymous namespace

// Test ZeroLagIndicator with default parameters
DEFINE_INDICATOR_TEST(ZeroLagIndicator_Default, ZeroLagIndicator, ZEROLAGINDICATOR_EXPECTED_VALUES, ZEROLAGINDICATOR_MIN_PERIOD)