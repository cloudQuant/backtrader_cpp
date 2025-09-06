#pragma once

#include "../feed.h"
#include <vector>
#include <memory>
#include <chrono>
#include <string>
#include <queue>

namespace backtrader {
namespace feeds {

/**
 * Chainer - Data Feed Chaining and Concatenation
 * 
 * The Chainer class provides functionality to link and concatenate multiple data feeds
 * together to create continuous data streams. This is particularly useful for:
 * 
 * 1. Historical Data Continuation - Combining multiple historical data files
 * 2. Live-to-Historical Transition - Chaining historical data with live feeds
 * 3. Multiple Time Period Data - Joining data from different time periods
 * 4. Data Source Failover - Automatic switching between data sources
 * 5. Contract Rolling - Futures contract continuation
 * 
 * The chainer ensures:
 * - Time-ordered data delivery (no backward time movement)
 * - Seamless transitions between data sources
 * - Proper environment and timezone propagation
 * - Live data compatibility
 */
class Chainer : public AbstractDataBase {
public:
    // Parameters structure
    struct Params : public AbstractDataBase::Params {
        bool allow_time_overlap = false;    // Allow overlapping timestamps between feeds
        bool strict_time_order = true;      // Enforce strict time ordering
        bool auto_fill_gaps = false;        // Auto-fill gaps between data sources
        double gap_fill_value = std::numeric_limits<double>::quiet_NaN();  // Value for gap filling
    };
    
    // Constructor variants
    Chainer(const Params& params = Params{});
    Chainer(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds, const Params& params = Params{});
    virtual ~Chainer() = default;
    
    // Add data feeds to the chain
    void add_data_feed(std::shared_ptr<AbstractDataBase> data_feed);
    void set_data_feeds(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds);
    void insert_data_feed(size_t position, std::shared_ptr<AbstractDataBase> data_feed);
    void remove_data_feed(size_t position);
    void clear_data_feeds();
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // Live data support
    bool is_live() const override { return true; }  // Disable preloading and runonce
    
    // Chain management
    size_t get_chain_length() const { return data_feeds_.size(); }
    int get_current_feed_index() const { return current_feed_index_; }
    std::shared_ptr<AbstractDataBase> get_current_feed() const { return current_feed_; }
    std::shared_ptr<AbstractDataBase> get_feed_at(size_t index) const;
    
    // Chain status
    bool is_chain_complete() const { return current_feed_index_ >= static_cast<int>(data_feeds_.size()); }
    bool has_more_feeds() const { return current_feed_index_ < static_cast<int>(data_feeds_.size()) - 1; }
    
    // Statistics
    struct ChainStats {
        int total_feeds;
        int completed_feeds;
        int current_feed_index;
        size_t total_bars_delivered;
        std::chrono::system_clock::time_point chain_start_time;
        std::chrono::system_clock::time_point chain_end_time;
        std::vector<size_t> bars_per_feed;
        std::vector<std::string> feed_names;
    };
    
    ChainStats get_chain_statistics() const;
    
    // Transition tracking
    struct TransitionEvent {
        std::chrono::system_clock::time_point timestamp;
        int from_feed_index;
        int to_feed_index;
        std::string from_feed_name;
        std::string to_feed_name;
        std::string transition_reason;
        size_t bars_from_previous_feed;
    };
    
    std::vector<TransitionEvent> get_transition_history() const { return transition_history_; }
    
    // Configuration
    void enable_transition_logging(bool enable = true) { log_transitions_ = enable; }
    bool is_transition_logging_enabled() const { return log_transitions_; }
    
    void set_gap_fill_strategy(bool auto_fill, double fill_value = std::numeric_limits<double>::quiet_NaN());
    
    // Data validation
    bool validate_chain_continuity() const;
    std::vector<std::chrono::system_clock::time_point> find_time_gaps() const;
    
protected:
    Params params_;
    
