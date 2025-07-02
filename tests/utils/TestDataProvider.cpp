#include "TestDataProvider.h"
#include <random>
#include <cmath>

namespace backtrader {
namespace test {

std::vector<double> TestDataProvider::generateRandomData(size_t count, 
                                                        double mean, 
                                                        double stddev,
                                                        uint32_t seed) {
    std::mt19937 generator(seed);
    std::normal_distribution<double> distribution(mean, stddev);
    
    std::vector<double> data;
    data.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        data.push_back(distribution(generator));
    }
    
    return data;
}

std::vector<double> TestDataProvider::generateTrendingData(size_t count, 
                                                          double start_value,
                                                          double trend_rate,
                                                          double noise_level,
                                                          uint32_t seed) {
    std::mt19937 generator(seed);
    std::normal_distribution<double> noise(0.0, noise_level);
    
    std::vector<double> data;
    data.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        double trend_value = start_value + i * trend_rate;
        double noise_value = noise(generator);
        data.push_back(trend_value + noise_value);
    }
    
    return data;
}

std::vector<double> TestDataProvider::generateSineWave(size_t count,
                                                      double amplitude,
                                                      double frequency,
                                                      double phase,
                                                      double offset) {
    std::vector<double> data;
    data.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        double t = static_cast<double>(i);
        double value = offset + amplitude * std::sin(2.0 * M_PI * frequency * t / count + phase);
        data.push_back(value);
    }
    
    return data;
}

std::vector<double> TestDataProvider::generateStepFunction(size_t count,
                                                          const std::vector<double>& levels,
                                                          size_t step_size) {
    std::vector<double> data;
    data.reserve(count);
    
    if (levels.empty()) {
        return data;
    }
    
    size_t level_index = 0;
    for (size_t i = 0; i < count; ++i) {
        if (i > 0 && i % step_size == 0) {
            level_index = (level_index + 1) % levels.size();
        }
        data.push_back(levels[level_index]);
    }
    
    return data;
}

std::shared_ptr<LineRoot> TestDataProvider::createLineRootFromData(const std::vector<double>& data,
                                                                  const std::string& name) {
    auto line = std::make_shared<LineRoot>(data.size() + 100, name);
    
    for (double value : data) {
        line->forward(value);
    }
    
    return line;
}

bool TestDataProvider::compareDoubleVectors(const std::vector<double>& a,
                                           const std::vector<double>& b,
                                           double tolerance) {
    if (a.size() != b.size()) {
        return false;
    }
    
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::isnan(a[i]) && std::isnan(b[i])) {
            continue;  // 两个都是NaN，认为相等
        }
        
        if (std::isnan(a[i]) || std::isnan(b[i])) {
            return false;  // 一个是NaN，另一个不是
        }
        
        if (std::abs(a[i] - b[i]) > tolerance) {
            return false;
        }
    }
    
    return true;
}

std::vector<OHLCVData> TestDataProvider::generateOHLCVData(size_t count,
                                                          double initial_price,
                                                          double volatility,
                                                          double trend,
                                                          uint32_t seed) {
    std::mt19937 generator(seed);
    std::normal_distribution<double> price_change(trend, volatility);
    std::uniform_real_distribution<double> range_factor(0.8, 1.2);
    std::uniform_real_distribution<double> volume_factor(0.5, 2.0);
    
    std::vector<OHLCVData> data;
    data.reserve(count);
    
    double current_price = initial_price;
    
    for (size_t i = 0; i < count; ++i) {
        OHLCVData bar;
        
        // 计算收盘价
        current_price += price_change(generator);
        if (current_price <= 0) {
            current_price = 0.01;  // 确保价格为正
        }
        
        // 生成开盘价（基于前一收盘价的小幅波动）
        bar.open = current_price * range_factor(generator);
        bar.close = current_price;
        
        // 生成最高价和最低价
        double high_factor = range_factor(generator);
        double low_factor = range_factor(generator);
        
        bar.high = std::max(bar.open, bar.close) * high_factor;
        bar.low = std::min(bar.open, bar.close) * low_factor;
        
        // 确保高低价的逻辑正确性
        bar.high = std::max({bar.open, bar.high, bar.low, bar.close});
        bar.low = std::min({bar.open, bar.high, bar.low, bar.close});
        
        // 生成成交量
        bar.volume = 1000000.0 * volume_factor(generator);
        
        data.push_back(bar);
    }
    
    return data;
}

} // namespace test
} // namespace backtrader