/**
 * @file test_ind_downmove_simple.cpp
 * @brief Simplified DownMove indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['15.720000', '0.000000', '0.000000']]
 * chkmin = 2
 * chkargs = dict()
 */

#include "test_common_simple.h"

#include "indicators/upmove.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> DOWNMOVE_EXPECTED_VALUES = {
    {"15.720000", "0.000000", "0.000000"}
};

const int DOWNMOVE_MIN_PERIOD = 2;

} // anonymous namespace

// Test DownMove with default parameters
DEFINE_INDICATOR_TEST(DownMove_Default, DownMove, DOWNMOVE_EXPECTED_VALUES, DOWNMOVE_MIN_PERIOD)