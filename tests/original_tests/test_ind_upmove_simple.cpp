/**
 * @file test_ind_upmove_simple.cpp
 * @brief Simplified UpMove indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['-10.720000', '10.010000', '14.000000']]
 * chkmin = 2
 * chkargs = dict()
 */

#include "test_common_simple.h"

#include "indicators/upmove.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> UPMOVE_EXPECTED_VALUES = {
    {"-10.720000", "10.010000", "14.000000"}
};

const int UPMOVE_MIN_PERIOD = 2;

} // anonymous namespace

// Test UpMove with default parameters
DEFINE_INDICATOR_TEST(UpMove_Default, UpMove, UPMOVE_EXPECTED_VALUES, UPMOVE_MIN_PERIOD)