#include "../../include/plot/formatters.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <ctime>

namespace backtrader {
namespace plot {

// Static member definitions
const std::vector<std::string> VolumeFormatter::SUFFIXES = {"", "K", "M", "G", "T", "P"};

// VolumeFormatter implementation
VolumeFormatter::VolumeFormatter(double vol_max, int precision, bool hide_large_values)
    : vol_max_(vol_max), precision_(precision), hide_large_values_(hide_large_values) {
    calculate_divisor_and_suffix();
}

std::string VolumeFormatter::format(double value, int position) const {
    if (hide_large_values_ && value > vol_max_ * 1.20) {
        return "";
    }
    
    return format_volume(value, divisor_, suffix_, precision_);
}

std::string VolumeFormatter::get_format_info() const {
    std::ostringstream oss;
    oss << "VolumeFormatter(max=" << vol_max_ << ", divisor=" << divisor_ 
        << ", suffix=" << suffix_ << ", precision=" << precision_ << ")";
    return oss.str();
}

std::pair<double, std::string> VolumeFormatter::calculate_scale(double max_value) {
    int magnitude = 0;
    double divisor = 1.0;
    
    while (std::abs(max_value / divisor) >= 1000 && magnitude < static_cast<int>(SUFFIXES.size() - 1)) {
        magnitude++;
        divisor *= 1000.0;
    }
    
    return {divisor, SUFFIXES[magnitude]};
}

std::string VolumeFormatter::format_volume(double volume, double divisor, const std::string& suffix, int precision) {
    double scaled_volume = volume / divisor;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << scaled_volume << suffix;
    return oss.str();
}

void VolumeFormatter::calculate_divisor_and_suffix() {
    auto scale = calculate_scale(vol_max_);
    divisor_ = scale.first;
    suffix_ = scale.second;
}

// DateTimeFormatter implementation
DateTimeFormatter::DateTimeFormatter(const std::vector<std::chrono::system_clock::time_point>& dates, 
                                     const std::string& format_string)
    : dates_(dates), format_string_(format_string) {}

DateTimeFormatter::DateTimeFormatter(const std::vector<double>& dates, const std::string& format_string)
    : format_string_(format_string) {
    dates_.reserve(dates.size());
    for (double date : dates) {
        dates_.push_back(numeric_to_timepoint(date));
    }
}

std::string DateTimeFormatter::format(double value, int position) const {
    int index = clamp_index(static_cast<int>(std::round(value)));
    
    if (index >= 0 && index < static_cast<int>(dates_.size())) {
        return format_timepoint(dates_[index], format_string_);
    }
    
    return "";
}

std::string DateTimeFormatter::get_format_info() const {
    std::ostringstream oss;
    oss << "DateTimeFormatter(count=" << dates_.size() << ", format=" << format_string_ << ")";
    return oss.str();
}

std::chrono::system_clock::time_point DateTimeFormatter::get_date_at_index(int index) const {
    index = clamp_index(index);
    return dates_[index];
}

std::string DateTimeFormatter::format_timepoint(const std::chrono::system_clock::time_point& tp, 
                                                const std::string& format_string) {
    std::time_t time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm_ptr = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, format_string.c_str());
    return oss.str();
}

int DateTimeFormatter::clamp_index(int index) const {
    if (index < 0) return 0;
    if (index >= static_cast<int>(dates_.size())) return static_cast<int>(dates_.size()) - 1;
    return index;
}

std::chrono::system_clock::time_point DateTimeFormatter::numeric_to_timepoint(double numeric_date) const {
    // Convert numeric date to time_point
    // This is a simplified implementation - in practice, you'd need to handle
    // the specific numeric date format used by your system
    return std::chrono::system_clock::from_time_t(static_cast<std::time_t>(numeric_date));
}

// PriceFormatter implementation
PriceFormatter::PriceFormatter(int precision, const std::string& currency_symbol, bool thousands_separator)
    : precision_(precision), currency_symbol_(currency_symbol), thousands_separator_(thousands_separator) {}

std::string PriceFormatter::format(double value, int position) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision_) << value;
    
    std::string result = oss.str();
    
    if (thousands_separator_) {
        result = add_thousands_separator(result);
    }
    
    if (!currency_symbol_.empty()) {
        result = currency_symbol_ + result;
    }
    
    return result;
}

