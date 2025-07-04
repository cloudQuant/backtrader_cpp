/**
 * @file test_ind_alln_simple.cpp
 * @brief Simplified AllN indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * AllN checks if all values in the period are non-zero
 * chkvals = [['1.000000', '1.000000', '1.000000']]
 * chkmin = 14
 * chkargs = dict(period=14)
 */

#include "test_common_simple.h"

#include "indicators/basicops.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> ALLN_EXPECTED_VALUES = {
    {"1.000000", "1.000000", "1.000000"}
};

const int ALLN_MIN_PERIOD = 14;

} // anonymous namespace

// Test AllN with period=14
DEFINE_INDICATOR_TEST(AllN_Default, AllN, ALLN_EXPECTED_VALUES, ALLN_MIN_PERIOD)