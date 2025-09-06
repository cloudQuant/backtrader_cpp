#pragma once

#include "../feed.h"
#include "../timeframe.h"
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <fstream>
#include <filesystem>

namespace backtrader {
namespace feeds {

/**
 * VChartData - VisualChart Binary File Support
 * 
 * This class provides support for VisualChart binary on-disk files for both
 * daily and intraday formats. VisualChart stores market data in proprietary
 * binary formats optimized for fast access and compact storage.
 * 
 * File Format Support:
 * - .fd files: Daily data format (28 bytes per bar)
 * - .min files: Intraday data format (32 bytes per bar)
 * - Automatic format detection based on file extension
 * - Direct file and stream support
 * 
 * Binary Structure:
 * Daily (.fd): Date(4) + OHLCV(20) + OI(4) = 28 bytes
 * Intraday (.min): Date(4) + Time(4) + OHLCV(20) + OI(4) = 32 bytes
 * 
 * Features:
 * - High-performance binary data reading
 * - Memory-mapped file access for large files
 * - Automatic timeframe detection
 * - Date encoding compatible with VB/Excel
 * - Support for streaming and file-based access
 */
class VChartData : public AbstractDataBase {
public:
    // VisualChart binary file parameters
    struct VChartParams : public AbstractDataBase::Params {
        // File access options
        bool use_memory_mapping = false;       // Use memory mapping for large files
        bool validate_file_integrity = true;   // Validate file structure
        bool cache_file_info = true;           // Cache file metadata
        size_t read_buffer_size = 8192;        // Read buffer size in bytes
        
        // Data processing options
        bool strict_date_validation = true;    // Strict date format validation
        bool auto_detect_timeframe = true;     // Auto-detect from file extension
        bool validate_ohlc = true;             // Validate OHLC relationships
        bool skip_invalid_bars = true;         // Skip bars with invalid data
        
        // Performance options
        bool preload_all_data = false;         // Load entire file into memory
        bool enable_compression = false;       // Enable data compression (if supported)
        int max_bars_in_memory = 100000;      // Maximum bars to keep in memory
        
        // Date handling
        bool adjust_timezone = false;          // Adjust for timezone differences
        std::string timezone = "UTC";          // Target timezone
        bool use_market_timezone = false;      // Use market-specific timezone
        
        // Quality control
        double min_volume = 0.0;               // Minimum volume filter
        double max_price_deviation = 10.0;     // Maximum price deviation (%)
        bool filter_outliers = false;          // Filter price outliers
        bool validate_sequence = true;         // Validate chronological sequence
    };
    
    // Binary file formats
    enum class FileFormat {
        UNKNOWN,
        DAILY_FD,      // .fd format for daily data
        INTRADAY_MIN   // .min format for intraday data
    };
    
    // Constructor variants
    VChartData(const VChartParams& params = VChartParams{});
    VChartData(const std::string& filename, const VChartParams& params = VChartParams{});
    VChartData(std::shared_ptr<std::istream> stream, 
               TimeFrame timeframe,
               const VChartParams& params = VChartParams{});
    virtual ~VChartData() = default;
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // File format detection
    FileFormat detect_file_format() const;
    std::string get_format_description() const;
    bool is_valid_vchart_file() const;
    
    // File information
    struct FileInfo {
        std::string filename;
        FileFormat format;
        size_t file_size_bytes;
        size_t estimated_bars;
        size_t bar_size_bytes;
        TimeFrame timeframe;
        std::chrono::system_clock::time_point creation_time;
        std::chrono::system_clock::time_point modification_time;
        bool is_memory_mapped;
        std::string file_version;
    };
    
    FileInfo get_file_info() const;
    
    // Data access statistics
    struct AccessStatistics {
        size_t bars_read = 0;
        size_t bytes_read = 0;
        size_t invalid_bars = 0;
        size_t skipped_bars = 0;
        std::chrono::milliseconds total_read_time{0};
        double average_read_speed_bars_per_sec = 0.0;
        size_t memory_usage_bytes = 0;
        std::string performance_category;
    };
    
