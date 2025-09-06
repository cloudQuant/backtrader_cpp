#include "../../include/feeds/influxfeed.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <ctime>
#include <chrono>

namespace backtrader {
namespace feeds {

InfluxFeed::InfluxFeed(const Params& params) : params_(params) {
    // Initialize HTTP client
    client_ = std::make_unique<InfluxClient>(params_.host, params_.port, params_.use_ssl);
    client_->set_timeout(params_.timeout);
    client_->set_verify_ssl(params_.verify_ssl);
    
    // Validate parameters
    if (!validate_connection_params()) {
        throw std::invalid_argument("Invalid connection parameters");
    }
    
    if (!validate_query_params()) {
        throw std::invalid_argument("Invalid query parameters");
    }
    
    if (!validate_field_mapping()) {
        throw std::invalid_argument("Invalid field mapping");
    }
}

void InfluxFeed::start() {
    std::cout << "Starting InfluxFeed for measurement: " << params_.measurement << std::endl;
    
    // Execute initial query to load data
    if (!execute_query()) {
        throw std::runtime_error("Failed to execute initial query");
    }
    
    last_query_time_ = std::chrono::system_clock::now();
}

void InfluxFeed::stop() {
    std::cout << "Stopping InfluxFeed" << std::endl;
    clear_cache();
}

bool InfluxFeed::next() {
    // Check if we need to refresh data in real-time mode
    if (params_.real_time && should_refresh_data()) {
        update_real_time_data();
    }
    
    // Return next data point if available
    if (current_index_ < data_.size()) {
        const auto& point = data_[current_index_];
        
        // Convert to backtrader format
        std::vector<double> bar_data;
        
        try {
            // Extract timestamp
            auto timestamp = extract_timestamp(point);
            auto time_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                timestamp.time_since_epoch()).count();
            
            // Extract OHLCV data
            double open = extract_field_value(point, params_.open_field);
            double high = extract_field_value(point, params_.high_field);
            double low = extract_field_value(point, params_.low_field);
            double close = extract_field_value(point, params_.close_field);
            double volume = extract_field_value(point, params_.volume_field);
            double openinterest = extract_field_value(point, params_.openinterest_field);
            
            bar_data = {
                static_cast<double>(time_seconds),
                open, high, low, close, volume, openinterest
            };
            
            // Update lines with new data
            update_lines(bar_data);
            
            current_index_++;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error processing data point: " << e.what() << std::endl;
            current_index_++;
            return false;
        }
    }
    
