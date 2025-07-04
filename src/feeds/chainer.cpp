#include "../../include/feeds/chainer.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace backtrader {
namespace feeds {

// Chainer implementation

Chainer::Chainer(const Params& params) : params_(params) {
    // Initialize with default parameters
    chain_started_ = false;
    chain_completed_ = false;
    current_feed_index_ = -1;
    total_bars_delivered_ = 0;
    log_transitions_ = true;
    
    // Set last delivered time to minimum value
    last_delivered_time_ = std::chrono::system_clock::time_point::min();
}

Chainer::Chainer(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds, const Params& params)
    : params_(params), data_feeds_(data_feeds) {
    
    if (data_feeds_.empty()) {
        throw std::invalid_argument("Chainer requires at least one data feed");
    }
    
    // Copy timeframe and compression from first data feed
    if (!data_feeds_.empty()) {
        set_timeframe(data_feeds_[0]->get_timeframe());
        set_compression(data_feeds_[0]->get_compression());
    }
    
    // Initialize state
    chain_started_ = false;
    chain_completed_ = false;
    current_feed_index_ = -1;
    total_bars_delivered_ = 0;
    log_transitions_ = true;
    
    last_delivered_time_ = std::chrono::system_clock::time_point::min();
    
    // Initialize bars per feed tracking
    bars_per_feed_.resize(data_feeds_.size(), 0);
    
    initialize_chain();
}

void Chainer::add_data_feed(std::shared_ptr<AbstractDataBase> data_feed) {
    if (!data_feed) {
        throw std::invalid_argument("Data feed cannot be null");
    }
    
    data_feeds_.push_back(data_feed);
    bars_per_feed_.push_back(0);
    
    // If this is the first data feed, copy its properties
    if (data_feeds_.size() == 1) {
        set_timeframe(data_feed->get_timeframe());
        set_compression(data_feed->get_compression());
    }
    
    // Validate compatibility
    if (!validate_feed_compatibility(data_feed)) {
        std::cerr << "Warning: Added data feed may not be compatible with existing feeds" << std::endl;
    }
}

void Chainer::set_data_feeds(const std::vector<std::shared_ptr<AbstractDataBase>>& data_feeds) {
    if (data_feeds.empty()) {
        throw std::invalid_argument("Chainer requires at least one data feed");
    }
    
    data_feeds_ = data_feeds;
    bars_per_feed_.resize(data_feeds_.size(), 0);
    
    // Copy properties from first data feed
    set_timeframe(data_feeds_[0]->get_timeframe());
    set_compression(data_feeds_[0]->get_compression());
    
    initialize_chain();
}

void Chainer::insert_data_feed(size_t position, std::shared_ptr<AbstractDataBase> data_feed) {
    if (!data_feed) {
        throw std::invalid_argument("Data feed cannot be null");
    }
    
    if (position > data_feeds_.size()) {
        throw std::out_of_range("Position out of range");
    }
    
    data_feeds_.insert(data_feeds_.begin() + position, data_feed);
    bars_per_feed_.insert(bars_per_feed_.begin() + position, 0);
    
    // Re-initialize chain if needed
    if (chain_started_) {
        initialize_chain();
    }
}

void Chainer::remove_data_feed(size_t position) {
    if (position >= data_feeds_.size()) {
        throw std::out_of_range("Position out of range");
    }
    
    data_feeds_.erase(data_feeds_.begin() + position);
    bars_per_feed_.erase(bars_per_feed_.begin() + position);
    
    // Re-initialize chain if needed
    if (chain_started_) {
        initialize_chain();
    }
}

void Chainer::clear_data_feeds() {
    data_feeds_.clear();
    bars_per_feed_.clear();
    current_feed_ = nullptr;
    current_feed_index_ = -1;
    chain_started_ = false;
    chain_completed_ = false;
    total_bars_delivered_ = 0;
    transition_history_.clear();
}

void Chainer::start() {
    AbstractDataBase::start();
    
    if (data_feeds_.empty()) {
        throw std::runtime_error("No data feeds configured for chaining");
    }
    
    // Propagate environment to all feeds
    propagate_environment_to_feeds();
    
    // Start all data feeds
    for (auto& feed : data_feeds_) {
        if (feed) {
            feed->start();
        }
    }
    
    // Initialize chain state
    current_feed_index_ = 0;
    current_feed_ = data_feeds_[0];
    chain_started_ = true;
    chain_completed_ = false;
    
    // Reset statistics
    total_bars_delivered_ = 0;
    std::fill(bars_per_feed_.begin(), bars_per_feed_.end(), 0);
    transition_history_.clear();
    last_delivered_time_ = std::chrono::system_clock::time_point::min();
    
    if (log_transitions_) {
        std::cout << "Chainer started with " << data_feeds_.size() << " data feeds" << std::endl;
        std::cout << "Initial feed: " << get_feed_name(current_feed_) << std::endl;
    }
}

void Chainer::stop() {
    AbstractDataBase::stop();
    
    // Stop all data feeds
    for (auto& feed : data_feeds_) {
        if (feed) {
            feed->stop();
        }
    }
    
    chain_started_ = false;
    chain_completed_ = true;
    
    if (log_transitions_) {
        std::cout << "Chainer stopped. Total bars delivered: " << total_bars_delivered_ << std::endl;
        std::cout << "Total transitions: " << transition_history_.size() << std::endl;
    }
}

bool Chainer::next() {
    if (!chain_started_ || chain_completed_) {
        return false;
    }
    
    return load_next_data_point();
}

void Chainer::preload() {
    // Chainer is designed for live-like data delivery, so preloading is disabled
    // This method intentionally does nothing
}

std::shared_ptr<AbstractDataBase> Chainer::get_feed_at(size_t index) const {
    if (index >= data_feeds_.size()) {
        return nullptr;
    }
    return data_feeds_[index];
}

Chainer::ChainStats Chainer::get_chain_statistics() const {
    ChainStats stats = {};
    
    stats.total_feeds = static_cast<int>(data_feeds_.size());
    stats.completed_feeds = std::max(0, current_feed_index_);
    stats.current_feed_index = current_feed_index_;
    stats.total_bars_delivered = total_bars_delivered_;
    stats.bars_per_feed = bars_per_feed_;
    
    // Collect feed names
    for (auto& feed : data_feeds_) {
        stats.feed_names.push_back(get_feed_name(feed));
    }
    
    // Set timing information
    if (!transition_history_.empty()) {
        stats.chain_start_time = transition_history_.front().timestamp;
        stats.chain_end_time = transition_history_.back().timestamp;
    }
    
    return stats;
}

void Chainer::set_gap_fill_strategy(bool auto_fill, double fill_value) {
    params_.auto_fill_gaps = auto_fill;
    params_.gap_fill_value = fill_value;
}

bool Chainer::validate_chain_continuity() const {
    if (data_feeds_.size() < 2) {
        return true; // Single feed is always continuous
    }
    
    // Check time gaps between feeds
    for (size_t i = 1; i < data_feeds_.size(); ++i) {
        auto prev_feed = data_feeds_[i-1];
        auto curr_feed = data_feeds_[i];
        
        if (!prev_feed || !curr_feed) {
            continue;
        }
        
        // In a full implementation, this would check actual time gaps
        // For now, we assume feeds are properly ordered
    }
    
    return true;
}

std::vector<std::chrono::system_clock::time_point> Chainer::find_time_gaps() const {
    std::vector<std::chrono::system_clock::time_point> gaps;
    
    // Analyze transition history for gaps
    for (size_t i = 1; i < transition_history_.size(); ++i) {
        auto gap_duration = transition_history_[i].timestamp - transition_history_[i-1].timestamp;
        
        // Consider gaps > 1 hour as significant
        if (gap_duration > std::chrono::hours(1)) {
            gaps.push_back(transition_history_[i-1].timestamp);
        }
    }
    
    return gaps;
}

void Chainer::initialize_chain() {
    // Validate feed compatibility
    if (!check_timeframe_consistency()) {
        std::cerr << "Warning: Data feeds have inconsistent timeframes" << std::endl;
    }
    
    // Synchronize feed properties
    synchronize_feed_properties();
    
    // Initialize tracking arrays
    bars_per_feed_.resize(data_feeds_.size(), 0);
}

bool Chainer::advance_to_next_feed() {
    if (current_feed_index_ >= static_cast<int>(data_feeds_.size()) - 1) {
        chain_completed_ = true;
        return false;
    }
    
    // Log transition
    log_transition("End of data feed reached");
    
    // Move to next feed
    current_feed_index_++;
    current_feed_ = data_feeds_[current_feed_index_];
    
    if (log_transitions_) {
        std::cout << "Transitioned to feed " << current_feed_index_ 
                  << ": " << get_feed_name(current_feed_) << std::endl;
    }
    
    return true;
}

bool Chainer::load_next_data_point() {
    while (current_feed_ && current_feed_index_ < static_cast<int>(data_feeds_.size())) {
        // Try to get next data point from current feed
        if (current_feed_->next()) {
            auto current_time = current_feed_->get_datetime();
            
            // Validate time ordering
            if (is_time_valid(current_time)) {
                copy_current_data_to_lines();
                total_bars_delivered_++;
                bars_per_feed_[current_feed_index_]++;
                last_delivered_time_ = current_time;
                return true;
            } else {
                // Handle time overlap
                handle_time_overlap(current_time);
                continue;
            }
        } else {
            // Current feed exhausted, try to advance to next feed
            if (!advance_to_next_feed()) {
                return false;
            }
            continue;
        }
    }
    
    return false;
}

void Chainer::copy_current_data_to_lines() {
    if (!current_feed_) {
        return;
    }
    
    const auto& source_lines = current_feed_->get_lines();
    
    // Ensure we have enough lines
    if (lines_.size() != source_lines.size()) {
        lines_.resize(source_lines.size());
    }
    
    // Copy data from current feed to our lines
    for (size_t i = 0; i < source_lines.size() && i < lines_.size(); ++i) {
        if (!source_lines[i].empty()) {
            lines_[i].push_back(source_lines[i].back());
        }
    }
}

bool Chainer::is_time_valid(std::chrono::system_clock::time_point new_time) {
    if (!params_.strict_time_order) {
        return true;
    }
    
    // Check for backward time movement
    if (new_time <= last_delivered_time_) {
        return false;
    }
    
    return true;
}

void Chainer::handle_time_overlap(std::chrono::system_clock::time_point new_time) {
    if (params_.allow_time_overlap) {
        // Allow the overlapping data point
        return;
    }
    
    // Skip overlapping data points
    if (log_transitions_) {
        std::cout << "Skipping overlapping timestamp in feed " << current_feed_index_ << std::endl;
    }
}

void Chainer::handle_data_gap(std::chrono::system_clock::time_point last_time, 
                              std::chrono::system_clock::time_point new_time) {
    if (!params_.auto_fill_gaps) {
        return;
    }
    
    auto gap_duration = new_time - last_time;
    
    // Only fill gaps larger than the timeframe
    auto timeframe_duration = std::chrono::minutes(1); // Simplified
    if (gap_duration <= timeframe_duration) {
        return;
    }
    
    // Fill the gap with synthetic data
    auto fill_time = last_time + timeframe_duration;
    while (fill_time < new_time) {
        auto fill_bar = create_gap_fill_bar(fill_time);
        
        // Add filled bar to lines
        for (size_t i = 0; i < lines_.size() && i < fill_bar.size(); ++i) {
            lines_[i].push_back(fill_bar[i]);
        }
        
        total_bars_delivered_++;
        fill_time += timeframe_duration;
    }
}

std::vector<double> Chainer::create_gap_fill_bar(std::chrono::system_clock::time_point fill_time) {
    std::vector<double> fill_bar;
    
    // Create a simple gap fill bar with NaN values or last known values
    size_t num_lines = lines_.empty() ? 7 : lines_.size(); // OHLCV + date/time
    
    for (size_t i = 0; i < num_lines; ++i) {
        if (std::isnan(params_.gap_fill_value)) {
            // Use last known value if available
            if (!lines_[i].empty()) {
                fill_bar.push_back(lines_[i].back());
            } else {
                fill_bar.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        } else {
            fill_bar.push_back(params_.gap_fill_value);
        }
    }
    
    return fill_bar;
}

void Chainer::log_transition(const std::string& reason) {
    if (!log_transitions_) {
        return;
    }
    
    TransitionEvent event;
    event.timestamp = std::chrono::system_clock::now();
    event.from_feed_index = current_feed_index_;
    event.to_feed_index = current_feed_index_ + 1;
    event.from_feed_name = get_feed_name(current_feed_);
    event.to_feed_name = (current_feed_index_ + 1 < static_cast<int>(data_feeds_.size())) ? 
                         get_feed_name(data_feeds_[current_feed_index_ + 1]) : "None";
    event.transition_reason = reason;
    event.bars_from_previous_feed = bars_per_feed_[current_feed_index_];
    
    transition_history_.push_back(event);
}

std::string Chainer::get_feed_name(std::shared_ptr<AbstractDataBase> feed) const {
    if (!feed) {
        return "Unknown";
    }
    
    // Try to extract feed name from data feed
    // This is a simplified implementation
    std::ostringstream oss;
    oss << "Feed_" << std::hex << reinterpret_cast<uintptr_t>(feed.get());
    return oss.str();
}

void Chainer::propagate_environment_to_feeds() {
    for (auto& feed : data_feeds_) {
        if (feed) {
            setup_feed_environment(feed);
        }
    }
}

void Chainer::setup_feed_environment(std::shared_ptr<AbstractDataBase> feed) {
    if (!feed) {
        return;
    }
    
    // Copy environment settings from this chainer to the feed
    // This would include timezone, date format, etc.
    // Simplified implementation
}

bool Chainer::validate_feed_compatibility(std::shared_ptr<AbstractDataBase> feed) const {
    if (!feed || data_feeds_.empty()) {
        return true;
    }
    
    // Check timeframe compatibility
    auto reference_timeframe = data_feeds_[0]->get_timeframe();
    auto reference_compression = data_feeds_[0]->get_compression();
    
    if (feed->get_timeframe() != reference_timeframe ||
        feed->get_compression() != reference_compression) {
        return false;
    }
    
    return true;
}

bool Chainer::check_timeframe_consistency() const {
    if (data_feeds_.size() <= 1) {
        return true;
    }
    
    auto reference_timeframe = data_feeds_[0]->get_timeframe();
    auto reference_compression = data_feeds_[0]->get_compression();
    
    for (size_t i = 1; i < data_feeds_.size(); ++i) {
        if (data_feeds_[i]->get_timeframe() != reference_timeframe ||
            data_feeds_[i]->get_compression() != reference_compression) {
            return false;
        }
    }
    
    return true;
}

void Chainer::synchronize_feed_properties() {
    if (data_feeds_.empty()) {
        return;
    }
    
    // Synchronize all feeds to use the same timeframe and compression
    auto reference_timeframe = data_feeds_[0]->get_timeframe();
    auto reference_compression = data_feeds_[0]->get_compression();
    
    for (auto& feed : data_feeds_) {
        if (feed) {
            feed->set_timeframe(reference_timeframe);
            feed->set_compression(reference_compression);
        }
    }
}

// ChainerBuilder implementation

ChainerBuilder::ChainerBuilder() {
    logging_enabled_ = true;
}

ChainerBuilder& ChainerBuilder::add_feed(std::shared_ptr<AbstractDataBase> feed) {
    if (feed) {
        feeds_.push_back(feed);
    }
    return *this;
}

ChainerBuilder& ChainerBuilder::add_feeds(const std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    for (auto& feed : feeds) {
        if (feed) {
            feeds_.push_back(feed);
        }
    }
    return *this;
}

ChainerBuilder& ChainerBuilder::allow_time_overlap(bool allow) {
    params_.allow_time_overlap = allow;
    return *this;
}

ChainerBuilder& ChainerBuilder::strict_time_order(bool strict) {
    params_.strict_time_order = strict;
    return *this;
}

ChainerBuilder& ChainerBuilder::auto_fill_gaps(bool fill, double fill_value) {
    params_.auto_fill_gaps = fill;
    params_.gap_fill_value = fill_value;
    return *this;
}

ChainerBuilder& ChainerBuilder::enable_logging(bool enable) {
    logging_enabled_ = enable;
    return *this;
}

std::shared_ptr<Chainer> ChainerBuilder::build() {
    if (feeds_.empty()) {
        throw std::runtime_error("ChainerBuilder requires at least one data feed");
    }
    
    auto chainer = std::make_shared<Chainer>(feeds_, params_);
    chainer->enable_transition_logging(logging_enabled_);
    
    return chainer;
}

// Chaining strategies implementation

namespace chaining_strategies {

std::shared_ptr<Chainer> create_sequential_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = false;
    params.auto_fill_gaps = false;
    
    return std::make_shared<Chainer>(feeds, params);
}

std::shared_ptr<Chainer> create_time_merged_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds,
    bool allow_overlap) {
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = allow_overlap;
    params.auto_fill_gaps = false;
    
    return std::make_shared<Chainer>(feeds, params);
}

std::shared_ptr<Chainer> create_gap_filled_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds,
    double fill_value) {
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = false;
    params.auto_fill_gaps = true;
    params.gap_fill_value = fill_value;
    
    return std::make_shared<Chainer>(feeds, params);
}

