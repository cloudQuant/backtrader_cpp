/**
 * @file test_ind_oscillator_simple.cpp
 * @brief Simplified Oscillator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['56.477000', '51.185333', '2.386667']]
 * chkmin = 30
 * chkargs = dict()
 */

#include "test_common_simple.h"

#include "indicators/oscillator.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> OSCILLATOR_EXPECTED_VALUES = {
    {"56.477000", "51.185333", "2.386667"}
};

const int OSCILLATOR_MIN_PERIOD = 30;

} // anonymous namespace

// Test Oscillator with default parameters
DEFINE_INDICATOR_TEST(Oscillator_Default, Oscillator, OSCILLATOR_EXPECTED_VALUES, OSCILLATOR_MIN_PERIOD)