/**
 * @file test_ind_sumn_simple.cpp
 * @brief Simplified SumN indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['57406.490000', '50891.010000', '50424.690000']]
 * chkmin = 14
 * chkargs = dict(period=14)
 */

#include "test_common_simple.h"

#include "indicators/basicops.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> SUMN_EXPECTED_VALUES = {
    {"57406.490000", "50891.010000", "50424.690000"}
};

const int SUMN_MIN_PERIOD = 14;

} // anonymous namespace

// Test SumN with period=14
DEFINE_INDICATOR_TEST(SumN_Default, SumN, SUMN_EXPECTED_VALUES, SUMN_MIN_PERIOD)