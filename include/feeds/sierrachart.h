#pragma once

#include "csvgeneric.h"
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <map>

namespace backtrader {
namespace feeds {

/**
 * SierraChartCSVData - Support for Sierra Chart CSV Files
 * 
 * Sierra Chart (http://www.sierrachart.com) is a professional trading platform
 * that exports market data in a specific CSV format. This class provides seamless
 * integration with Sierra Chart data files.
 * 
 * Sierra Chart CSV format characteristics:
 * - Date format: YYYY/MM/DD
 * - Time format: HH:MM:SS (optional, may be combined with date)
 * - Standard OHLCV columns
 * - Optional volume and open interest data
 * - Support for intraday and daily data
 * - Tick data and bar data formats
 * 
 * Features:
 * - Automatic Sierra Chart format detection
 * - Support for both intraday and daily data formats
 * - Flexible column mapping
 * - Time zone handling
 * - Data validation and error handling
 * - Performance optimizations for large files
 */
class SierraChartCSVData : public GenericCSVData {
public:
    // Sierra Chart specific parameters
    struct SierraParams : public GenericCSVData::Params {
        // Sierra Chart specific date/time formats
        std::string dtformat = "%Y/%m/%d";        // Default Sierra Chart date format
        std::string tmformat = "%H:%M:%S";        // Time format if separate
        std::string dtmformat = "%Y/%m/%d %H:%M:%S"; // Combined datetime format
        
        // Sierra Chart file type detection
        bool auto_detect_format = true;           // Auto-detect file format
        std::string file_type = "auto";           // "intraday", "daily", "tick", "auto"
        
        // Data processing options
        bool combine_date_time = true;            // Combine separate date/time columns
        bool validate_ohlc = true;                // Validate OHLC relationships
        bool fill_missing_volume = false;         // Fill missing volume with 0
        bool skip_invalid_rows = true;            // Skip rows with invalid data
        
        // Time zone handling
        std::string timezone = "UTC";             // Data timezone
        bool convert_to_utc = false;              // Convert to UTC
        
        // Performance options
        bool use_memory_map = false;              // Use memory mapping for large files
        size_t buffer_size = 8192;                // Read buffer size
        bool cache_parsed_data = false;           // Cache parsed data in memory
        
        // Sierra Chart specific column names
        std::string date_name = "Date";
        std::string time_name = "Time";
        std::string datetime_name = "DateTime";
        std::string open_name = "Open";
        std::string high_name = "High";
        std::string low_name = "Low";
        std::string close_name = "Close";
        std::string volume_name = "Volume";
        std::string openinterest_name = "OpenInterest";
        std::string trades_name = "NumTrades";    // Number of trades per bar
        std::string bid_name = "Bid";             // Bid price (tick data)
        std::string ask_name = "Ask";             // Ask price (tick data)
    };
    
    // Constructor variants
    SierraChartCSVData(const SierraParams& params = SierraParams{});
    SierraChartCSVData(const std::string& filename, const SierraParams& params = SierraParams{});
    virtual ~SierraChartCSVData() = default;
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // Sierra Chart specific methods
    bool auto_detect_file_format();
    void set_sierra_chart_format(const std::string& format_type);
    
    // File format detection
    enum class SierraFileType {
        UNKNOWN,
        INTRADAY_BARS,      // Intraday bar data (1min, 5min, etc.)
        DAILY_BARS,         // Daily bar data
        TICK_DATA,          // Tick-by-tick data
        MARKET_DEPTH,       // Level II market depth data
        TIME_AND_SALES      // Time and sales data
    };
    
    SierraFileType detect_file_type() const;
    std::string get_file_type_description(SierraFileType type) const;
    
    // Data validation
    struct ValidationResult {
        bool is_valid;
        size_t total_rows;
        size_t valid_rows;
        size_t invalid_rows;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::string summary;
    };
    
    ValidationResult validate_data() const;
    bool validate_ohlc_relationship(double open, double high, double low, double close) const;
    
    // Statistics
    struct FileStats {
        std::string filename;
        SierraFileType file_type;
        size_t file_size_bytes;
        size_t total_rows;
        size_t data_rows;
        std::chrono::system_clock::time_point start_date;
        std::chrono::system_clock::time_point end_date;
        std::chrono::seconds timespan;
        std::vector<std::string> column_names;
        double average_volume;
        double total_volume;
        std::string data_quality_assessment;
    };
    
    FileStats get_file_statistics() const;
    
    // Column mapping utilities
    void auto_map_sierra_columns();
    bool verify_column_mapping() const;
    void print_column_mapping() const;
    
    // Time zone support
    void set_timezone(const std::string& tz);
    std::string get_timezone() const { return params_.timezone; }
    std::chrono::system_clock::time_point convert_to_timezone(
        const std::chrono::system_clock::time_point& dt,
        const std::string& target_tz) const;
    
