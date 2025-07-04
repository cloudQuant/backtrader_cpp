#include "../../../include/indicators/contrib/vortex.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace backtrader {
namespace indicators {
namespace contrib {

// Vortex implementation

Vortex::Vortex(const Params& params) : params_(params) {
    if (params_.period <= 0) {
        throw std::invalid_argument("Vortex period must be positive");
    }
    
    initialize_lines();
    setup_plot_info();
    
    // Reserve space for calculation vectors
    vm_plus_values_.reserve(params_.period + 10);
    vm_minus_values_.reserve(params_.period + 10);
    true_range_values_.reserve(params_.period + 10);
}

Vortex::Vortex(int period) {
    params_.period = period;
    
    if (params_.period <= 0) {
        throw std::invalid_argument("Vortex period must be positive");
    }
    
    initialize_lines();
    setup_plot_info();
    
    vm_plus_values_.reserve(params_.period + 10);
    vm_minus_values_.reserve(params_.period + 10);
    true_range_values_.reserve(params_.period + 10);
}

void Vortex::start() {
    Indicator::start();
    
    // Clear calculation vectors
    vm_plus_values_.clear();
    vm_minus_values_.clear();
    true_range_values_.clear();
    
    // Reset previous values
    previous_high_ = std::numeric_limits<double>::quiet_NaN();
    previous_low_ = std::numeric_limits<double>::quiet_NaN();
    previous_close_ = std::numeric_limits<double>::quiet_NaN();
    has_previous_data_ = false;
}

void Vortex::stop() {
    Indicator::stop();
    
    // Clean up calculation vectors
    vm_plus_values_.clear();
    vm_minus_values_.clear();
    true_range_values_.clear();
    vm_plus_values_.shrink_to_fit();
    vm_minus_values_.shrink_to_fit();
    true_range_values_.shrink_to_fit();
}

void Vortex::prenext() {
    if (!data_ || data_->lines.size() < 4) {  // Need OHLC
        return;
    }
    
    const auto& open_line = data_->lines[0];
    const auto& high_line = data_->lines[1];
    const auto& low_line = data_->lines[2];
    const auto& close_line = data_->lines[3];
    
    if (open_line.empty() || high_line.empty() || low_line.empty() || close_line.empty()) {
        return;
    }
    
    double high = high_line.back();
    double low = low_line.back();
    double close = close_line.back();
    
    update_calculation_data(high, low, close);
}

void Vortex::next() {
    prenext();  // Update calculation data
    
    if (has_enough_data()) {
        calculate_vortex_values();
    }
}

void Vortex::once(int start, int end) {
    if (!data_ || data_->lines.size() < 4) {
        return;
    }
    
    const auto& open_line = data_->lines[0];
    const auto& high_line = data_->lines[1];
    const auto& low_line = data_->lines[2];
    const auto& close_line = data_->lines[3];
    
    for (int i = start; i <= end && i < static_cast<int>(open_line.size()); ++i) {
        double high = high_line[i];
        double low = low_line[i];
        double close = close_line[i];
        
        update_calculation_data(high, low, close);
        
        if (has_enough_data()) {
            calculate_vortex_values();
        }
    }
}

double Vortex::get_vi_plus(int ago) const {
    return get_line_value(Lines::VI_PLUS, ago);
}

double Vortex::get_vi_minus(int ago) const {
    return get_line_value(Lines::VI_MINUS, ago);
}

std::pair<double, double> Vortex::get_vi_values(int ago) const {
    return {get_vi_plus(ago), get_vi_minus(ago)};
}

bool Vortex::is_uptrend_signal(int ago) const {
    double vi_plus = get_vi_plus(ago);
    double vi_minus = get_vi_minus(ago);
    
    return !std::isnan(vi_plus) && !std::isnan(vi_minus) && vi_plus > vi_minus;
}

bool Vortex::is_downtrend_signal(int ago) const {
    double vi_plus = get_vi_plus(ago);
    double vi_minus = get_vi_minus(ago);
    
    return !std::isnan(vi_plus) && !std::isnan(vi_minus) && vi_minus > vi_plus;
}

bool Vortex::is_uptrend_crossover(int ago) const {
    if (ago >= static_cast<int>(lines_[Lines::VI_PLUS].size()) - 1) {
        return false;
    }
    
    double current_plus = get_vi_plus(ago);
    double current_minus = get_vi_minus(ago);
    double prev_plus = get_vi_plus(ago + 1);
    double prev_minus = get_vi_minus(ago + 1);
    
    return detect_crossover(current_plus, current_minus, prev_plus, prev_minus) &&
           current_plus > current_minus;
}

bool Vortex::is_downtrend_crossover(int ago) const {
    if (ago >= static_cast<int>(lines_[Lines::VI_MINUS].size()) - 1) {
        return false;
    }
    
    double current_plus = get_vi_plus(ago);
    double current_minus = get_vi_minus(ago);
    double prev_plus = get_vi_plus(ago + 1);
    double prev_minus = get_vi_minus(ago + 1);
    
    return detect_crossover(current_minus, current_plus, prev_minus, prev_plus) &&
           current_minus > current_plus;
}

std::vector<double> Vortex::get_vi_plus_history(int count) const {
    return get_line_history(Lines::VI_PLUS, count);
}

std::vector<double> Vortex::get_vi_minus_history(int count) const {
    return get_line_history(Lines::VI_MINUS, count);
}

double Vortex::get_trend_strength(int ago) const {
    double vi_plus = get_vi_plus(ago);
    double vi_minus = get_vi_minus(ago);
    
    if (std::isnan(vi_plus) || std::isnan(vi_minus)) {
        return 0.0;
    }
    
    return std::abs(vi_plus - vi_minus);
}

double Vortex::get_trend_direction(int ago) const {
    double vi_plus = get_vi_plus(ago);
    double vi_minus = get_vi_minus(ago);
    
    if (std::isnan(vi_plus) || std::isnan(vi_minus)) {
        return 0.0;
    }
    
    double diff = vi_plus - vi_minus;
    if (diff > 0.0) return 1.0;
    if (diff < 0.0) return -1.0;
    return 0.0;
}

std::vector<Vortex::DivergencePoint> Vortex::find_divergences(const std::vector<double>& prices, int lookback) const {
    std::vector<DivergencePoint> divergences;
    
    auto vi_plus_history = get_vi_plus_history(static_cast<int>(prices.size()));
    auto vi_minus_history = get_vi_minus_history(static_cast<int>(prices.size()));
    
    if (vi_plus_history.size() != prices.size() || vi_plus_history.size() < static_cast<size_t>(lookback * 2)) {
        return divergences;
    }
    
    // Simplified divergence detection
    for (size_t i = lookback; i < prices.size() - lookback; ++i) {
        // Look for local extremes
        bool price_peak = true, price_trough = true;
        bool vi_peak = true, vi_trough = true;
        
        for (int j = 1; j <= lookback; ++j) {
            // Price peaks/troughs
            if (prices[i] <= prices[i-j] || prices[i] <= prices[i+j]) {
                price_peak = false;
            }
            if (prices[i] >= prices[i-j] || prices[i] >= prices[i+j]) {
                price_trough = false;
            }
            
            // VI divergence (using difference between VI+ and VI-)
            double vi_diff_current = vi_plus_history[i] - vi_minus_history[i];
            double vi_diff_before = vi_plus_history[i-j] - vi_minus_history[i-j];
            double vi_diff_after = vi_plus_history[i+j] - vi_minus_history[i+j];
            
            if (vi_diff_current <= vi_diff_before || vi_diff_current <= vi_diff_after) {
                vi_peak = false;
            }
            if (vi_diff_current >= vi_diff_before || vi_diff_current >= vi_diff_after) {
                vi_trough = false;
            }
        }
        
        // Check for divergences
        if (price_peak && vi_trough) {
            // Bearish divergence: price peak while VI shows weakness
            DivergencePoint point;
            point.index = static_cast<int>(i);
            point.vi_plus = vi_plus_history[i];
            point.vi_minus = vi_minus_history[i];
            point.price = prices[i];
            point.is_bullish_divergence = false;
            divergences.push_back(point);
        } else if (price_trough && vi_peak) {
            // Bullish divergence: price trough while VI shows strength
            DivergencePoint point;
            point.index = static_cast<int>(i);
            point.vi_plus = vi_plus_history[i];
            point.vi_minus = vi_minus_history[i];
            point.price = prices[i];
            point.is_bullish_divergence = true;
            divergences.push_back(point);
        }
    }
    
    return divergences;
}

void Vortex::set_period(int period) {
    if (period <= 0) {
        throw std::invalid_argument("Vortex period must be positive");
    }
    
    params_.period = period;
    
    // Clear existing data to recalculate
    vm_plus_values_.clear();
    vm_minus_values_.clear();
    true_range_values_.clear();
    
    // Reserve new space
    vm_plus_values_.reserve(period + 10);
    vm_minus_values_.reserve(period + 10);
    true_range_values_.reserve(period + 10);
}

Vortex::VortexStats Vortex::calculate_statistics(int lookback_period) const {
    VortexStats stats = {};
    
    auto vi_plus_history = get_vi_plus_history(lookback_period);
    auto vi_minus_history = get_vi_minus_history(lookback_period);
    
    if (vi_plus_history.empty() || vi_minus_history.empty()) {
        return stats;
    }
    
    // Calculate averages
    double sum_plus = 0.0, sum_minus = 0.0;
    int valid_count = 0;
    
    for (size_t i = 0; i < vi_plus_history.size(); ++i) {
        if (!std::isnan(vi_plus_history[i]) && !std::isnan(vi_minus_history[i])) {
            sum_plus += vi_plus_history[i];
            sum_minus += vi_minus_history[i];
            valid_count++;
        }
    }
    
    if (valid_count > 0) {
        stats.avg_vi_plus = sum_plus / valid_count;
        stats.avg_vi_minus = sum_minus / valid_count;
    }
    
    // Calculate min/max
    stats.max_vi_plus = *std::max_element(vi_plus_history.begin(), vi_plus_history.end());
    stats.max_vi_minus = *std::max_element(vi_minus_history.begin(), vi_minus_history.end());
    stats.min_vi_plus = *std::min_element(vi_plus_history.begin(), vi_plus_history.end());
    stats.min_vi_minus = *std::min_element(vi_minus_history.begin(), vi_minus_history.end());
    
    // Count signals and crossovers
    for (size_t i = 1; i < vi_plus_history.size(); ++i) {
        bool current_bullish = vi_plus_history[i] > vi_minus_history[i];
        bool previous_bullish = vi_plus_history[i-1] > vi_minus_history[i-1];
        
        if (current_bullish && !previous_bullish) {
            stats.bullish_signals++;
            stats.total_crossovers++;
        } else if (!current_bullish && previous_bullish) {
            stats.bearish_signals++;
            stats.total_crossovers++;
        }
    }
    
    return stats;
}

void Vortex::initialize_lines() {
    lines_.resize(2);
    line_names_.resize(2);
    
    line_names_[Lines::VI_PLUS] = "vi_plus";
    line_names_[Lines::VI_MINUS] = "vi_minus";
    
    for (auto& line : lines_) {
        line.clear();
    }
}

void Vortex::setup_plot_info() {
    plot_info_.subplot = false;  // Plot on main chart
    plot_info_.plot = true;
    
    // Set custom names for plotting
    plot_info_.line_names.resize(2);
    plot_info_.line_names[Lines::VI_PLUS] = "+VI";
    plot_info_.line_names[Lines::VI_MINUS] = "-VI";
}

void Vortex::calculate_vortex_values() {
    if (!has_enough_data()) {
        return;
    }
    
    // Calculate sums over the period
    double vm_plus_sum = sum_over_period(vm_plus_values_);
    double vm_minus_sum = sum_over_period(vm_minus_values_);
    double tr_sum = sum_over_period(true_range_values_);
    
    if (tr_sum == 0.0) {
        // Avoid division by zero
        lines_[Lines::VI_PLUS].push_back(std::numeric_limits<double>::quiet_NaN());
        lines_[Lines::VI_MINUS].push_back(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate VI+ and VI-
    double vi_plus = vm_plus_sum / tr_sum;
    double vi_minus = vm_minus_sum / tr_sum;
    
    lines_[Lines::VI_PLUS].push_back(vi_plus);
    lines_[Lines::VI_MINUS].push_back(vi_minus);
}

void Vortex::update_calculation_data(double high, double low, double close) {
    if (!has_previous_data_) {
        // Store first values
        previous_high_ = high;
        previous_low_ = low;
        previous_close_ = close;
        has_previous_data_ = true;
        return;
    }
    
    // Calculate VM+, VM-, and TR for this period
    double vm_plus = calculate_vm_plus(high, previous_low_);
    double vm_minus = calculate_vm_minus(low, previous_high_);
    double tr = calculate_true_range(high, low, close, previous_close_);
    
    // Add to calculation vectors
    add_to_vector(vm_plus_values_, vm_plus);
    add_to_vector(vm_minus_values_, vm_minus);
    add_to_vector(true_range_values_, tr);
    
    // Update previous values
    previous_high_ = high;
    previous_low_ = low;
    previous_close_ = close;
}

double Vortex::calculate_vm_plus(double current_high, double previous_low) {
    return std::abs(current_high - previous_low);
}

double Vortex::calculate_vm_minus(double current_low, double previous_high) {
    return std::abs(current_low - previous_high);
}

double Vortex::calculate_true_range(double high, double low, double close, double prev_close) {
    double hl = std::abs(high - low);
    double hc = std::abs(high - prev_close);
    double lc = std::abs(low - prev_close);
    
    return std::max({hl, hc, lc});
}

double Vortex::sum_over_period(const std::vector<double>& values) const {
    if (values.empty()) {
        return 0.0;
    }
    
    // Sum the last 'period' values
    int start_idx = std::max(0, static_cast<int>(values.size()) - params_.period);
    double sum = 0.0;
    
    for (size_t i = start_idx; i < values.size(); ++i) {
        if (!std::isnan(values[i])) {
            sum += values[i];
        }
    }
    
    return sum;
}

void Vortex::add_to_vector(std::vector<double>& vec, double value) {
    vec.push_back(value);
    maintain_vector_size(vec);
}

void Vortex::maintain_vector_size(std::vector<double>& vec) {
    // Keep only enough values for calculation
    if (static_cast<int>(vec.size()) > params_.period * 2) {
        vec.erase(vec.begin(), vec.begin() + params_.period);
    }
}

bool Vortex::has_enough_data() const {
    return static_cast<int>(vm_plus_values_.size()) >= params_.period &&
           static_cast<int>(vm_minus_values_.size()) >= params_.period &&
           static_cast<int>(true_range_values_.size()) >= params_.period;
}

bool Vortex::detect_crossover(double current_a, double current_b, double prev_a, double prev_b) const {
    if (std::isnan(current_a) || std::isnan(current_b) || std::isnan(prev_a) || std::isnan(prev_b)) {
        return false;
    }
    
    // Check if the relationship between a and b changed
    bool current_a_greater = current_a > current_b;
    bool prev_a_greater = prev_a > prev_b;
    
    return current_a_greater != prev_a_greater;
}

// Factory functions implementation

namespace vortex_factory {

std::shared_ptr<Vortex> create_vortex(int period) {
    return std::make_shared<Vortex>(period);
}

std::shared_ptr<Vortex> create_short_term_vortex(int period) {
    return std::make_shared<Vortex>(period);
}

std::shared_ptr<Vortex> create_long_term_vortex(int period) {
    return std::make_shared<Vortex>(period);
}

} // namespace vortex_factory

// Utility functions implementation

namespace vortex_utils {

double calculate_single_vm_plus(double current_high, double previous_low) {
    return std::abs(current_high - previous_low);
}

double calculate_single_vm_minus(double current_low, double previous_high) {
    return std::abs(current_low - previous_high);
}

double calculate_single_true_range(double high, double low, double close, double prev_close) {
    double hl = std::abs(high - low);
    double hc = std::abs(high - prev_close);
    double lc = std::abs(low - prev_close);
    
    return std::max({hl, hc, lc});
}

TrendStrength categorize_trend_strength(double vi_plus, double vi_minus) {
    if (std::isnan(vi_plus) || std::isnan(vi_minus)) {
        return TrendStrength::VERY_WEAK;
    }
    
    double diff = std::abs(vi_plus - vi_minus);
    
    if (diff < 0.05) return TrendStrength::VERY_WEAK;
    if (diff < 0.10) return TrendStrength::WEAK;
    if (diff < 0.20) return TrendStrength::MODERATE;
    if (diff < 0.35) return TrendStrength::STRONG;
    return TrendStrength::VERY_STRONG;
}

double calculate_trend_momentum(const std::vector<double>& vi_plus_history,
                               const std::vector<double>& vi_minus_history,
                               int lookback) {
    if (vi_plus_history.size() < static_cast<size_t>(lookback + 1) ||
        vi_minus_history.size() < static_cast<size_t>(lookback + 1)) {
        return 0.0;
    }
    
    // Calculate current and previous differences
    double current_diff = vi_plus_history.back() - vi_minus_history.back();
    double previous_diff = vi_plus_history[vi_plus_history.size() - 1 - lookback] - 
                          vi_minus_history[vi_minus_history.size() - 1 - lookback];
    
    return current_diff - previous_diff;
}

OptimizationResult optimize_vortex_period(const std::vector<double>& highs,
                                         const std::vector<double>& lows,
                                         const std::vector<double>& closes,
                                         int min_period,
                                         int max_period) {
    OptimizationResult result;
    result.best_score = -1.0;
    result.optimal_period = min_period;
    
    for (int period = min_period; period <= max_period; ++period) {
        // Create temporary Vortex with this period
        Vortex temp_vortex(period);
        
        // Simulate calculation (simplified)
        double score = 0.0;
        int valid_signals = 0;
        
        // This is a simplified scoring mechanism
        // In practice, you would run the full Vortex calculation
        // and evaluate signal quality
        
        for (size_t i = period; i < highs.size() - 1; ++i) {
            // Simplified score calculation
            score += std::abs(highs[i] - lows[i]);
            valid_signals++;
        }
        
        if (valid_signals > 0) {
            score /= valid_signals;
        }
        
        result.tested_periods.push_back(period);
        result.scores.push_back(score);
        
        if (score > result.best_score) {
            result.best_score = score;
            result.optimal_period = period;
        }
    }
    
    return result;
}

SignalValidation validate_vortex_signals(const std::vector<double>& vi_plus,
                                        const std::vector<double>& vi_minus,
                                        const std::vector<double>& prices,
                                        int hold_period) {
    SignalValidation validation = {};
    
    if (vi_plus.size() != vi_minus.size() || vi_plus.size() != prices.size()) {
        return validation;
    }
    
    std::vector<double> profits;
    
    for (size_t i = 1; i < vi_plus.size() - hold_period; ++i) {
        // Check for crossover signals
        bool bullish_crossover = (vi_plus[i] > vi_minus[i]) && (vi_plus[i-1] <= vi_minus[i-1]);
        bool bearish_crossover = (vi_plus[i] < vi_minus[i]) && (vi_plus[i-1] >= vi_minus[i-1]);
        
        if (bullish_crossover || bearish_crossover) {
            validation.total_signals++;
            
            double entry_price = prices[i];
            double exit_price = prices[i + hold_period];
            double profit = 0.0;
            
            if (bullish_crossover) {
                profit = (exit_price - entry_price) / entry_price;
            } else {
                profit = (entry_price - exit_price) / entry_price;
            }
            
            profits.push_back(profit);
            
            if (profit > 0) {
                validation.correct_signals++;
            }
        }
    }
    
    if (validation.total_signals > 0) {
        validation.accuracy_rate = static_cast<double>(validation.correct_signals) / validation.total_signals;
        
        if (!profits.empty()) {
            validation.average_profit = std::accumulate(profits.begin(), profits.end(), 0.0) / profits.size();
            validation.max_profit = *std::max_element(profits.begin(), profits.end());
            validation.max_loss = *std::min_element(profits.begin(), profits.end());
        }
    }
    
    return validation;
}

CombinedSignal combine_with_trend_indicator(double vi_plus, double vi_minus,
                                           double trend_value, double threshold) {
    CombinedSignal signal = {};
    signal.confidence_level = 0.0;
    
    bool vortex_bullish = vi_plus > vi_minus;
    bool vortex_bearish = vi_minus > vi_plus;
    bool trend_bullish = trend_value > threshold;
    bool trend_bearish = trend_value < -threshold;
    
    if (vortex_bullish && trend_bullish) {
        signal.is_bullish = true;
        signal.confidence_level = 0.8;
        signal.signal_sources = "Vortex+Trend";
    } else if (vortex_bearish && trend_bearish) {
        signal.is_bearish = true;
        signal.confidence_level = 0.8;
        signal.signal_sources = "Vortex+Trend";
    } else if (vortex_bullish || trend_bullish) {
        signal.is_bullish = true;
        signal.confidence_level = 0.4;
        signal.signal_sources = vortex_bullish ? "Vortex" : "Trend";
    } else if (vortex_bearish || trend_bearish) {
        signal.is_bearish = true;
        signal.confidence_level = 0.4;
        signal.signal_sources = vortex_bearish ? "Vortex" : "Trend";
    }
    
    return signal;
}

CombinedSignal combine_with_momentum_indicator(double vi_plus, double vi_minus,
                                              double momentum_value, double threshold) {
    CombinedSignal signal = {};
    signal.confidence_level = 0.0;
    
    bool vortex_bullish = vi_plus > vi_minus;
    bool vortex_bearish = vi_minus > vi_plus;
    bool momentum_bullish = momentum_value > threshold;
    bool momentum_bearish = momentum_value < -threshold;
    
    if (vortex_bullish && momentum_bullish) {
        signal.is_bullish = true;
        signal.confidence_level = 0.7;
        signal.signal_sources = "Vortex+Momentum";
    } else if (vortex_bearish && momentum_bearish) {
        signal.is_bearish = true;
        signal.confidence_level = 0.7;
        signal.signal_sources = "Vortex+Momentum";
    }
    
    return signal;
}

TradingSignal generate_trading_signal(double vi_plus, double vi_minus,
                                     double prev_vi_plus, double prev_vi_minus,
                                     double current_price,
                                     double atr_value) {
    TradingSignal signal;
    signal.type = SignalType::HOLD;
    signal.confidence = 0.0;
    signal.reason = "No signal";
    signal.stop_loss_level = current_price;
    signal.take_profit_level = current_price;
    
    // Check for crossovers
    bool bullish_crossover = (vi_plus > vi_minus) && (prev_vi_plus <= prev_vi_minus);
    bool bearish_crossover = (vi_plus < vi_minus) && (prev_vi_plus >= prev_vi_minus);
    
    if (bullish_crossover) {
        signal.type = SignalType::BUY;
        signal.confidence = std::abs(vi_plus - vi_minus);
        signal.reason = "VI+ crossed above VI-";
        
        if (atr_value > 0) {
            signal.stop_loss_level = current_price - (2.0 * atr_value);
            signal.take_profit_level = current_price + (3.0 * atr_value);
        }
    } else if (bearish_crossover) {
        signal.type = SignalType::SELL;
        signal.confidence = std::abs(vi_minus - vi_plus);
        signal.reason = "VI- crossed above VI+";
        
        if (atr_value > 0) {
            signal.stop_loss_level = current_price + (2.0 * atr_value);
            signal.take_profit_level = current_price - (3.0 * atr_value);
        }
    }
    
    return signal;
}

MarketRegime detect_market_regime(const std::vector<double>& vi_plus_history,
                                 const std::vector<double>& vi_minus_history,
                                 int analysis_period) {
    if (vi_plus_history.size() < static_cast<size_t>(analysis_period) ||
        vi_minus_history.size() < static_cast<size_t>(analysis_period)) {
        return MarketRegime::RANGING;
    }
    
    // Analyze recent behavior
    int bullish_periods = 0;
    int bearish_periods = 0;
    int sideways_periods = 0;
    
    size_t start_idx = vi_plus_history.size() - analysis_period;
    
    for (size_t i = start_idx; i < vi_plus_history.size(); ++i) {
        double diff = vi_plus_history[i] - vi_minus_history[i];
        
        if (diff > 0.05) {
            bullish_periods++;
        } else if (diff < -0.05) {
            bearish_periods++;
        } else {
            sideways_periods++;
        }
    }
    
    double bullish_ratio = static_cast<double>(bullish_periods) / analysis_period;
    double bearish_ratio = static_cast<double>(bearish_periods) / analysis_period;
    double sideways_ratio = static_cast<double>(sideways_periods) / analysis_period;
    
    // Determine dominant regime
    if (bullish_ratio > 0.6) {
        return MarketRegime::TRENDING_UP;
    } else if (bearish_ratio > 0.6) {
        return MarketRegime::TRENDING_DOWN;
    } else if (sideways_ratio > 0.6) {
        return MarketRegime::RANGING;
    } else {
        return MarketRegime::VOLATILE;
    }
}

} // namespace vortex_utils

} // namespace contrib
} // namespace indicators
} // namespace backtrader