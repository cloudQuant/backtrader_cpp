#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <cmath>

namespace backtrader {
namespace plot {

/**
 * Base formatter interface for axis labeling
 */
class Formatter {
public:
    virtual ~Formatter() = default;
    
    /**
     * Format a value for display on an axis
     * @param value The value to format
     * @param position The position index (optional)
     * @return Formatted string representation
     */
    virtual std::string format(double value, int position = 0) const = 0;
    
    /**
     * Get format information for optimization
     */
    virtual std::string get_format_info() const { return ""; }
};

/**
 * Volume formatter with automatic suffix selection (K, M, G, T, P)
 * 
 * This formatter automatically determines the appropriate suffix based on
 * the maximum volume value and formats volumes accordingly.
 * For example: 1,500,000 becomes "1.5M"
 */
class VolumeFormatter : public Formatter {
public:
    // Predefined suffixes for different magnitudes
    static const std::vector<std::string> SUFFIXES;
    
    /**
     * Constructor
     * @param vol_max Maximum volume value for determining scale
     * @param precision Number of decimal places (default: 1)
     * @param hide_large_values Hide values above 120% of vol_max (default: true)
     */
    explicit VolumeFormatter(double vol_max, int precision = 1, bool hide_large_values = true);
    
    std::string format(double value, int position = 0) const override;
    std::string get_format_info() const override;
    
    // Getters
    double get_divisor() const { return divisor_; }
    const std::string& get_suffix() const { return suffix_; }
    double get_max_volume() const { return vol_max_; }
    
    // Static utility methods
    static std::pair<double, std::string> calculate_scale(double max_value);
    static std::string format_volume(double volume, double divisor, const std::string& suffix, int precision = 1);
    
private:
    double vol_max_;
    double divisor_;
    std::string suffix_;
    int precision_;
    bool hide_large_values_;
    
    void calculate_divisor_and_suffix();
};

/**
 * Date/time formatter for temporal data
 * 
 * Formats datetime values for axis labels with customizable format strings.
 * Handles conversion from numeric indices to actual datetime values.
 */
class DateTimeFormatter : public Formatter {
public:
    /**
     * Constructor
     * @param dates Vector of datetime values (as time_t or similar numeric representation)
     * @param format_string strftime-compatible format string (default: "%Y-%m-%d")
     */
    DateTimeFormatter(const std::vector<std::chrono::system_clock::time_point>& dates, 
                      const std::string& format_string = "%Y-%m-%d");
    
    /**
     * Alternative constructor with numeric dates
     * @param dates Vector of numeric date representations
     * @param format_string strftime-compatible format string
     */
    DateTimeFormatter(const std::vector<double>& dates, 
                      const std::string& format_string = "%Y-%m-%d");
    
    std::string format(double value, int position = 0) const override;
    std::string get_format_info() const override;
    
    // Configuration
    void set_format_string(const std::string& format_string) { format_string_ = format_string; }
    const std::string& get_format_string() const { return format_string_; }
    
    size_t get_date_count() const { return dates_.size(); }
    
    // Utility methods
    std::chrono::system_clock::time_point get_date_at_index(int index) const;
    static std::string format_timepoint(const std::chrono::system_clock::time_point& tp, 
                                       const std::string& format_string);
    
private:
    std::vector<std::chrono::system_clock::time_point> dates_;
    std::string format_string_;
    
    int clamp_index(int index) const;
    std::chrono::system_clock::time_point numeric_to_timepoint(double numeric_date) const;
};

/**
 * Price formatter for financial data
 * 
 * Formats price values with appropriate precision and optional currency symbols.
 */
class PriceFormatter : public Formatter {
public:
    /**
     * Constructor
     * @param precision Number of decimal places (default: 2)
     * @param currency_symbol Currency symbol to prepend (default: empty)
     * @param thousands_separator Use thousands separator (default: false)
     */
    PriceFormatter(int precision = 2, 
                   const std::string& currency_symbol = "", 
                   bool thousands_separator = false);
    
    std::string format(double value, int position = 0) const override;
    std::string get_format_info() const override;
    
    // Configuration
    void set_precision(int precision) { precision_ = precision; }
    int get_precision() const { return precision_; }
    
    void set_currency_symbol(const std::string& symbol) { currency_symbol_ = symbol; }
    const std::string& get_currency_symbol() const { return currency_symbol_; }
    
private:
    int precision_;
    std::string currency_symbol_;
    bool thousands_separator_;
    
    std::string add_thousands_separator(const std::string& number) const;
};

/**
 * Percentage formatter
 * 
 * Formats values as percentages with configurable precision.
 */
class PercentageFormatter : public Formatter {
public:
    /**
     * Constructor
     * @param precision Number of decimal places (default: 1)
     * @param multiply_by_100 Whether to multiply by 100 (default: true)
     */
    PercentageFormatter(int precision = 1, bool multiply_by_100 = true);
    
    std::string format(double value, int position = 0) const override;
    std::string get_format_info() const override;
    
private:
    int precision_;
    bool multiply_by_100_;
};

/**
 * Scientific notation formatter
 * 
 * Formats very large or very small numbers in scientific notation.
 */
class ScientificFormatter : public Formatter {
public:
    /**
     * Constructor
     * @param precision Number of decimal places (default: 2)
     * @param threshold_low Values below this will use scientific notation
     * @param threshold_high Values above this will use scientific notation
     */
    ScientificFormatter(int precision = 2, 
                       double threshold_low = 1e-3, 
                       double threshold_high = 1e6);
    
