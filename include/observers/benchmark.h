#pragma once

#include "../observer.h"
#include "../feed.h"
#include <memory>
#include <string>

namespace backtrader {
namespace observers {

/**
 * BenchMark - Benchmark observer
 * 
 * Tracks the performance of a benchmark asset (typically an index or ETF)
 * alongside the strategy performance for comparison.
 * 
 * The benchmark data is typically provided as a separate data feed.
 */
class BenchMark : public Observer {
public:
    // Parameters structure
    struct Params {
        std::shared_ptr<AbstractDataBase> data;  // Benchmark data feed
        bool timereturn = true;                  // Use time-based returns
        bool fund = false;                       // Fund mode (different calculation)
    };

    // Lines
    enum Lines {
        BENCHMARK = 0
    };

    BenchMark(const Params& params = Params{});
    virtual ~BenchMark() = default;

    // Observer interface
    void next() override;
    void start() override;
    void stop() override;

    // Benchmark-specific methods
    void set_benchmark_data(std::shared_ptr<AbstractDataBase> data);
    double get_benchmark_return() const;
    double get_benchmark_value() const;
    
    // Performance comparison
    double get_relative_performance() const;
    double get_tracking_error() const;
    double get_beta() const;
    double get_alpha() const;

private:
    // Parameters
    Params params_;
    
    // Benchmark tracking
    std::shared_ptr<AbstractDataBase> benchmark_data_;
    double initial_benchmark_value_ = 0.0;
    double current_benchmark_value_ = 0.0;
    double initial_portfolio_value_ = 0.0;
    
    // Return calculations
    std::vector<double> benchmark_returns_;
    std::vector<double> portfolio_returns_;
    
    // Internal methods
    void calculate_benchmark_return();
    void calculate_portfolio_return();
    void update_returns_history();
    
    // Statistical calculations
    double calculate_beta();
    double calculate_alpha();
    double calculate_correlation() const;
    double calculate_tracking_error() const;
    
    // Utility methods
    double get_current_benchmark_price() const;
    double calculate_simple_return(double start_value, double end_value) const;
    double calculate_log_return(double start_value, double end_value) const;
};

} // namespace observers
} // namespace backtrader