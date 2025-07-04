/**
 * @file test_ind_smma_simple.cpp
 * @brief Simplified SMMA indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['4064.120000', '3644.321667', '3556.021667']]
 * chkmin = 30
 * chkargs = dict(period=30)
 */

#include "test_common_simple.h"

#include "indicators/smma.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> SMMA_EXPECTED_VALUES = {
    {"4064.120000", "3644.321667", "3556.021667"}
};

const int SMMA_MIN_PERIOD = 30;

} // anonymous namespace

// Test SMMA with period=30
DEFINE_INDICATOR_TEST(SMMA_Default, SMMA, SMMA_EXPECTED_VALUES, SMMA_MIN_PERIOD)