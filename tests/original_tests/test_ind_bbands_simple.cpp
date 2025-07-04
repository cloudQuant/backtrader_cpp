/**
 * @file test_ind_bbands_simple.cpp
 * @brief Bollinger Bands指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/bollinger.h"

using namespace backtrader::tests::original;

namespace {

// 来自Python版本的期望值
const std::vector<std::vector<std::string>> BBANDS_EXPECTED_VALUES = {
    {"3980.593333", "3644.444667", "3554.693333"}
};

const int BBANDS_MIN_PERIOD = 20;

} // anonymous namespace

// 使用默认参数的Bollinger Bands测试
DEFINE_INDICATOR_TEST(BollingerBands_Default, BollingerBands, BBANDS_EXPECTED_VALUES, BBANDS_MIN_PERIOD)