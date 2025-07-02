#include <gtest/gtest.h>
#include "core/CircularBuffer.h"
#include "core/Common.h"

namespace backtrader {
namespace test {

class CircularBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<CircularBuffer<double>>(5);
    }
    
    std::unique_ptr<CircularBuffer<double>> buffer;
};

TEST_F(CircularBufferTest, InitialState) {
    EXPECT_EQ(buffer->len(), 0);
    EXPECT_TRUE(buffer->empty());
    EXPECT_EQ(buffer->capacity(), 5);
}

TEST_F(CircularBufferTest, BasicForward) {
    buffer->forward(1.0);
    EXPECT_EQ(buffer->len(), 1);
    EXPECT_FALSE(buffer->empty());
    EXPECT_DOUBLE_EQ(buffer->get(0), 1.0);
}

TEST_F(CircularBufferTest, MultipleForward) {
    buffer->forward(1.0);
    buffer->forward(2.0);
    buffer->forward(3.0);
    
    EXPECT_EQ(buffer->len(), 3);
    EXPECT_DOUBLE_EQ(buffer->get(0), 3.0);   // 最新值
    EXPECT_DOUBLE_EQ(buffer->get(-1), 2.0);  // 前一个值
    EXPECT_DOUBLE_EQ(buffer->get(-2), 1.0);  // 前两个值
}

TEST_F(CircularBufferTest, NegativeIndexing) {
    for (int i = 1; i <= 5; ++i) {
        buffer->forward(static_cast<double>(i));
    }
    
    // 测试负索引访问
    EXPECT_DOUBLE_EQ((*buffer)[0], 5.0);
    EXPECT_DOUBLE_EQ((*buffer)[-1], 4.0);
    EXPECT_DOUBLE_EQ((*buffer)[-2], 3.0);
    EXPECT_DOUBLE_EQ((*buffer)[-3], 2.0);
    EXPECT_DOUBLE_EQ((*buffer)[-4], 1.0);
}

TEST_F(CircularBufferTest, CircularOverwrite) {
    // 填充超过容量的数据
    for (int i = 1; i <= 7; ++i) {
        buffer->forward(static_cast<double>(i));
    }
    
    EXPECT_EQ(buffer->len(), 5);  // 长度不应超过容量
    EXPECT_DOUBLE_EQ((*buffer)[0], 7.0);
    EXPECT_DOUBLE_EQ((*buffer)[-1], 6.0);
    EXPECT_DOUBLE_EQ((*buffer)[-2], 5.0);
    EXPECT_DOUBLE_EQ((*buffer)[-3], 4.0);
    EXPECT_DOUBLE_EQ((*buffer)[-4], 3.0);
}

TEST_F(CircularBufferTest, MultiStepForward) {
    buffer->forward(1.0);
    buffer->forward(NaN, 3);  // 前进3步，设置NaN
    buffer->forward(5.0);
    
    EXPECT_EQ(buffer->len(), 5);
    EXPECT_DOUBLE_EQ((*buffer)[0], 5.0);
    EXPECT_TRUE(isNaN((*buffer)[-1]));
    EXPECT_TRUE(isNaN((*buffer)[-2]));
    EXPECT_TRUE(isNaN((*buffer)[-3]));
    EXPECT_DOUBLE_EQ((*buffer)[-4], 1.0);
}

TEST_F(CircularBufferTest, BackwardMovement) {
    for (int i = 1; i <= 5; ++i) {
        buffer->forward(static_cast<double>(i));
    }
    
    buffer->backward(2);
    EXPECT_EQ(buffer->len(), 3);
    EXPECT_DOUBLE_EQ((*buffer)[0], 3.0);
    EXPECT_DOUBLE_EQ((*buffer)[-1], 2.0);
    EXPECT_DOUBLE_EQ((*buffer)[-2], 1.0);
}

TEST_F(CircularBufferTest, HomeReset) {
    for (int i = 1; i <= 3; ++i) {
        buffer->forward(static_cast<double>(i));
    }
    
    buffer->home();
    EXPECT_EQ(buffer->len(), 0);
    EXPECT_TRUE(buffer->empty());
}

TEST_F(CircularBufferTest, BatchOperations) {
    // 准备测试数据
    for (int i = 1; i <= 5; ++i) {
        buffer->forward(static_cast<double>(i));
    }
    
    // 测试批量获取
    std::vector<double> output(3);
    buffer->getBatch(-2, 3, output.data());
    
    EXPECT_DOUBLE_EQ(output[0], 3.0);
    EXPECT_DOUBLE_EQ(output[1], 2.0);
    EXPECT_DOUBLE_EQ(output[2], 1.0);
}

TEST_F(CircularBufferTest, ContinuousView) {
    // 测试连续数据视图
    for (int i = 1; i <= 4; ++i) {
        buffer->forward(static_cast<double>(i));
    }
    
    const double* view = buffer->getContinuousView(-3, 3);
    if (view != nullptr) {
        EXPECT_DOUBLE_EQ(view[0], 1.0);
        EXPECT_DOUBLE_EQ(view[1], 2.0);
        EXPECT_DOUBLE_EQ(view[2], 3.0);
    }
}

TEST_F(CircularBufferTest, ErrorConditions) {
    // 测试空缓冲区访问
    EXPECT_THROW(buffer->get(0), std::out_of_range);
    
    // 添加数据后测试
    buffer->forward(1.0);
    
    // 测试正索引（不支持）
    EXPECT_THROW(buffer->get(1), std::invalid_argument);
    
    // 测试超出范围的负索引
    EXPECT_THROW(buffer->get(-2), std::out_of_range);
    
    // 测试过度向后移动
    EXPECT_THROW(buffer->backward(2), std::out_of_range);
}

TEST_F(CircularBufferTest, ConstAccess) {
    buffer->forward(1.0);
    buffer->forward(2.0);
    
    const auto& const_buffer = *buffer;
    EXPECT_DOUBLE_EQ(const_buffer[0], 2.0);
    EXPECT_DOUBLE_EQ(const_buffer[-1], 1.0);
    EXPECT_DOUBLE_EQ(const_buffer.get(0), 2.0);
    EXPECT_DOUBLE_EQ(const_buffer.get(-1), 1.0);
}

// 性能测试
TEST_F(CircularBufferTest, PerformanceTest) {
    const size_t large_size = 100000;
    CircularBuffer<double> large_buffer(large_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 大量数据写入
    for (size_t i = 0; i < large_size * 2; ++i) {
        large_buffer.forward(static_cast<double>(i));
    }
    
    // 大量数据读取
    double sum = 0.0;
    for (int i = 0; i < static_cast<int>(large_size); ++i) {
        sum += large_buffer[-i];
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 性能要求：应该在合理时间内完成
    EXPECT_LT(duration.count(), 100000); // 小于100ms
    EXPECT_GT(sum, 0.0); // 确保计算有效
}

} // namespace test
} // namespace backtrader