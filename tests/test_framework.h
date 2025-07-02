#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <memory>
#include <cmath>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>

// 测试框架基础设施
namespace backtrader {
namespace testing {

/**
 * @brief 测试数据生成器
 */
class TestDataGenerator {
public:
    /**
     * @brief 生成随机价格数据
     * @param count 数据点数量
     * @param base_price 基础价格
     * @param volatility 波动率
     * @param seed 随机种子
     * @return OHLCV数据
     */
    static std::vector<std::vector<double>> generateRandomOHLCV(
        size_t count, 
        double base_price = 100.0, 
        double volatility = 0.02,
        unsigned int seed = 42) {
        
        std::mt19937 rng(seed);
        std::normal_distribution<double> dist(0.0, volatility);
        
        std::vector<std::vector<double>> data(5); // OHLCV
        double price = base_price;
        
        for (size_t i = 0; i < count; ++i) {
            double change = dist(rng);
            price *= (1.0 + change);
            price = std::max(1.0, price);
            
            // 生成OHLC
            double high = price * (1.0 + std::abs(dist(rng)) * 0.5);
            double low = price * (1.0 - std::abs(dist(rng)) * 0.5);
            double open = low + (high - low) * (0.2 + 0.6 * std::uniform_real_distribution<double>(0, 1)(rng));
            double close = price;
            double volume = 1000 + std::abs(dist(rng)) * 5000;
            
            data[0].push_back(open);
            data[1].push_back(high);
            data[2].push_back(low);
            data[3].push_back(close);
            data[4].push_back(volume);
        }
        
        return data;
    }
    
    /**
     * @brief 生成正弦波数据
     * @param count 数据点数量
     * @param amplitude 振幅
     * @param frequency 频率
     * @param base_value 基础值
     * @param noise 噪声水平
     * @return 正弦波价格数据
     */
    static std::vector<std::vector<double>> generateSineWaveOHLCV(
        size_t count,
        double amplitude = 10.0,
        double frequency = 0.1,
        double base_value = 100.0,
        double noise = 0.01) {
        
        std::mt19937 rng(42);
        std::normal_distribution<double> noise_dist(0.0, noise);
        
        std::vector<std::vector<double>> data(5);
        
        for (size_t i = 0; i < count; ++i) {
            double angle = 2.0 * M_PI * frequency * i;
            double sine_value = amplitude * std::sin(angle);
            double close = base_value + sine_value + noise_dist(rng) * base_value;
            
            double high = close * (1.0 + std::abs(noise_dist(rng)) * 2);
            double low = close * (1.0 - std::abs(noise_dist(rng)) * 2);
            double open = low + (high - low) * 0.5;
            double volume = 1000 + std::abs(noise_dist(rng)) * 2000;
            
            data[0].push_back(open);
            data[1].push_back(high);
            data[2].push_back(low);
            data[3].push_back(close);
            data[4].push_back(volume);
        }
        
        return data;
    }
    
    /**
     * @brief 生成趋势数据
     * @param count 数据点数量
     * @param trend_slope 趋势斜率
     * @param base_price 基础价格
     * @param volatility 波动率
     * @return 趋势价格数据
     */
    static std::vector<std::vector<double>> generateTrendOHLCV(
        size_t count,
        double trend_slope = 0.1,
        double base_price = 100.0,
        double volatility = 0.015) {
        
        std::mt19937 rng(42);
        std::normal_distribution<double> dist(0.0, volatility);
        
        std::vector<std::vector<double>> data(5);
        
        for (size_t i = 0; i < count; ++i) {
            double trend_value = base_price + trend_slope * i;
            double noise = dist(rng) * trend_value;
            double close = trend_value + noise;
            
            double high = close * (1.0 + std::abs(dist(rng)));
            double low = close * (1.0 - std::abs(dist(rng)));
            double open = low + (high - low) * 0.4;
            double volume = 1000 + std::abs(dist(rng)) * 3000;
            
            data[0].push_back(open);
            data[1].push_back(high);
            data[2].push_back(low);
            data[3].push_back(close);
            data[4].push_back(volume);
        }
        
        return data;
    }
    
    /**
     * @brief 从CSV文件加载测试数据
     * @param filename 文件名
     * @return OHLCV数据
     */
    static std::vector<std::vector<double>> loadFromCSV(const std::string& filename) {
        std::vector<std::vector<double>> data(5);
        std::ifstream file(filename);
        std::string line;
        
        // 跳过标题行
        if (std::getline(file, line)) {
            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string item;
                std::vector<double> row;
                
                while (std::getline(ss, item, ',')) {
                    row.push_back(std::stod(item));
                }
                
                if (row.size() >= 5) {
                    data[0].push_back(row[0]); // Open
                    data[1].push_back(row[1]); // High
                    data[2].push_back(row[2]); // Low
                    data[3].push_back(row[3]); // Close
                    data[4].push_back(row[4]); // Volume
                }
            }
        }
        
        return data;
    }
};

/**
 * @brief 测试工具类
 */
class TestUtils {
public:
    /**
     * @brief 比较浮点数是否近似相等
     * @param a 值a
     * @param b 值b
     * @param tolerance 容差
     * @return 是否近似相等
     */
    static bool isApproximatelyEqual(double a, double b, double tolerance = 1e-10) {
        if (std::isnan(a) && std::isnan(b)) return true;
        if (std::isnan(a) || std::isnan(b)) return false;
        return std::abs(a - b) < tolerance;
    }
    
