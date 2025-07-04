/**
 * @file test_ind_dv2_simple.cpp
 * @brief Simplified DV2 indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['17.460317', '55.952381', '80.555556']]
 * chkmin = 253
 */

#include "test_common_simple.h"

#include "indicators/dv2.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> DV2_EXPECTED_VALUES = {
    {"17.460317", "55.952381", "80.555556"}
};

const int DV2_MIN_PERIOD = 253;

} // anonymous namespace

// Test DV2 with default parameters (period=252, maperiod=2)
DEFINE_INDICATOR_TEST(DV2_Default, DV2, DV2_EXPECTED_VALUES, DV2_MIN_PERIOD)