    return false;
}

void InfluxFeed::preload() {
    // InfluxFeed loads all data during start()
    // This method can be used to pre-fetch additional data if needed
    if (data_.empty()) {
        execute_query();
    }
}

void InfluxFeed::set_database(const std::string& database) {
    params_.database = database;
}

void InfluxFeed::set_measurement(const std::string& measurement) {
    params_.measurement = measurement;
    clear_cache();  // Clear cache when measurement changes
}

void InfluxFeed::set_time_range(const std::string& start_time, const std::string& end_time) {
    params_.start_time = start_time;
    params_.end_time = end_time;
    clear_cache();  // Clear cache when time range changes
}

void InfluxFeed::set_tag_filters(const std::string& tag_filters) {
    params_.tag_filters = tag_filters;
    clear_cache();  // Clear cache when filters change
}

bool InfluxFeed::execute_query() {
    try {
        std::string query;
        std::string response;
        bool success = false;
        
        if (params_.version == "2.x") {
            build_query_v2(query);
            auto resp = client_->query_v2(params_.org, params_.database, query, params_.token);
            response = resp.body;
            success = (resp.code == 200);
            
            if (!success) {
                handle_query_error(resp.code, response);
                return false;
            }
            
            parse_influx_response_v2(response);
        } else {
            build_query_v1(query);
            auto resp = client_->query_v1(params_.database, query, params_.username, params_.password);
            response = resp.body;
            success = (resp.code == 200);
            
            if (!success) {
                handle_query_error(resp.code, response);
                return false;
            }
            
            parse_influx_response_v1(response);
        }
        
        std::cout << "Query executed successfully. Retrieved " << data_.size() << " data points." << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        handle_connection_error(e);
        return false;
    }
}

void InfluxFeed::clear_cache() {
    data_.clear();
    current_index_ = 0;
}

void InfluxFeed::enable_real_time() {
    params_.real_time = true;
    last_query_time_ = std::chrono::system_clock::now();
}

void InfluxFeed::disable_real_time() {
    params_.real_time = false;
}

void InfluxFeed::build_query_v1(std::string& query) const {
    std::ostringstream oss;
    
    // Build SELECT clause
    if (params_.aggregation.empty()) {
        oss << "SELECT ";
        if (!params_.field_filters.empty()) {
            oss << params_.field_filters;
        } else {
            // Select all OHLCV fields
            oss << params_.open_field << "," << params_.high_field << ","
                << params_.low_field << "," << params_.close_field << ","
                << params_.volume_field;
            
            if (!params_.openinterest_field.empty()) {
                oss << "," << params_.openinterest_field;
            }
        }
    } else {
        oss << "SELECT " << params_.aggregation << "(*)";
    }
    
    // FROM clause
    oss << " FROM " << escape_influx_string(params_.measurement);
    
    // WHERE clause
    std::vector<std::string> where_conditions;
    
    // Time filter
    std::string time_filter = build_time_filter();
    if (!time_filter.empty()) {
        where_conditions.push_back(time_filter);
    }
    
    // Tag filters
    std::string tag_filter = build_tag_filter();
    if (!tag_filter.empty()) {
        where_conditions.push_back(tag_filter);
    }
    
    if (!where_conditions.empty()) {
        oss << " WHERE ";
        for (size_t i = 0; i < where_conditions.size(); ++i) {
            if (i > 0) oss << " AND ";
            oss << where_conditions[i];
        }
    }
    
    // GROUP BY clause
    std::string group_by = build_group_by_clause();
    if (!group_by.empty()) {
        oss << " GROUP BY " << group_by;
    }
    
    // ORDER BY time
    oss << " ORDER BY time ASC";
    
    // LIMIT
    if (params_.max_points > 0) {
        oss << " LIMIT " << params_.max_points;
    }
    
    query = oss.str();
}

void InfluxFeed::build_query_v2(std::string& query) const {
    std::ostringstream oss;
    
    // Build Flux query for InfluxDB 2.x
    oss << "from(bucket: \"" << params_.database << "\")";
    
    // Time range filter
    std::string time_filter = build_time_filter();
    if (!time_filter.empty()) {
        oss << " |> range(" << time_filter << ")";
    }
    
    // Measurement filter
    oss << " |> filter(fn: (r) => r._measurement == \"" << params_.measurement << "\")";
    
    // Tag filters
    std::string tag_filter = build_tag_filter();
    if (!tag_filter.empty()) {
        oss << " |> filter(fn: (r) => " << tag_filter << ")";
    }
    
    // Field filters
    if (!params_.field_filters.empty()) {
        oss << " |> filter(fn: (r) => " << params_.field_filters << ")";
    }
    
    // Aggregation
    if (!params_.aggregation.empty() && !params_.window.empty()) {
        oss << " |> aggregateWindow(every: " << params_.window 
            << ", fn: " << params_.aggregation << ")";
    }
    
    // Pivot to get OHLCV columns
    oss << " |> pivot(rowKey: [\"_time\"], columnKey: [\"_field\"], valueColumn: \"_value\")";
    
    // Sort by time
    oss << " |> sort(columns: [\"_time\"])";
    
    // Limit
    if (params_.max_points > 0) {
        oss << " |> limit(n: " << params_.max_points << ")";
    }
    
    query = oss.str();
}

void InfluxFeed::parse_influx_response_v1(const std::string& response) {
    // Parse InfluxDB 1.x JSON response
    // This is a simplified parser - real implementation would use a JSON library
    
    // Clear existing data
    data_.clear();
    
    // Simple parsing for demonstration
    // In practice, use a proper JSON parser like nlohmann/json
    std::istringstream iss(response);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("\"values\"") != std::string::npos) {
            // Process data values
            // This is placeholder logic
            std::map<std::string, std::any> point;
            point["time"] = std::chrono::system_clock::now();
            point[params_.open_field] = 100.0;
            point[params_.high_field] = 105.0;
            point[params_.low_field] = 95.0;
            point[params_.close_field] = 102.0;
            point[params_.volume_field] = 1000.0;
            
            data_.push_back(point);
        }
    }
}

void InfluxFeed::parse_influx_response_v2(const std::string& response) {
    // Parse InfluxDB 2.x CSV response
    std::istringstream iss(response);
    std::string line;
    std::vector<std::string> headers;
    
    // Read headers
    if (std::getline(iss, line)) {
        std::istringstream header_stream(line);
        std::string header;
        
        while (std::getline(header_stream, header, ',')) {
            headers.push_back(header);
        }
    }
    
    // Read data rows
    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream row_stream(line);
        std::string value;
        std::map<std::string, std::any> point;
        
        for (size_t i = 0; i < headers.size() && std::getline(row_stream, value, ','); ++i) {
            // Parse and store values
            if (headers[i] == "_time") {
                point["time"] = parse_influx_time(value);
            } else if (headers[i] == params_.open_field) {
                point[params_.open_field] = std::stod(value);
            } else if (headers[i] == params_.high_field) {
                point[params_.high_field] = std::stod(value);
            } else if (headers[i] == params_.low_field) {
                point[params_.low_field] = std::stod(value);
            } else if (headers[i] == params_.close_field) {
                point[params_.close_field] = std::stod(value);
            } else if (headers[i] == params_.volume_field) {
                point[params_.volume_field] = std::stod(value);
            } else if (headers[i] == params_.openinterest_field) {
                point[params_.openinterest_field] = std::stod(value);
            }
        }
        
        if (!point.empty()) {
            data_.push_back(point);
        }
    }
}

