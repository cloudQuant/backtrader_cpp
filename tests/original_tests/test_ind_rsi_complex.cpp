/**
 * @file test_ind_rsi_complex.cpp
 * @brief Simplified RSI test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['57.644284', '41.630968', '53.352553']]
 * chkmin = 15
 * chkargs = dict()
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/rsi.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> RSI_EXPECTED_VALUES = {
    {"57.644284", "41.630968", "53.352553"}
};

const int RSI_MIN_PERIOD = 15;

} // anonymous namespace

// Test RSI with default parameters
DEFINE_INDICATOR_TEST(RSI_Complex, RSI, RSI_EXPECTED_VALUES, RSI_MIN_PERIOD)