#include "indicators/BollingerBands.h"
#include "Common.h"
#include <cmath>
#include <numeric>

namespace backtrader {

void BollingerBands::calculate() {
    if (!hasValidInput()) {
        setAllOutputsNaN();
        return;
    }
    
    auto input = getInput(0);
    double current_value = input->get(0);
    
    if (isNaN(current_value)) {
        setAllOutputsNaN();
        return;
    }
    
    // 首先计算SMA（中轨）
    sma_->calculate();
    double middle_band = sma_->get(0);
    
    if (isNaN(middle_band)) {
        setAllOutputsNaN();
        return;
    }
    
    // 设置中轨
    setOutput(0, middle_band);
    
    // 计算标准差
    double std_dev = calculateStandardDeviation(middle_band);
    
    if (isNaN(std_dev)) {
        setOutput(1, NaN);  // 上轨
        setOutput(2, NaN);  // 下轨
        return;
    }
    
    // 计算上轨和下轨
    double upper_band = middle_band + dev_factor_ * std_dev;
    double lower_band = middle_band - dev_factor_ * std_dev;
    
    setOutput(1, upper_band);
    setOutput(2, lower_band);
}

void BollingerBands::calculateBatch(size_t start, size_t end) {
    if (!hasValidInput()) {
        return;
    }
    
    auto input = getInput(0);
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            input->forward();
        }
    }
}

double BollingerBands::calculateStandardDeviation(double mean) const {
    if (use_incremental_) {
        return const_cast<BollingerBands*>(this)->calculateStandardDeviationIncremental(mean);
    } else {
        return const_cast<BollingerBands*>(this)->calculateStandardDeviationDirect(mean);
    }
}

double BollingerBands::calculateStandardDeviationIncremental(double mean) {
    auto input = getInput(0);
    double current_value = input->get(0);
    
    // 维护滑动窗口
    window_.push_back(current_value);
    if (window_.size() > period_) {
        window_.pop_front();
    }
    
    // 如果窗口大小不足，返回NaN
    if (window_.size() < period_) {
        return NaN;
    }
    
    // 计算方差
    double variance = 0.0;
    for (double value : window_) {
        double diff = value - mean;
        variance += diff * diff;
    }
    variance /= period_;
    
    return std::sqrt(variance);
}

double BollingerBands::calculateStandardDeviationDirect(double mean) {
    auto input = getInput(0);
    
    // 检查是否有足够的数据
    if (input->len() < period_) {
        return NaN;
    }
    
    // 从输入中获取最近period_个值并计算标准差
    double variance = 0.0;
    for (size_t i = 0; i < period_; ++i) {
        double value = input->get(-static_cast<int>(i));
        if (isNaN(value)) {
            return NaN;
        }
        double diff = value - mean;
        variance += diff * diff;
    }
    variance /= period_;
    
    return std::sqrt(variance);
}

double BollingerBands::getTrendStrength() const {
    try {
        auto input = getInput(0);
        double price = input->get(0);
        double upper = getUpperBand(0);
        double lower = getLowerBand(0);
        double middle = getMiddleBand(0);
        
        if (isNaN(price) || isNaN(upper) || isNaN(lower) || isNaN(middle)) {
            return 0.0;
        }
        
        // 计算价格相对于布林带的位置
        double band_width = upper - lower;
        if (band_width == 0.0) {
            return 0.0;
        }
        
        // 距离中轨的相对距离
        double distance_from_middle = std::abs(price - middle);
        return std::min(1.0, distance_from_middle / (band_width / 2.0));
        
    } catch (...) {
        return 0.0;
    }
}

void BollingerBands::setAllOutputsNaN() {
    setOutput(0, NaN);  // 中轨
    setOutput(1, NaN);  // 上轨
    setOutput(2, NaN);  // 下轨
}

} // namespace backtrader