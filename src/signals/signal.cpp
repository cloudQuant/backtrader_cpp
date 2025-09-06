#include "../../include/signals/signal.h"
#include <algorithm>

namespace backtrader {
namespace signals {

// BaseSignal implementation
BaseSignal::BaseSignal(SignalType type) : type_(type), active_(true) {}

SignalType BaseSignal::get_type() const {
    return type_;
}

bool BaseSignal::is_active() const {
    return active_;
}

void BaseSignal::set_active(bool active) {
    active_ = active;
}

// SimpleSignal implementation
SimpleSignal::SimpleSignal(SignalType type, std::function<bool()> condition)
    : BaseSignal(type), condition_(std::move(condition)) {}

bool SimpleSignal::check() {
    if (!is_active() || !condition_) {
        return false;
    }
    
    return condition_();
}

// CrossoverSignal implementation
CrossoverSignal::CrossoverSignal(std::shared_ptr<Indicator> fast_line,
                                std::shared_ptr<Indicator> slow_line,
                                SignalType signal_type)
    : BaseSignal(signal_type), fast_line_(fast_line), slow_line_(slow_line),
      previous_state_(CrossState::Unknown) {}

bool CrossoverSignal::check() {
    if (!is_active() || !fast_line_ || !slow_line_) {
        return false;
    }
    
    // Get current values
    double fast_current = fast_line_->get_value();
    double slow_current = slow_line_->get_value();
    
    // Get previous values (simplified - in real implementation would access historical data)
    double fast_prev = fast_line_->get_previous_value();
    double slow_prev = slow_line_->get_previous_value();
    
    // Determine current state
    CrossState current_state = CrossState::Unknown;
    if (fast_current > slow_current) {
        current_state = CrossState::Above;
    } else if (fast_current < slow_current) {
        current_state = CrossState::Below;
    }
    
    bool signal_triggered = false;
    
    // Check for crossover based on signal type
    if (get_type() == SignalType::Long) {
        // Long signal: fast crosses above slow
        signal_triggered = (previous_state_ == CrossState::Below && 
                           current_state == CrossState::Above);
    } else if (get_type() == SignalType::Short) {
        // Short signal: fast crosses below slow
        signal_triggered = (previous_state_ == CrossState::Above && 
                           current_state == CrossState::Below);
    }
    
    previous_state_ = current_state;
    return signal_triggered;
}

// ThresholdSignal implementation
ThresholdSignal::ThresholdSignal(std::shared_ptr<Indicator> indicator,
                                double threshold, SignalType signal_type,
                                ThresholdType threshold_type)
    : BaseSignal(signal_type), indicator_(indicator), threshold_(threshold),
      threshold_type_(threshold_type), previous_triggered_(false) {}

bool ThresholdSignal::check() {
    if (!is_active() || !indicator_) {
        return false;
    }
    
    double current_value = indicator_->get_value();
    bool currently_triggered = false;
    
    switch (threshold_type_) {
        case ThresholdType::Above:
            currently_triggered = (current_value > threshold_);
            break;
        case ThresholdType::Below:
            currently_triggered = (current_value < threshold_);
            break;
        case ThresholdType::CrossAbove:
            {
                double prev_value = indicator_->get_previous_value();
                currently_triggered = (prev_value <= threshold_ && current_value > threshold_);
            }
            break;
        case ThresholdType::CrossBelow:
            {
                double prev_value = indicator_->get_previous_value();
                currently_triggered = (prev_value >= threshold_ && current_value < threshold_);
            }
            break;
    }
    
    // For edge detection, only trigger once when condition becomes true
    bool signal_result = currently_triggered && !previous_triggered_;
    previous_triggered_ = currently_triggered;
    
    return signal_result;
}

// CompositeSignal implementation
CompositeSignal::CompositeSignal(SignalType type, LogicType logic_type)
    : BaseSignal(type), logic_type_(logic_type) {}

void CompositeSignal::add_signal(std::shared_ptr<BaseSignal> signal) {
    if (signal) {
        child_signals_.push_back(signal);
    }
}

void CompositeSignal::remove_signal(std::shared_ptr<BaseSignal> signal) {
    auto it = std::find(child_signals_.begin(), child_signals_.end(), signal);
    if (it != child_signals_.end()) {
        child_signals_.erase(it);
    }
}

bool CompositeSignal::check() {
    if (!is_active() || child_signals_.empty()) {
        return false;
    }
    
    switch (logic_type_) {
        case LogicType::And:
            {
                // All signals must be true
                for (auto& signal : child_signals_) {
                    if (!signal->check()) {
                        return false;
                    }
                }
                return true;
            }
            
        case LogicType::Or:
            {
                // At least one signal must be true
                for (auto& signal : child_signals_) {
                    if (signal->check()) {
                        return true;
                    }
                }
                return false;
            }
            
        case LogicType::Xor:
            {
                // Exactly one signal must be true
                int true_count = 0;
                for (auto& signal : child_signals_) {
                    if (signal->check()) {
                        true_count++;
                    }
                }
                return (true_count == 1);
            }
            
        case LogicType::Nand:
            {
                // Not all signals are true
                for (auto& signal : child_signals_) {
                    if (!signal->check()) {
                        return true;
                    }
                }
                return false;
            }
            
        case LogicType::Nor:
            {
                // None of the signals are true
                for (auto& signal : child_signals_) {
                    if (signal->check()) {
                        return false;
                    }
                }
                return true;
            }
    }
    
    return false;
}

size_t CompositeSignal::get_signal_count() const {
    return child_signals_.size();
}

std::vector<std::shared_ptr<BaseSignal>> CompositeSignal::get_child_signals() const {
    return child_signals_;
}

// DelayedSignal implementation
DelayedSignal::DelayedSignal(std::shared_ptr<BaseSignal> base_signal, int delay_periods)
    : BaseSignal(base_signal ? base_signal->get_type() : SignalType::Long),
      base_signal_(base_signal), delay_periods_(delay_periods), current_period_(0) {}

bool DelayedSignal::check() {
    if (!is_active() || !base_signal_) {
        return false;
    }
    
    bool base_result = base_signal_->check();
    
    // Store signal history
    signal_history_.push_back(base_result);
    
    // Keep only required history
    if (signal_history_.size() > static_cast<size_t>(delay_periods_ + 1)) {
        signal_history_.erase(signal_history_.begin());
    }
    
    // Check if we have enough history and return delayed signal
    if (signal_history_.size() > static_cast<size_t>(delay_periods_)) {
        return signal_history_[signal_history_.size() - delay_periods_ - 1];
    }
    
    return false;
}

// FilteredSignal implementation
FilteredSignal::FilteredSignal(std::shared_ptr<BaseSignal> base_signal,
                              std::function<bool()> filter_condition)
    : BaseSignal(base_signal ? base_signal->get_type() : SignalType::Long),
      base_signal_(base_signal), filter_condition_(std::move(filter_condition)) {}

bool FilteredSignal::check() {
    if (!is_active() || !base_signal_) {
        return false;
    }
    
    bool base_result = base_signal_->check();
    
    // Apply filter if base signal is true
    if (base_result && filter_condition_) {
        return filter_condition_();
    }
    
    return base_result;
}

// CountingSignal implementation
CountingSignal::CountingSignal(std::shared_ptr<BaseSignal> base_signal, 
                              int required_count, int reset_after)
    : BaseSignal(base_signal ? base_signal->get_type() : SignalType::Long),
      base_signal_(base_signal), required_count_(required_count),
      reset_after_(reset_after), current_count_(0), periods_since_last_(0) {}

bool CountingSignal::check() {
    if (!is_active() || !base_signal_) {
        return false;
    }
    
    bool base_result = base_signal_->check();
    periods_since_last_++;
    
    if (base_result) {
        current_count_++;
        periods_since_last_ = 0;
        
        // Check if we've reached the required count
        if (current_count_ >= required_count_) {
            current_count_ = 0; // Reset for next cycle
            return true;
        }
    } else {
        // Reset counter if too much time has passed
        if (reset_after_ > 0 && periods_since_last_ >= reset_after_) {
            current_count_ = 0;
        }
    }
    
    return false;
}

int CountingSignal::get_current_count() const {
    return current_count_;
}

// Factory functions
std::shared_ptr<BaseSignal> create_simple_signal(SignalType type, std::function<bool()> condition) {
    return std::make_shared<SimpleSignal>(type, std::move(condition));
}

std::shared_ptr<BaseSignal> create_crossover_signal(std::shared_ptr<Indicator> fast_line,
                                                   std::shared_ptr<Indicator> slow_line,
                                                   SignalType signal_type) {
    return std::make_shared<CrossoverSignal>(fast_line, slow_line, signal_type);
}

std::shared_ptr<BaseSignal> create_threshold_signal(std::shared_ptr<Indicator> indicator,
                                                   double threshold, SignalType signal_type,
                                                   ThresholdType threshold_type) {
    return std::make_shared<ThresholdSignal>(indicator, threshold, signal_type, threshold_type);
}

std::shared_ptr<BaseSignal> create_composite_signal(SignalType type, LogicType logic_type) {
    return std::make_shared<CompositeSignal>(type, logic_type);
}

std::shared_ptr<BaseSignal> create_delayed_signal(std::shared_ptr<BaseSignal> base_signal, 
                                                 int delay_periods) {
    return std::make_shared<DelayedSignal>(base_signal, delay_periods);
}

std::shared_ptr<BaseSignal> create_filtered_signal(std::shared_ptr<BaseSignal> base_signal,
                                                  std::function<bool()> filter_condition) {
    return std::make_shared<FilteredSignal>(base_signal, std::move(filter_condition));
}

std::shared_ptr<BaseSignal> create_counting_signal(std::shared_ptr<BaseSignal> base_signal,
                                                  int required_count, int reset_after) {
    return std::make_shared<CountingSignal>(base_signal, required_count, reset_after);
}

} // namespace signals
} // namespace backtrader