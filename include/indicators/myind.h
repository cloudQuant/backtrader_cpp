#pragma once

#include "../indicator.h"
#include "mabase.h"
#include <memory>
#include <functional>
#include <vector>
#include <cmath>

namespace backtrader {
namespace indicators {

/**
 * MaBetweenHighAndLow - 判断均线是否在最高价和最低价之间
 * 
 * This indicator determines if a moving average is between the high and low prices.
 * It returns true when the SMA of the close price is between the current bar's
 * high and low prices.
 * 
 * 判断均线是否在最高价和最低价之间的指标
 */
class MaBetweenHighAndLow : public Indicator {
public:
    // Parameters structure
    struct Params : public Indicator::Params {
        int period = 5;  // Period for moving average calculation
    };
    
    MaBetweenHighAndLow(const Params& params = Params{});
    virtual ~MaBetweenHighAndLow() = default;
    
    // Indicator interface
    void start() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Line definitions
    enum Lines {
        TARGET = 0  // Boolean result: 1 if MA between high/low, 0 otherwise
    };
    
    // Accessors
    double get_target(int ago = 0) const;
    bool is_ma_between_high_low(int ago = 0) const;
    
protected:
    Params params_;
    std::shared_ptr<SimpleMovingAverage> sma_;  // Internal SMA indicator
    
private:
    void initialize_lines();
    void calculate_current_value();
    bool check_ma_between_high_low(double ma_value, double high, double low) const;
};

/**
 * BarsLast - 这个指标用于分析最近一次满足条件之后到现在的bar的个数
 * 
 * This indicator counts the number of bars since the last time a condition was met.
 * It uses another indicator as the condition function and resets the counter to 0
 * when the condition becomes true.
 * 
 * 分析最近一次满足条件之后到现在的bar的个数
 */
class BarsLast : public Indicator {
public:
    // Type alias for condition function
    using ConditionFunc = std::function<bool()>;
    using ConditionIndicator = std::shared_ptr<Indicator>;
    
    // Parameters structure
    struct Params : public Indicator::Params {
        int period = 5;                           // Period parameter (passed to condition)
        ConditionIndicator func = nullptr;        // Condition indicator to monitor
        ConditionFunc custom_func = nullptr;      // Custom condition function
    };
    
    BarsLast(const Params& params = Params{});
    virtual ~BarsLast() = default;
    
    // Alternative constructors
    BarsLast(ConditionIndicator condition_indicator, int period = 5);
    BarsLast(ConditionFunc condition_func, int period = 5);
    
    // Indicator interface
    void start() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Line definitions
    enum Lines {
        BAR_NUM = 0  // Number of bars since condition was last true
    };
    
    // Accessors
    double get_bar_num(int ago = 0) const;
    int get_bars_since_condition(int ago = 0) const;
    
    // Condition management
    void set_condition_indicator(ConditionIndicator indicator);
    void set_condition_function(ConditionFunc func);
    
protected:
    Params params_;
    int bar_counter_ = 0;                    // Current bar counter
    bool condition_met_last_bar_ = false;    // Track if condition was met last bar
    
private:
    void initialize_lines();
    void update_counter();
    bool evaluate_condition();
    void reset_counter();
};

/**
 * NewDiff - 根据国泰君安alpha因子编写的指标
 * 
 * This indicator implements a custom alpha factor from Guotai Junan Securities.
 * Formula: SUM((CLOSE=DELAY(CLOSE,1)?0:CLOSE-(CLOSE>DELAY(CLOSE,1)?MIN(LOW,DELAY(CLOSE,1)):MAX(HIGH,DELAY(CLOSE,1)))),6)
 * 
 * 基于国泰君安证券alpha因子的自定义指标
 */
class NewDiff : public Indicator {
public:
    // Parameters structure
    struct Params : public Indicator::Params {
        int period = 5;  // Period for summation (note: formula uses 6 in original)
    };
    
    NewDiff(const Params& params = Params{});
    virtual ~NewDiff() = default;
    
    // Indicator interface
    void start() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Line definitions
    enum Lines {
        FACTOR = 0  // The calculated alpha factor value
    };
    
    // Accessors
    double get_factor(int ago = 0) const;
    double get_alpha_factor(int ago = 0) const;
    
    // Analysis methods
    std::vector<double> get_factor_history(int count) const;
    double get_average_factor(int period) const;
    
protected:
    Params params_;
    std::vector<double> daily_values_;  // Store daily calculation values for summation
    double previous_close_ = std::numeric_limits<double>::quiet_NaN();
    bool has_previous_data_ = false;
    
private:
    void initialize_lines();
    double calculate_daily_value(double close, double high, double low, double prev_close);
    double calculate_sum_over_period();
    void update_daily_values(double daily_value);
    
    // Helper methods for the complex formula
    double calculate_close_adjustment(double close, double high, double low, double prev_close);
    bool is_close_equal_to_previous(double close, double prev_close) const;
    bool is_close_greater_than_previous(double close, double prev_close) const;
    double get_min_low_prev_close(double low, double prev_close) const;
    double get_max_high_prev_close(double high, double prev_close) const;
};

/**
 * Factory functions for creating custom indicators
 */
namespace myind_factory {

/**
 * Create MaBetweenHighAndLow indicator
 */
std::shared_ptr<MaBetweenHighAndLow> create_ma_between_high_low(int period = 5);

/**
 * Create BarsLast indicator with MaBetweenHighAndLow as default condition
 */
std::shared_ptr<BarsLast> create_bars_last_ma_condition(int period = 5);

/**
 * Create BarsLast indicator with custom condition function
 */
std::shared_ptr<BarsLast> create_bars_last_custom(BarsLast::ConditionFunc func, int period = 5);

/**
 * Create NewDiff alpha factor indicator
 */
std::shared_ptr<NewDiff> create_new_diff(int period = 5);

/**
 * Create a complete analysis chain with all three indicators
 */
struct CustomIndicatorChain {
    std::shared_ptr<MaBetweenHighAndLow> ma_between;
    std::shared_ptr<BarsLast> bars_last;
    std::shared_ptr<NewDiff> new_diff;
};

CustomIndicatorChain create_full_analysis_chain(int period = 5);

} // namespace myind_factory

/**
 * Utility functions for working with custom indicators
 */
namespace myind_utils {

/**
 * Helper function to check if a value is approximately equal (for floating point comparison)
 */
bool is_approximately_equal(double a, double b, double epsilon = 1e-9);

/**
 * Helper function to safely handle NaN values in calculations
 */
double safe_value(double value, double default_value = 0.0);

/**
 * Calculate moving sum over a period
 */
double calculate_moving_sum(const std::vector<double>& values, int period);

/**
 * Calculate precise sum using Kahan summation algorithm for better numerical stability
 */
double calculate_precise_sum(const std::vector<double>& values);

/**
 * Delay function - get value from N bars ago
 */
template<typename Container>
double delay(const Container& values, int periods, double default_value = std::numeric_limits<double>::quiet_NaN()) {
    if (values.empty() || periods >= static_cast<int>(values.size()) || periods < 0) {
        return default_value;
    }
    
    return values[values.size() - 1 - periods];
}

/**
 * AND operation for boolean indicators (equivalent to bt.And())
 */
bool logical_and(bool condition1, bool condition2);

/**
 * Conditional selection (equivalent to Python's conditional expression)
 */
template<typename T>
T conditional_select(bool condition, T value_if_true, T value_if_false) {
    return condition ? value_if_true : value_if_false;
}

} // namespace myind_utils

} // namespace indicators
} // namespace backtrader