std::string PriceFormatter::get_format_info() const {
    std::ostringstream oss;
    oss << "PriceFormatter(precision=" << precision_ << ", currency=" << currency_symbol_ 
        << ", thousands=" << thousands_separator_ << ")";
    return oss.str();
}

std::string PriceFormatter::add_thousands_separator(const std::string& number) const {
    // Find decimal point
    size_t decimal_pos = number.find('.');
    std::string integer_part = (decimal_pos != std::string::npos) ? 
                               number.substr(0, decimal_pos) : number;
    std::string decimal_part = (decimal_pos != std::string::npos) ? 
                               number.substr(decimal_pos) : "";
    
    // Add commas to integer part
    std::string result;
    int count = 0;
    for (int i = static_cast<int>(integer_part.length()) - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) {
            result = "," + result;
        }
        result = integer_part[i] + result;
        count++;
    }
    
    return result + decimal_part;
}

// PercentageFormatter implementation
PercentageFormatter::PercentageFormatter(int precision, bool multiply_by_100)
    : precision_(precision), multiply_by_100_(multiply_by_100) {}

std::string PercentageFormatter::format(double value, int position) const {
    double formatted_value = multiply_by_100_ ? value * 100.0 : value;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision_) << formatted_value << "%";
    return oss.str();
}

std::string PercentageFormatter::get_format_info() const {
    std::ostringstream oss;
    oss << "PercentageFormatter(precision=" << precision_ << ", multiply=" << multiply_by_100_ << ")";
    return oss.str();
}

// ScientificFormatter implementation
ScientificFormatter::ScientificFormatter(int precision, double threshold_low, double threshold_high)
    : precision_(precision), threshold_low_(threshold_low), threshold_high_(threshold_high) {}

std::string ScientificFormatter::format(double value, int position) const {
    if (should_use_scientific(value)) {
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(precision_) << value;
        return oss.str();
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision_) << value;
        return oss.str();
    }
}

std::string ScientificFormatter::get_format_info() const {
    std::ostringstream oss;
    oss << "ScientificFormatter(precision=" << precision_ << ", low=" << threshold_low_ 
        << ", high=" << threshold_high_ << ")";
    return oss.str();
}

bool ScientificFormatter::should_use_scientific(double value) const {
    double abs_value = std::abs(value);
    return (abs_value != 0.0 && (abs_value < threshold_low_ || abs_value > threshold_high_));
}

// DateTimeLocator implementation
DateTimeLocator::DateTimeLocator(const std::vector<std::chrono::system_clock::time_point>& dates, int num_ticks)
    : dates_(dates), num_ticks_(num_ticks), interval_type_(IntervalType::DAYS) {
    
    if (!dates_.empty()) {
        interval_type_ = determine_best_interval(dates_.front(), dates_.back());
    }
}

std::vector<double> DateTimeLocator::get_tick_positions(double min_value, double max_value, int max_ticks) const {
    if (dates_.empty()) {
        return {};
    }
    
    int min_index = std::max(0, static_cast<int>(min_value));
    int max_index = std::min(static_cast<int>(dates_.size()) - 1, static_cast<int>(max_value));
    
    if (min_index >= max_index) {
        return {static_cast<double>(min_index)};
    }
    
    std::chrono::system_clock::time_point start_time = dates_[min_index];
    std::chrono::system_clock::time_point end_time = dates_[max_index];
    
    return generate_tick_positions(start_time, end_time, interval_type_, max_ticks);
}

std::string DateTimeLocator::get_locator_info() const {
    std::ostringstream oss;
    oss << "DateTimeLocator(count=" << dates_.size() << ", ticks=" << num_ticks_ << ")";
    return oss.str();
}

