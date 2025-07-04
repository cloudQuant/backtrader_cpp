#pragma once

#include "csvgeneric.h"
#include "../timeframe.h"
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <map>
#include <regex>

namespace backtrader {
namespace feeds {

/**
 * VChartCSVData - VisualChart CSV File Support
 * 
 * This class provides support for VisualChart exported CSV files.
 * VisualChart (http://www.visualchart.com) exports market data in a specific
 * CSV format that includes ticker name, timeframe indicators, and standard OHLCV data.
 * 
 * VisualChart CSV format characteristics:
 * - First column: Ticker/Symbol name
 * - Second column: Timeframe indicator (I=Minutes, D=Days, W=Weeks, M=Months)
 * - Third column: Date in YYYYMMDD format
 * - Fourth column: Time in HHMMSS format (for intraday) or session end time (for daily+)
 * - Remaining columns: Standard OHLCV + Open Interest data
 * 
 * Features:
 * - Automatic timeframe detection from file content
 * - Session time handling for different timeframes
 * - Symbol name extraction from data
 * - Flexible date/time parsing
 * - Support for multiple asset classes
 */
class VChartCSVData : public GenericCSVData {
public:
    // VisualChart specific parameters
    struct VChartParams : public GenericCSVData::Params {
        // Session timing for daily data
        std::chrono::system_clock::time_point session_end = 
            std::chrono::system_clock::time_point{} + std::chrono::hours(16);  // 4:00 PM default
        
        // Symbol processing
        bool auto_extract_symbol = true;       // Extract symbol from data
        std::string override_symbol;           // Override extracted symbol
        bool validate_symbol_consistency = true; // Validate symbol consistency across rows
        
        // Timeframe handling
        bool auto_detect_timeframe = true;     // Auto-detect timeframe from data
        TimeFrame override_timeframe = TimeFrame::Minutes; // Override detected timeframe
        bool strict_timeframe_validation = false; // Strict timeframe validation
        
        // Data processing options
        bool skip_invalid_timeframes = true;   // Skip rows with invalid timeframes
        bool normalize_timestamps = true;      // Normalize timestamp format
        bool validate_ohlc = true;             // Validate OHLC relationships
        bool sort_by_timestamp = false;        // Sort data by timestamp
        
        // Format variations
        bool use_microseconds = false;         // Use microsecond precision
        std::string date_format = "YYYYMMDD";  // Date format string
        std::string time_format = "HHMMSS";    // Time format string
        bool allow_empty_times = true;         // Allow empty time fields for daily data
        
        // Quality control
        double min_volume = 0.0;               // Minimum volume filter
        double max_price_change = 0.5;         // Maximum price change ratio (50%)
        bool filter_weekends = false;          // Filter weekend data
        bool filter_holidays = false;          // Filter holiday data
        std::vector<std::string> holiday_dates; // Holiday dates to filter
    };
    
    // VisualChart timeframe mappings
    static const std::map<char, TimeFrame> VC_TIMEFRAMES;
    
    // Constructor variants
    VChartCSVData(const VChartParams& params = VChartParams{});
    VChartCSVData(const std::string& filename, const VChartParams& params = VChartParams{});
    virtual ~VChartCSVData() = default;
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // VisualChart specific methods
    std::string get_extracted_symbol() const { return extracted_symbol_; }
    TimeFrame get_detected_timeframe() const { return detected_timeframe_; }
    
    // Data validation and statistics
    struct ValidationResult {
        bool is_valid;
        size_t total_rows;
        size_t valid_rows;
        size_t invalid_rows;
        size_t timeframe_mismatches;
        size_t symbol_mismatches;
        size_t ohlc_violations;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::string summary;
    };
    
    ValidationResult validate_file() const;
    
    // File analysis
    struct FileAnalysis {
        std::string primary_symbol;
        TimeFrame primary_timeframe;
        std::map<std::string, size_t> symbol_counts;
        std::map<TimeFrame, size_t> timeframe_counts;
        std::chrono::system_clock::time_point start_date;
        std::chrono::system_clock::time_point end_date;
        std::chrono::seconds data_span;
        size_t total_records;
        double average_volume;
        std::string data_quality_assessment;
    };
    
    FileAnalysis analyze_file() const;
    
    // Session management
    void set_session_end(int hour, int minute, int second = 0);
    std::chrono::system_clock::time_point get_session_end() const;
    
    // Symbol handling
    void set_symbol_override(const std::string& symbol);
    bool is_symbol_consistent() const;
    std::vector<std::string> get_found_symbols() const;
    
    // Timeframe utilities
    void set_timeframe_override(TimeFrame tf);
    bool is_timeframe_consistent() const;
    std::vector<TimeFrame> get_found_timeframes() const;
    
    // Data filtering
    void add_holiday_date(const std::string& date);
    void set_holiday_dates(const std::vector<std::string>& dates);
    bool is_holiday(const std::chrono::system_clock::time_point& dt) const;
    bool is_weekend(const std::chrono::system_clock::time_point& dt) const;
    
