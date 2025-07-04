#pragma once

#include "../feed.h"
#include <functional>
#include <vector>
#include <memory>
#include <chrono>
#include <string>

namespace backtrader {
namespace feeds {

/**
 * RollOver - Futures Contract Rollover Data Feed
 * 
 * This data feed handles the automatic rollover from one futures contract to another
 * when specified conditions are met. It creates a continuous contract by seamlessly
 * transitioning between different contract months.
 * 
 * The rollover mechanism ensures that:
 * 1. Data flows continuously without gaps
 * 2. Volume and liquidity thresholds are respected
 * 3. Custom rollover conditions can be applied
 * 4. Timezone handling is consistent across contracts
 * 
 * Typical use case: Rolling from December 2024 crude oil contract to January 2025
 * contract when volume in December drops below a threshold or expiration approaches.
 */
class RollOver : public AbstractDataBase {
public:
    // Type aliases for rollover condition functions
    using CheckDateFunc = std::function<bool(std::chrono::system_clock::time_point, std::shared_ptr<AbstractDataBase>)>;
    using CheckConditionFunc = std::function<bool(std::shared_ptr<AbstractDataBase>, std::shared_ptr<AbstractDataBase>)>;
    
    // Parameters structure
    struct Params : public AbstractDataBase::Params {
        CheckDateFunc checkdate = nullptr;           // Function to check if rollover date conditions are met
        CheckConditionFunc checkcondition = nullptr; // Function to validate rollover conditions (volume, etc.)
    };
    
    // Alternative constructor types
    RollOver(const Params& params = Params{});
    RollOver(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds, const Params& params = Params{});
    virtual ~RollOver() = default;
    
    // Add data feeds for rollover chain
    void add_data_feed(std::shared_ptr<AbstractDataBase> data_feed);
    void set_data_feeds(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds);
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // Live data support
    bool is_live() const override { return true; }  // Disable preloading and runonce
    
    // Rollover condition setters
    void set_check_date_function(CheckDateFunc func);
    void set_check_condition_function(CheckConditionFunc func);
    
    // Rollover status
    bool has_rolled_over() const { return has_rolled_over_; }
    int get_current_contract_index() const { return current_contract_index_; }
    std::shared_ptr<AbstractDataBase> get_current_contract() const { return current_data_; }
    std::shared_ptr<AbstractDataBase> get_expiring_contract() const { return expiring_data_; }
    
    // Rollover history
    struct RolloverEvent {
        std::chrono::system_clock::time_point timestamp;
        int from_contract_index;
        int to_contract_index;
        std::string from_contract_name;
        std::string to_contract_name;
        std::string reason;
    };
    
    std::vector<RolloverEvent> get_rollover_history() const { return rollover_history_; }
    
    // Configuration
    void enable_rollover_logging(bool enable = true) { log_rollovers_ = enable; }
    bool is_rollover_logging_enabled() const { return log_rollovers_; }
    
    // Statistics
    struct RolloverStats {
        int total_rollovers;
        int successful_rollovers;
        double average_rollover_gap;  // Average time between rollovers
        std::chrono::system_clock::time_point first_rollover;
        std::chrono::system_clock::time_point last_rollover;
    };
    
    RolloverStats get_rollover_statistics() const;
    
protected:
    Params params_;
    
    // Data feed management
    std::vector<std::shared_ptr<AbstractDataBase>> data_feeds_;      // All contracts in rollover chain
    std::vector<std::shared_ptr<AbstractDataBase>> remaining_feeds_; // Remaining contracts to process
    std::shared_ptr<AbstractDataBase> current_data_;                 // Currently active contract
    std::shared_ptr<AbstractDataBase> expiring_data_;               // Contract being rolled out
    
    // State management
    int current_contract_index_ = 0;
    bool has_rolled_over_ = false;
    bool is_transitioning_ = false;
    bool log_rollovers_ = true;
    
    // Synchronization
    std::vector<std::chrono::system_clock::time_point> datetime_stamps_;
    std::chrono::system_clock::time_point current_datetime_;
    
    // Rollover tracking
    std::vector<RolloverEvent> rollover_history_;
    
private:
    void initialize_rollover_chain();
    bool load_next_data();
    void synchronize_data_feeds();
    bool check_rollover_conditions();
    void execute_rollover();
    void copy_data_to_lines();
    
    // Condition checking
    bool check_date_condition(std::chrono::system_clock::time_point dt, std::shared_ptr<AbstractDataBase> data);
    bool check_rollover_condition(std::shared_ptr<AbstractDataBase> current, std::shared_ptr<AbstractDataBase> next);
    
    // Helper methods
    void log_rollover_event(const std::string& reason);
    std::string get_contract_name(std::shared_ptr<AbstractDataBase> data) const;
    std::chrono::system_clock::time_point get_timezone_aware_time() const;
    
