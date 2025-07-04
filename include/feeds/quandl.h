#pragma once

#include "../feed.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <map>

namespace backtrader {
namespace feeds {

/**
 * QuandlFeed - Quandl data feed
 * 
 * Provides historical financial and economic data from Quandl.
 * Supports various data formats and frequencies.
 */
class QuandlFeed : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::string dataset_code;     // Quandl dataset code (e.g., "WIKI/AAPL")
        std::string api_key;          // Quandl API key
        std::string start_date;       // Start date (YYYY-MM-DD)
        std::string end_date;         // End date (YYYY-MM-DD)
        std::string collapse;         // Data frequency (daily, weekly, monthly, etc.)
        std::string transform;        // Data transformation (diff, rdiff, cumul, normalize)
        int rows = 0;                 // Number of rows to return (0 = all)
        std::string order = "asc";    // Sort order (asc, desc)
        
        // Data format parameters
        std::string format = "json";  // Response format (json, csv)
        bool reverse = false;         // Reverse chronological order
        
        // Column mapping (for datasets with non-standard column names)
        std::map<std::string, std::string> column_mapping;
    };

    QuandlFeed(const Params& params);
    virtual ~QuandlFeed() = default;

    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;

    // Quandl-specific methods
    void set_dataset_code(const std::string& dataset_code);
    void set_api_key(const std::string& api_key);
    void set_date_range(const std::string& start_date, const std::string& end_date);
    void set_collapse(const std::string& collapse);
    
    // Data fetching
    bool fetch_data();
    void clear_cache();

    // Properties
    const std::string& get_dataset_code() const { return params_.dataset_code; }
    const std::string& get_collapse() const { return params_.collapse; }
    size_t get_data_size() const { return data_.size(); }

private:
    // Parameters
    Params params_;
    
    // Data storage
    std::vector<std::map<std::string, std::any>> data_;
    size_t current_index_ = 0;
    
    // Column indices for OHLCV data
    int date_col_ = 0;
    int open_col_ = 1;
    int high_col_ = 2;
    int low_col_ = 3;
    int close_col_ = 4;
    int volume_col_ = 5;
    int adj_close_col_ = -1;  // Optional
    
    // Internal methods
    void build_request_url(std::string& url) const;
    bool send_request(const std::string& url, std::string& response);
    void parse_json_response(const std::string& response);
    void parse_csv_response(const std::string& response);
    
    // Data processing
    void detect_column_mapping(const std::vector<std::string>& headers);
    void process_data_row(const std::vector<std::string>& row);
    void validate_data();
    
    // Column mapping
    void auto_detect_columns(const std::vector<std::string>& headers);
    void apply_column_mapping();
    int find_column_index(const std::vector<std::string>& headers, 
                         const std::vector<std::string>& possible_names) const;
    
    // Data conversion
    double parse_numeric_value(const std::string& value) const;
    std::chrono::system_clock::time_point parse_date(const std::string& date_str) const;
    
    // Data transformation
    void apply_transformations();
    void apply_diff_transform();
    void apply_rdiff_transform();
    void apply_cumul_transform();
    void apply_normalize_transform();
    
    // Validation
    bool validate_dataset_code() const;
    bool validate_date_format(const std::string& date) const;
    bool validate_collapse_value() const;
    bool has_required_columns() const;
    
    // Error handling
    void handle_request_error(int http_code, const std::string& response);
    void handle_parse_error(const std::string& error);
    
    // Utility methods
    std::string format_date_for_quandl(const std::chrono::system_clock::time_point& date) const;
    std::string url_encode(const std::string& value) const;
    
    // HTTP client wrapper
    class HttpClient {
    public:
        struct Response {
            int code;
            std::string body;
            std::map<std::string, std::string> headers;
        };
        
        static Response get(const std::string& url);
        static void set_user_agent(const std::string& user_agent);
        static void set_timeout(int seconds);
    };
    
    // Default column name mappings for common datasets
    static std::map<std::string, std::vector<std::string>> create_column_mappings();
    static const std::map<std::string, std::vector<std::string>> default_column_names_;
};

} // namespace feeds
} // namespace backtrader