    // Data feed management
    std::vector<std::shared_ptr<AbstractDataBase>> data_feeds_;  // All data feeds in chain
    std::shared_ptr<AbstractDataBase> current_feed_;            // Currently active feed
    int current_feed_index_ = -1;                               // Index of current feed
    
    // Time tracking
    std::chrono::system_clock::time_point last_delivered_time_;
    
    // Chain statistics
    size_t total_bars_delivered_ = 0;
    std::vector<size_t> bars_per_feed_;
    
    // Transition tracking
    std::vector<TransitionEvent> transition_history_;
    bool log_transitions_ = true;
    
    // State management
    bool chain_started_ = false;
    bool chain_completed_ = false;
    
private:
    void initialize_chain();
    bool advance_to_next_feed();
    bool load_next_data_point();
    void copy_current_data_to_lines();
    
    // Time validation
    bool is_time_valid(std::chrono::system_clock::time_point new_time);
    void handle_time_overlap(std::chrono::system_clock::time_point new_time);
    
    // Gap handling
    void handle_data_gap(std::chrono::system_clock::time_point last_time, 
                        std::chrono::system_clock::time_point new_time);
    std::vector<double> create_gap_fill_bar(std::chrono::system_clock::time_point fill_time);
    
    // Transition management
    void log_transition(const std::string& reason);
    std::string get_feed_name(std::shared_ptr<AbstractDataBase> feed) const;
    
    // Environment propagation
    void propagate_environment_to_feeds();
    void setup_feed_environment(std::shared_ptr<AbstractDataBase> feed);
    
    // Validation helpers
    bool validate_feed_compatibility(std::shared_ptr<AbstractDataBase> feed) const;
    bool check_timeframe_consistency() const;
    void synchronize_feed_properties();
};

/**
 * ChainerBuilder - Fluent interface for building data chains
 */
class ChainerBuilder {
public:
    ChainerBuilder();
    
    // Add data feeds
    ChainerBuilder& add_feed(std::shared_ptr<AbstractDataBase> feed);
    ChainerBuilder& add_feeds(const std::vector<std::shared_ptr<AbstractDataBase>>& feeds);
    
    // Configuration
    ChainerBuilder& allow_time_overlap(bool allow = true);
    ChainerBuilder& strict_time_order(bool strict = true);
    ChainerBuilder& auto_fill_gaps(bool fill = true, double fill_value = std::numeric_limits<double>::quiet_NaN());
    ChainerBuilder& enable_logging(bool enable = true);
    
    // Build the chainer
    std::shared_ptr<Chainer> build();
    
private:
    std::vector<std::shared_ptr<AbstractDataBase>> feeds_;
    Chainer::Params params_;
    bool logging_enabled_ = true;
};

/**
 * Pre-defined chaining strategies
 */
namespace chaining_strategies {

/**
 * Sequential chaining - process feeds one after another
 */
std::shared_ptr<Chainer> create_sequential_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds
);

/**
 * Time-merged chaining - merge feeds based on timestamps
 */
std::shared_ptr<Chainer> create_time_merged_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds,
    bool allow_overlap = false
);

/**
 * Gap-filled chaining - automatically fill gaps between feeds
 */
std::shared_ptr<Chainer> create_gap_filled_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds,
    double fill_value = std::numeric_limits<double>::quiet_NaN()
);

/**
 * Failover chaining - switch to backup feeds on failure
 */
std::shared_ptr<Chainer> create_failover_chain(
    std::shared_ptr<AbstractDataBase> primary_feed,
    const std::vector<std::shared_ptr<AbstractDataBase>>& backup_feeds
);

} // namespace chaining_strategies

/**
 * Factory functions for common chaining scenarios
 */
