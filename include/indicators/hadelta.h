#pragma once

#include "../indicator.h"
#include "mabase.h"
#include <memory>
#include <string>
#include <vector>

namespace backtrader {
namespace indicators {

// Forward declaration
class HeikinAshi;

/**
 * HeikinAshi - Heikin-Ashi Candlestick Transformation
 * 
 * Transforms regular OHLC data into Heikin-Ashi format for smoother price action.
 * Heikin-Ashi uses modified OHLC values that reduce noise and make trends clearer.
 * 
 * Formula:
 * - HA_Close = (Open + High + Low + Close) / 4
 * - HA_Open = (Previous HA_Open + Previous HA_Close) / 2
 * - HA_High = max(High, HA_Open, HA_Close)
 * - HA_Low = min(Low, HA_Open, HA_Close)
 */
class HeikinAshi : public Indicator {
public:
    // Parameters structure
    struct Params : public Indicator::Params {
        // No additional parameters for basic Heikin-Ashi
    };
    
    HeikinAshi(const Params& params = Params{});
    virtual ~HeikinAshi() = default;
    
    // Indicator interface
    void start() override;
    void stop() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Line definitions - matches OHLC structure
    enum Lines {
        OPEN = 0,      // Heikin-Ashi Open
        HIGH = 1,      // Heikin-Ashi High
        LOW = 2,       // Heikin-Ashi Low
        CLOSE = 3      // Heikin-Ashi Close
    };
    
    // Accessors for Heikin-Ashi values
    double get_ha_open(int ago = 0) const;
    double get_ha_high(int ago = 0) const;
    double get_ha_low(int ago = 0) const;
    double get_ha_close(int ago = 0) const;
    
    // Get all OHLC as a vector
    std::vector<double> get_ha_ohlc(int ago = 0) const;
    
protected:
    Params params_;
    
    // State for calculation
    double previous_ha_open_ = std::numeric_limits<double>::quiet_NaN();
    double previous_ha_close_ = std::numeric_limits<double>::quiet_NaN();
    bool first_bar_ = true;
    
private:
    void initialize_lines();
    void calculate_heikin_ashi_values();
    void seed_first_bar(double open, double close);
    
    // Heikin-Ashi calculation methods
    double calculate_ha_close(double open, double high, double low, double close);
    double calculate_ha_open(double prev_ha_open, double prev_ha_close);
    double calculate_ha_high(double high, double ha_open, double ha_close);
    double calculate_ha_low(double low, double ha_open, double ha_close);
};

/**
 * HaDelta - Heikin-Ashi Delta Indicator
 * 
 * Calculates the difference between Heikin-Ashi close and open prices.
 * This represents the "body" of the Heikin-Ashi candle and provides
 * insight into momentum direction and strength.
 * 
 * Based on Dan Valcu's "Heikin-Ashi: How to Trade Without Candlestick Patterns"
 * 
 * Lines:
 * - haDelta: Raw delta (HA_Close - HA_Open)
 * - smoothed: Moving average smoothed delta
 */
class HaDelta : public Indicator {
public:
    // Type alias for moving average type
    using MovingAverageType = std::shared_ptr<MovingAverageBase>;
    
    // Parameters structure
    struct Params : public Indicator::Params {
        int period = 3;                           // Period for smoothing moving average
        MovingAverageType movav = nullptr;        // Moving average type (default: SMA)
        std::string movav_type = "SMA";          // Moving average type as string
        bool autoheikin = true;                  // Auto-apply Heikin-Ashi transformation
    };
    
    HaDelta(const Params& params = Params{});
    virtual ~HaDelta() = default;
    
    // Alternative constructors
    HaDelta(int period, const std::string& ma_type = "SMA", bool auto_heikin = true);
    HaDelta(int period, MovingAverageType ma, bool auto_heikin = true);
    
    // Indicator interface
    void start() override;
    void stop() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Line definitions
    enum Lines {
        HADELTA = 0,   // Raw Heikin-Ashi delta (close - open)
        SMOOTHED = 1   // Smoothed delta using moving average
    };
    
    // Accessors
    double get_ha_delta(int ago = 0) const;
    double get_smoothed_delta(int ago = 0) const;
    
    // Analysis methods
    bool is_bullish(int ago = 0) const;          // Delta > 0
    bool is_bearish(int ago = 0) const;          // Delta < 0
    bool is_smoothed_bullish(int ago = 0) const; // Smoothed delta > 0
    bool is_smoothed_bearish(int ago = 0) const; // Smoothed delta < 0
    
