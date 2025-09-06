#include "../../include/feeds/rollover.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace backtrader {
namespace feeds {

// RollOver implementation

RollOver::RollOver(const Params& params) : params_(params) {
    // Initialize with inherited timeframe and compression
    // These will be copied from the first data feed when available
}

RollOver::RollOver(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds, const Params& params)
    : params_(params), data_feeds_(data_feeds) {
    
    if (data_feeds_.empty()) {
        throw std::invalid_argument("RollOver requires at least one data feed");
    }
    
    // Copy timeframe and compression from first data feed
    if (!data_feeds_.empty()) {
        set_timeframe(data_feeds_[0]->get_timeframe());
        set_compression(data_feeds_[0]->get_compression());
    }
    
    initialize_rollover_chain();
}

void RollOver::add_data_feed(std::shared_ptr<AbstractDataBase> data_feed) {
    if (!data_feed) {
        throw std::invalid_argument("Data feed cannot be null");
    }
    
    data_feeds_.push_back(data_feed);
    
    // If this is the first data feed, copy its properties
    if (data_feeds_.size() == 1) {
        set_timeframe(data_feed->get_timeframe());
        set_compression(data_feed->get_compression());
    }
}

void RollOver::set_data_feeds(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds) {
    if (data_feeds.empty()) {
        throw std::invalid_argument("RollOver requires at least one data feed");
    }
    
    data_feeds_ = data_feeds;
    
    // Copy properties from first data feed
    set_timeframe(data_feeds_[0]->get_timeframe());
    set_compression(data_feeds_[0]->get_compression());
    
    initialize_rollover_chain();
}

void RollOver::start() {
    AbstractDataBase::start();
    
    if (data_feeds_.empty()) {
        throw std::runtime_error("No data feeds configured for rollover");
    }
    
    // Start all data feeds
    for (auto& feed : data_feeds_) {
        if (feed) {
            feed->start();
        }
    }
    
    // Initialize rollover state
    remaining_feeds_ = data_feeds_;
    current_contract_index_ = 0;
    has_rolled_over_ = false;
    is_transitioning_ = false;
    
    // Set initial current data feed
    if (!remaining_feeds_.empty()) {
        current_data_ = remaining_feeds_[0];
        
        if (log_rollovers_) {
            std::cout << "RollOver started with contract: " << get_contract_name(current_data_) << std::endl;
        }
    }
    
    // Clear rollover history
    rollover_history_.clear();
    datetime_stamps_.clear();
}

void RollOver::stop() {
    AbstractDataBase::stop();
    
    // Stop all data feeds
    for (auto& feed : data_feeds_) {
        if (feed) {
            feed->stop();
        }
    }
    
    if (log_rollovers_) {
        std::cout << "RollOver stopped. Total rollovers: " << rollover_history_.size() << std::endl;
    }
}

bool RollOver::next() {
    return load_next_data();
}

void RollOver::preload() {
    // RollOver is designed for live data, so preloading is disabled
    // This method intentionally does nothing
}

void RollOver::set_check_date_function(CheckDateFunc func) {
    params_.checkdate = func;
}

void RollOver::set_check_condition_function(CheckConditionFunc func) {
    params_.checkcondition = func;
}

RollOver::RolloverStats RollOver::get_rollover_statistics() const {
    RolloverStats stats = {};
    
    stats.total_rollovers = static_cast<int>(rollover_history_.size());
    stats.successful_rollovers = stats.total_rollovers; // All recorded rollovers are successful
    
    if (!rollover_history_.empty()) {
        stats.first_rollover = rollover_history_.front().timestamp;
        stats.last_rollover = rollover_history_.back().timestamp;
        
        // Calculate average rollover gap
        if (rollover_history_.size() > 1) {
            auto total_gap = std::chrono::duration_cast<std::chrono::seconds>(
                stats.last_rollover - stats.first_rollover);
            stats.average_rollover_gap = total_gap.count() / (rollover_history_.size() - 1);
        }
    }
    
    return stats;
}

void RollOver::initialize_rollover_chain() {
    // Validate that all data feeds have compatible timeframes
    if (data_feeds_.size() > 1) {
        auto reference_timeframe = data_feeds_[0]->get_timeframe();
        auto reference_compression = data_feeds_[0]->get_compression();
        
        for (size_t i = 1; i < data_feeds_.size(); ++i) {
            if (data_feeds_[i]->get_timeframe() != reference_timeframe ||
                data_feeds_[i]->get_compression() != reference_compression) {
                
                std::cerr << "Warning: Data feed " << i << " has different timeframe/compression" << std::endl;
            }
        }
    }
}

bool RollOver::load_next_data() {
    if (!current_data_) {
        return false;
    }
    
    // Synchronize all data feeds to current time
    synchronize_data_feeds();
    
    // Check if we need to rollover
    if (check_rollover_conditions()) {
        execute_rollover();
    }
    
    // Load next data point from current contract
    if (advance_current_data()) {
        copy_data_to_lines();
        return true;
    }
    
    return false;
}

void RollOver::synchronize_data_feeds() {
    datetime_stamps_.clear();
    
    // Collect current datetime from all active feeds
    for (auto& feed : remaining_feeds_) {
        if (feed) {
            auto dt = feed->get_datetime();
            datetime_stamps_.push_back(dt);
        }
    }
    
    if (!datetime_stamps_.empty()) {
        // Use the earliest datetime as synchronization point
        current_datetime_ = *std::min_element(datetime_stamps_.begin(), datetime_stamps_.end());
        
        // Advance all feeds to this synchronized time
        advance_all_data_to_time(current_datetime_);
    }
}

bool RollOver::check_rollover_conditions() {
    // Need at least 2 contracts to rollover
    if (remaining_feeds_.size() < 2) {
        return false;
    }
    
    auto next_contract = remaining_feeds_[1];
    
    // Check date condition if provided
    if (params_.checkdate) {
        if (!check_date_condition(current_datetime_, current_data_)) {
            return false;
        }
    }
    
    // Check rollover condition if provided
    if (params_.checkcondition) {
        if (!check_rollover_condition(current_data_, next_contract)) {
            return false;
        }
    }
    
    // If no conditions are specified, use default behavior
    // (this would typically not trigger rollover without explicit conditions)
    if (!params_.checkdate && !params_.checkcondition) {
        return false;
    }
    
    return true;
}

void RollOver::execute_rollover() {
    if (remaining_feeds_.size() < 2) {
        return;
    }
    
    expiring_data_ = current_data_;
    current_data_ = remaining_feeds_[1];
    
    // Remove the expired contract from remaining feeds
    remaining_feeds_.erase(remaining_feeds_.begin());
    current_contract_index_++;
    has_rolled_over_ = true;
    is_transitioning_ = true;
    
    // Log the rollover event
    log_rollover_event("Rollover conditions met");
    
    if (log_rollovers_) {
        std::cout << "Rolled over from " << get_contract_name(expiring_data_) 
                  << " to " << get_contract_name(current_data_) << std::endl;
    }
    
    // Reset transition flag after processing
    is_transitioning_ = false;
}

void RollOver::copy_data_to_lines() {
    if (!current_data_ || current_data_->get_lines().empty()) {
        return;
    }
    
    const auto& source_lines = current_data_->get_lines();
    
    // Ensure we have enough lines
    if (lines_.size() != source_lines.size()) {
        lines_.resize(source_lines.size());
    }
    
    // Copy data from current contract to our lines
    for (size_t i = 0; i < source_lines.size() && i < lines_.size(); ++i) {
        if (!source_lines[i].empty()) {
            lines_[i].push_back(source_lines[i].back());
        }
    }
}

bool RollOver::check_date_condition(std::chrono::system_clock::time_point dt, std::shared_ptr<AbstractDataBase> data) {
    if (!params_.checkdate) {
        return true;
    }
    
    try {
        return params_.checkdate(dt, data);
    } catch (const std::exception& e) {
        std::cerr << "Error in date condition check: " << e.what() << std::endl;
        return false;
    }
}

bool RollOver::check_rollover_condition(std::shared_ptr<AbstractDataBase> current, std::shared_ptr<AbstractDataBase> next) {
    if (!params_.checkcondition) {
        return true;
    }
    
    try {
        return params_.checkcondition(current, next);
    } catch (const std::exception& e) {
        std::cerr << "Error in rollover condition check: " << e.what() << std::endl;
        return false;
    }
}

void RollOver::log_rollover_event(const std::string& reason) {
    RolloverEvent event;
    event.timestamp = current_datetime_;
    event.from_contract_index = current_contract_index_;
    event.to_contract_index = current_contract_index_ + 1;
    event.from_contract_name = get_contract_name(expiring_data_);
    event.to_contract_name = get_contract_name(current_data_);
    event.reason = reason;
    
    rollover_history_.push_back(event);
}

std::string RollOver::get_contract_name(std::shared_ptr<AbstractDataBase> data) const {
    if (!data) {
        return "Unknown";
    }
    
    // Try to extract contract name from data feed
    // This is a simplified implementation - in practice, you might have
    // a more sophisticated naming scheme
    std::ostringstream oss;
    oss << "Contract_" << std::hex << reinterpret_cast<uintptr_t>(data.get());
    return oss.str();
}

std::chrono::system_clock::time_point RollOver::get_timezone_aware_time() const {
    // Get timezone from first data feed if available
    if (!data_feeds_.empty() && data_feeds_[0]) {
        // In a full implementation, this would extract timezone info
        // For now, return current system time
        return std::chrono::system_clock::now();
    }
    
    return current_datetime_;
}

bool RollOver::advance_current_data() {
    if (!current_data_) {
        return false;
    }
    
    return current_data_->next();
}

bool RollOver::advance_all_data_to_time(std::chrono::system_clock::time_point target_time) {
    bool all_advanced = true;
    
    for (auto& feed : remaining_feeds_) {
        if (feed) {
            // Advance this feed until it reaches the target time
            while (feed->get_datetime() < target_time) {
                if (!feed->next()) {
                    all_advanced = false;
                    break;
                }
            }
        }
    }
    
    return all_advanced;
}

void RollOver::reset_data_feeds() {
    for (auto& feed : data_feeds_) {
        if (feed) {
            feed->start(); // Restart the data feed
        }
    }
}

// Rollover conditions implementation

namespace rollover_conditions {

CheckConditionFunc volume_threshold_condition(double threshold_ratio) {
    return [threshold_ratio](std::shared_ptr<AbstractDataBase> current, std::shared_ptr<AbstractDataBase> next) -> bool {
        if (!current || !next) {
            return false;
        }
        
        // Get volume from current and next contracts
        double current_volume = current->get_volume();
        double next_volume = next->get_volume();
        
        if (current_volume <= 0 || next_volume <= 0) {
            return false;
        }
        
        double ratio = current_volume / next_volume;
        return ratio < threshold_ratio;
    };
}

CheckDateFunc days_before_expiration_condition(int days_before) {
    return [days_before](std::chrono::system_clock::time_point dt, std::shared_ptr<AbstractDataBase> data) -> bool {
        if (!data) {
            return false;
        }
        
        // This is a simplified implementation
        // In practice, you would have expiration date metadata
        auto days_until_expiry = std::chrono::hours(24 * days_before);
        auto expiry_threshold = dt + days_until_expiry;
        
        // Placeholder logic - would need actual expiry date from contract metadata
        return false; // Implement based on your contract metadata structure
    };
}

CheckConditionFunc volume_and_oi_condition(double volume_ratio, double oi_ratio) {
    return [volume_ratio, oi_ratio](std::shared_ptr<AbstractDataBase> current, std::shared_ptr<AbstractDataBase> next) -> bool {
        if (!current || !next) {
            return false;
        }
        
        // Check volume condition
        double current_volume = current->get_volume();
        double next_volume = next->get_volume();
        bool volume_condition = (current_volume > 0 && next_volume > 0) && 
                               (current_volume / next_volume < volume_ratio);
        
        // Check open interest condition
        double current_oi = current->get_openinterest();
        double next_oi = next->get_openinterest();
        bool oi_condition = (current_oi > 0 && next_oi > 0) && 
                           (current_oi / next_oi < oi_ratio);
        
        return volume_condition && oi_condition;
    };
}

CheckDateFunc time_of_day_condition(int hour, int minute) {
    return [hour, minute](std::chrono::system_clock::time_point dt, std::shared_ptr<AbstractDataBase> data) -> bool {
        auto time_t = std::chrono::system_clock::to_time_t(dt);
        auto* tm = std::localtime(&time_t);
        
        if (!tm) {
            return false;
        }
        
        return (tm->tm_hour == hour && tm->tm_min >= minute) || tm->tm_hour > hour;
    };
}

CheckConditionFunc liquidity_condition(double max_spread_ratio) {
    return [max_spread_ratio](std::shared_ptr<AbstractDataBase> current, std::shared_ptr<AbstractDataBase> next) -> bool {
        if (!current || !next) {
            return false;
        }
        
        // Simplified liquidity check based on volume
        // In practice, you would check bid-ask spreads and market depth
        double current_liquidity = current->get_volume();
        double next_liquidity = next->get_volume();
        
        if (current_liquidity <= 0) {
            return true; // Roll over if no liquidity in current
        }
        
        return next_liquidity > current_liquidity * (1.0 + max_spread_ratio);
    };
}

// ConditionBuilder implementation

ConditionBuilder& ConditionBuilder::volume_ratio(double ratio) {
    criteria_.volume_ratio = ratio;
    return *this;
}

ConditionBuilder& ConditionBuilder::open_interest_ratio(double ratio) {
    criteria_.oi_ratio = ratio;
    return *this;
}

ConditionBuilder& ConditionBuilder::days_before_expiry(int days) {
    criteria_.days_before_expiry = days;
    return *this;
}

ConditionBuilder& ConditionBuilder::time_of_day(int hour, int minute) {
    criteria_.rollover_hour = hour;
    criteria_.rollover_minute = minute;
    return *this;
}

ConditionBuilder& ConditionBuilder::minimum_volume(double min_volume) {
    criteria_.min_volume = min_volume;
    return *this;
}

ConditionBuilder& ConditionBuilder::use_and_logic(bool use_and) {
    criteria_.use_and_logic = use_and;
    return *this;
}

CheckConditionFunc ConditionBuilder::build_condition() {
    auto criteria = criteria_; // Capture by value
    
    return [criteria](std::shared_ptr<AbstractDataBase> current, std::shared_ptr<AbstractDataBase> next) -> bool {
        std::vector<bool> conditions;
        
        // Volume ratio condition
        if (criteria.volume_ratio > 0) {
            double current_vol = current->get_volume();
            double next_vol = next->get_volume();
            bool vol_condition = (current_vol > 0 && next_vol > 0) && 
                                (current_vol / next_vol < criteria.volume_ratio);
            conditions.push_back(vol_condition);
        }
        
        // Open interest ratio condition
        if (criteria.oi_ratio > 0) {
            double current_oi = current->get_openinterest();
            double next_oi = next->get_openinterest();
            bool oi_condition = (current_oi > 0 && next_oi > 0) && 
                               (current_oi / next_oi < criteria.oi_ratio);
            conditions.push_back(oi_condition);
        }
        
        // Minimum volume condition
        if (criteria.min_volume > 0) {
            bool min_vol_condition = next->get_volume() >= criteria.min_volume;
            conditions.push_back(min_vol_condition);
        }
        
        if (conditions.empty()) {
            return false;
        }
        
        // Apply AND/OR logic
        if (criteria.use_and_logic) {
            return std::all_of(conditions.begin(), conditions.end(), [](bool b) { return b; });
        } else {
            return std::any_of(conditions.begin(), conditions.end(), [](bool b) { return b; });
        }
    };
}

CheckDateFunc ConditionBuilder::build_date_condition() {
    auto criteria = criteria_; // Capture by value
    
    return [criteria](std::chrono::system_clock::time_point dt, std::shared_ptr<AbstractDataBase> data) -> bool {
        std::vector<bool> conditions;
        
        // Time of day condition
        if (criteria.rollover_hour >= 0) {
            auto time_t = std::chrono::system_clock::to_time_t(dt);
            auto* tm = std::localtime(&time_t);
            if (tm) {
                bool time_condition = (tm->tm_hour == criteria.rollover_hour && tm->tm_min >= criteria.rollover_minute) || 
                                     tm->tm_hour > criteria.rollover_hour;
                conditions.push_back(time_condition);
            }
        }
        
        // Days before expiry would require contract metadata
        if (criteria.days_before_expiry > 0) {
            // Placeholder - implement based on your expiry date structure
            conditions.push_back(false);
        }
        
        if (conditions.empty()) {
            return true; // No date conditions specified
        }
        
        // Apply AND/OR logic
        if (criteria.use_and_logic) {
            return std::all_of(conditions.begin(), conditions.end(), [](bool b) { return b; });
        } else {
            return std::any_of(conditions.begin(), conditions.end(), [](bool b) { return b; });
        }
    };
}

} // namespace rollover_conditions

// Factory functions implementation

namespace rollover_factory {

std::shared_ptr<RollOver> create_volume_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    double volume_threshold) {
    
    RollOver::Params params;
    params.checkcondition = rollover_conditions::volume_threshold_condition(volume_threshold);
    
    return std::make_shared<RollOver>(contracts, params);
}

std::shared_ptr<RollOver> create_date_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    int days_before_expiry) {
    
    RollOver::Params params;
    params.checkdate = rollover_conditions::days_before_expiration_condition(days_before_expiry);
    
    return std::make_shared<RollOver>(contracts, params);
}

