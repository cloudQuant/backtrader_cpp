#include "feeds/DataFeed.h"
#include "Common.h"
#include <random>
#include <cmath>

namespace backtrader {
namespace data {

// DataFeedFactory implementations

std::unique_ptr<DataFeed> DataFeedFactory::createRandom(size_t count,
                                                       double initial_price,
                                                       double volatility,
                                                       const std::string& name) {
    std::vector<OHLCVData> data;
    data.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dis(0.0, volatility);
    
    auto current_time = std::chrono::system_clock::now();
    double current_price = initial_price;
    
    for (size_t i = 0; i < count; ++i) {
        // 生成随机价格变化
        double change = dis(gen);
        current_price *= (1.0 + change);
        
        // 生成OHLC数据
        double open = current_price;
        double high_change = std::abs(dis(gen)) * 0.5;
        double low_change = std::abs(dis(gen)) * 0.5;
        double close_change = dis(gen) * 0.3;
        
        double high = open + high_change;
        double low = open - low_change;
        double close = open + close_change;
        
        // 确保OHLC关系正确
        high = std::max({open, high, close});
        low = std::min({open, low, close});
        
        // 生成随机成交量
        std::uniform_real_distribution<double> vol_dis(1000, 10000);
        double volume = vol_dis(gen);
        
        // 时间递增（每天）
        auto timestamp = current_time + std::chrono::hours(24 * i);
        
        data.emplace_back(timestamp, open, high, low, close, volume);
        current_price = close;
    }
    
    return std::make_unique<StaticDataFeed>(data, name);
}

std::unique_ptr<DataFeed> DataFeedFactory::createSineWave(size_t count,
                                                         double amplitude,
                                                         double frequency,
                                                         double base_price,
                                                         const std::string& name) {
    std::vector<OHLCVData> data;
    data.reserve(count);
    
    auto current_time = std::chrono::system_clock::now();
    
    for (size_t i = 0; i < count; ++i) {
        // 生成正弦波价格
        double phase = frequency * i;
        double sine_value = std::sin(phase);
        double price = base_price + amplitude * sine_value;
        
        // 生成小幅随机波动
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> noise(0.0, amplitude * 0.05);
        
        double open = price + noise(gen);
        double close = price + noise(gen);
        double high = std::max(open, close) + std::abs(noise(gen));
        double low = std::min(open, close) - std::abs(noise(gen));
        
        // 固定成交量
        double volume = 5000.0;
        
        // 时间递增
        auto timestamp = current_time + std::chrono::hours(24 * i);
        
        data.emplace_back(timestamp, open, high, low, close, volume);
    }
    
    return std::make_unique<StaticDataFeed>(data, name);
}

} // namespace data
} // namespace backtrader