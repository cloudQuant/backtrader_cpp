#pragma once

#include "../../indicator.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace indicators {
namespace contrib {

/**
 * Vortex Indicator - Technical Analysis Indicator
 * 
 * The Vortex Indicator (VI) is an oscillator that measures the relationship
 * between closing prices and true range to identify the start of trends.
 * It consists of two oscillators that capture positive and negative trend movement.
 * 
 * Developed by Etienne Botes and Douglas Siepman, presented in 
 * "The Vortex Indicator" article in Technical Analysis of Stocks & Commodities magazine.
 * 
 * Reference: http://www.vortexindicator.com/VFX_VORTEX.PDF
 * 
 * Formula:
 * VM+ = Sum of |High[i] - Low[i-1]| over period
 * VM- = Sum of |Low[i] - High[i-1]| over period
 * TR = Sum of True Range over period
 * VI+ = VM+ / TR
 * VI- = VM- / TR
 * 
 * Where True Range = Max(|High-Low|, |High-PrevClose|, |Low-PrevClose|)
 * 
 * Usage:
 * - When VI+ crosses above VI-, it suggests an uptrend
 * - When VI- crosses above VI+, it suggests a downtrend
 * - Values typically range around 1.0
 */
class Vortex : public Indicator {
public:
    // Parameters structure
    struct Params : public Indicator::Params {
        int period = 14;  // Period for calculation (default: 14)
    };
    
    Vortex(const Params& params = Params{});
    virtual ~Vortex() = default;
    
    // Alternative constructor with period
    explicit Vortex(int period);
    
    // Indicator interface
    void start() override;
    void stop() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Line definitions
    enum Lines {
        VI_PLUS = 0,   // Positive Vortex Indicator (+VI)
        VI_MINUS = 1   // Negative Vortex Indicator (-VI)
    };
    
    // Accessors for indicator values
    double get_vi_plus(int ago = 0) const;
    double get_vi_minus(int ago = 0) const;
    
    // Get both values as a pair
    std::pair<double, double> get_vi_values(int ago = 0) const;
    
    // Analysis methods
    bool is_uptrend_signal(int ago = 0) const;      // VI+ > VI-
    bool is_downtrend_signal(int ago = 0) const;    // VI- > VI+
    bool is_uptrend_crossover(int ago = 0) const;   // VI+ crosses above VI-
    bool is_downtrend_crossover(int ago = 0) const; // VI- crosses above VI+
    
    // Historical analysis
    std::vector<double> get_vi_plus_history(int count) const;
    std::vector<double> get_vi_minus_history(int count) const;
    
    // Trend strength analysis
    double get_trend_strength(int ago = 0) const;    // |VI+ - VI-|
    double get_trend_direction(int ago = 0) const;   // Sign of (VI+ - VI-)
    
    // Divergence analysis
    struct DivergencePoint {
        int index;
        double vi_plus;
        double vi_minus;
        double price;
        bool is_bullish_divergence;
    };
    
    std::vector<DivergencePoint> find_divergences(const std::vector<double>& prices, int lookback = 20) const;
    
    // Configuration
    void set_period(int period);
    int get_period() const { return params_.period; }
    
    // Statistics
    struct VortexStats {
        double avg_vi_plus;
        double avg_vi_minus;
        double max_vi_plus;
        double max_vi_minus;
        double min_vi_plus;
        double min_vi_minus;
        int bullish_signals;
        int bearish_signals;
        int total_crossovers;
    };
    
    VortexStats calculate_statistics(int lookback_period = 50) const;
    
protected:
    Params params_;
    
    // Calculation state
    std::vector<double> vm_plus_values_;    // Vortex Movement Plus values
    std::vector<double> vm_minus_values_;   // Vortex Movement Minus values
    std::vector<double> true_range_values_; // True Range values
    
    // Previous values for calculation
    double previous_high_ = std::numeric_limits<double>::quiet_NaN();
    double previous_low_ = std::numeric_limits<double>::quiet_NaN();
    double previous_close_ = std::numeric_limits<double>::quiet_NaN();
    
    bool has_previous_data_ = false;
    
private:
    void initialize_lines();
    void setup_plot_info();
    void calculate_vortex_values();
    void update_calculation_data(double high, double low, double close);
    
