/**
 * @file test_ind_momentum_simple.cpp
 * @brief Simplified Momentum indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['11.130000', '13.870000', '26.140000']]
 * chkmin = 13
 */

#include "test_common_simple.h"

#include "indicators/momentum.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> MOMENTUM_EXPECTED_VALUES = {
    {"11.130000", "13.870000", "26.140000"}
};

const int MOMENTUM_MIN_PERIOD = 13;

} // anonymous namespace

// Test Momentum with default parameters (period=12)
DEFINE_INDICATOR_TEST(Momentum_Default, Momentum, MOMENTUM_EXPECTED_VALUES, MOMENTUM_MIN_PERIOD)