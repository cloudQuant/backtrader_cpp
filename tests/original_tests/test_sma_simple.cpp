#include "test_common_simple.h"
#include "indicators/sma.h"
#include <gtest/gtest.h>

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> SMA_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"}
};

const int SMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMA测试
DEFINE_INDICATOR_TEST(SMA_Default, SMA, SMA_EXPECTED_VALUES, SMA_MIN_PERIOD)

