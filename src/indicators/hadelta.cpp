#include "../../include/indicators/hadelta.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace backtrader {
namespace indicators {

// HeikinAshi implementation

HeikinAshi::HeikinAshi(const Params& params) : params_(params) {
    initialize_lines();
}

void HeikinAshi::start() {
    Indicator::start();
    
    previous_ha_open_ = std::numeric_limits<double>::quiet_NaN();
    previous_ha_close_ = std::numeric_limits<double>::quiet_NaN();
    first_bar_ = true;
}

void HeikinAshi::stop() {
    Indicator::stop();
}

void HeikinAshi::prenext() {
    calculate_heikin_ashi_values();
}

void HeikinAshi::next() {
    calculate_heikin_ashi_values();
}

void HeikinAshi::once(int start, int end) {
    if (!data_ || data_->lines.size() < 4) {
        return;
    }
    
    const auto& open_line = data_->lines[0];
    const auto& high_line = data_->lines[1];
    const auto& low_line = data_->lines[2];
    const auto& close_line = data_->lines[3];
    
    for (int i = start; i <= end && i < static_cast<int>(open_line.size()); ++i) {
        double open = open_line[i];
        double high = high_line[i];
        double low = low_line[i];
        double close = close_line[i];
        
        if (i == 0 || first_bar_) {
            seed_first_bar(open, close);
            first_bar_ = false;
        }
        
        // Calculate Heikin-Ashi values
        double ha_close = calculate_ha_close(open, high, low, close);
        double ha_open = calculate_ha_open(previous_ha_open_, previous_ha_close_);
        double ha_high = calculate_ha_high(high, ha_open, ha_close);
        double ha_low = calculate_ha_low(low, ha_open, ha_close);
        
        // Store values
        lines_[OPEN].push_back(ha_open);
        lines_[HIGH].push_back(ha_high);
        lines_[LOW].push_back(ha_low);
        lines_[CLOSE].push_back(ha_close);
        
        // Update for next iteration
        previous_ha_open_ = ha_open;
        previous_ha_close_ = ha_close;
    }
}

double HeikinAshi::get_ha_open(int ago) const {
    return get_line_value(OPEN, ago);
}

double HeikinAshi::get_ha_high(int ago) const {
    return get_line_value(HIGH, ago);
}

double HeikinAshi::get_ha_low(int ago) const {
    return get_line_value(LOW, ago);
}

double HeikinAshi::get_ha_close(int ago) const {
    return get_line_value(CLOSE, ago);
}

std::vector<double> HeikinAshi::get_ha_ohlc(int ago) const {
    return {
        get_ha_open(ago),
        get_ha_high(ago),
        get_ha_low(ago),
        get_ha_close(ago)
    };
}

void HeikinAshi::initialize_lines() {
    lines_.resize(4);
    line_names_.resize(4);
    
    line_names_[OPEN] = "ha_open";
    line_names_[HIGH] = "ha_high";
    line_names_[LOW] = "ha_low";
    line_names_[CLOSE] = "ha_close";
    
    for (auto& line : lines_) {
        line.clear();
    }
}

void HeikinAshi::calculate_heikin_ashi_values() {
    if (!data_ || data_->lines.size() < 4) {
        return;
    }
    
    const auto& open_line = data_->lines[0];
    const auto& high_line = data_->lines[1];
    const auto& low_line = data_->lines[2];
    const auto& close_line = data_->lines[3];
    
    if (open_line.empty() || high_line.empty() || low_line.empty() || close_line.empty()) {
        return;
    }
    
    double open = open_line.back();
    double high = high_line.back();
    double low = low_line.back();
    double close = close_line.back();
    
    if (first_bar_) {
        seed_first_bar(open, close);
        first_bar_ = false;
    }
    
    // Calculate Heikin-Ashi values
    double ha_close = calculate_ha_close(open, high, low, close);
    double ha_open = calculate_ha_open(previous_ha_open_, previous_ha_close_);
    double ha_high = calculate_ha_high(high, ha_open, ha_close);
    double ha_low = calculate_ha_low(low, ha_open, ha_close);
    
    // Store values
    lines_[OPEN].push_back(ha_open);
    lines_[HIGH].push_back(ha_high);
    lines_[LOW].push_back(ha_low);
    lines_[CLOSE].push_back(ha_close);
    
    // Update for next iteration
    previous_ha_open_ = ha_open;
    previous_ha_close_ = ha_close;
}

void HeikinAshi::seed_first_bar(double open, double close) {
    previous_ha_open_ = (open + close) / 2.0;
    previous_ha_close_ = (open + close) / 2.0;  // Will be recalculated
}

double HeikinAshi::calculate_ha_close(double open, double high, double low, double close) {
    return (open + high + low + close) / 4.0;
}

double HeikinAshi::calculate_ha_open(double prev_ha_open, double prev_ha_close) {
    if (std::isnan(prev_ha_open) || std::isnan(prev_ha_close)) {
        return prev_ha_open;  // First bar case
    }
    return (prev_ha_open + prev_ha_close) / 2.0;
}

double HeikinAshi::calculate_ha_high(double high, double ha_open, double ha_close) {
    return std::max({high, ha_open, ha_close});
}

double HeikinAshi::calculate_ha_low(double low, double ha_open, double ha_close) {
    return std::min({low, ha_open, ha_close});
}

// HaDelta implementation

HaDelta::HaDelta(const Params& params) : params_(params) {
    validate_parameters();
    initialize_lines();
    setup_plot_info();
}

HaDelta::HaDelta(int period, const std::string& ma_type, bool auto_heikin) {
    params_.period = period;
    params_.movav_type = ma_type;
    params_.autoheikin = auto_heikin;
    
    validate_parameters();
    initialize_lines();
    setup_plot_info();
}

HaDelta::HaDelta(int period, MovingAverageType ma, bool auto_heikin) {
    params_.period = period;
    params_.movav = ma;
    params_.autoheikin = auto_heikin;
    
    validate_parameters();
    initialize_lines();
    setup_plot_info();
}

void HaDelta::start() {
    Indicator::start();
    
    initialize_indicators();
}

void HaDelta::stop() {
    Indicator::stop();
    
    if (heikin_ashi_) {
        heikin_ashi_->stop();
    }
    
    if (smoothing_ma_) {
        smoothing_ma_->stop();
    }
}

void HaDelta::prenext() {
    if (params_.autoheikin && heikin_ashi_) {
        heikin_ashi_->prenext();
    }
    
    calculate_delta();
    
    if (smoothing_ma_) {
        smoothing_ma_->prenext();
    }
}

void HaDelta::next() {
    if (params_.autoheikin && heikin_ashi_) {
        heikin_ashi_->next();
    }
    
    calculate_delta();
    
    if (smoothing_ma_) {
        smoothing_ma_->next();
        update_smoothed_delta();
    }
}

void HaDelta::once(int start, int end) {
    if (params_.autoheikin && heikin_ashi_) {
        heikin_ashi_->once(start, end);
    }
    
    for (int i = start; i <= end; ++i) {
        calculate_delta();
        
        if (smoothing_ma_) {
            smoothing_ma_->next();
            update_smoothed_delta();
        }

double HaDelta::get_ha_delta(int ago) const {
    return get_line_value(HADELTA, ago);
}

double HaDelta::get_smoothed_delta(int ago) const {
    return get_line_value(SMOOTHED, ago);
}

bool HaDelta::is_bullish(int ago) const {
    double delta = get_ha_delta(ago);
    return !std::isnan(delta) && delta > 0.0;
}

bool HaDelta::is_bearish(int ago) const {
    double delta = get_ha_delta(ago);
    return !std::isnan(delta) && delta < 0.0;
}

bool HaDelta::is_smoothed_bullish(int ago) const {
    double smoothed = get_smoothed_delta(ago);
    return !std::isnan(smoothed) && smoothed > 0.0;
}

bool HaDelta::is_smoothed_bearish(int ago) const {
    double smoothed = get_smoothed_delta(ago);
    return !std::isnan(smoothed) && smoothed < 0.0;
}

double HaDelta::get_momentum_strength(int ago) const {
    double delta = get_ha_delta(ago);
    return std::isnan(delta) ? 0.0 : std::abs(delta);
}

double HaDelta::get_momentum_direction(int ago) const {
    double delta = get_ha_delta(ago);
    if (std::isnan(delta)) return 0.0;
    if (delta > 0.0) return 1.0;
    if (delta < 0.0) return -1.0;
    return 0.0;
}

std::vector<double> HaDelta::get_delta_history(int count) const {
    return get_line_history(HADELTA, count);
}

std::vector<double> HaDelta::get_smoothed_history(int count) const {
    return get_line_history(SMOOTHED, count);
}

double HaDelta::get_average_delta(int period) const {
    auto history = get_delta_history(period);
    
    if (history.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double sum = 0.0;
    int valid_count = 0;
    
    for (double value : history) {
        if (!std::isnan(value)) {
            sum += value;
            valid_count++;
        }
    
    return valid_count > 0 ? sum / valid_count : std::numeric_limits<double>::quiet_NaN();
}

void HaDelta::set_moving_average_type(const std::string& ma_type) {
    params_.movav_type = ma_type;
    params_.movav = nullptr;  // Clear existing MA
    
    if (smoothing_ma_) {
        create_moving_average();
    }
}

void HaDelta::set_moving_average_type(MovingAverageType ma) {
    params_.movav = ma;
    params_.movav_type = "Custom";
    smoothing_ma_ = ma;
}

void HaDelta::set_period(int period) {
    if (period <= 0) {
        throw std::invalid_argument("Period must be positive");
    }
    
    params_.period = period;
    
    if (smoothing_ma_) {
        smoothing_ma_->set_period(period);
    }
}

void HaDelta::enable_auto_heikin(bool enable) {
    params_.autoheikin = enable;
    
    // Reinitialize indicators if already started
    if (data_) {
        initialize_indicators();
    }
}

void HaDelta::initialize_lines() {
    lines_.resize(2);
    line_names_.resize(2);
    
    line_names_[HADELTA] = "haDelta";
    line_names_[SMOOTHED] = "smoothed";
    
    for (auto& line : lines_) {
        line.clear();
    }
}

void HaDelta::setup_plot_info() {
    // HaDelta plots in a separate subplot
    plot_info_.subplot = true;
    plot_info_.plot = true;
    
    // Set up line plotting information
    plot_info_.line_colors.resize(2);
    plot_info_.line_colors[HADELTA] = "red";
    plot_info_.line_colors[SMOOTHED] = "grey";
    
    // Conditional coloring for smoothed line (green above 0, red below 0)
    plot_info_.conditional_colors[SMOOTHED] = {
        {0.0, "green", "red"}  // Fill green when > 0, red when < 0
    };
}

void HaDelta::initialize_indicators() {
    if (params_.autoheikin) {
        // Create Heikin-Ashi transformation
        heikin_ashi_ = std::make_shared<HeikinAshi>();
        heikin_ashi_->set_data(data_);
        heikin_ashi_->start();
        
        // Use Heikin-Ashi data as effective data source
        effective_data_ = std::dynamic_pointer_cast<DataSeries>(heikin_ashi_);
    } else {
        // Use raw data directly
        effective_data_ = data_;
    }
    
    // Create moving average for smoothing
    create_moving_average();
}

void HaDelta::create_moving_average() {
    if (params_.movav) {
        smoothing_ma_ = params_.movav;
    } else {
        smoothing_ma_ = create_ma_from_string(params_.movav_type);
    }
    
    if (smoothing_ma_) {
        smoothing_ma_->set_period(params_.period);
        
        // Create a data series from the delta line for the MA
        // This is a simplified approach - in a full implementation,
        // you would create a proper data series wrapper
        smoothing_ma_->start();
    }
}

double HaDelta::calculate_raw_delta() {
    if (params_.autoheikin && heikin_ashi_) {
        // Use Heikin-Ashi values
        double ha_close = heikin_ashi_->get_ha_close(0);
        double ha_open = heikin_ashi_->get_ha_open(0);
        
        if (std::isnan(ha_close) || std::isnan(ha_open)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        return ha_close - ha_open;
    } else {
        // Use raw OHLC data
        if (!data_ || data_->lines.size() < 4) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        const auto& open_line = data_->lines[0];
        const auto& close_line = data_->lines[3];
        
        if (open_line.empty() || close_line.empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        return close_line.back() - open_line.back();
    }
}

void HaDelta::calculate_delta() {
    double delta = calculate_raw_delta();
    lines_[HADELTA].push_back(delta);
    
    // Feed delta to moving average
    if (smoothing_ma_ && !std::isnan(delta)) {
        // In a complete implementation, you would feed this to the MA's data source
        // For now, we'll calculate it manually
    }
}

void HaDelta::update_smoothed_delta() {
    if (!smoothing_ma_) {
        return;
    }
    
    // Get smoothed value from moving average
    double smoothed = smoothing_ma_->get_ma_value(0);
    lines_[SMOOTHED].push_back(smoothed);
}

MovingAverageBase* HaDelta::create_ma_from_string(const std::string& ma_type) {
    // Use the MovingAverage factory
    try {
        Params ma_params;
        ma_params.period = params_.period;
        
        auto ma = MovingAverage::create(ma_type, ma_params);
        return ma.get();
    } catch (const std::exception& e) {
        std::cerr << "Failed to create moving average of type '" << ma_type 
                  << "': " << e.what() << ". Falling back to SMA." << std::endl;
        
        // Fallback to SMA
        return new SimpleMovingAverage(SimpleMovingAverage::Params{params_.period});
    }
}

void HaDelta::validate_parameters() {
    if (params_.period <= 0) {
        throw std::invalid_argument("Period must be positive");
    }
    
    if (params_.movav_type.empty() && !params_.movav) {
        params_.movav_type = "SMA";  // Default to SMA
    }
}

// Factory functions implementation

namespace heikin_ashi_factory {

std::shared_ptr<HeikinAshi> create_heikin_ashi() {
    return std::make_shared<HeikinAshi>();
}

std::shared_ptr<HaDelta> create_ha_delta(int period, const std::string& ma_type) {
    return std::make_shared<HaDelta>(period, ma_type, true);
}

std::shared_ptr<HaDelta> create_ha_delta_custom_ma(int period, std::shared_ptr<MovingAverageBase> ma) {
    return std::make_shared<HaDelta>(period, ma, true);
}

std::shared_ptr<HaDelta> create_ha_delta_no_transform(int period, const std::string& ma_type) {
    return std::make_shared<HaDelta>(period, ma_type, false);
}

} // namespace heikin_ashi_factory

// Utility functions implementation

namespace heikin_ashi_utils {

bool is_ha_candle_bullish(double ha_open, double ha_close) {
    return !std::isnan(ha_open) && !std::isnan(ha_close) && ha_close > ha_open;
}

bool is_ha_candle_bearish(double ha_open, double ha_close) {
    return !std::isnan(ha_open) && !std::isnan(ha_close) && ha_close < ha_open;
}

bool is_ha_candle_doji(double ha_open, double ha_close, double tolerance) {
    if (std::isnan(ha_open) || std::isnan(ha_close)) {
        return false;
    }
    return std::abs(ha_close - ha_open) <= tolerance;
}

double calculate_ha_body_size(double ha_open, double ha_close) {
    if (std::isnan(ha_open) || std::isnan(ha_close)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::abs(ha_close - ha_open);
}

double calculate_ha_body_percentage(double ha_open, double ha_high, double ha_low, double ha_close) {
    if (std::isnan(ha_open) || std::isnan(ha_high) || std::isnan(ha_low) || std::isnan(ha_close)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double range = ha_high - ha_low;
    if (range == 0.0) {
        return 100.0;  // Full body when no range
    }
    
    double body_size = std::abs(ha_close - ha_open);
    return (body_size / range) * 100.0;
}

TrendDirection analyze_delta_trend(const std::vector<double>& deltas, int lookback) {
    if (deltas.size() < static_cast<size_t>(lookback)) {
        return TrendDirection::SIDEWAYS;
    }
    
    int bullish_count = 0;
    int bearish_count = 0;
    
    // Look at the last 'lookback' deltas
    for (size_t i = deltas.size() - lookback; i < deltas.size(); ++i) {
        if (!std::isnan(deltas[i])) {
            if (deltas[i] > 0.0) {
                bullish_count++;
            } else if (deltas[i] < 0.0) {
                bearish_count++;
            }
    
    double bullish_ratio = static_cast<double>(bullish_count) / lookback;
    double bearish_ratio = static_cast<double>(bearish_count) / lookback;
    
    if (bullish_ratio > 0.6) {
        return TrendDirection::BULLISH;
    } else if (bearish_ratio > 0.6) {
        return TrendDirection::BEARISH;
    } else {
        return TrendDirection::SIDEWAYS;
    }

double calculate_delta_momentum(const std::vector<double>& deltas) {
    if (deltas.size() < 2) {
        return 0.0;
    }
    
    double current = deltas.back();
    double previous = deltas[deltas.size() - 2];
    
    if (std::isnan(current) || std::isnan(previous)) {
        return 0.0;
    }
    
    return current - previous;
}

std::vector<DivergencePoint> find_delta_divergences(
    const std::vector<double>& deltas,
    const std::vector<double>& prices,
    int lookback) {
    
    std::vector<DivergencePoint> divergences;
    
    if (deltas.size() != prices.size() || deltas.size() < static_cast<size_t>(lookback * 2)) {
        return divergences;
    }
    
    // Simplified divergence detection
    // In a complete implementation, this would be more sophisticated
    for (size_t i = lookback; i < deltas.size() - lookback; ++i) {
        // Look for local peaks/troughs in both delta and price
        bool delta_peak = true;
        bool delta_trough = true;
        bool price_peak = true;
        bool price_trough = true;
        
        for (int j = 1; j <= lookback; ++j) {
            if (deltas[i] <= deltas[i-j] || deltas[i] <= deltas[i+j]) {
                delta_peak = false;
            }
            if (deltas[i] >= deltas[i-j] || deltas[i] >= deltas[i+j]) {
                delta_trough = false;
            }
            if (prices[i] <= prices[i-j] || prices[i] <= prices[i+j]) {
                price_peak = false;
            }
            if (prices[i] >= prices[i-j] || prices[i] >= prices[i+j]) {
                price_trough = false;
            }
        
        // Check for divergences
        if (delta_peak && price_trough) {
            // Bullish divergence: delta peak while price trough
            DivergencePoint point;
            point.index = static_cast<int>(i);
            point.delta_value = deltas[i];
            point.price_value = prices[i];
            point.is_bullish_divergence = true;
            divergences.push_back(point);
        } else if (delta_trough && price_peak) {
            // Bearish divergence: delta trough while price peak
            DivergencePoint point;
            point.index = static_cast<int>(i);
            point.delta_value = deltas[i];
            point.price_value = prices[i];
            point.is_bullish_divergence = false;
            divergences.push_back(point);
        }
    
    return divergences;
}

} // namespace heikin_ashi_utils
}}}}}}}}
} // namespace indicators
} // namespace backtrader