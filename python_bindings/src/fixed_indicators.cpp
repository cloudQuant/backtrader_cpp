#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>

namespace py = pybind11;
using namespace pybind11::literals;

// Fixed RSI implementation using Wilder's smoothing (matches Backtrader)
std::vector<double> calculate_rsi_fixed(const std::vector<double>& prices, int period = 14) {
    std::vector<double> result(prices.size(), std::numeric_limits<double>::quiet_NaN());
    
    if (prices.size() < static_cast<size_t>(period + 1)) {
        return result;
    }
    
    // Calculate price changes
    std::vector<double> deltas;
    for (size_t i = 1; i < prices.size(); ++i) {
        deltas.push_back(prices[i] - prices[i-1]);
    }
    
    // Initialize with simple average for first period
    double avg_gain = 0.0;
    double avg_loss = 0.0;
    
    for (int i = 0; i < period; ++i) {
        if (deltas[i] > 0) {
            avg_gain += deltas[i];
        } else {
            avg_loss -= deltas[i];  // Make positive
        }
    }
    
    avg_gain /= period;
    avg_loss /= period;
    
    // Calculate first RSI value
    double rs = (avg_loss != 0) ? avg_gain / avg_loss : 0;
    result[period] = 100.0 - (100.0 / (1.0 + rs));
    
    // Use Wilder's smoothing for remaining values
    for (size_t i = period; i < deltas.size(); ++i) {
        double gain = (deltas[i] > 0) ? deltas[i] : 0;
        double loss = (deltas[i] < 0) ? -deltas[i] : 0;
        
        // Wilder's smoothing: (prev * (n-1) + current) / n
        avg_gain = (avg_gain * (period - 1) + gain) / period;
        avg_loss = (avg_loss * (period - 1) + loss) / period;
        
        rs = (avg_loss != 0) ? avg_gain / avg_loss : 0;
        result[i + 1] = 100.0 - (100.0 / (1.0 + rs));
    }
    
    return result;
}

// Fixed EMA implementation (matches Backtrader more closely)
std::vector<double> calculate_ema_fixed(const std::vector<double>& prices, int period) {
    std::vector<double> result(prices.size(), std::numeric_limits<double>::quiet_NaN());
    
    if (prices.size() < static_cast<size_t>(period)) {
        return result;
    }
    
    // Calculate initial SMA
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += prices[i];
    }
    result[period - 1] = sum / period;
    
    // Calculate multiplier (alpha)
    double multiplier = 2.0 / (period + 1.0);
    
    // Calculate EMA
    for (size_t i = period; i < prices.size(); ++i) {
        result[i] = prices[i] * multiplier + result[i - 1] * (1.0 - multiplier);
    }
    
    return result;
}

// Fixed ATR implementation (matches Backtrader exactly)
std::vector<double> calculate_atr_fixed(const std::vector<double>& highs, 
                                        const std::vector<double>& lows,
                                        const std::vector<double>& closes,
                                        int period = 14) {
    std::vector<double> result(highs.size(), std::numeric_limits<double>::quiet_NaN());
    
    if (highs.size() != lows.size() || highs.size() != closes.size() || 
        highs.size() < static_cast<size_t>(period + 1)) {
        return result;
    }
    
    // Calculate True Range values exactly like Backtrader
    std::vector<double> tr_values;
    
    // For the first bar, TR = high - low (no previous close)
    tr_values.push_back(highs[0] - lows[0]);
    
    // For subsequent bars, use Backtrader's simplified formula:
    // TR = TrueHigh - TrueLow where TrueHigh = max(high, prev_close), TrueLow = min(low, prev_close)
    for (size_t i = 1; i < highs.size(); ++i) {
        double true_high = std::max(highs[i], closes[i - 1]);
        double true_low = std::min(lows[i], closes[i - 1]);
        tr_values.push_back(true_high - true_low);
    }
    
    // Backtrader ATR starts calculation after 'period' bars
    // First ATR value is simple average of first 'period' TR values
    if (tr_values.size() >= static_cast<size_t>(period)) {
        double sum = 0.0;
        for (int i = 0; i < period; ++i) {
            sum += tr_values[i];
        }
        result[period - 1] = sum / period;  // First ATR at index period-1
        
        // Use Wilder's smoothing for remaining values: ATR = prev * (1-alpha) + TR * alpha
        double alpha = 1.0 / period;  // Wilder's alpha
        for (size_t i = period; i < tr_values.size(); ++i) {
            result[i] = result[i - 1] * (1.0 - alpha) + tr_values[i] * alpha;
        }
    }
    
    return result;
}

