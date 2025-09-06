/**
 * @file test_ind_highest.cpp
 * @brief Highest指标测试 - 对应Python test_ind_highest.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4140.660000', '3671.780000', '3670.750000'],
 * ]
 * chkmin = 14
 * chkind = btind.Highest
 * chkargs = dict(period=14)
 */

#include "test_common.h"
#include "lineseries.h"
#include "indicators/basicops.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> HIGHEST_EXPECTED_VALUES = {
    {"4140.660000", "3671.780000", "3670.750000"}
};

const int HIGHEST_MIN_PERIOD = 14;

} // anonymous namespace

// 使用默认参数的Highest测试
DEFINE_INDICATOR_TEST(Highest_Default, Highest, HIGHEST_EXPECTED_VALUES, HIGHEST_MIN_PERIOD)