    // Quality metrics
    struct QualityMetrics {
        double completeness_score;      // 0.0 to 1.0
        double consistency_score;       // 0.0 to 1.0
        double accuracy_score;          // 0.0 to 1.0
        size_t outlier_count;
        size_t gap_count;
        std::string quality_grade;      // A, B, C, D, F
        std::vector<std::string> quality_issues;
    };
    
    QualityMetrics assess_data_quality() const;
    
protected:
    VChartParams vchart_params_;
    
    // Extracted information
    std::string extracted_symbol_;
    TimeFrame detected_timeframe_ = TimeFrame::Minutes;
    
    // Processing state
    std::vector<std::string> found_symbols_;
    std::vector<TimeFrame> found_timeframes_;
    bool symbol_consistent_ = true;
    bool timeframe_consistent_ = true;
    
    // Statistics
    size_t processed_rows_ = 0;
    size_t skipped_rows_ = 0;
    size_t error_rows_ = 0;
    
private:
    // Line processing
    bool load_line_data(const std::vector<std::string>& tokens) override;
    
    // Data parsing
    bool parse_vchart_line(const std::vector<std::string>& tokens);
    std::chrono::system_clock::time_point parse_vchart_datetime(
        const std::string& date_str, 
        const std::string& time_str,
        TimeFrame timeframe) const;
    
    // Symbol processing
    bool process_symbol(const std::string& symbol);
    bool validate_symbol(const std::string& symbol) const;
    
    // Timeframe processing
    bool process_timeframe(char timeframe_char);
    TimeFrame convert_vchart_timeframe(char tf_char) const;
    bool validate_timeframe(TimeFrame tf) const;
    
    // Data validation
    bool validate_date_string(const std::string& date_str) const;
    bool validate_time_string(const std::string& time_str) const;
    bool validate_price_data(double open, double high, double low, double close) const;
    bool is_price_outlier(double price, double reference_price) const;
    
    // Time utilities
    std::chrono::system_clock::time_point combine_date_time(
        const std::string& date_str,
        const std::string& time_str) const;
    
    std::chrono::system_clock::time_point apply_session_time(
        const std::chrono::system_clock::time_point& date,
        TimeFrame timeframe) const;
    
    // Data filtering
    bool should_skip_row(const std::chrono::system_clock::time_point& dt) const;
    bool passes_volume_filter(double volume) const;
    bool passes_price_change_filter(double current_price, double previous_price) const;
    
    // Quality assessment helpers
    void update_quality_metrics();
    double calculate_completeness_score() const;
    double calculate_consistency_score() const;
    double calculate_accuracy_score() const;
    
    // Error handling
    void log_parsing_error(const std::string& error, size_t line_number = 0);
    void log_validation_warning(const std::string& warning, size_t line_number = 0);
    
    // File analysis helpers
    void analyze_symbols();
    void analyze_timeframes();
    void analyze_time_range();
    void analyze_data_quality();
};

/**
 * VChartCSV - Feed class for VisualChart CSV files
 * Simple wrapper that uses VChartCSVData as the data class
 */
class VChartCSV {
public:
    using DataClass = VChartCSVData;
    
    // Factory method for creating VChart CSV feeds
    static std::shared_ptr<VChartCSVData> create(
        const std::string& filename,
        const VChartCSVData::VChartParams& params = VChartCSVData::VChartParams{}
    ) {
        return std::make_shared<VChartCSVData>(filename, params);
    }
};

/**
 * Specialized VisualChart CSV feeds for different use cases
 */

/**
 * Intraday VisualChart CSV feed
 * Optimized for minute and sub-minute data
 */
class VChartIntradayCSV : public VChartCSVData {
public:
    struct IntradayParams : public VChartParams {
        bool validate_time_sequence = true;     // Validate chronological order
        bool fill_time_gaps = false;            // Fill missing time periods
        int max_gap_minutes = 5;                // Maximum acceptable gap
        bool session_filtering = false;         // Filter by trading session
        std::chrono::system_clock::time_point session_start = 
            std::chrono::system_clock::time_point{} + std::chrono::hours(9) + std::chrono::minutes(30);
        std::chrono::system_clock::time_point session_end = 
            std::chrono::system_clock::time_point{} + std::chrono::hours(16);
        bool weekend_filtering = true;          // Filter weekend data
    };
    
    VChartIntradayCSV(const IntradayParams& params = IntradayParams{});
    VChartIntradayCSV(const std::string& filename, const IntradayParams& params = IntradayParams{});
    
    // Intraday-specific methods
    void set_trading_session(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end
    );
    
    bool is_within_trading_session(const std::chrono::system_clock::time_point& dt) const;
    
protected:
    IntradayParams intraday_params_;
    
private:
    bool validate_intraday_sequence() const;
    void fill_missing_minutes();
};

/**
 * Daily VisualChart CSV feed
 * Optimized for daily and higher timeframe data
 */
class VChartDailyCSV : public VChartCSVData {
public:
    struct DailyParams : public VChartParams {
        bool validate_date_sequence = true;     // Validate chronological order
        bool fill_weekend_gaps = false;         // Fill weekend gaps
        bool skip_holidays = true;              // Skip holiday dates
        std::vector<std::string> custom_holidays; // Custom holiday dates
        bool validate_business_days = true;     // Validate business day sequence
        int max_gap_days = 7;                   // Maximum acceptable gap in days
    };
    
