/**
 * @file test_ind_highest_simple.cpp
 * @brief Simplified Highest indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['4140.660000', '3671.780000', '3670.750000']]
 * chkmin = 14
 * chkargs = dict(period=14)
 */

#include "test_common.h"

#include "indicators/basicops.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> HIGHEST_EXPECTED_VALUES = {
    {"4140.660000", "3671.780000", "3670.750000"}
};

const int HIGHEST_MIN_PERIOD = 14;

} // anonymous namespace

// Test Highest with period=14
DEFINE_INDICATOR_TEST(Highest_Default, Highest, HIGHEST_EXPECTED_VALUES, HIGHEST_MIN_PERIOD)