double InfluxFeed::extract_field_value(const std::map<std::string, std::any>& point, 
                                      const std::string& field_name) const {
    auto it = point.find(field_name);
    if (it != point.end()) {
        try {
            return std::any_cast<double>(it->second);
        } catch (const std::bad_any_cast& e) {
            // Try to convert from other numeric types
            try {
                return static_cast<double>(std::any_cast<int>(it->second));
            } catch (const std::bad_any_cast&) {
                try {
                    return static_cast<double>(std::any_cast<float>(it->second));
                } catch (const std::bad_any_cast&) {
                    return 0.0;
                }
            }
        }
    }
    return 0.0;
}

std::chrono::system_clock::time_point InfluxFeed::extract_timestamp(
    const std::map<std::string, std::any>& point) const {
    
    auto it = point.find("time");
    if (it != point.end()) {
        try {
            return std::any_cast<std::chrono::system_clock::time_point>(it->second);
        } catch (const std::bad_any_cast&) {
            // Try to parse as string
            try {
                std::string time_str = std::any_cast<std::string>(it->second);
                return parse_influx_time(time_str);
            } catch (const std::bad_any_cast&) {
                return std::chrono::system_clock::now();
            }
        }
    }
    return std::chrono::system_clock::now();
}

std::string InfluxFeed::build_time_filter() const {
    std::ostringstream oss;
    
    if (!params_.start_time.empty() || !params_.end_time.empty()) {
        if (params_.version == "2.x") {
            // Flux query format
            if (!params_.start_time.empty()) {
                oss << "start: " << params_.start_time;
            }
            if (!params_.end_time.empty()) {
                if (!params_.start_time.empty()) oss << ", ";
                oss << "stop: " << params_.end_time;
            }
        } else {
            // InfluxQL format
            if (!params_.start_time.empty()) {
                oss << "time >= '" << params_.start_time << "'";
            }
            if (!params_.end_time.empty()) {
                if (!params_.start_time.empty()) oss << " AND ";
                oss << "time <= '" << params_.end_time << "'";
            }
        }
    }
    
    return oss.str();
}

std::string InfluxFeed::build_tag_filter() const {
    if (params_.tag_filters.empty()) {
        return "";
    }
    
    if (params_.version == "2.x") {
        // Convert to Flux format
        return params_.tag_filters;  // Assume already in correct format
    } else {
        // InfluxQL format
        return params_.tag_filters;
    }
}

std::string InfluxFeed::build_field_filter() const {
    return params_.field_filters;
}

std::string InfluxFeed::build_group_by_clause() const {
    return params_.group_by;
}

std::string InfluxFeed::build_aggregation_clause() const {
    return params_.aggregation;
}

void InfluxFeed::update_real_time_data() {
    // Get new data since last query
    std::string original_start = params_.start_time;
    
    // Set start time to last data timestamp
    if (!data_.empty()) {
        auto last_time = extract_timestamp(data_.back());
        params_.start_time = format_influx_time(last_time);
    }
    
    // Execute query for new data
    size_t original_size = data_.size();
    execute_query();
    
    // Restore original start time
    params_.start_time = original_start;
    
    // Update query time
    last_query_time_ = std::chrono::system_clock::now();
    
    if (data_.size() > original_size) {
        std::cout << "Updated with " << (data_.size() - original_size) << " new data points" << std::endl;
    }
}

bool InfluxFeed::should_refresh_data() const {
    auto now = std::chrono::system_clock::now();
    auto time_since_last_query = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_query_time_).count();
    
    return time_since_last_query >= params_.refresh_interval;
}

bool InfluxFeed::validate_connection_params() const {
    if (params_.host.empty()) {
        std::cerr << "Host cannot be empty" << std::endl;
        return false;
    }
    
    if (params_.port <= 0 || params_.port > 65535) {
        std::cerr << "Invalid port number" << std::endl;
        return false;
    }
    
    if (params_.database.empty()) {
        std::cerr << "Database/bucket cannot be empty" << std::endl;
        return false;
    }
    
    if (params_.version == "2.x" && params_.token.empty()) {
        std::cerr << "Token required for InfluxDB 2.x" << std::endl;
        return false;
    }
    
    return true;
}

