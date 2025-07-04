#include "../../include/utils/fractal.h"
#include <algorithm>
#include <stdexcept>

namespace backtrader {
namespace utils {

Fractal::Fractal(const Params& params) : p(params) {
    if (p.period <= 0) {
        throw std::invalid_argument("Period must be positive");
    }
    
    // Initialize data containers
    high_fractals_.clear();
    low_fractals_.clear();
    data_high_.clear();
    data_low_.clear();
}

void Fractal::update(double high, double low, double close, double volume) {
    // Store the new data point
    data_high_.push_back(high);
    data_low_.push_back(low);
    
    // Keep only the required period of data
    if (data_high_.size() > static_cast<size_t>(p.period * 2 + 1)) {
        data_high_.erase(data_high_.begin());
        data_low_.erase(data_low_.begin());
    }
    
    // Need at least period*2+1 data points to identify fractals
    if (data_high_.size() < static_cast<size_t>(p.period * 2 + 1)) {
        return;
    }
    
    // Check for high fractal at the middle point
    size_t middle = data_high_.size() / 2;
    if (isHighFractal(middle)) {
        high_fractals_.push_back({
            static_cast<int>(high_fractals_.size()),
            data_high_[middle],
            true  // is_high
        });
    }
    
    // Check for low fractal at the middle point
    if (isLowFractal(middle)) {
        low_fractals_.push_back({
            static_cast<int>(low_fractals_.size()),
            data_low_[middle],
            false  // is_high
        });
    }
    
    // Limit the number of stored fractals
    if (high_fractals_.size() > p.max_fractals) {
        high_fractals_.erase(high_fractals_.begin());
    }
    if (low_fractals_.size() > p.max_fractals) {
        low_fractals_.erase(low_fractals_.begin());
    }
}

bool Fractal::isHighFractal(size_t index) const {
    if (index < p.period || index >= data_high_.size() - p.period) {
        return false;
    }
    
    double current_high = data_high_[index];
    
    // Check if current high is higher than all highs within the period
    for (int i = 1; i <= p.period; ++i) {
        if (data_high_[index - i] >= current_high || 
            data_high_[index + i] >= current_high) {
            return false;
        }
    }
    
    return true;
}

bool Fractal::isLowFractal(size_t index) const {
    if (index < p.period || index >= data_low_.size() - p.period) {
        return false;
    }
    
    double current_low = data_low_[index];
    
    // Check if current low is lower than all lows within the period
    for (int i = 1; i <= p.period; ++i) {
        if (data_low_[index - i] <= current_low || 
            data_low_[index + i] <= current_low) {
            return false;
        }
    }
    
    return true;
}

std::vector<Fractal::FractalPoint> Fractal::getHighFractals() const {
    return high_fractals_;
}

std::vector<Fractal::FractalPoint> Fractal::getLowFractals() const {
    return low_fractals_;
}

std::vector<Fractal::FractalPoint> Fractal::getAllFractals() const {
    std::vector<FractalPoint> all_fractals;
    
    // Combine high and low fractals
    all_fractals.insert(all_fractals.end(), high_fractals_.begin(), high_fractals_.end());
    all_fractals.insert(all_fractals.end(), low_fractals_.begin(), low_fractals_.end());
    
    // Sort by index
    std::sort(all_fractals.begin(), all_fractals.end(), 
              [](const FractalPoint& a, const FractalPoint& b) {
                  return a.index < b.index;
              });
    
    return all_fractals;
}

Fractal::FractalPoint Fractal::getLastHighFractal() const {
    if (high_fractals_.empty()) {
        return FractalPoint{-1, 0.0, true};
    }
    return high_fractals_.back();
}

Fractal::FractalPoint Fractal::getLastLowFractal() const {
    if (low_fractals_.empty()) {
        return FractalPoint{-1, 0.0, false};
    }
    return low_fractals_.back();
}

bool Fractal::hasHighFractal() const {
    return !high_fractals_.empty();
}

bool Fractal::hasLowFractal() const {
    return !low_fractals_.empty();
}

int Fractal::getHighFractalCount() const {
    return static_cast<int>(high_fractals_.size());
}

int Fractal::getLowFractalCount() const {
    return static_cast<int>(low_fractals_.size());
}

void Fractal::clear() {
    high_fractals_.clear();
    low_fractals_.clear();
    data_high_.clear();
    data_low_.clear();
}

double Fractal::getSupport() const {
    if (low_fractals_.empty()) {
        return 0.0;
    }
    
    // Find the lowest fractal point as support
    auto min_it = std::min_element(low_fractals_.begin(), low_fractals_.end(),
                                   [](const FractalPoint& a, const FractalPoint& b) {
                                       return a.value < b.value;
                                   });
    
    return min_it->value;
}

double Fractal::getResistance() const {
    if (high_fractals_.empty()) {
        return 0.0;
    }
    
    // Find the highest fractal point as resistance
    auto max_it = std::max_element(high_fractals_.begin(), high_fractals_.end(),
                                   [](const FractalPoint& a, const FractalPoint& b) {
                                       return a.value < b.value;
                                   });
    
    return max_it->value;
}

std::vector<double> Fractal::getSupportLevels(int count) const {
    std::vector<double> levels;
    
    if (low_fractals_.empty()) {
        return levels;
    }
    
    // Sort low fractals by value
    std::vector<FractalPoint> sorted_lows = low_fractals_;
    std::sort(sorted_lows.begin(), sorted_lows.end(),
              [](const FractalPoint& a, const FractalPoint& b) {
                  return a.value < b.value;
              });
    
    // Take the lowest 'count' levels
    int actual_count = std::min(count, static_cast<int>(sorted_lows.size()));
    for (int i = 0; i < actual_count; ++i) {
        levels.push_back(sorted_lows[i].value);
    }
    
    return levels;
}

std::vector<double> Fractal::getResistanceLevels(int count) const {
    std::vector<double> levels;
    
    if (high_fractals_.empty()) {
        return levels;
    }
    
    // Sort high fractals by value (descending)
    std::vector<FractalPoint> sorted_highs = high_fractals_;
    std::sort(sorted_highs.begin(), sorted_highs.end(),
              [](const FractalPoint& a, const FractalPoint& b) {
                  return a.value > b.value;
              });
    
    // Take the highest 'count' levels
    int actual_count = std::min(count, static_cast<int>(sorted_highs.size()));
    for (int i = 0; i < actual_count; ++i) {
        levels.push_back(sorted_highs[i].value);
    }
    
    return levels;
}

bool Fractal::isNearSupport(double price, double tolerance) const {
    double support = getSupport();
    if (support == 0.0) {
        return false;
    }
    
    double diff = std::abs(price - support);
    return diff <= tolerance;
}

bool Fractal::isNearResistance(double price, double tolerance) const {
    double resistance = getResistance();
    if (resistance == 0.0) {
        return false;
    }
    
    double diff = std::abs(price - resistance);
    return diff <= tolerance;
}

} // namespace utils
} // namespace backtrader