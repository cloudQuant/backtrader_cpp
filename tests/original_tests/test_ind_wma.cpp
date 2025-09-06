/**
 * @file test_ind_wma.cpp
 * @brief WMA指标测试 - 对应Python test_ind_wma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4076.212366', '3655.193634', '3576.228000'],
 * ]
 * chkmin = 30
 * chkind = btind.WMA
 */

#include "test_common.h"
#include "lineseries.h"
#include "linebuffer.h"
#include "indicators/wma.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> WMA_EXPECTED_VALUES = {
    {"4076.212366", "3655.193634", "3576.228000"}
};

const int WMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的WMA测试
DEFINE_INDICATOR_TEST(WMA_Default, WMA, WMA_EXPECTED_VALUES, WMA_MIN_PERIOD)