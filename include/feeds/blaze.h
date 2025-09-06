#pragma once

#include "../feed.h"
#include <iterator>
#include <memory>
#include <vector>
#include <any>
#include <functional>
#include <chrono>
#include <string>
#include <map>

namespace backtrader {
namespace feeds {

/**
 * BlazeData - Support for Blaze Data Objects
 * 
 * This class provides support for Blaze (https://blaze.pydata.org) Data objects
 * in C++. Blaze is a library that provides a unified interface to various data
 * sources and formats.
 * 
 * The BlazeData feed allows integration with:
 * - CSV files through Blaze
 * - SQL databases through Blaze
 * - HDF5 files through Blaze
 * - JSON data through Blaze
 * - Remote data sources through Blaze
 * 
 * Features:
 * - Automatic data type inference
 * - Column index mapping
 * - Iterator-based data access
 * - Flexible datetime handling
 * - Configurable column mapping
 * 
 * Note: Only numeric indices to columns are supported.
 * A negative value in any of the parameters for the Data lines indicates
 * it's not present in the data source.
 */
class BlazeData : public AbstractDataBase {
public:
    // Forward declaration for Blaze data abstraction
    class BlazeDataSource;
    
    // Parameters structure
    struct Params : public AbstractDataBase::Params {
        // Column indices for standard OHLCV data
        int datetime = 0;           // datetime column index (required)
        int open = 1;              // open price column index (-1 if not present)
        int high = 2;              // high price column index (-1 if not present)
        int low = 3;               // low price column index (-1 if not present)
        int close = 4;             // close price column index (-1 if not present)
        int volume = 5;            // volume column index (-1 if not present)
        int openinterest = 6;      // open interest column index (-1 if not present)
        
        // Additional configuration
        std::string datetime_format = "";  // Custom datetime format if needed
        bool auto_detect_columns = true;   // Auto-detect column layout
        bool cache_data = false;           // Cache all data in memory
        size_t max_cache_size = 10000;     // Maximum cache size
    };
    
    // Column data fields
    static const std::vector<std::string> datafields;
    
    // Constructor variants
    BlazeData(const Params& params = Params{});
    BlazeData(std::shared_ptr<BlazeDataSource> data_source, const Params& params = Params{});
    virtual ~BlazeData() = default;
    
    // Data source management
    void set_data_source(std::shared_ptr<BlazeDataSource> data_source);
    std::shared_ptr<BlazeDataSource> get_data_source() const { return data_source_; }
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // Column mapping
    void set_column_mapping(const std::map<std::string, int>& mapping);
    std::map<std::string, int> get_column_mapping() const;
    
    // Auto-detection
    bool auto_detect_column_layout();
    void print_column_info() const;
    
    // Data access
    size_t get_total_rows() const;
    bool has_more_data() const;
    void reset_iterator();
    
    // Statistics
    struct DataStats {
        size_t total_rows;
        size_t processed_rows;
        size_t skipped_rows;
        size_t error_rows;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        std::vector<std::string> column_names;
        std::vector<std::string> column_types;
    };
    
    DataStats get_data_statistics() const;
    
protected:
    Params params_;
    std::shared_ptr<BlazeDataSource> data_source_;
    
    // Data processing state
    size_t current_row_index_ = 0;
    size_t total_rows_ = 0;
    bool data_exhausted_ = false;
    
    // Statistics tracking
    mutable DataStats stats_;
    
    // Data cache
    std::vector<std::vector<std::any>> cached_data_;
    bool cache_loaded_ = false;
    
private:
    bool load_next_row();
    bool process_row_data(const std::vector<std::any>& row_data);
    void update_statistics();
    
    // Data conversion helpers
    double convert_to_double(const std::any& value) const;
    std::chrono::system_clock::time_point convert_to_datetime(const std::any& value) const;
    
    // Column mapping helpers
    int get_column_index(const std::string& field_name) const;
    bool is_column_present(const std::string& field_name) const;
    
    // Cache management
    void load_all_data_to_cache();
    void clear_cache();
    
    // Validation
    bool validate_data_source() const;
    bool validate_column_mapping() const;
};

/**
 * BlazeDataSource - Abstract interface for Blaze data sources
 * 
 * This class provides an abstraction layer for different types of data sources
 * that can be accessed through the Blaze ecosystem.
 */
class BlazeData::BlazeDataSource {
public:
    virtual ~BlazeDataSource() = default;
    
    // Core data access interface
    virtual bool has_next() const = 0;
    virtual std::vector<std::any> get_next_row() = 0;
    virtual void reset() = 0;
    virtual size_t get_total_rows() const = 0;
    
    // Schema information
    virtual std::vector<std::string> get_column_names() const = 0;
    virtual std::vector<std::string> get_column_types() const = 0;
    virtual std::map<std::string, int> get_column_mapping() const = 0;
    
