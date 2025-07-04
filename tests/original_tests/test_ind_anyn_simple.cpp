/**
 * @file test_ind_anyn_simple.cpp
 * @brief Simplified AnyN indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * AnyN checks if any value in the period is non-zero
 * chkvals = [['1.000000', '1.000000', '1.000000']]
 * chkmin = 14
 * chkargs = dict(period=14)
 */

#include "test_common_simple.h"

#include "indicators/basicops.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> ANYN_EXPECTED_VALUES = {
    {"1.000000", "1.000000", "1.000000"}
};

const int ANYN_MIN_PERIOD = 14;

} // anonymous namespace

// Test AnyN with period=14
DEFINE_INDICATOR_TEST(AnyN_Default, AnyN, ANYN_EXPECTED_VALUES, ANYN_MIN_PERIOD)