#include "indicators/hurst.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {

// HurstExponent implementation
HurstExponent::HurstExponent() : Indicator() {
    setup_lines();
    
    // Set lag range
    lag_start_ = params.lag_start;
    lag_end_ = (params.lag_end > 0) ? params.lag_end : (params.period / 2);
    
    _minperiod(params.period);
    
    // Prepare lags array
    for (int lag = lag_start_; lag < lag_end_; ++lag) {
        lags_.push_back(lag);
        log10_lags_.push_back(std::log10(static_cast<double>(lag)));
    }
}

void HurstExponent::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void HurstExponent::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto hurst_line = lines->getline(hurst);
    
    if (!data_line || !hurst_line) return;
    
    // Get period data
    std::vector<double> ts;
    for (int i = params.period - 1; i >= 0; --i) {
        ts.push_back((*data_line)[-i]);
    }
    
    // Calculate Hurst exponent
    double hurst_value = calculate_hurst(ts);
    hurst_line->set(0, hurst_value);
}

void HurstExponent::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto hurst_line = lines->getline(hurst);
    
    if (!data_line || !hurst_line) return;
    
    for (int i = start; i < end; ++i) {
        // Get period data
        std::vector<double> ts;
        for (int j = 0; j < params.period; ++j) {
            ts.push_back((*data_line)[i - j]);
        }
        
        // Calculate Hurst exponent
        double hurst_value = calculate_hurst(ts);
        hurst_line->set(i, hurst_value);
    }
}

double HurstExponent::calculate_hurst(const std::vector<double>& data) {
    if (data.size() < params.period || lags_.empty()) {
        return 0.5; // Default to random walk
    }
    
    // Calculate the array of the variances of the lagged differences
    std::vector<double> tau;
    
    for (int lag : lags_) {
        if (lag >= data.size()) continue;
        
        // Calculate lagged differences
        std::vector<double> diffs;
        for (size_t i = lag; i < data.size(); ++i) {
            diffs.push_back(data[i] - data[i - lag]);
        }
        
        if (diffs.empty()) {
            tau.push_back(0.0);
            continue;
        }
        
        // Calculate standard deviation of differences
        double mean = std::accumulate(diffs.begin(), diffs.end(), 0.0) / diffs.size();
        double variance = 0.0;
        
        for (double diff : diffs) {
            variance += (diff - mean) * (diff - mean);
        }
        variance /= diffs.size();
        
        tau.push_back(std::sqrt(variance));
    }
    
    // Convert tau to log10
    std::vector<double> log10_tau;
    for (double t : tau) {
        if (t > 0.0) {
            log10_tau.push_back(std::log10(t));
        } else {
            log10_tau.push_back(std::log10(1e-10)); // Avoid log(0)
        }
    
    // Ensure we have matching sizes
    size_t min_size = std::min(log10_lags_.size(), log10_tau.size());
    if (min_size < 2) {
        return 0.5; // Not enough data for regression
    }
    
    std::vector<double> x_data(log10_lags_.begin(), log10_lags_.begin() + min_size);
    std::vector<double> y_data(log10_tau.begin(), log10_tau.begin() + min_size);
    
    // Use linear regression to estimate the Hurst Exponent
    auto [slope, intercept] = linear_regression(x_data, y_data);
    
    // Return the Hurst exponent (slope * 2.0)
    return slope * 2.0;
}

std::pair<double, double> HurstExponent::linear_regression(const std::vector<double>& x, 
                                                          const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) {
        return {0.5, 0.0}; // Default slope for random walk
    }
    
    size_t n = x.size();
    
    // Calculate means
    double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / n;
    double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / n;
    
    // Calculate slope (beta)
    double numerator = 0.0;
    double denominator = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double x_diff = x[i] - mean_x;
        double y_diff = y[i] - mean_y;
        
        numerator += x_diff * y_diff;
        denominator += x_diff * x_diff;
    }
    
    double slope = (denominator != 0.0) ? numerator / denominator : 0.5;
    double intercept = mean_y - slope * mean_x;
    
    return {slope, intercept};
}

}
} // namespace backtrader