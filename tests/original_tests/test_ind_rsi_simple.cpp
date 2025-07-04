/**
 * @file test_ind_rsi_simple.cpp
 * @brief RSI指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/rsi.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

// 来自Python版本的期望值
const std::vector<std::vector<std::string>> RSI_EXPECTED_VALUES = {
    {"57.644284", "41.630968", "53.352553"}
};

const int RSI_MIN_PERIOD = 15; // RSI(14) + 1

} // anonymous namespace

// 使用默认参数的RSI测试
DEFINE_INDICATOR_TEST(RSI_Default, RSI, RSI_EXPECTED_VALUES, RSI_MIN_PERIOD)