    // Vortex calculation components
    double calculate_vm_plus(double current_high, double previous_low);
    double calculate_vm_minus(double current_low, double previous_high);
    double calculate_true_range(double high, double low, double close, double prev_close);
    
    // Sum calculations over period
    double sum_over_period(const std::vector<double>& values) const;
    
    // Helper methods
    void add_to_vector(std::vector<double>& vec, double value);
    void maintain_vector_size(std::vector<double>& vec);
    bool has_enough_data() const;
    
    // Crossover detection
    bool detect_crossover(double current_a, double current_b, double prev_a, double prev_b) const;
};

/**
 * Factory functions for creating Vortex indicators
 */
namespace vortex_factory {

/**
 * Create Vortex indicator with default parameters
 */
std::shared_ptr<Vortex> create_vortex(int period = 14);

/**
 * Create Vortex indicator optimized for short-term trading
 */
std::shared_ptr<Vortex> create_short_term_vortex(int period = 7);

/**
 * Create Vortex indicator optimized for long-term analysis
 */
std::shared_ptr<Vortex> create_long_term_vortex(int period = 21);

} // namespace vortex_factory

/**
 * Utility functions for Vortex analysis
 */
namespace vortex_utils {

/**
 * Calculate Vortex Movement Plus for a single period
 */
double calculate_single_vm_plus(double current_high, double previous_low);

/**
 * Calculate Vortex Movement Minus for a single period
 */
double calculate_single_vm_minus(double current_low, double previous_high);

/**
 * Calculate True Range for a single period
 */
double calculate_single_true_range(double high, double low, double close, double prev_close);

/**
 * Determine trend strength category
 */
enum class TrendStrength {
    VERY_WEAK,
    WEAK,
    MODERATE,
    STRONG,
    VERY_STRONG
};

TrendStrength categorize_trend_strength(double vi_plus, double vi_minus);

/**
 * Calculate trend momentum based on VI difference rate of change
 */
double calculate_trend_momentum(const std::vector<double>& vi_plus_history,
                               const std::vector<double>& vi_minus_history,
                               int lookback = 3);

/**
 * Find optimal Vortex period for given data
 */
struct OptimizationResult {
    int optimal_period;
    double best_score;
    std::vector<int> tested_periods;
    std::vector<double> scores;
};

OptimizationResult optimize_vortex_period(const std::vector<double>& highs,
                                         const std::vector<double>& lows,
                                         const std::vector<double>& closes,
                                         int min_period = 5,
                                         int max_period = 30);

/**
 * Validate Vortex signals against actual price movements
 */
struct SignalValidation {
    int total_signals;
    int correct_signals;
    double accuracy_rate;
    double average_profit;
    double max_profit;
    double max_loss;
};

SignalValidation validate_vortex_signals(const std::vector<double>& vi_plus,
                                        const std::vector<double>& vi_minus,
                                        const std::vector<double>& prices,
                                        int hold_period = 5);

/**
 * Combine Vortex with other indicators for enhanced signals
 */
struct CombinedSignal {
    bool is_bullish;
    bool is_bearish;
    double confidence_level;  // 0.0 to 1.0
    std::string signal_sources;
};

CombinedSignal combine_with_trend_indicator(double vi_plus, double vi_minus,
                                           double trend_value, double threshold = 0.0);

CombinedSignal combine_with_momentum_indicator(double vi_plus, double vi_minus,
                                              double momentum_value, double threshold = 0.0);

/**
 * Generate trading signals based on Vortex crossovers
 */
enum class SignalType {
    BUY,
    SELL,
    HOLD
};

struct TradingSignal {
    SignalType type;
    double confidence;
    std::string reason;
    double stop_loss_level;
    double take_profit_level;
};

TradingSignal generate_trading_signal(double vi_plus, double vi_minus,
                                     double prev_vi_plus, double prev_vi_minus,
                                     double current_price,
                                     double atr_value = 0.0);  // For stop loss calculation

/**
 * Market regime detection using Vortex
 */
enum class MarketRegime {
    TRENDING_UP,
    TRENDING_DOWN,
    RANGING,
    VOLATILE
};

MarketRegime detect_market_regime(const std::vector<double>& vi_plus_history,
                                 const std::vector<double>& vi_minus_history,
                                 int analysis_period = 20);

} // namespace vortex_utils

} // namespace contrib
} // namespace indicators
} // namespace backtrader