    // Data management
    bool advance_current_data();
    bool advance_all_data_to_time(std::chrono::system_clock::time_point target_time);
    void reset_data_feeds();
};

/**
 * Pre-defined rollover condition functions
 */
namespace rollover_conditions {

/**
 * Volume-based rollover condition
 * Rolls over when the current contract's volume drops below a threshold
 * relative to the next contract's volume
 */
CheckConditionFunc volume_threshold_condition(double threshold_ratio = 0.5);

/**
 * Date-based rollover condition  
 * Rolls over when a specific number of days before expiration
 */
CheckDateFunc days_before_expiration_condition(int days_before = 5);

/**
 * Combined volume and open interest condition
 * Rolls over when both volume and open interest favor the next contract
 */
CheckConditionFunc volume_and_oi_condition(double volume_ratio = 0.5, double oi_ratio = 0.5);

/**
 * Time-based rollover condition
 * Rolls over at a specific time of day (e.g., after market close)
 */
CheckDateFunc time_of_day_condition(int hour = 16, int minute = 0); // Default: 4:00 PM

/**
 * Liquidity-based rollover condition
 * Rolls over based on bid-ask spread and market depth
 */
CheckConditionFunc liquidity_condition(double max_spread_ratio = 0.02);

/**
 * Custom rollover condition builder
 * Allows combining multiple conditions with AND/OR logic
 */
class ConditionBuilder {
public:
    ConditionBuilder& volume_ratio(double ratio);
    ConditionBuilder& open_interest_ratio(double ratio);
    ConditionBuilder& days_before_expiry(int days);
    ConditionBuilder& time_of_day(int hour, int minute = 0);
    ConditionBuilder& minimum_volume(double min_volume);
    ConditionBuilder& use_and_logic(bool use_and = true);
    
    CheckConditionFunc build_condition();
    CheckDateFunc build_date_condition();
    
private:
    struct ConditionCriteria {
        double volume_ratio = -1.0;
        double oi_ratio = -1.0;
        int days_before_expiry = -1;
        int rollover_hour = -1;
        int rollover_minute = 0;
        double min_volume = -1.0;
        bool use_and_logic = true;
    } criteria_;
};

} // namespace rollover_conditions

/**
 * Factory functions for creating rollover data feeds
 */
namespace rollover_factory {

/**
 * Create a simple volume-based rollover
 */
std::shared_ptr<RollOver> create_volume_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    double volume_threshold = 0.5
);

/**
 * Create a date-based rollover  
 */
std::shared_ptr<RollOver> create_date_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    int days_before_expiry = 5
);

/**
 * Create a combined volume and date rollover
 */
std::shared_ptr<RollOver> create_combined_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    double volume_threshold = 0.5,
    int days_before_expiry = 5
);

/**
 * Create a custom rollover with user-defined conditions
 */
std::shared_ptr<RollOver> create_custom_rollover(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    CheckDateFunc date_func,
    CheckConditionFunc condition_func
);

} // namespace rollover_factory

/**
 * Utility functions for rollover analysis
 */
namespace rollover_utils {

/**
 * Analyze contract volume patterns to suggest optimal rollover timing
 */
struct VolumeAnalysis {
    double current_volume;
    double next_volume;
    double volume_ratio;
    bool should_rollover;
    std::string recommendation;
};

VolumeAnalysis analyze_contract_volumes(
    std::shared_ptr<AbstractDataBase> current_contract,
    std::shared_ptr<AbstractDataBase> next_contract,
    int analysis_period = 10
);

/**
 * Calculate the cost of rollover (spread between contracts)
 */
struct RolloverCost {
    double spread_absolute;
    double spread_percentage;
    double estimated_slippage;
    std::string cost_assessment;
};

RolloverCost calculate_rollover_cost(
    std::shared_ptr<AbstractDataBase> current_contract,
    std::shared_ptr<AbstractDataBase> next_contract
);

/**
 * Validate rollover data continuity
 */
struct ContinuityCheck {
    bool has_gaps;
    std::vector<std::chrono::system_clock::time_point> gap_periods;
    double largest_gap_seconds;
    bool data_integrity_ok;
    std::string status_message;
};

ContinuityCheck validate_rollover_continuity(const RollOver& rollover_feed);

/**
 * Generate rollover schedule based on contract specifications
 */
struct RolloverSchedule {
    struct ScheduleEntry {
        std::string contract_name;
        std::chrono::system_clock::time_point start_date;
        std::chrono::system_clock::time_point rollover_date;
        std::chrono::system_clock::time_point expiry_date;
    };
    
    std::vector<ScheduleEntry> entries;
    std::string market_name;
    std::string schedule_type;
};

RolloverSchedule generate_rollover_schedule(
    const std::vector<std::string>& contract_names,
    const std::vector<std::chrono::system_clock::time_point>& expiry_dates,
    int rollover_days_before = 5
);

/**
 * Optimize rollover parameters based on historical data
 */
struct OptimizationResult {
    double optimal_volume_threshold;
    int optimal_days_before_expiry;
    double performance_score;
    std::string optimization_method;
    std::map<std::string, double> tested_parameters;
};

OptimizationResult optimize_rollover_parameters(
    const std::vector<std::shared_ptr<AbstractDataBase>>& historical_contracts,
    const std::vector<double>& volume_thresholds = {0.3, 0.4, 0.5, 0.6, 0.7},
    const std::vector<int>& days_before_expiry = {3, 5, 7, 10}
);

/**
 * Compare different rollover strategies
 */
struct StrategyComparison {
    struct StrategyResult {
        std::string strategy_name;
        int total_rollovers;
        double average_cost;
        double continuity_score;
        double overall_score;
    };
    
    std::vector<StrategyResult> results;
    std::string best_strategy;
};

StrategyComparison compare_rollover_strategies(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts,
    const std::vector<std::pair<std::string, std::shared_ptr<RollOver>>>& strategies
);

} // namespace rollover_utils

} // namespace feeds
} // namespace backtrader