    /**
     * @brief 验证数组是否近似相等
     * @param expected 期望值数组
     * @param actual 实际值数组
     * @param tolerance 容差
     * @return 是否近似相等
     */
    static bool areArraysApproximatelyEqual(
        const std::vector<double>& expected,
        const std::vector<double>& actual,
        double tolerance = 1e-10) {
        
        if (expected.size() != actual.size()) return false;
        
        for (size_t i = 0; i < expected.size(); ++i) {
            if (!isApproximatelyEqual(expected[i], actual[i], tolerance)) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief 计算数组的统计信息
     * @param data 数据数组
     * @return 统计信息 {mean, std, min, max}
     */
    static std::vector<double> calculateStatistics(const std::vector<double>& data) {
        if (data.empty()) return {0, 0, 0, 0};
        
        double sum = 0;
        double min_val = data[0];
        double max_val = data[0];
        
        for (double value : data) {
            if (!std::isnan(value)) {
                sum += value;
                min_val = std::min(min_val, value);
                max_val = std::max(max_val, value);
            }
        }
        
        double mean = sum / data.size();
        
        double variance = 0;
        for (double value : data) {
            if (!std::isnan(value)) {
                variance += (value - mean) * (value - mean);
            }
        }
        variance /= data.size();
        double std_dev = std::sqrt(variance);
        
        return {mean, std_dev, min_val, max_val};
    }
    
    /**
     * @brief 性能计时器
     */
    class Timer {
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        
    public:
        Timer() : start_time_(std::chrono::high_resolution_clock::now()) {}
        
        double elapsed() const {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time_);
            return duration.count() / 1000.0; // 返回毫秒
        }
        
        void reset() {
            start_time_ = std::chrono::high_resolution_clock::now();
        }
    };
};

/**
 * @brief 内存使用监控器
 */
class MemoryMonitor {
private:
    size_t initial_memory_;
    
public:
    MemoryMonitor() : initial_memory_(getCurrentMemoryUsage()) {}
    
    size_t getCurrentMemoryUsage() const {
        // 简化的内存使用监控
        // 在实际项目中可以使用更精确的系统调用
        return 0;
    }
    
    size_t getMemoryIncrease() const {
        return getCurrentMemoryUsage() - initial_memory_;
    }
};

/**
 * @brief 测试基类
 */
class BacktraderTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
        timer_.reset();
        memory_monitor_ = std::make_unique<MemoryMonitor>();
    }
    
    void TearDown() override {
        // 清理测试环境
        double elapsed = timer_.elapsed();
        if (elapsed > 1000.0) { // 如果超过1秒，记录警告
            std::cout << "Warning: Test took " << elapsed << "ms" << std::endl;
        }
    }
    
    TestUtils::Timer timer_;
    std::unique_ptr<MemoryMonitor> memory_monitor_;
};

/**
 * @brief 参数化测试助手
 */
template<typename T>
class ParameterizedTestHelper {
private:
    std::vector<T> test_cases_;
    
public:
    void addTestCase(const T& test_case) {
        test_cases_.push_back(test_case);
    }
    
    const std::vector<T>& getTestCases() const {
        return test_cases_;
    }
    
    template<typename Func>
    void runAllTests(Func test_function) {
        for (const auto& test_case : test_cases_) {
            test_function(test_case);
        }
    }
};

/**
 * @brief 模拟数据行为的Mock类基础
 */
class MockDataLine {
private:
    std::vector<double> data_;
    size_t position_;
    
public:
    MockDataLine() : position_(0) {}
    
    void setData(const std::vector<double>& data) {
        data_ = data;
        position_ = 0;
    }
    
    void forward(double value) {
        data_.push_back(value);
        position_ = data_.size() - 1;
    }
    
    void forward() {
        if (position_ < data_.size() - 1) {
            position_++;
        }
    }
    
    double get(int ago = 0) const {
        if (data_.empty()) return std::numeric_limits<double>::quiet_NaN();
        
        int index = static_cast<int>(position_) - ago;
        if (index < 0 || index >= static_cast<int>(data_.size())) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        return data_[index];
    }
    
    size_t size() const { return data_.size(); }
    size_t position() const { return position_; }
    void setPosition(size_t pos) { 
        position_ = std::min(pos, data_.size() - 1); 
    }
};

} // namespace testing
} // namespace backtrader

// 有用的测试宏
#define EXPECT_DOUBLE_NEAR(expected, actual, tolerance) \
    EXPECT_TRUE(backtrader::testing::TestUtils::isApproximatelyEqual(expected, actual, tolerance)) \
    << "Expected: " << expected << ", Actual: " << actual << ", Tolerance: " << tolerance

#define EXPECT_ARRAY_NEAR(expected, actual, tolerance) \
    EXPECT_TRUE(backtrader::testing::TestUtils::areArraysApproximatelyEqual(expected, actual, tolerance))

#define EXPECT_NO_NAN(value) \
    EXPECT_FALSE(std::isnan(value)) << "Value should not be NaN: " << value

#define EXPECT_FINITE(value) \
    EXPECT_TRUE(std::isfinite(value)) << "Value should be finite: " << value

#define BENCHMARK_START() \
    auto benchmark_timer = backtrader::testing::TestUtils::Timer()

#define BENCHMARK_END(max_time_ms) \
    do { \
        double elapsed = benchmark_timer.elapsed(); \
        EXPECT_LT(elapsed, max_time_ms) << "Benchmark exceeded time limit: " << elapsed << "ms"; \
    } while(0)