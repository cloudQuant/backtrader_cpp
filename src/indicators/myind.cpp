#include "../../include/indicators/myind.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

// MaBetweenHighAndLow implementation

MaBetweenHighAndLow::MaBetweenHighAndLow(const Params& params) : params_(params) {
    if (params_.period <= 0) {
        throw std::invalid_argument("Period must be positive");
    }
    
    initialize_lines();
    
    // Create internal SMA indicator
    SimpleMovingAverage::Params sma_params;
    sma_params.period = params_.period;
    sma_ = std::make_shared<SimpleMovingAverage>(sma_params);
}

void MaBetweenHighAndLow::start() {
    Indicator::start();
    
    if (sma_) {
        sma_->set_data(data_);
        sma_->start();
    }
}

void MaBetweenHighAndLow::prenext() {
    if (sma_) {
        sma_->prenext();
    }
}

void MaBetweenHighAndLow::next() {
    if (!data_ || data_->lines.size() < 3) {  // Need close, high, low
        return;
    }
    
    if (sma_) {
        sma_->next();
    }
    
    calculate_current_value();
}

void MaBetweenHighAndLow::once(int start, int end) {
    if (!data_ || data_->lines.size() < 3) {
        return;
    }
    
    if (sma_) {
        sma_->once(start, end);
    }
    
    // Calculate for each bar in range
    for (int i = start; i <= end; ++i) {
        if (i < static_cast<int>(data_->lines[0].size())) {
            calculate_current_value();
        }

double MaBetweenHighAndLow::get_target(int ago) const {
    if (lines_.empty() || lines_[TARGET].empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    const auto& target_line = lines_[TARGET];
    if (ago >= static_cast<int>(target_line.size()) || ago < 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return target_line[target_line.size() - 1 - ago];
}

bool MaBetweenHighAndLow::is_ma_between_high_low(int ago) const {
    double target_value = get_target(ago);
    return !std::isnan(target_value) && target_value > 0.5;  // Treating as boolean
}

void MaBetweenHighAndLow::initialize_lines() {
    lines_.resize(1);
    lines_[TARGET].clear();
    
    line_names_.resize(1);
    line_names_[TARGET] = "target";
}

void MaBetweenHighAndLow::calculate_current_value() {
    if (!data_ || data_->lines.size() < 3 || !sma_) {
        return;
    }
    
    const auto& close_line = data_->lines[0];  // Close
    const auto& high_line = data_->lines[1];   // High  
    const auto& low_line = data_->lines[2];    // Low
    
    if (close_line.empty() || high_line.empty() || low_line.empty()) {
        return;
    }
    
    // Get current values
    double high = high_line.back();
    double low = low_line.back();
    
    // Get MA value from SMA indicator
    double ma_value = sma_->get_ma_value(0);
    
    if (std::isnan(ma_value)) {
        lines_[TARGET].push_back(0.0);  // False when MA not available
        return;
    }
    
    // Check if MA is between high and low
    bool is_between = check_ma_between_high_low(ma_value, high, low);
    lines_[TARGET].push_back(is_between ? 1.0 : 0.0);
}

bool MaBetweenHighAndLow::check_ma_between_high_low(double ma_value, double high, double low) const {
    return (ma_value < high) && (ma_value > low);
}

// BarsLast implementation

BarsLast::BarsLast(const Params& params) : params_(params) {
    initialize_lines();
}

BarsLast::BarsLast(ConditionIndicator condition_indicator, int period) {
    params_.func = condition_indicator;
    params_.period = period;
    initialize_lines();
}

BarsLast::BarsLast(ConditionFunc condition_func, int period) {
    params_.custom_func = condition_func;
    params_.period = period;
    initialize_lines();
}

void BarsLast::start() {
    Indicator::start();
    
    bar_counter_ = 0;
    condition_met_last_bar_ = false;
    
    // Initialize condition indicator if present
    if (params_.func) {
        params_.func->set_data(data_);
        params_.func->start();
    }
}

void BarsLast::prenext() {
    if (params_.func) {
        params_.func->prenext();
    }
    
    update_counter();
}

void BarsLast::next() {
    if (params_.func) {
        params_.func->next();
    }
    
    update_counter();
}

void BarsLast::once(int start, int end) {
    if (params_.func) {
        params_.func->once(start, end);
    }
    
    for (int i = start; i <= end; ++i) {
        update_counter();
    }
}

double BarsLast::get_bar_num(int ago) const {
    if (lines_.empty() || lines_[BAR_NUM].empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    const auto& bar_num_line = lines_[BAR_NUM];
    if (ago >= static_cast<int>(bar_num_line.size()) || ago < 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return bar_num_line[bar_num_line.size() - 1 - ago];
}

int BarsLast::get_bars_since_condition(int ago) const {
    double bar_num = get_bar_num(ago);
    return std::isnan(bar_num) ? -1 : static_cast<int>(bar_num);
}

void BarsLast::set_condition_indicator(ConditionIndicator indicator) {
    params_.func = indicator;
    if (indicator && data_) {
        indicator->set_data(data_);
    }
}

void BarsLast::set_condition_function(ConditionFunc func) {
    params_.custom_func = func;
}

void BarsLast::initialize_lines() {
    lines_.resize(1);
    lines_[BAR_NUM].clear();
    
    line_names_.resize(1);
    line_names_[BAR_NUM] = "bar_num";
}

void BarsLast::update_counter() {
    bool condition_met = evaluate_condition();
    
    if (condition_met) {
        reset_counter();
    } else {
        bar_counter_++;
    }
    
    lines_[BAR_NUM].push_back(static_cast<double>(bar_counter_));
    condition_met_last_bar_ = condition_met;
}

bool BarsLast::evaluate_condition() {
    // Check custom function first
    if (params_.custom_func) {
        return params_.custom_func();
    }
    
    // Check condition indicator
    if (params_.func) {
        // For MaBetweenHighAndLow, check if the target line indicates true
        if (auto ma_between = std::dynamic_pointer_cast<MaBetweenHighAndLow>(params_.func)) {
            return ma_between->is_ma_between_high_low(0);
        }
        
        // For general indicators, check if the first line has a positive value
        if (!params_.func->get_lines().empty() && !params_.func->get_lines()[0].empty()) {
            double value = params_.func->get_lines()[0].back();
            return !std::isnan(value) && value > 0.5;
        }
    
    return false;  // Default: condition not met
}

void BarsLast::reset_counter() {
    bar_counter_ = 0;
}

// NewDiff implementation

NewDiff::NewDiff(const Params& params) : params_(params) {
    if (params_.period <= 0) {
        throw std::invalid_argument("Period must be positive");
    }
    
    initialize_lines();
    daily_values_.reserve(params_.period + 10);  // Reserve space for efficiency
}

void NewDiff::start() {
    Indicator::start();
    
    daily_values_.clear();
    previous_close_ = std::numeric_limits<double>::quiet_NaN();
    has_previous_data_ = false;
}

void NewDiff::prenext() {
    if (!data_ || data_->lines.size() < 3) {
        return;
    }
    
    const auto& close_line = data_->lines[0];
    const auto& high_line = data_->lines[1];
    const auto& low_line = data_->lines[2];
    
    if (close_line.empty() || high_line.empty() || low_line.empty()) {
        return;
    }
    
    double close = close_line.back();
    double high = high_line.back();
    double low = low_line.back();
    
    if (!has_previous_data_) {
        previous_close_ = close;
        has_previous_data_ = true;
        return;
    }
    
    double daily_value = calculate_daily_value(close, high, low, previous_close_);
    update_daily_values(daily_value);
    
    previous_close_ = close;
}

void NewDiff::next() {
    prenext();  // Same logic for next()
    
    // Calculate and store the factor
    double factor = calculate_sum_over_period();
    lines_[FACTOR].push_back(factor);
}

void NewDiff::once(int start, int end) {
    if (!data_ || data_->lines.size() < 3) {
        return;
    }
    
    const auto& close_line = data_->lines[0];
    const auto& high_line = data_->lines[1];
    const auto& low_line = data_->lines[2];
    
    for (int i = start; i <= end && i < static_cast<int>(close_line.size()); ++i) {
        double close = close_line[i];
        double high = high_line[i];
        double low = low_line[i];
        
        if (i == 0) {
            previous_close_ = close;
            has_previous_data_ = true;
            continue;
        }
        
        double daily_value = calculate_daily_value(close, high, low, previous_close_);
        update_daily_values(daily_value);
        
        double factor = calculate_sum_over_period();
        lines_[FACTOR].push_back(factor);
        
        previous_close_ = close;
    }
}

double NewDiff::get_factor(int ago) const {
    if (lines_.empty() || lines_[FACTOR].empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    const auto& factor_line = lines_[FACTOR];
    if (ago >= static_cast<int>(factor_line.size()) || ago < 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return factor_line[factor_line.size() - 1 - ago];
}

double NewDiff::get_alpha_factor(int ago) const {
    return get_factor(ago);  // Alias for get_factor
}

std::vector<double> NewDiff::get_factor_history(int count) const {
    std::vector<double> history;
    
    if (lines_.empty() || lines_[FACTOR].empty()) {
        return history;
    }
    
    const auto& factor_line = lines_[FACTOR];
    int available = static_cast<int>(factor_line.size());
    int actual_count = std::min(count, available);
    
    history.reserve(actual_count);
    
    for (int i = actual_count - 1; i >= 0; --i) {
        history.push_back(factor_line[factor_line.size() - 1 - i]);
    }
    
    return history;
}

double NewDiff::get_average_factor(int period) const {
    auto history = get_factor_history(period);
    
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

void NewDiff::initialize_lines() {
    lines_.resize(1);
    lines_[FACTOR].clear();
    
    line_names_.resize(1);
    line_names_[FACTOR] = "factor";
}

double NewDiff::calculate_daily_value(double close, double high, double low, double prev_close) {
    // Formula: CLOSE=DELAY(CLOSE,1)?0:CLOSE-(CLOSE>DELAY(CLOSE,1)?MIN(LOW,DELAY(CLOSE,1)):MAX(HIGH,DELAY(CLOSE,1)))
    
    if (is_close_equal_to_previous(close, prev_close)) {
        return 0.0;
    }
    
    double adjustment = calculate_close_adjustment(close, high, low, prev_close);
    return close - adjustment;
}

double NewDiff::calculate_sum_over_period() {
    if (daily_values_.empty()) {
        return 0.0;
    }
    
    // Use precise sum calculation
    return myind_utils::calculate_precise_sum(daily_values_);
}

void NewDiff::update_daily_values(double daily_value) {
    daily_values_.push_back(daily_value);
    
    // Keep only values within the period
    if (static_cast<int>(daily_values_.size()) > params_.period) {
        daily_values_.erase(daily_values_.begin());
    }
}

double NewDiff::calculate_close_adjustment(double close, double high, double low, double prev_close) {
    if (is_close_greater_than_previous(close, prev_close)) {
        return get_min_low_prev_close(low, prev_close);
    } else {
        return get_max_high_prev_close(high, prev_close);
    }
}

bool NewDiff::is_close_equal_to_previous(double close, double prev_close) const {
    return myind_utils::is_approximately_equal(close, prev_close);
}

bool NewDiff::is_close_greater_than_previous(double close, double prev_close) const {
    return close > prev_close && !is_close_equal_to_previous(close, prev_close);
}

double NewDiff::get_min_low_prev_close(double low, double prev_close) const {
    return std::min(low, prev_close);
}

double NewDiff::get_max_high_prev_close(double high, double prev_close) const {
    return std::max(high, prev_close);
}

// Factory functions implementation

namespace myind_factory {

std::shared_ptr<MaBetweenHighAndLow> create_ma_between_high_low(int period) {
    MaBetweenHighAndLow::Params params;
    params.period = period;
    return std::make_shared<MaBetweenHighAndLow>(params);
}

std::shared_ptr<BarsLast> create_bars_last_ma_condition(int period) {
    auto ma_condition = create_ma_between_high_low(period);
    return std::make_shared<BarsLast>(ma_condition, period);
}

std::shared_ptr<BarsLast> create_bars_last_custom(BarsLast::ConditionFunc func, int period) {
    return std::make_shared<BarsLast>(func, period);
}

std::shared_ptr<NewDiff> create_new_diff(int period) {
    NewDiff::Params params;
    params.period = period;
    return std::make_shared<NewDiff>(params);
}

CustomIndicatorChain create_full_analysis_chain(int period) {
    CustomIndicatorChain chain;
    
    chain.ma_between = create_ma_between_high_low(period);
    chain.bars_last = create_bars_last_ma_condition(period);
    chain.new_diff = create_new_diff(period);
    
    return chain;
}

} // namespace myind_factory

// Utility functions implementation

namespace myind_utils {

bool is_approximately_equal(double a, double b, double epsilon) {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }
    
    return std::abs(a - b) < epsilon;
}

double safe_value(double value, double default_value) {
    return std::isnan(value) ? default_value : value;
}

double calculate_moving_sum(const std::vector<double>& values, int period) {
    if (values.empty() || period <= 0) {
        return 0.0;
    }
    
    int start_idx = std::max(0, static_cast<int>(values.size()) - period);
    double sum = 0.0;
    
    for (size_t i = start_idx; i < values.size(); ++i) {
        if (!std::isnan(values[i])) {
            sum += values[i];
        }
    
    return sum;
}

double calculate_precise_sum(const std::vector<double>& values) {
    // Kahan summation algorithm for better numerical stability
    double sum = 0.0;
    double compensation = 0.0;
    
    for (double value : values) {
        if (!std::isnan(value)) {
            double y = value - compensation;
            double t = sum + y;
            compensation = (t - sum) - y;
            sum = t;
        }
    
    return sum;
}

bool logical_and(bool condition1, bool condition2) {
    return condition1 && condition2;
}

} // namespace myind_utils
}}}}}}
} // namespace indicators
} // namespace backtrader