std::shared_ptr<RollOver> create_combined_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    double volume_threshold,
    int days_before_expiry) {
    
    RollOver::Params params;
    params.checkcondition = rollover_conditions::volume_threshold_condition(volume_threshold);
    params.checkdate = rollover_conditions::days_before_expiration_condition(days_before_expiry);
    
    return std::make_shared<RollOver>(contracts, params);
}

std::shared_ptr<RollOver> create_custom_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    CheckDateFunc date_func,
    CheckConditionFunc condition_func) {
    
    RollOver::Params params;
    params.checkdate = date_func;
    params.checkcondition = condition_func;
    
    return std::make_shared<RollOver>(contracts, params);
}

} // namespace rollover_factory

// Utility functions implementation

namespace rollover_utils {

VolumeAnalysis analyze_contract_volumes(
    std::shared_ptr<AbstractDataBase> current_contract,
    std::shared_ptr<AbstractDataBase> next_contract,
    int analysis_period) {
    
    VolumeAnalysis analysis = {};
    
    if (!current_contract || !next_contract) {
        analysis.recommendation = "Invalid contracts provided";
        return analysis;
    }
    
    analysis.current_volume = current_contract->get_volume();
    analysis.next_volume = next_contract->get_volume();
    
    if (analysis.current_volume > 0 && analysis.next_volume > 0) {
        analysis.volume_ratio = analysis.current_volume / analysis.next_volume;
        analysis.should_rollover = analysis.volume_ratio < 0.5;
        
        if (analysis.should_rollover) {
            analysis.recommendation = "Recommend rollover - next contract has higher volume";
        } else {
            analysis.recommendation = "Current contract still has sufficient volume";
        }
    } else {
        analysis.recommendation = "Insufficient volume data for analysis";
    }
    
    return analysis;
}

RolloverCost calculate_rollover_cost(
    std::shared_ptr<AbstractDataBase> current_contract,
    std::shared_ptr<AbstractDataBase> next_contract) {
    
    RolloverCost cost = {};
    
    if (!current_contract || !next_contract) {
        cost.cost_assessment = "Invalid contracts provided";
        return cost;
    }
    
    double current_price = current_contract->get_close();
    double next_price = next_contract->get_close();
    
    if (current_price > 0 && next_price > 0) {
        cost.spread_absolute = std::abs(next_price - current_price);
        cost.spread_percentage = (cost.spread_absolute / current_price) * 100.0;
        cost.estimated_slippage = cost.spread_absolute * 0.5; // Simplified estimate
        
        if (cost.spread_percentage < 0.1) {
            cost.cost_assessment = "Low rollover cost";
        } else if (cost.spread_percentage < 0.5) {
            cost.cost_assessment = "Moderate rollover cost";
        } else {
            cost.cost_assessment = "High rollover cost";
        }
    } else {
        cost.cost_assessment = "Cannot calculate cost - missing price data";
    }
    
    return cost;
}

ContinuityCheck validate_rollover_continuity(const RollOver& rollover_feed) {
    ContinuityCheck check = {};
    check.data_integrity_ok = true;
    check.has_gaps = false;
    check.largest_gap_seconds = 0.0;
    
    // Get rollover history
    auto history = rollover_feed.get_rollover_history();
    
    if (history.size() < 2) {
        check.status_message = "Insufficient rollover history for analysis";
        return check;
    }
    
    // Check gaps between rollovers
    for (size_t i = 1; i < history.size(); ++i) {
        auto gap = std::chrono::duration_cast<std::chrono::seconds>(
            history[i].timestamp - history[i-1].timestamp).count();
        
        if (gap > check.largest_gap_seconds) {
            check.largest_gap_seconds = gap;
        }
        
        // Consider gaps > 1 day as potential issues
        if (gap > 86400) {
            check.has_gaps = true;
            check.gap_periods.push_back(history[i-1].timestamp);
        }
    }
    
    if (check.has_gaps) {
        check.status_message = "Data gaps detected in rollover history";
        check.data_integrity_ok = false;
    } else {
        check.status_message = "Rollover data continuity looks good";
    }
    
    return check;
}

RolloverSchedule generate_rollover_schedule(
    const std::vector<std::string>& contract_names,
    const std::vector<std::chrono::system_clock::time_point>& expiry_dates,
    int rollover_days_before) {
    
    RolloverSchedule schedule;
    schedule.market_name = "Generic";
    schedule.schedule_type = "Volume-based";
    
    if (contract_names.size() != expiry_dates.size()) {
        return schedule; // Invalid input
    }
    
    for (size_t i = 0; i < contract_names.size(); ++i) {
        RolloverSchedule::ScheduleEntry entry;
        entry.contract_name = contract_names[i];
        entry.expiry_date = expiry_dates[i];
        entry.rollover_date = expiry_dates[i] - std::chrono::hours(24 * rollover_days_before);
        
        if (i == 0) {
            entry.start_date = std::chrono::system_clock::now();
        } else {
            entry.start_date = schedule.entries[i-1].rollover_date;
        }
        
        schedule.entries.push_back(entry);
    }
    
    return schedule;
}

OptimizationResult optimize_rollover_parameters(
    const std::vector<std::shared_ptr<AbstractDataBase>>& historical_contracts,
    const std::vector<double>& volume_thresholds,
    const std::vector<int>& days_before_expiry) {
    
    OptimizationResult result = {};
    result.performance_score = -1.0;
    result.optimization_method = "Grid Search";
    
    // Simplified optimization - test all parameter combinations
    for (double vol_threshold : volume_thresholds) {
        for (int days_before : days_before_expiry) {
            // Create test rollover with these parameters
            auto test_rollover = rollover_factory::create_combined_rollover(
                historical_contracts, vol_threshold, days_before);
            
            // Simulate and score performance (simplified)
            double score = vol_threshold * 0.5 + (10.0 - days_before) * 0.1; // Placeholder scoring
            
            std::string param_key = "vol_" + std::to_string(vol_threshold) + 
                                   "_days_" + std::to_string(days_before);
            result.tested_parameters[param_key] = score;
            
            if (score > result.performance_score) {
                result.performance_score = score;
                result.optimal_volume_threshold = vol_threshold;
                result.optimal_days_before_expiry = days_before;
            }
        }
    }
    
    return result;
}

StrategyComparison compare_rollover_strategies(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    const std::vector<std::pair<std::string, std::shared_ptr<RollOver>>>& strategies) {
    
    StrategyComparison comparison;
    
    for (const auto& strategy_pair : strategies) {
        StrategyComparison::StrategyResult result;
        result.strategy_name = strategy_pair.first;
        
        auto rollover = strategy_pair.second;
        if (rollover) {
            auto stats = rollover->get_rollover_statistics();
            result.total_rollovers = stats.total_rollovers;
            result.average_cost = 0.1; // Placeholder
            result.continuity_score = stats.successful_rollovers / 
                                     std::max(1.0, static_cast<double>(stats.total_rollovers));
            result.overall_score = result.continuity_score * 0.6 + (1.0 - result.average_cost) * 0.4;
        }
        
        comparison.results.push_back(result);
    }
    
    // Find best strategy
    if (!comparison.results.empty()) {
        auto best_it = std::max_element(comparison.results.begin(), comparison.results.end(),
            [](const auto& a, const auto& b) { return a.overall_score < b.overall_score; });
        comparison.best_strategy = best_it->strategy_name;
    }
    
    return comparison;
}

} // namespace rollover_utils

} // namespace feeds
} // namespace backtrader