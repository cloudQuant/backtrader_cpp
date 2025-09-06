#pragma once

#include "../analyzer.h"
#include "../position.h"
#include <vector>
#include <map>
#include <memory>

namespace backtrader {
namespace analyzers {

/**
 * Positions - Position analyzer
 * 
 * Analyzes position statistics including size, duration, and performance.
 * Tracks all positions opened and closed during the strategy execution.
 */
class Positions : public Analyzer {
public:
    // Position statistics structure
    struct PositionStats {
        std::string symbol;
        double entry_price;
        double exit_price;
        double size;
        double pnl;
        double pnl_pct;
        std::chrono::system_clock::time_point entry_time;
        std::chrono::system_clock::time_point exit_time;
        std::chrono::duration<double> duration;
        bool is_long;
        double max_size;
        double commission;
    };

    Positions();
    virtual ~Positions() = default;

    // Analyzer interface
    void start() override;
    void next() override;
    void stop() override;
    AnalysisResult get_analysis() override;
    
    // Position tracking
    void notify_order(std::shared_ptr<Order> order) override;
    void notify_trade(std::shared_ptr<Trade> trade) override;

    // Position statistics
    int get_total_positions() const;
    int get_long_positions() const;
    int get_short_positions() const;
    int get_winning_positions() const;
    int get_losing_positions() const;
    
    // Performance metrics
    double get_average_win() const;
    double get_average_loss() const;
    double get_win_rate() const;
    double get_profit_factor() const;
    double get_total_pnl() const;
    
    // Position size analysis
    double get_average_position_size() const;
    double get_max_position_size() const;
    double get_min_position_size() const;
    
    // Duration analysis
    std::chrono::duration<double> get_average_duration() const;
    std::chrono::duration<double> get_max_duration() const;
    std::chrono::duration<double> get_min_duration() const;
    
    // Historical data
    const std::vector<PositionStats>& get_all_positions() const;
    std::vector<PositionStats> get_winning_positions_list() const;
    std::vector<PositionStats> get_losing_positions_list() const;

private:
    // Position tracking
    std::vector<PositionStats> all_positions_;
    std::map<std::shared_ptr<DataSeries>, PositionStats> open_positions_;
    
    // Statistics
    int long_positions_ = 0;
    int short_positions_ = 0;
    int winning_positions_ = 0;
    int losing_positions_ = 0;
    double total_wins_ = 0.0;
    double total_losses_ = 0.0;
    double total_pnl_ = 0.0;
    double total_commission_ = 0.0;
    
    // Size tracking
    double sum_position_sizes_ = 0.0;
    double max_position_size_ = 0.0;
    double min_position_size_ = std::numeric_limits<double>::max();
    
    // Duration tracking
    std::chrono::duration<double> sum_durations_{0};
    std::chrono::duration<double> max_duration_{0};
    std::chrono::duration<double> min_duration_{std::numeric_limits<double>::max()};
    
    // Internal methods
    void process_position_entry(std::shared_ptr<DataSeries> data, 
                               std::shared_ptr<Order> order);
    void process_position_exit(std::shared_ptr<DataSeries> data,
                              std::shared_ptr<Order> order);
    void update_position_stats(PositionStats& stats, std::shared_ptr<Order> order);
    void finalize_position(PositionStats& stats);
    
    // Calculation helpers
    double calculate_pnl(const PositionStats& stats) const;
    double calculate_pnl_percentage(const PositionStats& stats) const;
    bool is_winning_position(const PositionStats& stats) const;
    
    // Statistics updates
    void update_win_loss_statistics(const PositionStats& stats);
    void update_size_statistics(double size);
    void update_duration_statistics(const std::chrono::duration<double>& duration);
    
    // Utility methods
    std::string get_symbol_name(std::shared_ptr<DataSeries> data) const;
    bool is_position_open(std::shared_ptr<DataSeries> data) const;
    void close_all_open_positions();
};

} // namespace analyzers
} // namespace backtrader