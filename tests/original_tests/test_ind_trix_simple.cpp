/**
 * @file test_ind_trix_simple.cpp
 * @brief Simplified TRIX test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['0.071304', '0.181480', '0.050954']]
 * chkmin = 44
 * chkargs = dict()
 */

#include "test_common_simple.h"

#include "indicators/trix.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> TRIX_EXPECTED_VALUES = {
    {"0.071304", "0.181480", "0.050954"}
};

const int TRIX_MIN_PERIOD = 44;

} // anonymous namespace

// Test TRIX with default parameters
DEFINE_INDICATOR_TEST(Trix_Default, Trix, TRIX_EXPECTED_VALUES, TRIX_MIN_PERIOD)