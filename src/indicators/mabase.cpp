#include "../../include/indicators/mabase.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace backtrader {
namespace indicators {

// Static member initialization
MovingAverage::MovAvRegistry MovingAverage::registry_;
bool MovingAverage::initialized_ = false;

// MovingAverage implementation

void MovingAverage::register_moving_average(const std::string& name, MovAvCreatorFunc creator) {
    if (!initialized_) {
        initialize_registry();
    }
    
    registry_[name] = creator;
    std::cout << "Registered moving average: " << name << std::endl;
}

void MovingAverage::register_alias(const std::string& alias, const std::string& original_name) {
    if (!initialized_) {
        initialize_registry();
    }
    
    auto it = registry_.find(original_name);
    if (it != registry_.end()) {
        registry_[alias] = it->second;
        std::cout << "Registered alias '" << alias << "' for '" << original_name << "'" << std::endl;
    } else {
        std::cerr << "Cannot create alias '" << alias << "' - original '" << original_name << "' not found" << std::endl;
    }

std::shared_ptr<MovingAverageBase> MovingAverage::create(const std::string& name, const Params& params) {
    if (!initialized_) {
        initialize_registry();
    }
    
    auto it = registry_.find(name);
    if (it != registry_.end()) {
        return it->second(params);
    }
    
    throw std::invalid_argument("Unknown moving average type: " + name);
}

const MovingAverage::MovAvRegistry& MovingAverage::get_registry() {
    if (!initialized_) {
        initialize_registry();
    }
    return registry_;
}

std::vector<std::string> MovingAverage::get_available_types() {
    if (!initialized_) {
        initialize_registry();
    }
    
    std::vector<std::string> types;
    types.reserve(registry_.size());
    
    for (const auto& pair : registry_) {
        types.push_back(pair.first);
    }
    
    return types;
}

bool MovingAverage::is_registered(const std::string& name) {
    if (!initialized_) {
        initialize_registry();
    }
    
    return registry_.find(name) != registry_.end();
}

std::shared_ptr<MovingAverageBase> MovingAverage::Simple(const Params& params) {
    return create("Simple", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::SMA(const Params& params) {
    return create("SMA", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::Exponential(const Params& params) {
    return create("Exponential", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::EMA(const Params& params) {
    return create("EMA", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::Weighted(const Params& params) {
    return create("Weighted", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::WMA(const Params& params) {
    return create("WMA", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::Adaptive(const Params& params) {
    return create("Adaptive", params);
}

std::shared_ptr<MovingAverageBase> MovingAverage::KAMA(const Params& params) {
    return create("KAMA", params);
}

void MovingAverage::initialize_registry() {
    if (initialized_) {
        return;
    }
    
    // Register basic moving averages
    register_moving_average("SimpleMovingAverage", [](const Params& params) {
        return std::make_shared<SimpleMovingAverage>(static_cast<const SimpleMovingAverage::Params&>(params));
    });
    
    register_moving_average("ExponentialMovingAverage", [](const Params& params) {
        return std::make_shared<ExponentialMovingAverage>(static_cast<const ExponentialMovingAverage::Params&>(params));
    });
    
    register_moving_average("WeightedMovingAverage", [](const Params& params) {
        return std::make_shared<WeightedMovingAverage>(static_cast<const WeightedMovingAverage::Params&>(params));
    });
    
    register_moving_average("AdaptiveMovingAverage", [](const Params& params) {
        return std::make_shared<AdaptiveMovingAverage>(static_cast<const AdaptiveMovingAverage::Params&>(params));
    });
    
    // Register aliases
    register_alias("Simple", "SimpleMovingAverage");
    register_alias("SMA", "SimpleMovingAverage");
    register_alias("Exponential", "ExponentialMovingAverage");
    register_alias("EMA", "ExponentialMovingAverage");
    register_alias("Weighted", "WeightedMovingAverage");
    register_alias("WMA", "WeightedMovingAverage");
    register_alias("Adaptive", "AdaptiveMovingAverage");
    register_alias("KAMA", "AdaptiveMovingAverage");
    
    initialized_ = true;
    std::cout << "Moving average registry initialized with " << registry_.size() << " types" << std::endl;
}

// MovingAverageBase implementation

MovingAverageBase::MovingAverageBase(const Params& params) : params_(params) {
    if (params_.period <= 0) {
        throw std::invalid_argument("Moving average period must be positive");
    }
    
    initialize_lines();
    setup_plot_info();
    
    // Reserve space for values
    values_.reserve(params_.period + 10); // Extra space for efficiency
}

void MovingAverageBase::start() {
    Indicator::start();
    
    values_.clear();
    current_sum_ = 0.0;
    is_ready_ = false;
    calculation_start_ = -1;
    
    reset_calculation();
    
    if (can_use_sum_optimization()) {
        initialize_sum_optimization();
    }
}

void MovingAverageBase::stop() {
    Indicator::stop();
    
    // Clean up calculation state
    values_.clear();
    values_.shrink_to_fit();
}

void MovingAverageBase::prenext() {
    // During prenext, we collect data but don't calculate the full MA yet
    if (data_ && !data_->lines.empty() && !data_->lines[0].empty()) {
        double current_value = data_->lines[0].back();
        add_value(current_value);
    }
}

void MovingAverageBase::next() {
    if (!data_ || data_->lines.empty() || data_->lines[0].empty()) {
        return;
    }
    
    double current_value = data_->lines[0].back();
    add_value(current_value);
    
    if (has_enough_data()) {
        double ma_value = calculate_value(static_cast<int>(values_.size()) - 1);
        
        // Update the moving average line
        if (!lines_.empty()) {
            lines_[MA].push_back(ma_value);
        }
        
        is_ready_ = true;
    }
}

void MovingAverageBase::once(int start, int end) {
    if (!data_ || data_->lines.empty() || data_->lines[0].empty()) {
        return;
    }
    
    // Batch calculation for efficiency
    const auto& data_line = data_->lines[0];
    
    // Initialize with first values
    for (int i = start; i <= end && i < static_cast<int>(data_line.size()); ++i) {
        add_value(data_line[i]);
        
        if (has_enough_data()) {
            double ma_value = calculate_value(i);
            
            if (!lines_.empty()) {
                lines_[MA].push_back(ma_value);
            }
    
    is_ready_ = true;
}

void MovingAverageBase::reset_calculation() {
    // Base implementation - subclasses can override
    current_sum_ = 0.0;
    is_ready_ = false;
}

void MovingAverageBase::update_calculation(double value) {
    // Base implementation - subclasses can override
    if (can_use_sum_optimization()) {
        if (values_.size() >= static_cast<size_t>(params_.period)) {
            double old_value = values_[values_.size() - params_.period];
            update_sum_optimization(value, old_value);
        } else {
            current_sum_ += value;
        }

void MovingAverageBase::set_period(int period) {
    if (period <= 0) {
        throw std::invalid_argument("Moving average period must be positive");
    }
    
    params_.period = period;
    values_.clear();
    values_.reserve(period + 10);
    reset_calculation();
}

double MovingAverageBase::get_ma_value(int ago) const {
    if (!is_ready_ || lines_.empty() || lines_[MA].empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    const auto& ma_line = lines_[MA];
    if (ago >= static_cast<int>(ma_line.size()) || ago < 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return ma_line[ma_line.size() - 1 - ago];
}

std::vector<double> MovingAverageBase::get_ma_values(int count) const {
    std::vector<double> result;
    
    if (!is_ready_ || lines_.empty() || lines_[MA].empty()) {
        return result;
    }
    
    const auto& ma_line = lines_[MA];
    int available = static_cast<int>(ma_line.size());
    int actual_count = std::min(count, available);
    
    result.reserve(actual_count);
    
    for (int i = actual_count - 1; i >= 0; --i) {
        result.push_back(ma_line[ma_line.size() - 1 - i]);
    }
    
    return result;
}

void MovingAverageBase::add_value(double value) {
    values_.push_back(value);
    update_calculation(value);
    
    // Keep only necessary values for calculation
    if (values_.size() > static_cast<size_t>(params_.period * 2)) {
        values_.erase(values_.begin(), values_.begin() + params_.period);
    }
}

void MovingAverageBase::remove_oldest_value() {
    if (!values_.empty()) {
        values_.erase(values_.begin());
    }
}

bool MovingAverageBase::has_enough_data() const {
    return static_cast<int>(values_.size()) >= params_.period;
}

void MovingAverageBase::ensure_capacity() {
    if (values_.capacity() < static_cast<size_t>(params_.period * 2)) {
        values_.reserve(params_.period * 2);
    }
}

void MovingAverageBase::initialize_sum_optimization() {
    current_sum_ = 0.0;
}

void MovingAverageBase::update_sum_optimization(double new_value, double old_value) {
    current_sum_ = current_sum_ + new_value - old_value;
}

void MovingAverageBase::initialize_lines() {
    // Initialize the MA line
    lines_.resize(1);
    lines_[MA].clear();
    
    // Set line names
    line_names_.resize(1);
    line_names_[MA] = "ma";
}

void MovingAverageBase::setup_plot_info() {
    // Moving averages plot on the main chart by default
    plot_info_.subplot = false;
    plot_info_.plot = true;
    plot_info_.plotskip = false;
}

// SimpleMovingAverage implementation

SimpleMovingAverage::SimpleMovingAverage(const Params& params) 
    : MetaMovingAverage<SimpleMovingAverage>(params) {
}

double SimpleMovingAverage::calculate_value(int index) {
    if (!has_enough_data()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    if (can_use_sum_optimization() && sum_initialized_) {
        return sum_ / params_.period;
    }
    
    // Calculate from scratch if optimization not available
    int start_idx = std::max(0, static_cast<int>(values_.size()) - params_.period);
    int end_idx = static_cast<int>(values_.size());
    
    double sum = 0.0;
    for (int i = start_idx; i < end_idx; ++i) {
        sum += values_[i];
    }
    
    return sum / params_.period;
}

void SimpleMovingAverage::initialize_sum_optimization() {
    MovingAverageBase::initialize_sum_optimization();
    sum_ = 0.0;
    sum_initialized_ = false;
}

void SimpleMovingAverage::update_sum_optimization(double new_value, double old_value) {
    if (!sum_initialized_) {
        // Initialize sum with current values
        sum_ = std::accumulate(values_.end() - params_.period, values_.end(), 0.0);
        sum_initialized_ = true;
    } else {
        sum_ = sum_ + new_value - old_value;
    }
}

// ExponentialMovingAverage implementation

ExponentialMovingAverage::ExponentialMovingAverage(const Params& params) 
    : MetaMovingAverage<ExponentialMovingAverage>(params) {
    
    if (derived_params_.alpha < 0) {
        // Auto-calculate alpha from period
        alpha_ = 2.0 / (params_.period + 1.0);
    } else {
        alpha_ = derived_params_.alpha;
    }
}

double ExponentialMovingAverage::calculate_value(int index) {
    if (values_.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    if (first_value_) {
        previous_ema_ = values_.back();
        first_value_ = false;
        return previous_ema_;
    }
    
    double current_value = values_.back();
    double ema = alpha_ * current_value + (1.0 - alpha_) * previous_ema_;
    previous_ema_ = ema;
    
    return ema;
}

void ExponentialMovingAverage::reset_calculation() {
    MovingAverageBase::reset_calculation();
    previous_ema_ = 0.0;
    first_value_ = true;
}

// WeightedMovingAverage implementation

WeightedMovingAverage::WeightedMovingAverage(const Params& params) 
    : MetaMovingAverage<WeightedMovingAverage>(params) {
    calculate_weight_sum();
}

double WeightedMovingAverage::calculate_value(int index) {
    if (!has_enough_data()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    int start_idx = std::max(0, static_cast<int>(values_.size()) - params_.period);
    double weighted_sum = 0.0;
    
    for (int i = 0; i < params_.period; ++i) {
        int value_idx = start_idx + i;
        if (value_idx < static_cast<int>(values_.size())) {
            weighted_sum += values_[value_idx] * (i + 1);
        }
    
    return weighted_sum / weight_sum_;
}

void WeightedMovingAverage::calculate_weight_sum() {
    weight_sum_ = 0.0;
    for (int i = 1; i <= params_.period; ++i) {
        weight_sum_ += i;
    }
}

// AdaptiveMovingAverage implementation

AdaptiveMovingAverage::AdaptiveMovingAverage(const Params& params) 
    : MetaMovingAverage<AdaptiveMovingAverage>(params) {
    
    fast_alpha_ = 2.0 / (derived_params_.fast_period + 1.0);
    slow_alpha_ = 2.0 / (derived_params_.slow_period + 1.0);
}

double AdaptiveMovingAverage::calculate_value(int index) {
    if (!has_enough_data()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    if (first_value_) {
        previous_ama_ = values_.back();
        first_value_ = false;
        return previous_ama_;
    }
    
    double current_value = values_.back();
    double efficiency_ratio = calculate_efficiency_ratio(index);
    double smoothing_constant = calculate_smoothing_constant(efficiency_ratio);
    
    double ama = previous_ama_ + smoothing_constant * (current_value - previous_ama_);
    previous_ama_ = ama;
    
    return ama;
}

void AdaptiveMovingAverage::reset_calculation() {
    MovingAverageBase::reset_calculation();
    previous_ama_ = 0.0;
    first_value_ = true;
}

double AdaptiveMovingAverage::calculate_efficiency_ratio(int index) {
    if (static_cast<int>(values_.size()) < params_.period) {
        return 0.0;
    }
    
    int start_idx = std::max(0, static_cast<int>(values_.size()) - params_.period);
    
    // Calculate change over period
    double change = std::abs(values_.back() - values_[start_idx]);
    
    // Calculate volatility (sum of absolute changes)
    double volatility = 0.0;
    for (size_t i = start_idx + 1; i < values_.size(); ++i) {
        volatility += std::abs(values_[i] - values_[i-1]);
    }
    
    if (volatility == 0.0) {
        return 1.0; // Maximum efficiency when no volatility
    }
    
    return change / volatility;
}

double AdaptiveMovingAverage::calculate_smoothing_constant(double efficiency_ratio) {
    double sc = efficiency_ratio * (fast_alpha_ - slow_alpha_) + slow_alpha_;
    return sc * sc; // Square the smoothing constant
}
}}}}}}
} // namespace indicators
} // namespace backtrader