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
 * InfluxFeed - InfluxDB data feed
 * 
 * Provides time series data from InfluxDB database.
 * Supports both InfluxDB 1.x and 2.x APIs.
 */
class InfluxFeed : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::string host = "localhost";
        int port = 8086;
        std::string database;         // Database name (v1.x) or bucket (v2.x)
        std::string measurement;      // Measurement name
        std::string username;         // Username (optional)
        std::string password;         // Password (optional)
        std::string token;            // Token for v2.x
        std::string org;              // Organization for v2.x
        
        // Query parameters
        std::string start_time;       // Start time (RFC3339 or relative)
        std::string end_time;         // End time (RFC3339 or relative)
        std::string tag_filters;     // Tag filters (e.g., "symbol=AAPL")
        std::string field_filters;   // Field filters
        std::string group_by;         // Group by clause
        std::string aggregation;     // Aggregation function (mean, sum, etc.)
        std::string window;          // Time window for aggregation
        
        // Field mapping
        std::string datetime_field = "time";
        std::string open_field = "open";
        std::string high_field = "high";
        std::string low_field = "low";
        std::string close_field = "close";
        std::string volume_field = "volume";
        std::string openinterest_field = "openinterest";
        
        // Connection parameters
        bool use_ssl = false;
        bool verify_ssl = true;
        int timeout = 30;             // Timeout in seconds
        std::string version = "1.x";  // InfluxDB version (1.x or 2.x)
        
        // Data parameters
        bool real_time = false;       // Enable real-time querying
        int refresh_interval = 60;   // Refresh interval for real-time (seconds)
        int max_points = 10000;      // Maximum points per query
    };

    InfluxFeed(const Params& params);
    virtual ~InfluxFeed() = default;

    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;

    // InfluxDB-specific methods
    void set_database(const std::string& database);
    void set_measurement(const std::string& measurement);
    void set_time_range(const std::string& start_time, const std::string& end_time);
    void set_tag_filters(const std::string& tag_filters);
    
    // Query execution
    bool execute_query();
    void clear_cache();
    void enable_real_time();
    void disable_real_time();

    // Properties
    const std::string& get_database() const { return params_.database; }
    const std::string& get_measurement() const { return params_.measurement; }
    size_t get_data_size() const { return data_.size(); }
    bool is_real_time() const { return params_.real_time; }

private:
    // Parameters
    Params params_;
    
    // Data storage
    std::vector<std::map<std::string, std::any>> data_;
    size_t current_index_ = 0;
    
    // Real-time state
    std::chrono::system_clock::time_point last_query_time_;
    std::chrono::system_clock::time_point last_data_time_;
    
    // Internal methods
    void build_query_v1(std::string& query) const;
    void build_query_v2(std::string& query) const;
    void build_request_url(std::string& url) const;
    
    bool send_query_v1(const std::string& query, std::string& response);
    bool send_query_v2(const std::string& query, std::string& response);
    
    void parse_influx_response_v1(const std::string& response);
    void parse_influx_response_v2(const std::string& response);
    
    // Data processing
    void process_series_data(const std::map<std::string, std::any>& series);
    void process_data_point(const std::vector<std::any>& point, 
                           const std::vector<std::string>& columns);
    
    // Field extraction
    double extract_field_value(const std::map<std::string, std::any>& point, 
                              const std::string& field_name) const;
    std::chrono::system_clock::time_point extract_timestamp(
        const std::map<std::string, std::any>& point) const;
    
    // Query building helpers
    std::string build_time_filter() const;
    std::string build_tag_filter() const;
    std::string build_field_filter() const;
    std::string build_group_by_clause() const;
    std::string build_aggregation_clause() const;
    
    // Real-time querying
    void update_real_time_data();
    bool should_refresh_data() const;
    std::string get_last_timestamp_filter() const;
    
    // Validation
    bool validate_connection_params() const;
    bool validate_query_params() const;
    bool validate_field_mapping() const;
    
    // Error handling
    void handle_query_error(int http_code, const std::string& response);
    void handle_connection_error(const std::exception& e);
    void handle_parse_error(const std::string& error);
    
    // Time utilities
    std::string format_influx_time(const std::chrono::system_clock::time_point& time) const;
    std::chrono::system_clock::time_point parse_influx_time(const std::string& time_str) const;
    bool is_relative_time(const std::string& time_str) const;
    std::string resolve_relative_time(const std::string& relative_time) const;
    
    // HTTP client wrapper
    class InfluxClient {
    public:
        struct Response {
            int code;
            std::string body;
            std::map<std::string, std::string> headers;
        };
        
        InfluxClient(const std::string& host, int port, bool use_ssl);
        
        Response query_v1(const std::string& database, const std::string& query,
                         const std::string& username = "", const std::string& password = "");
        Response query_v2(const std::string& org, const std::string& bucket,
                         const std::string& query, const std::string& token);
        
        void set_timeout(int seconds);
        void set_verify_ssl(bool verify);
        
    private:
        std::string host_;
        int port_;
        bool use_ssl_;
        int timeout_;
        bool verify_ssl_;
    };
    
    std::unique_ptr<InfluxClient> client_;
    
    // Utility methods
    std::string escape_influx_string(const std::string& str) const;
    std::string build_base_url() const;
    std::map<std::string, std::string> get_auth_headers() const;
};

} // namespace feeds
} // namespace backtrader