std::shared_ptr<Chainer> create_failover_chain(
    std::shared_ptr<AbstractDataBase> primary_feed,
    const std::vector<std::shared_ptr<AbstractDataBase>>& backup_feeds) {
    
    std::vector<std::shared_ptr<AbstractDataBase>> all_feeds;
    all_feeds.push_back(primary_feed);
    all_feeds.insert(all_feeds.end(), backup_feeds.begin(), backup_feeds.end());
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = true;
    params.auto_fill_gaps = false;
    
    return std::make_shared<Chainer>(all_feeds, params);
}

} // namespace chaining_strategies

// Factory functions implementation

namespace chainer_factory {

std::shared_ptr<Chainer> create_historical_to_live_chain(
    std::shared_ptr<AbstractDataBase> historical_feed,
    std::shared_ptr<AbstractDataBase> live_feed) {
    
    std::vector<std::shared_ptr<AbstractDataBase>> feeds = {historical_feed, live_feed};
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = false;
    params.auto_fill_gaps = true;
    
    return std::make_shared<Chainer>(feeds, params);
}

std::shared_ptr<Chainer> create_multi_file_chain(
    const std::vector<std::string>& file_paths,
    const std::string& file_format) {
    
    std::vector<std::shared_ptr<AbstractDataBase>> feeds;
    
    // This would create data feeds from file paths
    // Simplified implementation - in practice, you'd create appropriate feed types
    for (const auto& path : file_paths) {
        // auto feed = create_feed_from_file(path, file_format);
        // feeds.push_back(feed);
    }
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = false;
    params.auto_fill_gaps = false;
    
    return std::make_shared<Chainer>(feeds, params);
}

std::shared_ptr<Chainer> create_contract_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& contracts) {
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = false;
    params.auto_fill_gaps = true;
    
    return std::make_shared<Chainer>(contracts, params);
}