    AccessStatistics get_statistics() const { return statistics_; }
    
    // Memory management
    void enable_memory_mapping(bool enable = true);
    void set_buffer_size(size_t size);
    void preload_file_data();
    void clear_cache();
    size_t get_memory_usage() const;
    
    // Data validation
    struct ValidationResult {
        bool is_valid_file;
        size_t total_bars;
        size_t valid_bars;
        size_t invalid_bars;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::string integrity_status;
    };
    
    ValidationResult validate_file() const;
    
    // File conversion utilities
    bool export_to_csv(const std::string& output_file) const;
    bool export_to_json(const std::string& output_file) const;
    
    // Date range information
    std::chrono::system_clock::time_point get_first_date() const;
    std::chrono::system_clock::time_point get_last_date() const;
    std::chrono::seconds get_date_range() const;
    
protected:
    VChartParams params_;
    
    // File handling
    std::shared_ptr<std::istream> file_stream_;
    std::string filename_;
    FileFormat file_format_ = FileFormat::UNKNOWN;
    
    // Binary format specifics
    size_t bar_size_ = 0;               // Size of each bar in bytes
    size_t datetime_size_ = 0;          // Size of datetime field (1 or 2 ints)
    std::string bar_format_;            // Binary format string
    
    // Performance tracking
    mutable AccessStatistics statistics_;
    std::chrono::system_clock::time_point start_time_;
    
    // Memory mapping
    bool memory_mapped_ = false;
    const char* mapped_data_ = nullptr;
    size_t mapped_size_ = 0;
    size_t current_position_ = 0;
    
    // Data validation
    std::chrono::system_clock::time_point last_datetime_;
    double last_close_price_ = 0.0;
    
private:
    // Initialization
    void initialize_file_format();
    void setup_binary_format();
    void setup_memory_mapping();
    
    // Data reading
    bool read_next_bar();
    bool read_bar_from_stream();
    bool read_bar_from_memory();
    
    // Binary data parsing
    std::chrono::system_clock::time_point parse_vchart_date(uint32_t vc_date) const;
    std::chrono::system_clock::time_point parse_vchart_datetime(
        uint32_t vc_date, 
        uint32_t vc_time) const;
    
    // Date conversion utilities
    uint32_t convert_to_vchart_date(const std::chrono::system_clock::time_point& dt) const;
    std::chrono::system_clock::time_point vchart_epoch_to_datetime(uint32_t days) const;
    
    // Data validation
    bool validate_bar_data(double open, double high, double low, double close, 
                          double volume, double openinterest) const;
    bool is_valid_date(uint32_t vc_date) const;
    bool is_valid_time(uint32_t vc_time) const;
    bool is_chronological(const std::chrono::system_clock::time_point& dt) const;
    
    // Error handling
    void handle_read_error(const std::string& error);
    void log_validation_warning(const std::string& warning);
    
    // Statistics updates
    void update_statistics();
    void calculate_performance_metrics();
    
    // Memory management
    void cleanup_memory_mapping();
    void allocate_read_buffer();
    
    // File integrity
    bool check_file_header() const;
    bool verify_file_size() const;
    size_t calculate_expected_bars() const;
};

/**
 * VChartFeed - Factory class for VisualChart binary feeds
 * 
 * Provides convenient factory methods for creating VChart data feeds
 * with automatic path resolution and format detection.
 */
class VChartFeed {
public:
    // Factory parameters
    struct FeedParams {
        std::string base_path;              // Base directory for VC data files
        bool auto_resolve_paths = true;     // Auto-resolve relative paths
        bool validate_paths = true;         // Validate file paths exist
        bool cache_file_handles = false;    // Cache file handles
        VChartData::VChartParams data_params; // Data-specific parameters
    };
    
    VChartFeed(const FeedParams& params = FeedParams{});
    virtual ~VChartFeed() = default;
    
