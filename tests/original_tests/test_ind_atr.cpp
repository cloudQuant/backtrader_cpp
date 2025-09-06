/**
 * @file test_ind_atr.cpp
 * @brief ATR指标测试 - 对应Python test_ind_atr.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['35.866308', '34.264286', '54.329064'],
 * ]
 * chkmin = 15
 * chkind = btind.ATR
 */

#include "test_common.h"
#include "indicators/atr.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ATR_EXPECTED_VALUES = {
    {"35.866308", "34.264286", "54.329064"}
};

const int ATR_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的ATR测试
DEFINE_INDICATOR_TEST(ATR_Default, ATR, ATR_EXPECTED_VALUES, ATR_MIN_PERIOD)