/**
 * @file test_ind_average_simple.cpp
 * @brief Simplified Average indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * This is a simple arithmetic average (same as SMA)
 * chkvals = [['4079.463000', '3644.444667', '3554.693333']]
 * chkmin = 30
 * chkargs = dict(period=30)
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/basicops.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> AVERAGE_EXPECTED_VALUES = {
    {"4079.463000", "3644.444667", "3554.693333"}
};

const int AVERAGE_MIN_PERIOD = 30;

} // anonymous namespace

// Test Average with period=30
DEFINE_INDICATOR_TEST(Average_Default, Average, AVERAGE_EXPECTED_VALUES, AVERAGE_MIN_PERIOD)