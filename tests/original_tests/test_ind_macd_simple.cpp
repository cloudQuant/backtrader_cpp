/**
 * @file test_ind_macd_simple.cpp
 * @brief MACD指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/macd.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

// 来自Python版本的期望值 - MACD histogram
const std::vector<std::vector<std::string>> MACD_EXPECTED_VALUES = {
    {"3.843516", "5.999669", "4.618090"}
};

const int MACD_MIN_PERIOD = 34; // MACD histogram的最小周期

} // anonymous namespace

// 使用默认参数的MACD测试
DEFINE_INDICATOR_TEST(MACD_Default, MACD, MACD_EXPECTED_VALUES, MACD_MIN_PERIOD)