    VChartDailyCSV(const DailyParams& params = DailyParams{});
    VChartDailyCSV(const std::string& filename, const DailyParams& params = DailyParams{});
    
    // Daily-specific methods
    void add_holiday(const std::string& date);
    void set_holiday_list(const std::vector<std::string>& holidays);
    
protected:
    DailyParams daily_params_;
    
private:
    bool validate_daily_sequence() const;
    void process_holiday_filtering();
};

/**
 * Factory functions for VisualChart CSV feeds
 */
namespace vchart_csv_factory {

/**
 * Create appropriate VChart CSV feed based on auto-detection
 */
std::shared_ptr<VChartCSVData> create_auto_vchart_feed(
    const std::string& filename,
    const VChartCSVData::VChartParams& params = VChartCSVData::VChartParams{}
);

/**
 * Create intraday VChart CSV feed
 */
std::shared_ptr<VChartIntradayCSV> create_intraday_vchart_feed(
    const std::string& filename,
    bool enable_session_filtering = false
);

/**
 * Create daily VChart CSV feed
 */
std::shared_ptr<VChartDailyCSV> create_daily_vchart_feed(
    const std::string& filename,
    bool skip_holidays = true
);

/**
 * Batch create multiple VChart CSV feeds
 */
std::vector<std::shared_ptr<VChartCSVData>> create_vchart_feeds_from_directory(
    const std::string& directory_path,
    const std::string& file_pattern = "*.csv"
);

/**
 * Create VChart feed with optimal parameters for specific use case
 */
std::shared_ptr<VChartCSVData> create_optimized_vchart_feed(
    const std::string& filename,
    const std::string& use_case = "general"  // "backtesting", "analysis", "validation"
);

} // namespace vchart_csv_factory

/**
 * Utility functions for VisualChart CSV processing
 */
namespace vchart_csv_utils {

/**
 * File format detection and validation
 */
struct FormatDetection {
    bool is_vchart_format;
    double confidence_score;
    std::string detected_version;
    std::vector<std::string> format_issues;
    std::string recommendation;
};

FormatDetection detect_vchart_format(const std::string& filename);

/**
 * Data conversion utilities
 */
bool convert_to_standard_csv(
    const std::string& vchart_file,
    const std::string& output_file,
    const std::string& format = "generic"
);

bool merge_vchart_files(
    const std::vector<std::string>& input_files,
    const std::string& output_file,
    bool sort_by_timestamp = true
);

/**
 * Symbol and timeframe analysis
 */
struct SymbolAnalysis {
    std::map<std::string, size_t> symbol_distribution;
    std::string primary_symbol;
    bool has_multiple_symbols;
    std::vector<std::string> symbol_conflicts;
};

SymbolAnalysis analyze_symbols(const std::string& filename);

struct TimeframeAnalysis {
    std::map<TimeFrame, size_t> timeframe_distribution;
    TimeFrame primary_timeframe;
    bool has_multiple_timeframes;
    std::vector<TimeFrame> timeframe_conflicts;
};

TimeframeAnalysis analyze_timeframes(const std::string& filename);

/**
 * Data quality assessment
 */
struct QualityReport {
    double overall_quality_score;       // 0.0 to 1.0
    VChartCSVData::QualityMetrics metrics;
    VChartCSVData::ValidationResult validation;
    std::vector<std::string> recommendations;
    std::string quality_summary;
};

QualityReport assess_file_quality(const std::string& filename);

/**
 * Performance benchmarking
 */
struct PerformanceBenchmark {
    double parsing_speed_rows_per_second;
    std::chrono::milliseconds total_processing_time;
    size_t memory_usage_kb;
    std::string performance_category;
    std::vector<std::string> optimization_suggestions;
};

PerformanceBenchmark benchmark_parsing_performance(
    const std::string& filename,
    size_t test_iterations = 3
);

/**
 * Data statistics
 */
struct DataStatistics {
    size_t total_rows;
    std::chrono::system_clock::time_point date_range_start;
    std::chrono::system_clock::time_point date_range_end;
    std::chrono::seconds total_timespan;
    double average_volume;
    double price_volatility;
    std::vector<std::pair<std::string, double>> summary_stats;
};

DataStatistics calculate_statistics(const std::string& filename);

/**
 * File maintenance utilities
 */
bool validate_file_integrity(const std::string& filename);
bool repair_vchart_file(const std::string& input_file, const std::string& output_file);
bool optimize_vchart_file(const std::string& input_file, const std::string& output_file);

/**
 * Sample data extraction
 */
std::vector<std::map<std::string, std::string>> extract_sample_records(
    const std::string& filename,
    size_t num_samples = 10
);

/**
 * Format documentation
 */
std::string get_format_documentation();
std::vector<std::string> get_supported_timeframes();
std::map<std::string, std::string> get_field_descriptions();

} // namespace vchart_csv_utils

} // namespace feeds
} // namespace backtrader