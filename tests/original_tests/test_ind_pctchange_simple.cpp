/**
 * @file test_ind_pctchange_simple.cpp
 * @brief Simplified PctChange indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['0.002704', '0.034162', '0.043717']]
 * chkmin = 31
 */

#include "test_common_simple.h"

#include "indicators/percentchange.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> PCTCHANGE_EXPECTED_VALUES = {
    {"0.002704", "0.034162", "0.043717"}
};

const int PCTCHANGE_MIN_PERIOD = 31;

} // anonymous namespace

// Test PctChange with default parameters (period=30)
DEFINE_INDICATOR_TEST(PctChange_Default, PercentChange, PCTCHANGE_EXPECTED_VALUES, PCTCHANGE_MIN_PERIOD)