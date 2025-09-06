/**
 * @file test_ind_momentum.cpp
 * @brief Momentum指标测试 - 对应Python test_ind_momentum.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['67.050000', '-34.160000', '67.630000'],
 * ]
 * chkmin = 13
 * chkind = btind.Momentum
 */

#include "test_common.h"
#include "lineseries.h"
#include "indicators/momentum.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> MOMENTUM_EXPECTED_VALUES = {
    {"67.050000", "-34.160000", "67.630000"}
};

const int MOMENTUM_MIN_PERIOD = 13;

} // anonymous namespace

// 使用默认参数的Momentum测试
DEFINE_INDICATOR_TEST(Momentum_Default, Momentum, MOMENTUM_EXPECTED_VALUES, MOMENTUM_MIN_PERIOD)