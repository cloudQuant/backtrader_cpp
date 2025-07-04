/**
 * @file test_ind_dema_complex.cpp
 * @brief Simplified DEMA test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['4115.563246', '3852.837209', '3665.728415']]
 * chkmin = 59
 * chkargs = dict()
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

// Test DEMA with default parameters
DEFINE_INDICATOR_TEST(DEMA_Complex, DEMA, DEMA_EXPECTED_VALUES, DEMA_MIN_PERIOD)