    // Data creation methods
    std::shared_ptr<VChartData> get_data(
        const std::string& dataname,
        const VChartData::VChartParams& params = VChartData::VChartParams{}
    );
    
    std::shared_ptr<VChartData> create_daily_feed(const std::string& symbol);
    std::shared_ptr<VChartData> create_intraday_feed(const std::string& symbol);
    
    // Path resolution
    std::string resolve_data_path(const std::string& dataname) const;
    bool validate_data_path(const std::string& path) const;
    
    // Batch operations
    std::vector<std::shared_ptr<VChartData>> create_multiple_feeds(
        const std::vector<std::string>& symbols
    );
    
    // Configuration
    void set_base_path(const std::string& path);
    std::string get_base_path() const { return params_.base_path; }
    
protected:
    FeedParams params_;
    
private:
    // Path utilities
    std::string build_data_path(const std::string& dataname) const;
    std::string extract_main_code(const std::string& dataname) const;
    std::string extract_sub_code(const std::string& dataname) const;
    
    // File system operations
    bool ensure_directory_exists(const std::string& path) const;
    std::vector<std::string> find_matching_files(const std::string& pattern) const;
};

/**
 * Specialized VisualChart binary feeds
 */

/**
 * High-performance VisualChart feed optimized for backtesting
 */
class VChartHighPerformanceData : public VChartData {
public:
    struct HighPerfParams : public VChartParams {
        bool aggressive_caching = true;     // Aggressive memory caching
        bool parallel_reading = false;      // Parallel file reading (if supported)
        size_t chunk_size = 10000;          // Number of bars per chunk
        bool disable_validation = false;    // Disable data validation for speed
        bool memory_pool = true;            // Use memory pool for allocations
    };
    
    VChartHighPerformanceData(const HighPerfParams& params = HighPerfParams{});
    VChartHighPerformanceData(const std::string& filename, 
                             const HighPerfParams& params = HighPerfParams{});
    
    // Performance-specific methods
    void enable_parallel_processing(bool enable = true);
    void set_chunk_processing(size_t chunk_size);
    
    // Batch data access
    std::vector<MarketData> read_chunk(size_t num_bars);
    bool preload_chunks(size_t num_chunks);
    
protected:
    HighPerfParams high_perf_params_;
    
    // Performance optimization
    std::vector<std::vector<MarketData>> data_chunks_;
    size_t current_chunk_index_ = 0;
    size_t current_bar_index_ = 0;
    
private:
    void optimize_for_performance();
    void load_data_chunks();
};

/**
 * Memory-efficient VisualChart feed for large files
 */
class VChartMemoryEfficientData : public VChartData {
public:
    struct MemoryEfficientParams : public VChartParams {
        size_t max_memory_mb = 50;          // Maximum memory usage
        bool use_streaming = true;          // Use streaming mode
        bool lazy_loading = true;           // Load data on demand
        size_t sliding_window_size = 1000;  // Sliding window for recent data
        bool compress_old_data = false;     // Compress old data in memory
    };
    
    VChartMemoryEfficientData(const MemoryEfficientParams& params = MemoryEfficientParams{});
    VChartMemoryEfficientData(const std::string& filename,
                             const MemoryEfficientParams& params = MemoryEfficientParams{});
    
    // Memory management
    void set_memory_limit(size_t mb);
    void enable_data_compression(bool enable = true);
    size_t get_current_memory_usage() const;
    
protected:
    MemoryEfficientParams memory_params_;
    