DateTimeLocator::IntervalType DateTimeLocator::determine_best_interval(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) const {
    
    auto duration = end - start;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    
    if (seconds < 300) return IntervalType::SECONDS;  // < 5 minutes
    if (seconds < 3600 * 6) return IntervalType::MINUTES;  // < 6 hours
    if (seconds < 3600 * 24 * 2) return IntervalType::HOURS;  // < 2 days
    if (seconds < 3600 * 24 * 14) return IntervalType::DAYS;  // < 2 weeks
    if (seconds < 3600 * 24 * 60) return IntervalType::WEEKS;  // < ~2 months
    if (seconds < 3600 * 24 * 365 * 2) return IntervalType::MONTHS;  // < 2 years
    
    return IntervalType::YEARS;
}

std::vector<double> DateTimeLocator::generate_tick_positions(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end,
    IntervalType interval,
    int max_ticks) const {
    
    std::vector<double> positions;
    
    // This is a simplified implementation
    // In practice, you'd need more sophisticated logic for each interval type
    
    auto duration = end - start;
    int num_intervals = std::min(max_ticks - 1, static_cast<int>(dates_.size() / 10));
    if (num_intervals < 1) num_intervals = 1;
    
    auto step_duration = duration / num_intervals;
    
    for (int i = 0; i <= num_intervals; ++i) {
        auto target_time = start + step_duration * i;
        
        // Find closest date index
        auto it = std::lower_bound(dates_.begin(), dates_.end(), target_time);
        if (it != dates_.end()) {
            double index = static_cast<double>(std::distance(dates_.begin(), it));
            positions.push_back(index);
        }
    }
    
    return positions;
}

// LinearLocator implementation
LinearLocator::LinearLocator(bool include_zero) : include_zero_(include_zero) {}

std::vector<double> LinearLocator::get_tick_positions(double min_value, double max_value, int max_ticks) const {
    if (max_ticks < 2) max_ticks = 2;
    
    double range = max_value - min_value;
    if (range <= 0) {
        return {min_value};
    }
    
    double step = calculate_nice_step(range, max_ticks);
    
    // Adjust bounds to nice numbers
    double nice_min = round_to_nice_number(min_value, false);
    double nice_max = round_to_nice_number(max_value, true);
    
    // Include zero if requested and within range
    if (include_zero_ && min_value <= 0 && max_value >= 0) {
        nice_min = std::min(nice_min, 0.0);
        nice_max = std::max(nice_max, 0.0);
    }
    
    std::vector<double> positions;
    for (double pos = nice_min; pos <= nice_max + step * 0.5; pos += step) {
        positions.push_back(pos);
    }
    
    return positions;
}

std::string LinearLocator::get_locator_info() const {
    std::ostringstream oss;
    oss << "LinearLocator(include_zero=" << include_zero_ << ")";
    return oss.str();
}

double LinearLocator::calculate_nice_step(double range, int max_ticks) const {
    double rough_step = range / (max_ticks - 1);
    
    // Find the order of magnitude
    double magnitude = std::pow(10, std::floor(std::log10(rough_step)));
    
    // Normalize to 1-10 range
    double normalized = rough_step / magnitude;
    
    // Choose nice step
    double nice_normalized;
    if (normalized <= 1.0) nice_normalized = 1.0;
    else if (normalized <= 2.0) nice_normalized = 2.0;
    else if (normalized <= 5.0) nice_normalized = 5.0;
    else nice_normalized = 10.0;
    
    return nice_normalized * magnitude;
}

double LinearLocator::round_to_nice_number(double value, bool round_up) const {
    if (value == 0.0) return 0.0;
    
    double magnitude = std::pow(10, std::floor(std::log10(std::abs(value))));
    double normalized = value / magnitude;
    
    double nice_normalized;
    if (round_up) {
        if (normalized <= 1.0) nice_normalized = 1.0;
        else if (normalized <= 2.0) nice_normalized = 2.0;
        else if (normalized <= 5.0) nice_normalized = 5.0;
        else nice_normalized = 10.0;
    } else {
        if (normalized >= 10.0) nice_normalized = 10.0;
        else if (normalized >= 5.0) nice_normalized = 5.0;
        else if (normalized >= 2.0) nice_normalized = 2.0;
        else nice_normalized = 1.0;
    }
    
    return nice_normalized * magnitude * (value < 0 ? -1 : 1);
}