bool InfluxFeed::validate_query_params() const {
    if (params_.measurement.empty()) {
        std::cerr << "Measurement cannot be empty" << std::endl;
        return false;
    }
    
    return true;
}

bool InfluxFeed::validate_field_mapping() const {
    if (params_.datetime_field.empty()) {
        std::cerr << "Datetime field cannot be empty" << std::endl;
        return false;
    }
    
    return true;
}

void InfluxFeed::handle_query_error(int http_code, const std::string& response) {
    std::cerr << "Query failed with HTTP code " << http_code << ": " << response << std::endl;
}

void InfluxFeed::handle_connection_error(const std::exception& e) {
    std::cerr << "Connection error: " << e.what() << std::endl;
}

void InfluxFeed::handle_parse_error(const std::string& error) {
    std::cerr << "Parse error: " << error << std::endl;
}

std::string InfluxFeed::format_influx_time(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::chrono::system_clock::time_point InfluxFeed::parse_influx_time(const std::string& time_str) const {
    std::tm tm = {};
    std::istringstream ss(time_str);
    
    // Try RFC3339 format
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        // Try alternative format
        ss.clear();
        ss.str(time_str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

bool InfluxFeed::is_relative_time(const std::string& time_str) const {
    return time_str.find("now()") != std::string::npos || 
           time_str.find("-") == 0 ||
           std::regex_match(time_str, std::regex(R"(\d+[smhd])"));
}

std::string InfluxFeed::resolve_relative_time(const std::string& relative_time) const {
    // Convert relative time to absolute time
    auto now = std::chrono::system_clock::now();
    
    if (relative_time == "now()") {
        return format_influx_time(now);
    }
    
    // Parse relative time like "-1h", "-30m", etc.
    std::regex pattern(R"(-(\d+)([smhd]))");
    std::smatch match;
    
    if (std::regex_match(relative_time, match, pattern)) {
        int value = std::stoi(match[1]);
        char unit = match[2].str()[0];
        
        std::chrono::seconds offset(0);
        switch (unit) {
            case 's': offset = std::chrono::seconds(value); break;
            case 'm': offset = std::chrono::minutes(value); break;
            case 'h': offset = std::chrono::hours(value); break;
            case 'd': offset = std::chrono::hours(24 * value); break;
        }
        
        return format_influx_time(now - offset);
    }
    
    return relative_time;
}

std::string InfluxFeed::escape_influx_string(const std::string& str) const {
    std::string escaped = str;
    std::replace(escaped.begin(), escaped.end(), '"', '\'');
    return escaped;
}

// InfluxClient implementation
InfluxFeed::InfluxClient::InfluxClient(const std::string& host, int port, bool use_ssl)
    : host_(host), port_(port), use_ssl_(use_ssl), timeout_(30), verify_ssl_(true) {
}

InfluxFeed::InfluxClient::Response InfluxFeed::InfluxClient::query_v1(
    const std::string& database, const std::string& query,
    const std::string& username, const std::string& password) {
    
    // Simplified HTTP client implementation
    // In practice, use a proper HTTP library like libcurl or cpp-httplib
    
    Response response;
    response.code = 200;
    response.body = R"({"results":[{"series":[{"name":"measurement","columns":["time","open","high","low","close","volume"],"values":[]}]}]})";
    
    return response;
}

InfluxFeed::InfluxClient::Response InfluxFeed::InfluxClient::query_v2(
    const std::string& org, const std::string& bucket,
    const std::string& query, const std::string& token) {
    
    // Simplified HTTP client implementation
    Response response;
    response.code = 200;
    response.body = "#datatype,string,long,dateTime:RFC3339,string,string,double\n"
                   "#group,false,false,false,true,true,false\n"
                   "#default,_result,,,,,\n"
                   ",result,table,_time,_field,_measurement,_value\n";
    
    return response;
}

void InfluxFeed::InfluxClient::set_timeout(int seconds) {
    timeout_ = seconds;
}

void InfluxFeed::InfluxClient::set_verify_ssl(bool verify) {
    verify_ssl_ = verify;
}

void InfluxFeed::update_lines(const std::vector<double>& bar_data) {
    // Update the data lines with new bar data
    // This method would be implemented in the base class
    if (bar_data.size() >= 7) {
        std::cout << "Updated InfluxDB data - Close: " << bar_data[4] 
                  << ", Volume: " << bar_data[5] << std::endl;
    }
}

} // namespace feeds
} // namespace backtrader