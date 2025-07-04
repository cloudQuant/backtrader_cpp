/**
 * @file test_ind_dpo_simple.cpp
 * @brief Simplified DPO indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['83.271000', '105.625000', '1.187000']]
 * chkmin = 29
 */

#include "test_common_simple.h"

#include "indicators/dpo.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> DPO_EXPECTED_VALUES = {
    {"83.271000", "105.625000", "1.187000"}
};

const int DPO_MIN_PERIOD = 29;

} // anonymous namespace

// Test DPO with default parameters (period=20)
DEFINE_INDICATOR_TEST(DPO_Default, DPO, DPO_EXPECTED_VALUES, DPO_MIN_PERIOD)