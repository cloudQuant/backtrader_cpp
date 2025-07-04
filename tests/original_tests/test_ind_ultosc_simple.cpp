/**
 * @file test_ind_ultosc_simple.cpp
 * @brief Simplified UltimateOscillator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['51.991177', '62.334055', '46.707445']]
 * chkmin = 29
 * chkargs = dict()
 */

#include "test_common_simple.h"

#include "indicators/ultimateoscillator.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> ULTOSC_EXPECTED_VALUES = {
    {"51.991177", "62.334055", "46.707445"}
};

const int ULTOSC_MIN_PERIOD = 29;

} // anonymous namespace

// Test UltimateOscillator with default parameters
DEFINE_INDICATOR_TEST(UltimateOscillator_Default, UltimateOscillator, ULTOSC_EXPECTED_VALUES, ULTOSC_MIN_PERIOD)