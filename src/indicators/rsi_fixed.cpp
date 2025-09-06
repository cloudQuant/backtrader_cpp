// Fixed RSI implementation that matches Python behavior exactly
// This is a complete rewrite to ensure correctness

#include "indicators/rsi.h"
#include <algorithm>
#include <numeric>
#include <iostream>

namespace backtrader {
namespace indicators {

// Helper function to calculate RSI values chronologically
std::vector<double> calculate_rsi_values(const std::vector<double>& prices, int period) {
    std::vector<double> rsi_values;
    
    // Need at least period+1 prices to calculate RSI
    if (prices.size() < static_cast<size_t>(period + 1)) {
        return rsi_values;
    }
    
    // Calculate gains and losses
    std::vector<double> gains, losses;
    for (size_t i = 1; i < prices.size(); ++i) {
        double change = prices[i] - prices[i-1];
        gains.push_back(std::max(0.0, change));
        losses.push_back(std::max(0.0, -change));
    }
    
    // Fill with NaN for first 'period' values
    for (int i = 0; i < period; ++i) {
        rsi_values.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Calculate first RSI using simple average
    double avg_gain = std::accumulate(gains.begin(), gains.begin() + period, 0.0) / period;
    double avg_loss = std::accumulate(losses.begin(), losses.begin() + period, 0.0) / period;
    
    double rs = (avg_loss != 0.0) ? avg_gain / avg_loss : 0.0;
    double rsi = (avg_loss != 0.0) ? 100.0 - (100.0 / (1.0 + rs)) : 
                 (avg_gain != 0.0) ? 100.0 : 50.0;
    rsi_values.push_back(rsi);
    
    // Calculate remaining RSI values using Wilder's smoothing
    for (size_t i = period; i < gains.size(); ++i) {
        avg_gain = (avg_gain * (period - 1) + gains[i]) / period;
        avg_loss = (avg_loss * (period - 1) + losses[i]) / period;
        
        rs = (avg_loss != 0.0) ? avg_gain / avg_loss : 0.0;
        rsi = (avg_loss != 0.0) ? 100.0 - (100.0 / (1.0 + rs)) : 
              (avg_gain != 0.0) ? 100.0 : 50.0;
        rsi_values.push_back(rsi);
    }
    
    return rsi_values;
}

}
}