/**
 * @file test_ind_dema_simple.cpp
 * @brief Simplified DEMA indicator test
 * 
 * Original Python test values:
 * chkdatas = 1
 * chkvals = [
 *     ['4115.563246', '3852.837209', '3665.728415']
 * ]
 * chkmin = 59
 * chkind = btind.DEMA
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/dema.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DEMA_EXPECTED_VALUES = {
    {"4115.563246", "3852.837209", "3665.728415"}
};

const int DEMA_MIN_PERIOD = 59;

} // anonymous namespace

// Standard DEMA test using the DEFINE_INDICATOR_TEST macro
DEFINE_INDICATOR_TEST(DEMA_Simple, DoubleExponentialMovingAverage, DEMA_EXPECTED_VALUES, DEMA_MIN_PERIOD)