    // Memory management
    std::deque<MarketData> sliding_window_;
    size_t memory_usage_bytes_ = 0;
    
private:
    void manage_memory_usage();
    void evict_old_data();
    void compress_data_if_needed();
};

/**
 * Factory functions for VisualChart binary feeds
 */
namespace vchart_factory {

/**
 * Create standard VisualChart binary feed
 */
std::shared_ptr<VChartData> create_vchart_feed(
    const std::string& filename,
    const VChartData::VChartParams& params = VChartData::VChartParams{}
);

/**
 * Create high-performance VisualChart feed
 */
std::shared_ptr<VChartHighPerformanceData> create_high_performance_feed(
    const std::string& filename,
    bool enable_aggressive_caching = true
);

/**
 * Create memory-efficient VisualChart feed
 */
std::shared_ptr<VChartMemoryEfficientData> create_memory_efficient_feed(
    const std::string& filename,
    size_t max_memory_mb = 50
);

/**
 * Create VisualChart feed with auto-optimization
 */
std::shared_ptr<VChartData> create_optimized_feed(
    const std::string& filename,
    const std::string& optimization_target = "balanced"  // "speed", "memory", "balanced"
);

/**
 * Batch create feeds from directory
 */
std::vector<std::shared_ptr<VChartData>> create_feeds_from_directory(
    const std::string& directory_path,
    const std::string& file_pattern = "*"
);

/**
 * Create feed factory with custom base path
 */
std::unique_ptr<VChartFeed> create_feed_factory(
    const std::string& base_path,
    const VChartFeed::FeedParams& params = VChartFeed::FeedParams{}
);

} // namespace vchart_factory

/**
 * Utility functions for VisualChart binary files
 */
namespace vchart_utils {

/**
 * File format utilities
 */
VChartData::FileFormat detect_file_format(const std::string& filename);
bool is_valid_vchart_file(const std::string& filename);
std::string get_format_description(VChartData::FileFormat format);

/**
 * File information
 */
struct FileMetadata {
    std::string filename;
    VChartData::FileFormat format;
    size_t file_size;
    size_t estimated_bars;
    TimeFrame timeframe;
    std::chrono::system_clock::time_point first_date;
    std::chrono::system_clock::time_point last_date;
    std::string symbol_hint;
};

FileMetadata analyze_vchart_file(const std::string& filename);

/**
 * Batch file operations
 */
std::vector<FileMetadata> analyze_directory(const std::string& directory_path);
std::vector<std::string> find_vchart_files(
    const std::string& directory_path,
    VChartData::FileFormat format = VChartData::FileFormat::UNKNOWN
);

/**
 * File conversion
 */
bool convert_to_csv(const std::string& vchart_file, const std::string& csv_file);
bool convert_to_json(const std::string& vchart_file, const std::string& json_file);
bool merge_vchart_files(
    const std::vector<std::string>& input_files,
    const std::string& output_file
);

/**
 * Data validation
 */
struct ValidationReport {
    bool is_valid;
    size_t total_bars;
    size_t valid_bars;
    size_t corrupted_bars;
    std::vector<std::string> issues;
    std::string integrity_assessment;
};

ValidationReport validate_vchart_file(const std::string& filename);
bool repair_vchart_file(const std::string& input_file, const std::string& output_file);

/**
 * Performance analysis
 */
struct PerformanceProfile {
    double read_speed_mb_per_sec;
    double processing_speed_bars_per_sec;
    size_t memory_footprint_mb;
    std::chrono::milliseconds startup_time;
    std::string bottleneck_analysis;
    std::vector<std::string> optimization_suggestions;
};

PerformanceProfile profile_performance(const std::string& filename);

/**
 * Data statistics
 */
struct DataSummary {
    size_t total_bars;
    std::chrono::system_clock::time_point date_range_start;
    std::chrono::system_clock::time_point date_range_end;
    double price_range_min;
    double price_range_max;
    double average_volume;
    double total_volume;
    std::string data_characteristics;
};

DataSummary summarize_data(const std::string& filename);

/**
 * File maintenance
 */
bool optimize_file_layout(const std::string& input_file, const std::string& output_file);
bool compress_vchart_file(const std::string& input_file, const std::string& output_file);
bool defragment_vchart_file(const std::string& filename);

/**
 * Binary format documentation
 */
std::string get_format_specification();
std::map<std::string, std::string> get_field_definitions();
std::vector<std::string> get_supported_extensions();

} // namespace vchart_utils

} // namespace feeds
} // namespace backtrader