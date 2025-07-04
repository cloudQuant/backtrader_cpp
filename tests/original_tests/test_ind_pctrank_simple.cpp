/**
 * @file test_ind_pctrank_simple.cpp
 * @brief Simplified PercentRank indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['0.900000', '0.880000', '0.980000']]
 * chkmin = 50
 */

#include "test_common_simple.h"

#include "indicators/percentrank.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> PCTRANK_EXPECTED_VALUES = {
    {"0.900000", "0.880000", "0.980000"}
};

const int PCTRANK_MIN_PERIOD = 50;

} // anonymous namespace

// Test PercentRank with default parameters (period=50)
DEFINE_INDICATOR_TEST(PercentRank_Default, PercentRank, PCTRANK_EXPECTED_VALUES, PCTRANK_MIN_PERIOD)