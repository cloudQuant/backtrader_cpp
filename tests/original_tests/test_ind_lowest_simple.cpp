/**
 * @file test_ind_lowest_simple.cpp
 * @brief Simplified Lowest indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['3990.180000', '3516.490000', '3421.990000']]
 * chkmin = 14
 * chkargs = dict(period=14)
 */

#include "test_common_simple.h"

#include "indicators/basicops.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> LOWEST_EXPECTED_VALUES = {
    {"3990.180000", "3516.490000", "3421.990000"}
};

const int LOWEST_MIN_PERIOD = 14;

} // anonymous namespace

// Test Lowest with period=14
DEFINE_INDICATOR_TEST(Lowest_Default, Lowest, LOWEST_EXPECTED_VALUES, LOWEST_MIN_PERIOD)