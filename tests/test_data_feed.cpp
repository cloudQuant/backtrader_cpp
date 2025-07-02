#include <gtest/gtest.h>
#include "data/DataFeed.h"
#include "data/CSVDataFeed.h"
#include "test_utils/TestDataProvider.h"

using namespace backtrader::data;

class DataFeedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试数据
        test_data = createTestOHLCVData(100);
    }
    
    std::vector<OHLCVData> test_data;
};

TEST_F(DataFeedTest, StaticDataFeedBasic) {
    StaticDataFeed feed(test_data, "TestData");
    
    EXPECT_EQ(feed.getName(), "TestData");
    EXPECT_EQ(feed.getTotalDataCount(), 100);
    EXPECT_TRUE(feed.hasNext());
    EXPECT_EQ(feed.len(), 0);  // 初始长度为0
}

TEST_F(DataFeedTest, StaticDataFeedIteration) {
    StaticDataFeed feed(test_data, "TestData");
    
    size_t count = 0;
    while (feed.hasNext()) {
        EXPECT_TRUE(feed.next());
        count++;
        EXPECT_EQ(feed.len(), count);
    }
    
    EXPECT_EQ(count, 100);
    EXPECT_FALSE(feed.hasNext());
    EXPECT_FALSE(feed.next());
}

TEST_F(DataFeedTest, StaticDataFeedDataAccess) {
    StaticDataFeed feed(test_data, "TestData");
    
    // 加载一些数据
    for (int i = 0; i < 10; ++i) {
        feed.next();
    }
    
    // 测试数据访问
    EXPECT_FALSE(isNaN(feed.close()->get(0)));
    EXPECT_FALSE(isNaN(feed.open()->get(0)));
    EXPECT_FALSE(isNaN(feed.high()->get(0)));
    EXPECT_FALSE(isNaN(feed.low()->get(0)));
    
    // 测试OHLCV获取
    auto current_ohlcv = feed.getCurrentOHLCV();
    EXPECT_TRUE(current_ohlcv.isValid());
}

TEST_F(DataFeedTest, StaticDataFeedReset) {
    StaticDataFeed feed(test_data, "TestData");
    
    // 加载一些数据
    for (int i = 0; i < 50; ++i) {
        feed.next();
    }
    
    EXPECT_EQ(feed.len(), 50);
    
    // 重置
    feed.reset();
    EXPECT_EQ(feed.len(), 0);
    EXPECT_TRUE(feed.hasNext());
    EXPECT_EQ(feed.getRemainingDataCount(), 100);
}

TEST_F(DataFeedTest, StaticDataFeedBatchLoading) {
    StaticDataFeed feed(test_data, "TestData");
    
    // 批量加载
    size_t loaded = feed.loadBatch(30);
    EXPECT_EQ(loaded, 30);
    EXPECT_EQ(feed.len(), 30);
    EXPECT_EQ(feed.getRemainingDataCount(), 70);
    
    // 加载剩余全部
    loaded = feed.loadBatch();
    EXPECT_EQ(loaded, 70);
    EXPECT_EQ(feed.len(), 100);
    EXPECT_EQ(feed.getRemainingDataCount(), 0);
}

TEST_F(DataFeedTest, StaticDataFeedSkip) {
    StaticDataFeed feed(test_data, "TestData");
    
    // 跳过一些数据
    size_t skipped = feed.skip(20);
    EXPECT_EQ(skipped, 20);
    EXPECT_EQ(feed.len(), 20);
    EXPECT_EQ(feed.getRemainingDataCount(), 80);
}

TEST_F(DataFeedTest, DataFeedFactory) {
    // 测试随机数据生成
    auto random_feed = DataFeedFactory::createRandom(50, 100.0, 0.02, "Random");
    EXPECT_EQ(random_feed->getName(), "Random");
    
    auto static_feed = dynamic_cast<StaticDataFeed*>(random_feed.get());
    ASSERT_NE(static_feed, nullptr);
    EXPECT_EQ(static_feed->getTotalDataCount(), 50);
    
    // 测试正弦波数据生成
    auto sine_feed = DataFeedFactory::createSineWave(30, 10.0, 0.1, 100.0, "SineWave");
    EXPECT_EQ(sine_feed->getName(), "SineWave");
    
    static_feed = dynamic_cast<StaticDataFeed*>(sine_feed.get());
    ASSERT_NE(static_feed, nullptr);
    EXPECT_EQ(static_feed->getTotalDataCount(), 30);
}

TEST_F(DataFeedTest, DataValidation) {
    StaticDataFeed feed(test_data, "TestData");
    
    // 加载所有数据
    feed.loadBatch();
    
    // 验证数据
    EXPECT_TRUE(feed.validate());
}

TEST_F(DataFeedTest, ParameterManagement) {
    StaticDataFeed feed(test_data, "TestData");
    
    // 设置和获取参数
    feed.setParam("test_param", "test_value");
    EXPECT_EQ(feed.getParam("test_param"), "test_value");
    EXPECT_TRUE(feed.hasParam("test_param"));
    EXPECT_FALSE(feed.hasParam("non_existent"));
    
    // 默认值
    EXPECT_EQ(feed.getParam("non_existent", "default"), "default");
}