    // Data source information
    virtual std::string get_source_type() const = 0;
    virtual std::string get_source_description() const = 0;
    
    // Configuration
    virtual void set_parameters(const std::map<std::string, std::any>& params) = 0;
    virtual std::map<std::string, std::any> get_parameters() const = 0;
    
    // Status
    virtual bool is_connected() const = 0;
    virtual bool is_live() const { return false; }
    virtual void connect() = 0;
    virtual void disconnect() = 0;
};

/**
 * CSV-based Blaze data source
 */
class BlazeCSVDataSource : public BlazeData::BlazeDataSource {
public:
    struct CSVParams {
        std::string file_path;
        std::string delimiter = ",";
        bool has_header = true;
        std::string datetime_format = "";
        std::vector<std::string> column_names;
        size_t skip_rows = 0;
        size_t max_rows = 0;  // 0 means no limit
    };
    
    explicit BlazeCSVDataSource(const CSVParams& params);
    virtual ~BlazeCSVDataSource() = default;
    
    // BlazeDataSource interface
    bool has_next() const override;
    std::vector<std::any> get_next_row() override;
    void reset() override;
    size_t get_total_rows() const override;
    
    std::vector<std::string> get_column_names() const override;
    std::vector<std::string> get_column_types() const override;
    std::map<std::string, int> get_column_mapping() const override;
    
    std::string get_source_type() const override { return "CSV"; }
    std::string get_source_description() const override;
    
    void set_parameters(const std::map<std::string, std::any>& params) override;
    std::map<std::string, std::any> get_parameters() const override;
    
    bool is_connected() const override { return is_loaded_; }
    void connect() override;
    void disconnect() override;
    
private:
    CSVParams params_;
    std::vector<std::vector<std::string>> data_rows_;
    std::vector<std::string> column_names_;
    std::vector<std::string> column_types_;
    size_t current_row_index_ = 0;
    bool is_loaded_ = false;
    
    void load_csv_file();
    void parse_header();
    void detect_column_types();
    std::vector<std::string> split_csv_line(const std::string& line) const;
    std::any parse_cell_value(const std::string& cell, const std::string& column_type) const;
};

/**
 * SQL-based Blaze data source
 */
class BlazeSQLDataSource : public BlazeData::BlazeDataSource {
public:
    struct SQLParams {
        std::string connection_string;
        std::string query;
        std::string table_name;
        std::vector<std::string> column_names;
        std::map<std::string, std::string> connection_params;
        bool stream_results = false;
        size_t batch_size = 1000;
    };
    
    explicit BlazeSQLDataSource(const SQLParams& params);
    virtual ~BlazeSQLDataSource() = default;
    
    // BlazeDataSource interface
    bool has_next() const override;
    std::vector<std::any> get_next_row() override;
    void reset() override;
    size_t get_total_rows() const override;
    
    std::vector<std::string> get_column_names() const override;
    std::vector<std::string> get_column_types() const override;
    std::map<std::string, int> get_column_mapping() const override;
    
    std::string get_source_type() const override { return "SQL"; }
    std::string get_source_description() const override;
    
    void set_parameters(const std::map<std::string, std::any>& params) override;
    std::map<std::string, std::any> get_parameters() const override;
    
    bool is_connected() const override { return is_connected_; }
    void connect() override;
    void disconnect() override;
    
private:
    SQLParams params_;
    std::vector<std::vector<std::any>> result_rows_;
    std::vector<std::string> column_names_;
    std::vector<std::string> column_types_;
    size_t current_row_index_ = 0;
    bool is_connected_ = false;
    bool query_executed_ = false;
    
    void execute_query();
    void fetch_schema_info();
    void fetch_all_results();
};

/**
 * HDF5-based Blaze data source
 */
class BlazeHDF5DataSource : public BlazeData::BlazeDataSource {
public:
    struct HDF5Params {
        std::string file_path;
        std::string dataset_path;
        std::string group_name;
        std::vector<std::string> column_names;
        size_t chunk_size = 1000;
        bool use_compression = false;
    };
    
    explicit BlazeHDF5DataSource(const HDF5Params& params);
    virtual ~BlazeHDF5DataSource() = default;
    
    // BlazeDataSource interface
    bool has_next() const override;
    std::vector<std::any> get_next_row() override;
    void reset() override;
    size_t get_total_rows() const override;
    
    std::vector<std::string> get_column_names() const override;
    std::vector<std::string> get_column_types() const override;
    std::map<std::string, int> get_column_mapping() const override;
    
    std::string get_source_type() const override { return "HDF5"; }
    std::string get_source_description() const override;
    
    void set_parameters(const std::map<std::string, std::any>& params) override;
    std::map<std::string, std::any> get_parameters() const override;
    
