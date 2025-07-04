/**
 * @file test_ind_roc_simple.cpp
 * @brief Simplified ROC indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['0.016544', '-0.009477', '0.019050']]
 * chkmin = 13
 * chkargs = dict(period=12)
 */

#include "test_common_simple.h"

#include "indicators/momentum.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> ROC_EXPECTED_VALUES = {
    {"0.016544", "-0.009477", "0.019050"}
};

const int ROC_MIN_PERIOD = 13;

} // anonymous namespace

// Test ROC with period=12
DEFINE_INDICATOR_TEST(ROC_Default, ROC, ROC_EXPECTED_VALUES, ROC_MIN_PERIOD)