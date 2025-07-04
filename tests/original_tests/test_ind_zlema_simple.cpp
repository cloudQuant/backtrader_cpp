/**
 * @file test_ind_zlema_simple.cpp
 * @brief Simplified ZLEMA indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['4056.120000', '3644.321667', '3556.021667']]
 * chkmin = 45
 * chkargs = dict(period=30)
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/zlema.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ZLEMA_EXPECTED_VALUES = {
    {"4056.120000", "3644.321667", "3556.021667"}
};

const int ZLEMA_MIN_PERIOD = 45;

} // anonymous namespace

// Test ZLEMA with period=30
DEFINE_INDICATOR_TEST(ZLEMA_Default, ZLEMA, ZLEMA_EXPECTED_VALUES, ZLEMA_MIN_PERIOD)