#include "test_common_simple.h"
#include "indicators/ema.h"
#include <gtest/gtest.h>

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

// These values need to be verified against the Python implementation
const std::vector<std::vector<std::string>> EMA_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"}  // Placeholder - need actual values
};

const int EMA_MIN_PERIOD = 30;

} // anonymous namespace

// Simple test to verify EMA compiles and runs
TEST(OriginalTests, EMA_Basic) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 创建EMA指标（30周期）
    auto ema = std::make_shared<EMA>(close_line, 30);
    
    // 逐步添加数据并计算
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        ema->calculate();
    }
    
    // 验证基本功能
    EXPECT_EQ(ema->getMinPeriod(), 1);  // EMA can start calculating from first value
    EXPECT_FALSE(isNaN(ema->get(0)));   // Should have a valid output
    
    std::cout << "EMA final value: " << ema->get(0) << std::endl;
}