namespace chainer_factory {

/**
 * Create a historical-to-live data chain
 */
std::shared_ptr<Chainer> create_historical_to_live_chain(
    std::shared_ptr<AbstractDataBase> historical_feed,
    std::shared_ptr<AbstractDataBase> live_feed
);

/**
 * Create a multi-file historical data chain
 */
std::shared_ptr<Chainer> create_multi_file_chain(
    const std::vector<std::string>& file_paths,
    const std::string& file_format = "csv"
);

/**
 * Create a contract rollover chain (similar to rollover but with chaining)
 */
std::shared_ptr<Chainer> create_contract_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts
);

/**
 * Create a data source failover chain
 */
std::shared_ptr<Chainer> create_redundant_source_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& sources
);

} // namespace chainer_factory

/**
 * Utility functions for data chaining analysis
 */
namespace chainer_utils {

/**
 * Analyze data continuity across chained feeds
 */
struct ContinuityAnalysis {
    bool is_continuous;
    size_t total_gaps;
    std::chrono::seconds largest_gap;
    std::chrono::seconds total_gap_time;
    std::vector<std::pair<std::chrono::system_clock::time_point, std::chrono::seconds>> gaps;
    double continuity_percentage;
    std::string analysis_summary;
};

ContinuityAnalysis analyze_chain_continuity(const Chainer& chainer);

/**
 * Compare different chaining strategies
 */
struct ChainComparison {
    struct StrategyResult {
        std::string strategy_name;
        size_t total_bars;
        size_t gap_count;
        double data_quality_score;
        std::chrono::seconds processing_time;
    };
    
    std::vector<StrategyResult> results;
    std::string recommended_strategy;
};

ChainComparison compare_chaining_strategies(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds,
    const std::vector<std::string>& strategy_names
);

/**
 * Optimize chaining parameters
 */
struct OptimizationResult {
    bool optimal_allow_overlap;
    bool optimal_strict_order;
    bool optimal_auto_fill;
    double optimal_fill_value;
    double quality_score;
    std::string optimization_summary;
};

OptimizationResult optimize_chaining_parameters(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds
);

/**
 * Validate data feed compatibility for chaining
 */
struct CompatibilityCheck {
    bool feeds_compatible;
    std::vector<std::string> compatibility_issues;
    std::vector<std::string> warnings;
    std::string recommendation;
};

CompatibilityCheck check_feed_compatibility(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds
);

/**
 * Performance metrics for chained data
 */
struct ChainPerformance {
    double throughput_bars_per_second;
    std::chrono::milliseconds average_bar_processing_time;
    size_t memory_usage_bytes;
    double cpu_utilization_percentage;
    std::string performance_category;  // "Excellent", "Good", "Fair", "Poor"
};

ChainPerformance measure_chain_performance(const Chainer& chainer);

/**
 * Data quality assessment
 */
struct QualityAssessment {
    double completeness_score;      // 0.0 to 1.0
    double consistency_score;       // 0.0 to 1.0
    double accuracy_score;          // 0.0 to 1.0
    double overall_quality_score;   // 0.0 to 1.0
    std::vector<std::string> quality_issues;
    std::vector<std::string> improvement_suggestions;
};

QualityAssessment assess_chain_quality(const Chainer& chainer);

/**
 * Generate chaining report
 */
struct ChainReport {
    ChainStats statistics;
    ContinuityAnalysis continuity;
    ChainPerformance performance;
    QualityAssessment quality;
    std::vector<TransitionEvent> transitions;
    std::string summary;
    std::string recommendations;
};

ChainReport generate_comprehensive_report(const Chainer& chainer);

/**
 * Time series alignment utilities
 */
bool align_feed_timestamps(std::vector<std::shared_ptr<AbstractDataBase>>& feeds);
std::chrono::system_clock::time_point find_common_start_time(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds
);
std::chrono::system_clock::time_point find_common_end_time(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds
);

/**
 * Data synchronization helpers
 */
void synchronize_feed_environments(std::vector<std::shared_ptr<AbstractDataBase>>& feeds);
void standardize_feed_properties(std::vector<std::shared_ptr<AbstractDataBase>>& feeds);

} // namespace chainer_utils

} // namespace feeds
} // namespace backtrader