    bool is_connected() const override { return is_connected_; }
    void connect() override;
    void disconnect() override;
    
private:
    HDF5Params params_;
    std::vector<std::vector<std::any>> data_chunks_;
    std::vector<std::string> column_names_;
    std::vector<std::string> column_types_;
    size_t current_row_index_ = 0;
    size_t current_chunk_index_ = 0;
    bool is_connected_ = false;
    
    void load_hdf5_file();
    void read_dataset_schema();
    void read_data_chunks();
};

/**
 * Factory functions for creating Blaze data sources
 */
namespace blaze_factory {

/**
 * Create a CSV-based Blaze data source
 */
std::shared_ptr<BlazeData::BlazeDataSource> create_csv_source(
    const std::string& file_path,
    const std::string& delimiter = ",",
    bool has_header = true
);

/**
 * Create an SQL-based Blaze data source
 */
std::shared_ptr<BlazeData::BlazeDataSource> create_sql_source(
    const std::string& connection_string,
    const std::string& query
);

/**
 * Create an HDF5-based Blaze data source
 */
std::shared_ptr<BlazeData::BlazeDataSource> create_hdf5_source(
    const std::string& file_path,
    const std::string& dataset_path
);

/**
 * Create a Blaze data feed from a data source
 */
std::shared_ptr<BlazeData> create_blaze_feed(
    std::shared_ptr<BlazeData::BlazeDataSource> data_source,
    const BlazeData::Params& params = BlazeData::Params{}
);

/**
 * Create a Blaze data feed directly from a CSV file
 */
std::shared_ptr<BlazeData> create_csv_feed(
    const std::string& file_path,
    const BlazeData::Params& params = BlazeData::Params{}
);

/**
 * Create a Blaze data feed directly from an SQL query
 */
std::shared_ptr<BlazeData> create_sql_feed(
    const std::string& connection_string,
    const std::string& query,
    const BlazeData::Params& params = BlazeData::Params{}
);

/**
 * Create a Blaze data feed directly from an HDF5 file
 */
std::shared_ptr<BlazeData> create_hdf5_feed(
    const std::string& file_path,
    const std::string& dataset_path,
    const BlazeData::Params& params = BlazeData::Params{}
);

} // namespace blaze_factory

/**
 * Utility functions for Blaze data processing
 */
namespace blaze_utils {

/**
 * Auto-detect the optimal column mapping for a data source
 */
std::map<std::string, int> auto_detect_column_mapping(
    std::shared_ptr<BlazeData::BlazeDataSource> data_source
);

/**
 * Validate data source compatibility
 */
struct ValidationResult {
    bool is_valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::string recommendation;
};

ValidationResult validate_data_source(
    std::shared_ptr<BlazeData::BlazeDataSource> data_source
);

/**
 * Performance analysis for data source
 */
struct PerformanceAnalysis {
    double rows_per_second;
    std::chrono::milliseconds average_row_time;
    size_t memory_usage_bytes;
    std::string performance_category;
    std::vector<std::string> optimization_suggestions;
};

PerformanceAnalysis analyze_data_source_performance(
    std::shared_ptr<BlazeData::BlazeDataSource> data_source,
    size_t test_rows = 1000
);

/**
 * Data quality assessment
 */
struct QualityMetrics {
    double completeness_score;  // 0.0 to 1.0
    double consistency_score;   // 0.0 to 1.0
    size_t null_count;
    size_t duplicate_count;
    std::vector<std::string> quality_issues;
    std::string overall_assessment;
};

QualityMetrics assess_data_quality(
    std::shared_ptr<BlazeData::BlazeDataSource> data_source
);

/**
 * Convert between different data source types
 */
std::shared_ptr<BlazeData::BlazeDataSource> convert_data_source(
    std::shared_ptr<BlazeData::BlazeDataSource> source,
    const std::string& target_type,
    const std::map<std::string, std::any>& conversion_params = {}
);

/**
 * Optimize data source for specific use cases
 */
struct OptimizationResult {
    std::shared_ptr<BlazeData::BlazeDataSource> optimized_source;
    std::string optimization_type;
    double performance_improvement;
    std::string description;
};

OptimizationResult optimize_data_source(
    std::shared_ptr<BlazeData::BlazeDataSource> source,
    const std::string& optimization_type = "speed"
);

/**
 * Data source comparison utilities
 */
struct ComparisonResult {
    bool sources_compatible;
    std::vector<std::string> differences;
    std::string recommended_source;
    std::string comparison_summary;
};

ComparisonResult compare_data_sources(
    const std::vector<std::shared_ptr<BlazeData::BlazeDataSource>>& sources
);

} // namespace blaze_utils

} // namespace feeds
} // namespace backtrader