std::shared_ptr<Chainer> create_redundant_source_chain(
    const std::vector<std::shared_ptr<AbstractDataBase>>& sources) {
    
    Chainer::Params params;
    params.strict_time_order = true;
    params.allow_time_overlap = true;
    params.auto_fill_gaps = false;
    
    return std::make_shared<Chainer>(sources, params);
}

} // namespace chainer_factory

// Utility functions implementation

namespace chainer_utils {

ContinuityAnalysis analyze_chain_continuity(const Chainer& chainer) {
    ContinuityAnalysis analysis = {};
    
    auto transitions = chainer.get_transition_history();
    
    if (transitions.size() < 2) {
        analysis.is_continuous = true;
        analysis.continuity_percentage = 100.0;
        analysis.analysis_summary = "Insufficient transition history for analysis";
        return analysis;
    }
    
    // Calculate gaps between transitions
    for (size_t i = 1; i < transitions.size(); ++i) {
        auto gap = transitions[i].timestamp - transitions[i-1].timestamp;
        auto gap_seconds = std::chrono::duration_cast<std::chrono::seconds>(gap);
        
        analysis.gaps.push_back({transitions[i-1].timestamp, gap_seconds});
        analysis.total_gap_time += gap_seconds;
        
        if (gap_seconds > analysis.largest_gap) {
            analysis.largest_gap = gap_seconds;
        }
    }
    
    analysis.total_gaps = analysis.gaps.size();
    analysis.is_continuous = analysis.largest_gap < std::chrono::hours(1);
    
    // Calculate continuity percentage
    if (!transitions.empty()) {
        auto total_time = transitions.back().timestamp - transitions.front().timestamp;
        auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(total_time);
        
        if (total_seconds.count() > 0) {
            analysis.continuity_percentage = 100.0 * (1.0 - static_cast<double>(analysis.total_gap_time.count()) / total_seconds.count());
        }
    }
    
    if (analysis.is_continuous) {
        analysis.analysis_summary = "Data chain appears continuous";
    } else {
        analysis.analysis_summary = "Data chain has significant gaps";
    }
    
    return analysis;
}

ChainComparison compare_chaining_strategies(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds,
    const std::vector<std::string>& strategy_names) {
    
    ChainComparison comparison;
    
    for (const auto& strategy_name : strategy_names) {
        ChainComparison::StrategyResult result;
        result.strategy_name = strategy_name;
        
        // Create strategy-specific chainer
        std::shared_ptr<Chainer> chainer;
        
        if (strategy_name == "sequential") {
            chainer = chaining_strategies::create_sequential_chain(feeds);
        } else if (strategy_name == "time_merged") {
            chainer = chaining_strategies::create_time_merged_chain(feeds, false);
        } else if (strategy_name == "gap_filled") {
            chainer = chaining_strategies::create_gap_filled_chain(feeds, 0.0);
        } else {
            chainer = chaining_strategies::create_sequential_chain(feeds);
        }
        
        // Simulate and measure performance
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Simplified simulation - in practice, you'd run the strategy
        result.total_bars = 1000; // Placeholder
        result.gap_count = 5; // Placeholder
        result.data_quality_score = 0.85; // Placeholder
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.processing_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        comparison.results.push_back(result);
    }
    
    // Find recommended strategy
    if (!comparison.results.empty()) {
        auto best_it = std::max_element(comparison.results.begin(), comparison.results.end(),
            [](const auto& a, const auto& b) { return a.data_quality_score < b.data_quality_score; });
        comparison.recommended_strategy = best_it->strategy_name;
    }
    
    return comparison;
}

OptimizationResult optimize_chaining_parameters(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    
    OptimizationResult result = {};
    
    // Test different parameter combinations
    std::vector<bool> overlap_options = {false, true};
    std::vector<bool> strict_order_options = {true, false};
    std::vector<bool> auto_fill_options = {false, true};
    
    double best_score = -1.0;
    
    for (bool allow_overlap : overlap_options) {
        for (bool strict_order : strict_order_options) {
            for (bool auto_fill : auto_fill_options) {
                Chainer::Params params;
                params.allow_time_overlap = allow_overlap;
                params.strict_time_order = strict_order;
                params.auto_fill_gaps = auto_fill;
                
                // Create test chainer
                auto chainer = std::make_shared<Chainer>(feeds, params);
                
                // Calculate quality score (simplified)
                double score = 0.5;
                if (strict_order) score += 0.2;
                if (!allow_overlap) score += 0.2;
                if (auto_fill) score += 0.1;
                
                if (score > best_score) {
                    best_score = score;
                    result.optimal_allow_overlap = allow_overlap;
                    result.optimal_strict_order = strict_order;
                    result.optimal_auto_fill = auto_fill;
                    result.optimal_fill_value = 0.0;
                }
            }
        }
    }
    
    result.quality_score = best_score;
    result.optimization_summary = "Optimal parameters found through grid search";
    
    return result;
}

CompatibilityCheck check_feed_compatibility(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    
    CompatibilityCheck check = {};
    check.feeds_compatible = true;
    
    if (feeds.size() < 2) {
        check.recommendation = "Single feed is always compatible";
        return check;
    }
    
    // Check timeframe consistency
    auto reference_timeframe = feeds[0]->get_timeframe();
    auto reference_compression = feeds[0]->get_compression();
    
    for (size_t i = 1; i < feeds.size(); ++i) {
        if (feeds[i]->get_timeframe() != reference_timeframe) {
            check.feeds_compatible = false;
            check.compatibility_issues.push_back("Inconsistent timeframes detected");
        }
        
        if (feeds[i]->get_compression() != reference_compression) {
            check.feeds_compatible = false;
            check.compatibility_issues.push_back("Inconsistent compression detected");
        }
    }
    
    if (check.feeds_compatible) {
        check.recommendation = "All feeds are compatible for chaining";
    } else {
        check.recommendation = "Consider standardizing timeframes and compression before chaining";
    }
    
    return check;
}

ChainPerformance measure_chain_performance(const Chainer& chainer) {
    ChainPerformance performance = {};
    
    auto stats = chainer.get_chain_statistics();
    
    // Calculate throughput (simplified)
    performance.throughput_bars_per_second = 100.0; // Placeholder
    performance.average_bar_processing_time = std::chrono::milliseconds(10);
    performance.memory_usage_bytes = stats.total_bars_delivered * 64; // Rough estimate
    performance.cpu_utilization_percentage = 15.0; // Placeholder
    
    // Categorize performance
    if (performance.throughput_bars_per_second > 1000) {
        performance.performance_category = "Excellent";
    } else if (performance.throughput_bars_per_second > 500) {
        performance.performance_category = "Good";
    } else if (performance.throughput_bars_per_second > 100) {
        performance.performance_category = "Fair";
    } else {
        performance.performance_category = "Poor";
    }
    
    return performance;
}

QualityAssessment assess_chain_quality(const Chainer& chainer) {
    QualityAssessment assessment = {};
    
    auto continuity = analyze_chain_continuity(chainer);
    
    assessment.completeness_score = continuity.continuity_percentage / 100.0;
    assessment.consistency_score = 0.9; // Placeholder
    assessment.accuracy_score = 0.95; // Placeholder
    
    assessment.overall_quality_score = (assessment.completeness_score + 
                                       assessment.consistency_score + 
                                       assessment.accuracy_score) / 3.0;
    
    if (assessment.overall_quality_score > 0.9) {
        assessment.improvement_suggestions.push_back("Data quality is excellent");
    } else if (assessment.overall_quality_score > 0.7) {
        assessment.improvement_suggestions.push_back("Consider gap filling strategies");
    } else {
        assessment.quality_issues.push_back("Significant data quality issues detected");
        assessment.improvement_suggestions.push_back("Review data sources and chaining parameters");
    }
    
    return assessment;
}

ChainReport generate_comprehensive_report(const Chainer& chainer) {
    ChainReport report = {};
    
    report.statistics = chainer.get_chain_statistics();
    report.continuity = analyze_chain_continuity(chainer);
    report.performance = measure_chain_performance(chainer);
    report.quality = assess_chain_quality(chainer);
    report.transitions = chainer.get_transition_history();
    
    // Generate summary
    std::ostringstream summary;
    summary << "Chain Report Summary:\n";
    summary << "- Total feeds: " << report.statistics.total_feeds << "\n";
    summary << "- Bars delivered: " << report.statistics.total_bars_delivered << "\n";
    summary << "- Continuity: " << std::fixed << std::setprecision(1) << report.continuity.continuity_percentage << "%\n";
    summary << "- Quality score: " << std::fixed << std::setprecision(2) << report.quality.overall_quality_score << "\n";
    summary << "- Performance: " << report.performance.performance_category;
    
    report.summary = summary.str();
    
    // Generate recommendations
    if (report.quality.overall_quality_score > 0.9) {
        report.recommendations = "Chain is performing well. No immediate actions needed.";
    } else {
        report.recommendations = "Consider optimizing parameters or improving data sources.";
    }
    
    return report;
}

bool align_feed_timestamps(std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    if (feeds.size() < 2) {
        return true;
    }
    
    // Find common time range
    auto start_time = find_common_start_time(feeds);
    auto end_time = find_common_end_time(feeds);
    
    if (start_time >= end_time) {
        return false;
    }
    
    // Align all feeds to this time range
    for (auto& feed : feeds) {
        if (feed) {
            // In practice, you'd adjust the feed's time range
            // This is a simplified placeholder
        }
    }
    
    return true;
}

std::chrono::system_clock::time_point find_common_start_time(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    
    if (feeds.empty()) {
        return std::chrono::system_clock::now();
    }
    
    auto latest_start = std::chrono::system_clock::time_point::min();
    
    for (const auto& feed : feeds) {
        if (feed) {
            auto feed_start = feed->get_datetime();
            if (feed_start > latest_start) {
                latest_start = feed_start;
            }
        }
    }
    
    return latest_start;
}

std::chrono::system_clock::time_point find_common_end_time(
    const std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    
    if (feeds.empty()) {
        return std::chrono::system_clock::now();
    }
    
    auto earliest_end = std::chrono::system_clock::time_point::max();
    
    for (const auto& feed : feeds) {
        if (feed) {
            // In practice, you'd get the feed's end time
            // This is a simplified placeholder
            auto feed_end = std::chrono::system_clock::now();
            if (feed_end < earliest_end) {
                earliest_end = feed_end;
            }
        }
    }
    
    return earliest_end;
}

void synchronize_feed_environments(std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    if (feeds.empty()) {
        return;
    }
    
    // Use first feed as reference
    auto reference_feed = feeds[0];
    
    for (size_t i = 1; i < feeds.size(); ++i) {
        if (feeds[i] && reference_feed) {
            // Copy environment settings
            feeds[i]->set_timeframe(reference_feed->get_timeframe());
            feeds[i]->set_compression(reference_feed->get_compression());
        }
    }
}

void standardize_feed_properties(std::vector<std::shared_ptr<AbstractDataBase>>& feeds) {
    if (feeds.empty()) {
        return;
    }
    
    // Standardize to most common properties
    auto reference_timeframe = feeds[0]->get_timeframe();
    auto reference_compression = feeds[0]->get_compression();
    
    for (auto& feed : feeds) {
        if (feed) {
            feed->set_timeframe(reference_timeframe);
            feed->set_compression(reference_compression);
        }
    }
}

} // namespace chainer_utils

} // namespace feeds
} // namespace backtrader