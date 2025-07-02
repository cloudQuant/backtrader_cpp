#include <gtest/gtest.h>
#include "core/LineRoot.h"
#include "core/Common.h"
#include "../utils/TestDataProvider.h"

namespace backtrader {
namespace test {

class LineRootTest : public ::testing::Test {
protected:
    void SetUp() override {
        line = std::make_shared<LineRoot>(10, "test_line");
    }
    
    std::shared_ptr<LineRoot> line;
};

TEST_F(LineRootTest, InitialState) {
    EXPECT_EQ(line->len(), 0);
    EXPECT_TRUE(line->empty());
    EXPECT_EQ(line->buflen(), 10);
    EXPECT_EQ(line->getName(), "test_line");
    EXPECT_EQ(line->getMinPeriod(), 1);
}

TEST_F(LineRootTest, BasicDataAccess) {
    line->forward(1.0);
    line->forward(2.0);
    line->forward(3.0);
    
    EXPECT_EQ(line->len(), 3);
    EXPECT_DOUBLE_EQ((*line)[0], 3.0);
    EXPECT_DOUBLE_EQ((*line)[-1], 2.0);
    EXPECT_DOUBLE_EQ((*line)[-2], 1.0);
    
    // 测试函数调用语法
    EXPECT_DOUBLE_EQ((*line)(0), 3.0);
    EXPECT_DOUBLE_EQ((*line)(-1), 2.0);
    
    // 测试get方法
    EXPECT_DOUBLE_EQ(line->get(0), 3.0);
    EXPECT_DOUBLE_EQ(line->get(-1), 2.0);
}

TEST_F(LineRootTest, PropertiesManagement) {
    line->setName("new_name");
    EXPECT_EQ(line->getName(), "new_name");
    
    line->setMinPeriod(5);
    EXPECT_EQ(line->getMinPeriod(), 5);
}

TEST_F(LineRootTest, NavigationMethods) {
    for (int i = 1; i <= 5; ++i) {
        line->forward(static_cast<double>(i));
    }
    
    EXPECT_EQ(line->len(), 5);
    EXPECT_DOUBLE_EQ((*line)[0], 5.0);
    
    // 测试backward
    line->backward(2);
    EXPECT_EQ(line->len(), 3);
    EXPECT_DOUBLE_EQ((*line)[0], 3.0);
    
    // 测试home
    line->home();
    EXPECT_EQ(line->len(), 0);
    EXPECT_TRUE(line->empty());
}

TEST_F(LineRootTest, ArithmeticOperations) {
    // 准备测试数据
    line->forward(10.0);
    line->forward(20.0);
    line->forward(30.0);
    
    // 测试与标量的运算
    auto add_result = *line + 5.0;
    auto sub_result = *line - 5.0;
    auto mul_result = *line * 2.0;
    auto div_result = *line / 2.0;
    
    EXPECT_DOUBLE_EQ(add_result->get(0), 35.0);
    EXPECT_DOUBLE_EQ(sub_result->get(0), 25.0);
    EXPECT_DOUBLE_EQ(mul_result->get(0), 60.0);
    EXPECT_DOUBLE_EQ(div_result->get(0), 15.0);
    
    // 测试标量在左侧的运算
    auto left_add = 5.0 + *line;
    auto left_sub = 100.0 - *line;
    auto left_mul = 2.0 * *line;
    auto left_div = 60.0 / *line;
    
    EXPECT_DOUBLE_EQ(left_add->get(0), 35.0);
    EXPECT_DOUBLE_EQ(left_sub->get(0), 70.0);
    EXPECT_DOUBLE_EQ(left_mul->get(0), 60.0);
    EXPECT_DOUBLE_EQ(left_div->get(0), 2.0);
}

TEST_F(LineRootTest, LineToLineOperations) {
    // 创建两个数据线
    auto line1 = std::make_shared<LineRoot>(10, "line1");
    auto line2 = std::make_shared<LineRoot>(10, "line2");
    
    line1->forward(10.0);
    line1->forward(20.0);
    
    line2->forward(5.0);
    line2->forward(4.0);
    
    // 测试数据线间运算
    auto add_result = line1->operator+(*line2);
    auto sub_result = line1->operator-(*line2);
    auto mul_result = line1->operator*(*line2);
    auto div_result = line1->operator/(*line2);
    
    EXPECT_DOUBLE_EQ(add_result->get(0), 24.0);
    EXPECT_DOUBLE_EQ(sub_result->get(0), 16.0);
    EXPECT_DOUBLE_EQ(mul_result->get(0), 80.0);
    EXPECT_DOUBLE_EQ(div_result->get(0), 5.0);
}

TEST_F(LineRootTest, ComparisonOperations) {
    line->forward(10.0);
    line->forward(20.0);
    line->forward(15.0);
    
    // 测试比较运算
    auto gt_result = *line > 16.0;
    auto lt_result = *line < 16.0;
    auto eq_result = *line == 15.0;
    auto ne_result = *line != 15.0;
    
    EXPECT_DOUBLE_EQ(gt_result->get(0), 0.0);  // 15 > 16 = false = 0.0
    EXPECT_DOUBLE_EQ(lt_result->get(0), 1.0);  // 15 < 16 = true = 1.0
    EXPECT_DOUBLE_EQ(eq_result->get(0), 1.0);  // 15 == 15 = true = 1.0
    EXPECT_DOUBLE_EQ(ne_result->get(0), 0.0);  // 15 != 15 = false = 0.0
}

TEST_F(LineRootTest, NaNHandling) {
    line->forward(10.0);
    line->forward(NaN);
    line->forward(20.0);
    
    EXPECT_DOUBLE_EQ((*line)[-2], 10.0);
    EXPECT_TRUE(isNaN((*line)[-1]));
    EXPECT_DOUBLE_EQ((*line)[0], 20.0);
    
    // 测试NaN传播
    auto add_result = *line + 5.0;
    EXPECT_DOUBLE_EQ(add_result->get(-2), 15.0);
    EXPECT_TRUE(isNaN(add_result->get(-1)));
    EXPECT_DOUBLE_EQ(add_result->get(0), 25.0);
}

TEST_F(LineRootTest, DivisionByZero) {
    line->forward(10.0);
    
    auto div_result = *line / 0.0;
    EXPECT_TRUE(isNaN(div_result->get(0)));
    
    auto left_div_result = 10.0 / *line;
    line->forward(0.0);  // 添加零值
    EXPECT_TRUE(isNaN(left_div_result->get(0)));
}

TEST_F(LineRootTest, OperationLineRootBehavior) {
    // 测试运算结果的延迟计算特性
    auto line1 = std::make_shared<LineRoot>(10, "line1");
    auto line2 = std::make_shared<LineRoot>(10, "line2");
    
    line1->forward(10.0);
    line2->forward(5.0);
    
    auto sum_line = line1->operator+(*line2);
    EXPECT_DOUBLE_EQ(sum_line->get(0), 15.0);
    
    // 添加更多数据，测试延迟计算
    line1->forward(20.0);
    line2->forward(8.0);
    
    EXPECT_DOUBLE_EQ(sum_line->get(0), 28.0);  // 应该动态计算新值
    EXPECT_DOUBLE_EQ(sum_line->get(-1), 15.0); // 前一个值保持不变
}

TEST_F(LineRootTest, ChainedOperations) {
    // 测试链式运算
    line->forward(10.0);
    line->forward(20.0);
    
    auto temp1 = line->operator+(5.0);
    auto temp2 = temp1->operator*(2.0);
    auto complex_result = temp2->operator-(10.0);
    EXPECT_DOUBLE_EQ(complex_result->get(0), 40.0);  // (20 + 5) * 2 - 10 = 40
    EXPECT_DOUBLE_EQ(complex_result->get(-1), 20.0); // (10 + 5) * 2 - 10 = 20
}

TEST_F(LineRootTest, IntegrationWithTestData) {
    // 使用TestDataProvider生成测试数据
    auto data = TestDataProvider::generateTrendingData(100, 100.0, 0.5, 1.0);
    auto test_line = TestDataProvider::createLineRootFromData(data, "trend_data");
    
    EXPECT_EQ(test_line->len(), 100);
    EXPECT_EQ(test_line->getName(), "trend_data");
    
    // 测试数据的趋势性
    double first_value = test_line->get(-99);  // 第一个值
    double last_value = test_line->get(0);     // 最后一个值
    
    // 由于有正趋势，最后值应该大于第一个值
    EXPECT_GT(last_value, first_value);
}

TEST_F(LineRootTest, PerformanceOfOperations) {
    // 准备大量数据
    auto data = TestDataProvider::generateRandomData(10000);
    auto test_line = TestDataProvider::createLineRootFromData(data);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 执行复杂运算
    auto temp1 = test_line->operator+(10.0);
    auto temp2 = temp1->operator*(2.0);
    auto result = temp2->operator-(5.0);
    
    // 访问所有结果值
    double sum = 0.0;
    for (int i = 0; i < 10000; ++i) {
        sum += result->get(-i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 性能要求：应该在合理时间内完成
    EXPECT_LT(duration.count(), 50000); // 小于50ms
    EXPECT_GT(sum, 0.0); // 确保计算有效
}

} // namespace test
} // namespace backtrader