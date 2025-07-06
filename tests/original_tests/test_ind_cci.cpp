/**
 * @file test_ind_cci.cpp
 * @brief CCI指标测试 - 对应Python test_ind_cci.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['69.574287', '91.196363', '82.175663'],
 * ]
 * chkmin = 39
 * chkind = btind.CCI
 */

#include "test_common.h"
#include "indicators/cci.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> CCI_EXPECTED_VALUES = {
    {"69.574287", "91.196363", "82.175663"}
};

const int CCI_MIN_PERIOD = 39;

} // anonymous namespace

// 使用默认参数的CCI测试
DEFINE_INDICATOR_TEST(CCI_Default, CCI, CCI_EXPECTED_VALUES, CCI_MIN_PERIOD)