    // Momentum analysis
    double get_momentum_strength(int ago = 0) const;      // Absolute value of delta
    double get_momentum_direction(int ago = 0) const;     // Sign of delta (-1, 0, 1)
    
    // Historical analysis
    std::vector<double> get_delta_history(int count) const;
    std::vector<double> get_smoothed_history(int count) const;
    double get_average_delta(int period) const;
    
    // Configuration
    void set_moving_average_type(const std::string& ma_type);
    void set_moving_average_type(MovingAverageType ma);
    void set_period(int period);
    void enable_auto_heikin(bool enable = true);
    
    // Properties
    int get_period() const { return params_.period; }
    bool is_auto_heikin_enabled() const { return params_.autoheikin; }
    std::string get_ma_type() const { return params_.movav_type; }
    
protected:
    Params params_;
    
    // Internal indicators
    std::shared_ptr<indicators::HeikinAshi> heikin_ashi_;     // Heikin-Ashi transformation
    std::shared_ptr<MovingAverageBase> smoothing_ma_;  // Moving average for smoothing
    
    // Data source (either raw data or Heikin-Ashi transformed)
    std::shared_ptr<DataSeries> effective_data_;
    
private:
    void initialize_lines();
    void setup_plot_info();
    void initialize_indicators();
    void create_moving_average();
    void calculate_delta();
    void update_smoothed_delta();
    
    // Helper methods
    double calculate_raw_delta();
    MovingAverageType create_ma_from_string(const std::string& ma_type);
    void validate_parameters();
};

/**
 * HaD - Alias for HaDelta
 * 
 * Provides a shorter name for the Heikin-Ashi Delta indicator
 */
using HaD = HaDelta;

/**
 * Factory functions for creating Heikin-Ashi related indicators
 */
namespace heikin_ashi_factory {

/**
 * Create basic Heikin-Ashi transformation
 */
std::shared_ptr<indicators::HeikinAshi> create_heikin_ashi();

/**
 * Create HaDelta with default parameters
 */
std::shared_ptr<HaDelta> create_ha_delta(int period = 3, const std::string& ma_type = "SMA");

/**
 * Create HaDelta with custom moving average
 */
std::shared_ptr<HaDelta> create_ha_delta_custom_ma(int period, std::shared_ptr<MovingAverageBase> ma);

/**
 * Create HaDelta without auto Heikin-Ashi (for pre-transformed data)
 */
std::shared_ptr<HaDelta> create_ha_delta_no_transform(int period = 3, const std::string& ma_type = "SMA");

} // namespace heikin_ashi_factory

/**
 * Utility functions for Heikin-Ashi analysis
 */
namespace heikin_ashi_utils {

/**
 * Check if a Heikin-Ashi candle is bullish (close > open)
 */
bool is_ha_candle_bullish(double ha_open, double ha_close);

/**
 * Check if a Heikin-Ashi candle is bearish (close < open)
 */
bool is_ha_candle_bearish(double ha_open, double ha_close);

/**
 * Check if a Heikin-Ashi candle is doji (close ≈ open)
 */
bool is_ha_candle_doji(double ha_open, double ha_close, double tolerance = 1e-6);

/**
 * Calculate the body size of a Heikin-Ashi candle
 */
double calculate_ha_body_size(double ha_open, double ha_close);

/**
 * Calculate the body percentage relative to the range
 */
double calculate_ha_body_percentage(double ha_open, double ha_high, double ha_low, double ha_close);

/**
 * Determine trend direction based on consecutive deltas
 */
enum class TrendDirection {
    BULLISH,
    BEARISH,
    SIDEWAYS
};

TrendDirection analyze_delta_trend(const std::vector<double>& deltas, int lookback = 5);

/**
 * Calculate delta momentum (rate of change in delta)
 */
double calculate_delta_momentum(const std::vector<double>& deltas);

/**
 * Find delta divergence points
 */
struct DivergencePoint {
    int index;
    double delta_value;
    double price_value;
    bool is_bullish_divergence;
};

std::vector<DivergencePoint> find_delta_divergences(
    const std::vector<double>& deltas,
    const std::vector<double>& prices,
    int lookback = 10
);

} // namespace heikin_ashi_utils

} // namespace indicators
} // namespace backtrader