    std::string format(double value, int position = 0) const override;
    std::string get_format_info() const override;
    
private:
    int precision_;
    double threshold_low_;
    double threshold_high_;
    
    bool should_use_scientific(double value) const;
};

/**
 * Locator interface for determining tick positions
 */
class Locator {
public:
    virtual ~Locator() = default;
    
    /**
     * Generate tick positions for the given range
     * @param min_value Minimum value of the range
     * @param max_value Maximum value of the range
     * @param max_ticks Maximum number of ticks desired
     * @return Vector of tick positions
     */
    virtual std::vector<double> get_tick_positions(double min_value, double max_value, int max_ticks = 10) const = 0;
    
    /**
     * Get information about the locator
     */
    virtual std::string get_locator_info() const { return ""; }
};

/**
 * Date/time locator for temporal data
 * 
 * Determines appropriate tick positions for datetime axes.
 */
class DateTimeLocator : public Locator {
public:
    enum class IntervalType {
        SECONDS,
        MINUTES,
        HOURS,
        DAYS,
        WEEKS,
        MONTHS,
        YEARS
    };
    
    /**
     * Constructor
     * @param dates Vector of datetime values
     * @param num_ticks Desired number of ticks (default: 5)
     */
    DateTimeLocator(const std::vector<std::chrono::system_clock::time_point>& dates, int num_ticks = 5);
    
    std::vector<double> get_tick_positions(double min_value, double max_value, int max_ticks = 10) const override;
    std::string get_locator_info() const override;
    
    // Configuration
    void set_interval_type(IntervalType type) { interval_type_ = type; }
    IntervalType get_interval_type() const { return interval_type_; }
    
private:
    std::vector<std::chrono::system_clock::time_point> dates_;
    int num_ticks_;
    IntervalType interval_type_;
    
    IntervalType determine_best_interval(const std::chrono::system_clock::time_point& start,
                                        const std::chrono::system_clock::time_point& end) const;
    
    std::vector<double> generate_tick_positions(const std::chrono::system_clock::time_point& start,
                                               const std::chrono::system_clock::time_point& end,
                                               IntervalType interval,
                                               int max_ticks) const;
};

/**
 * Linear locator for numeric data
 * 
 * Generates evenly spaced tick positions.
 */
class LinearLocator : public Locator {
public:
    /**
     * Constructor
     * @param include_zero Force inclusion of zero if within range (default: true)
     */
    LinearLocator(bool include_zero = true);
    
    std::vector<double> get_tick_positions(double min_value, double max_value, int max_ticks = 10) const override;
    std::string get_locator_info() const override;
    
private:
    bool include_zero_;
    
    double calculate_nice_step(double range, int max_ticks) const;
    double round_to_nice_number(double value, bool round_up = false) const;
};

/**
 * Formatter and locator factory functions
 */
namespace formatter_factory {
    /**
     * Create appropriate formatter based on data characteristics
     * @param data_type Type of data ("volume", "price", "datetime", "percentage")
     * @param data_range Optional data range for optimization
     * @param format_options Optional format string or configuration
     */
    std::unique_ptr<Formatter> create_formatter(const std::string& data_type,
                                               const std::pair<double, double>& data_range = {0.0, 0.0},
                                               const std::string& format_options = "");
    
    /**
     * Create volume formatter with automatic scaling
     */
    std::unique_ptr<VolumeFormatter> create_volume_formatter(double max_volume, int precision = 1);
    
    /**
     * Create datetime formatter
     */
    std::unique_ptr<DateTimeFormatter> create_datetime_formatter(
        const std::vector<std::chrono::system_clock::time_point>& dates,
        const std::string& format_string = "%Y-%m-%d");
    
    /**
     * Create price formatter
     */
    std::unique_ptr<PriceFormatter> create_price_formatter(int precision = 2, 
                                                          const std::string& currency = "");
    
    /**
     * Create appropriate locator based on data type
     */
    std::unique_ptr<Locator> create_locator(const std::string& data_type,
                                           const std::vector<double>& data = {});
    
    /**
     * Create datetime locator and formatter pair
     */
    std::pair<std::unique_ptr<DateTimeLocator>, std::unique_ptr<DateTimeFormatter>>
    create_datetime_locator_formatter(const std::vector<std::chrono::system_clock::time_point>& dates,
                                     int num_ticks = 5,
                                     const std::string& format_string = "%Y-%m-%d");
}

/**
 * Utility functions for formatter patches and adjustments
 */
namespace formatter_utils {
    /**
     * Convert numeric date representation to time_point
     */
    std::chrono::system_clock::time_point numeric_to_timepoint(double numeric_date);
    
    /**
     * Convert time_point to numeric representation
     */
    double timepoint_to_numeric(const std::chrono::system_clock::time_point& tp);
    
    /**
     * Format time_point with given format string
     */
    std::string format_timepoint(const std::chrono::system_clock::time_point& tp, 
                                const std::string& format_string);
    
    /**
     * Calculate appropriate number of decimal places for a range
     */
    int calculate_precision(double min_value, double max_value);
    
    /**
     * Determine if values should use scientific notation
     */
    bool should_use_scientific_notation(double min_value, double max_value);
    
    /**
     * Create nice round numbers for axis bounds
     */
    std::pair<double, double> expand_bounds_to_nice_numbers(double min_value, double max_value);
}

} // namespace plot
} // namespace backtrader