// Fixed CCI implementation (matches Backtrader exactly)
std::vector<double> calculate_cci_fixed(const std::vector<double>& highs,
                                        const std::vector<double>& lows,
                                        const std::vector<double>& closes,
                                        int period = 20) {
    std::vector<double> result(highs.size(), std::numeric_limits<double>::quiet_NaN());
    
    if (highs.size() != lows.size() || highs.size() != closes.size() || 
        highs.size() < static_cast<size_t>(2 * period - 1)) {
        return result;
    }
    
    // Calculate Typical Price
    std::vector<double> tp;
    for (size_t i = 0; i < highs.size(); ++i) {
        tp.push_back((highs[i] + lows[i] + closes[i]) / 3.0);
    }
    
    // Calculate SMA of Typical Price first
    std::vector<double> tp_sma(tp.size(), std::numeric_limits<double>::quiet_NaN());
    for (size_t i = period - 1; i < tp.size(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < period; ++j) {
            sum += tp[i - j];
        }
        tp_sma[i] = sum / period;
    }
    
    // Calculate absolute deviations from SMA
    std::vector<double> abs_dev;
    for (size_t i = 0; i < tp.size(); ++i) {
        if (!std::isnan(tp_sma[i])) {
            abs_dev.push_back(std::abs(tp[i] - tp_sma[i]));
        } else {
            abs_dev.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Calculate SMA of absolute deviations (this is the mean deviation)
    std::vector<double> mean_dev(abs_dev.size(), std::numeric_limits<double>::quiet_NaN());
    for (size_t i = period - 1; i < abs_dev.size(); ++i) {
        if (i >= period - 1 && !std::isnan(abs_dev[i - period + 1])) {
            double sum = 0.0;
            for (int j = 0; j < period; ++j) {
                if (!std::isnan(abs_dev[i - j])) {
                    sum += abs_dev[i - j];
                } else {
                    sum = std::numeric_limits<double>::quiet_NaN();
                    break;
                }
            }
            if (!std::isnan(sum)) {
                mean_dev[i] = sum / period;
            }
        }
    }
    
    // Calculate CCI = (TP - TP_SMA) / (0.015 * Mean_Dev)
    // CCI starts at index (2 * period - 2) because we need:
    // - period values for TP SMA
    // - period values for Mean Deviation SMA
    for (size_t i = 2 * period - 2; i < tp.size(); ++i) {
        if (!std::isnan(tp_sma[i]) && !std::isnan(mean_dev[i]) && mean_dev[i] != 0) {
            result[i] = (tp[i] - tp_sma[i]) / (0.015 * mean_dev[i]);
        } else {
            result[i] = 0.0;
        }
    }
    
    return result;
}

// Fixed Stochastic implementation (matches Backtrader exactly)
py::dict calculate_stochastic_fixed(const std::vector<double>& highs,
                                    const std::vector<double>& lows,
                                    const std::vector<double>& closes,
                                    int k_period = 14,
                                    int d_period = 3) {
    std::vector<double> k_values(highs.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> d_values(highs.size(), std::numeric_limits<double>::quiet_NaN());
    
    if (highs.size() != lows.size() || highs.size() != closes.size()) {
        return py::dict("k"_a=k_values, "d"_a=d_values);
    }
    
    // Step 1: Calculate raw %K (Fast Stochastic %K)
    std::vector<double> raw_k(highs.size(), std::numeric_limits<double>::quiet_NaN());
    for (size_t i = k_period - 1; i < highs.size(); ++i) {
        // Look back exactly k_period values: [i-k_period+1, i]
        double highest = highs[i - k_period + 1];
        double lowest = lows[i - k_period + 1];
        
        for (int j = 1; j < k_period; ++j) {
            highest = std::max(highest, highs[i - k_period + 1 + j]);
            lowest = std::min(lowest, lows[i - k_period + 1 + j]);
        }
        
        if (highest != lowest) {
            raw_k[i] = 100.0 * (closes[i] - lowest) / (highest - lowest);
        } else {
            raw_k[i] = 50.0;  // Default when no range
        }
    }
    
    // Step 2: Calculate smoothed %K (SMA of raw %K over d_period)
    // This is what Backtrader calls "slow stochastic %K"
    for (size_t i = k_period + d_period - 2; i < raw_k.size(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < d_period; ++j) {
            sum += raw_k[i - j];
        }
        k_values[i] = sum / d_period;
    }
    
    // Step 3: Calculate %D (SMA of smoothed %K over d_period)
    // This needs another d_period values, so starts at k_period + 2*d_period - 3
    for (size_t i = k_period + 2 * d_period - 3; i < k_values.size(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < d_period; ++j) {
            if (!std::isnan(k_values[i - j])) {
                sum += k_values[i - j];
            }
        }
        d_values[i] = sum / d_period;
    }
    
    return py::dict("k"_a=k_values, "d"_a=d_values);
}

PYBIND11_MODULE(fixed_indicators, m) {
    m.doc() = "Fixed indicator implementations matching Backtrader";
    
    m.def("calculate_rsi", &calculate_rsi_fixed, 
          "Calculate RSI using Wilder's smoothing (matches Backtrader)",
          "prices"_a, "period"_a = 14);
          
    m.def("calculate_ema", &calculate_ema_fixed,
          "Calculate EMA matching Backtrader",
          "prices"_a, "period"_a);
          
    m.def("calculate_atr", &calculate_atr_fixed,
          "Calculate ATR using Wilder's smoothing (matches Backtrader)",
          "highs"_a, "lows"_a, "closes"_a, "period"_a = 14);
          
    m.def("calculate_cci", &calculate_cci_fixed,
          "Calculate CCI matching Backtrader",
          "highs"_a, "lows"_a, "closes"_a, "period"_a = 20);
          
    m.def("calculate_stochastic", &calculate_stochastic_fixed,
          "Calculate Stochastic matching Backtrader",
          "highs"_a, "lows"_a, "closes"_a, "k_period"_a = 14, "d_period"_a = 3);
}