    // Performance optimization
    void enable_memory_mapping(bool enable = true) { params_.use_memory_map = enable; }
    void set_buffer_size(size_t size) { params_.buffer_size = size; }
    void enable_data_caching(bool enable = true) { params_.cache_parsed_data = enable; }
    
    // Data access helpers
    size_t get_estimated_memory_usage() const;
    double get_processing_speed() const; // rows per second
    
protected:
    SierraParams params_;
    
    // File processing state
    SierraFileType detected_file_type_ = SierraFileType::UNKNOWN;
    bool format_detected_ = false;
    
    // Performance tracking
    mutable std::chrono::system_clock::time_point processing_start_;
    mutable size_t processed_rows_ = 0;
    
    // Data cache
    std::vector<std::vector<std::string>> cached_rows_;
    bool cache_loaded_ = false;
    
    // Column mapping
    std::map<std::string, int> column_mapping_;
    bool mapping_verified_ = false;
    
private:
    // Format detection helpers
    bool detect_intraday_format(const std::vector<std::string>& sample_rows) const;
    bool detect_daily_format(const std::vector<std::string>& sample_rows) const;
    bool detect_tick_format(const std::vector<std::string>& sample_rows) const;
    
    // Data parsing helpers
    std::chrono::system_clock::time_point parse_sierra_datetime(
        const std::string& date_str,
        const std::string& time_str = "") const;
    
    bool is_valid_sierra_date(const std::string& date_str) const;
    bool is_valid_sierra_time(const std::string& time_str) const;
    
    // Performance optimization
    void load_file_to_cache();
    void setup_memory_mapping();
    void cleanup_resources();
    
    // Validation helpers
    bool validate_header_row(const std::vector<std::string>& header) const;
    bool validate_data_row(const std::vector<std::string>& row) const;
    void log_validation_error(const std::string& error) const;
    
    // Utility functions
    std::vector<std::string> read_sample_rows(size_t num_rows = 10) const;
    size_t count_file_rows() const;
    void analyze_column_patterns(const std::vector<std::string>& sample_rows);
};

/**
 * Specialized Sierra Chart data feeds for different data types
 */

/**
 * Sierra Chart Intraday Data Feed
 * Optimized for intraday bar data (1min, 5min, 15min, etc.)
 */
class SierraChartIntradayData : public SierraChartCSVData {
public:
    struct IntradayParams : public SierraParams {
        int bar_interval_minutes = 1;           // Bar interval in minutes
        bool validate_time_sequence = true;     // Validate chronological order
        bool fill_time_gaps = false;            // Fill missing time periods
        std::string session_start = "09:30:00"; // Trading session start
        std::string session_end = "16:00:00";   // Trading session end
        bool session_filtering = false;         // Filter by trading session
    };
    
    SierraChartIntradayData(const IntradayParams& params = IntradayParams{});
    
    // Intraday-specific methods
    void set_bar_interval(int minutes);
    void set_trading_session(const std::string& start, const std::string& end);
    bool is_within_trading_session(const std::chrono::system_clock::time_point& dt) const;
    
protected:
    IntradayParams intraday_params_;
    
private:
    bool validate_time_sequence() const;
    void fill_missing_bars();
};

/**
 * Sierra Chart Daily Data Feed
 * Optimized for daily OHLCV data
 */
class SierraChartDailyData : public SierraChartCSVData {
public:
    struct DailyParams : public SierraParams {
        bool validate_date_sequence = true;     // Validate chronological order
        bool fill_weekend_gaps = false;         // Fill weekend gaps
        bool holiday_filtering = false;         // Filter holidays
        std::vector<std::string> holidays;      // Holiday dates to filter
    };
    
    SierraChartDailyData(const DailyParams& params = DailyParams{});
    
    // Daily-specific methods
    void add_holiday(const std::string& date);
    void set_holiday_list(const std::vector<std::string>& holidays);
    bool is_holiday(const std::chrono::system_clock::time_point& dt) const;
    
protected:
    DailyParams daily_params_;
    
private:
    bool validate_date_sequence() const;
    void filter_holidays();
};

/**
 * Sierra Chart Tick Data Feed
 * Optimized for tick-by-tick data processing
 */
class SierraChartTickData : public SierraChartCSVData {
public:
    struct TickParams : public SierraParams {
        bool aggregate_to_bars = false;         // Aggregate ticks to bars
        int aggregation_seconds = 60;           // Aggregation period
        bool validate_bid_ask = true;           // Validate bid <= ask
        double min_tick_size = 0.01;            // Minimum tick size
        bool filter_outliers = true;            // Filter price outliers
        double outlier_threshold = 5.0;         // Outlier detection threshold (std devs)
    };
    
