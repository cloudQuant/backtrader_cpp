/**
 * @file test_ind_hma_simple.cpp
 * @brief Simplified HMA indicator test using DEFINE_INDICATOR_TEST macro
 * 
 * Original Python test values:
 * chkvals = [['4135.661250', '3736.429214', '3578.389024']]
 * chkmin = 34
 */

#include "test_common_simple.h"

#include "indicators/hma.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> HMA_EXPECTED_VALUES = {
    {"4135.661250", "3736.429214", "3578.389024"}
};

const int HMA_MIN_PERIOD = 34;

} // anonymous namespace

// Test HMA with default parameters (period=30)
DEFINE_INDICATOR_TEST(HMA_Default, HMA, HMA_EXPECTED_VALUES, HMA_MIN_PERIOD)