// Factory functions
namespace formatter_factory {

std::unique_ptr<Formatter> create_formatter(const std::string& data_type,
                                           const std::pair<double, double>& data_range,
                                           const std::string& format_options) {
    if (data_type == "volume") {
        double max_volume = std::max(data_range.first, data_range.second);
        return std::make_unique<VolumeFormatter>(max_volume);
    } else if (data_type == "price") {
        return std::make_unique<PriceFormatter>(2, "$");
    } else if (data_type == "percentage") {
        return std::make_unique<PercentageFormatter>(1, true);
    } else if (data_type == "scientific") {
        return std::make_unique<ScientificFormatter>(2);
    }
    
    // Default to price formatter
    return std::make_unique<PriceFormatter>(2);
}

std::unique_ptr<VolumeFormatter> create_volume_formatter(double max_volume, int precision) {
    return std::make_unique<VolumeFormatter>(max_volume, precision);
}

std::unique_ptr<DateTimeFormatter> create_datetime_formatter(
    const std::vector<std::chrono::system_clock::time_point>& dates,
    const std::string& format_string) {
    return std::make_unique<DateTimeFormatter>(dates, format_string);
}

std::unique_ptr<PriceFormatter> create_price_formatter(int precision, const std::string& currency) {
    return std::make_unique<PriceFormatter>(precision, currency);
}

std::unique_ptr<Locator> create_locator(const std::string& data_type,
                                       const std::vector<double>& data) {
    if (data_type == "datetime") {
        // Convert double data to time_points if needed
        std::vector<std::chrono::system_clock::time_point> dates;
        for (double d : data) {
            dates.push_back(std::chrono::system_clock::from_time_t(static_cast<std::time_t>(d)));
        }
        return std::make_unique<DateTimeLocator>(dates);
    }
    
    return std::make_unique<LinearLocator>();
}

std::pair<std::unique_ptr<DateTimeLocator>, std::unique_ptr<DateTimeFormatter>>
create_datetime_locator_formatter(const std::vector<std::chrono::system_clock::time_point>& dates,
                                 int num_ticks,
                                 const std::string& format_string) {
    auto locator = std::make_unique<DateTimeLocator>(dates, num_ticks);
    auto formatter = std::make_unique<DateTimeFormatter>(dates, format_string);
    return {std::move(locator), std::move(formatter)};
}

} // namespace formatter_factory

// Utility functions
namespace formatter_utils {

std::chrono::system_clock::time_point numeric_to_timepoint(double numeric_date) {
    return std::chrono::system_clock::from_time_t(static_cast<std::time_t>(numeric_date));
}

double timepoint_to_numeric(const std::chrono::system_clock::time_point& tp) {
    return static_cast<double>(std::chrono::system_clock::to_time_t(tp));
}

std::string format_timepoint(const std::chrono::system_clock::time_point& tp, 
                            const std::string& format_string) {
    return DateTimeFormatter::format_timepoint(tp, format_string);
}

int calculate_precision(double min_value, double max_value) {
    double range = std::abs(max_value - min_value);
    if (range == 0.0) return 2;
    
    int precision = static_cast<int>(-std::log10(range)) + 2;
    return std::max(0, std::min(precision, 6));
}

bool should_use_scientific_notation(double min_value, double max_value) {
    double abs_min = std::abs(min_value);
    double abs_max = std::abs(max_value);
    double max_abs = std::max(abs_min, abs_max);
    
    return (max_abs != 0.0 && (max_abs < 1e-3 || max_abs > 1e6));
}

std::pair<double, double> expand_bounds_to_nice_numbers(double min_value, double max_value) {
    LinearLocator locator;
    double range = max_value - min_value;
    double step = range * 0.1;  // 10% padding
    
    return {min_value - step, max_value + step};
}

} // namespace formatter_utils

} // namespace plot
} // namespace backtrader