    SierraChartTickData(const TickParams& params = TickParams{});
    
    // Tick-specific methods
    void enable_bar_aggregation(int seconds);
    void set_tick_size(double min_size);
    void set_outlier_filtering(bool enable, double threshold = 5.0);
    
protected:
    TickParams tick_params_;
    
    // Tick aggregation
    struct TickBar {
        std::chrono::system_clock::time_point timestamp;
        double open, high, low, close;
        double volume;
        int tick_count;
    };
    
    std::vector<TickBar> aggregated_bars_;
    TickBar current_bar_;
    
private:
    bool validate_tick_data(double price, double bid, double ask) const;
    void aggregate_tick_to_bar(double price, double volume);
    bool is_price_outlier(double price) const;
    void finalize_current_bar();
};

/**
 * Factory functions for Sierra Chart data feeds
 */
namespace sierra_factory {

/**
 * Create appropriate Sierra Chart feed based on auto-detection
 */
std::shared_ptr<SierraChartCSVData> create_auto_sierra_feed(
    const std::string& filename,
    const SierraChartCSVData::SierraParams& params = SierraChartCSVData::SierraParams{}
);

/**
 * Create intraday Sierra Chart feed
 */
std::shared_ptr<SierraChartIntradayData> create_intraday_sierra_feed(
    const std::string& filename,
    int bar_interval_minutes = 1
);

/**
 * Create daily Sierra Chart feed
 */
std::shared_ptr<SierraChartDailyData> create_daily_sierra_feed(
    const std::string& filename
);

/**
 * Create tick Sierra Chart feed
 */
std::shared_ptr<SierraChartTickData> create_tick_sierra_feed(
    const std::string& filename,
    bool aggregate_to_bars = false
);

/**
 * Batch create multiple Sierra Chart feeds
 */
std::vector<std::shared_ptr<SierraChartCSVData>> create_sierra_feeds_from_directory(
    const std::string& directory_path,
    const std::string& file_pattern = "*.csv"
);

} // namespace sierra_factory

/**
 * Utility functions for Sierra Chart data processing
 */
namespace sierra_utils {

/**
 * Analyze Sierra Chart file characteristics
 */
struct FileAnalysis {
    SierraChartCSVData::SierraFileType detected_type;
    std::string format_description;
    std::vector<std::string> column_names;
    size_t estimated_rows;
    size_t estimated_memory_mb;
    std::string recommended_feed_type;
    std::vector<std::string> optimization_suggestions;
};

FileAnalysis analyze_sierra_file(const std::string& filename);

/**
 * Validate Sierra Chart file format
 */
struct FormatValidation {
    bool is_valid_sierra_format;
    std::vector<std::string> format_issues;
    std::vector<std::string> recommendations;
    double confidence_score;
};

FormatValidation validate_sierra_format(const std::string& filename);

/**
 * Convert Sierra Chart files to other formats
 */
bool convert_sierra_to_csv(
    const std::string& input_file,
    const std::string& output_file,
    const std::string& target_format = "generic"
);

/**
 * Merge multiple Sierra Chart files
 */
bool merge_sierra_files(
    const std::vector<std::string>& input_files,
    const std::string& output_file,
    bool sort_by_timestamp = true
);

/**
 * Performance benchmarking
 */
struct PerformanceBenchmark {
    double rows_per_second;
    double memory_usage_mb;
    std::chrono::milliseconds total_processing_time;
    std::string performance_category;
    std::vector<std::string> bottlenecks;
};

PerformanceBenchmark benchmark_sierra_processing(
    const std::string& filename,
    size_t test_iterations = 3
);

/**
 * Data quality assessment
 */
struct QualityReport {
    double completeness_score;      // 0.0 to 1.0
    double consistency_score;       // 0.0 to 1.0
    double accuracy_score;          // 0.0 to 1.0
    size_t total_records;
    size_t invalid_records;
    size_t missing_values;
    std::vector<std::string> quality_issues;
    std::string overall_assessment;
};

QualityReport assess_sierra_data_quality(const std::string& filename);

/**
 * Time series analysis helpers
 */
struct TimeSeriesInfo {
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::seconds total_duration;
    std::chrono::seconds average_interval;
    size_t gaps_detected;
    std::vector<std::pair<std::chrono::system_clock::time_point, std::chrono::seconds>> gaps;
    bool is_continuous;
};

TimeSeriesInfo analyze_time_series(const std::string& filename);

/**
 * Column mapping detection
 */
std::map<std::string, int> detect_sierra_column_mapping(const std::string& filename);

/**
 * Sample data extraction
 */
std::vector<std::map<std::string, std::string>> extract_sample_data(
    const std::string& filename,
    size_t num_samples = 10
);

} // namespace sierra_utils

} // namespace feeds
} // namespace backtrader