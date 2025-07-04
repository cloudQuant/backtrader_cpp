#include "../../include/feeds/quandl.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

namespace backtrader {
namespace feeds {

QuandlFeed::QuandlFeed(const Params& params) : p(params) {
    dataset_ = p.dataset;
    api_key_ = p.api_key;
    
    // Initialize data lines (datetime, open, high, low, close, volume)
    lines.resize(6);
    
    // Set up parameters
    params["dataset"] = std::any(dataset_);
    params["api_key"] = std::any(api_key_);
}

bool QuandlFeed::start() {
    try {
        if (p.from_file && !p.dataname.empty()) {
            // Load data from local file
            return load_from_file(p.dataname);
        } else {
            // Download data from Quandl API
            return download_from_quandl();
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to start Quandl feed: " << e.what() << std::endl;
        }
        return false;
    }
}

void QuandlFeed::stop() {
    // Nothing to stop for file-based feed
}

bool QuandlFeed::load() {
    if (data_index_ < data_records_.size()) {
        const auto& record = data_records_[data_index_++];
        
        lines[0].push_back(record.timestamp);   // datetime
        lines[1].push_back(record.open);        // open
        lines[2].push_back(record.high);        // high
        lines[3].push_back(record.low);         // low
        lines[4].push_back(record.close);       // close
        lines[5].push_back(record.volume);      // volume
        
        return true;
    }
    
    return false; // No more data
}

bool QuandlFeed::load_from_file(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        std::string line;
        bool header_read = false;
        std::vector<std::string> headers;
        
        data_records_.clear();
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            auto fields = split_csv_line(line);
            
            if (!header_read) {
                headers = fields;
                header_read = true;
                
                // Find column indices
                if (!find_column_indices(headers)) {
                    throw std::runtime_error("Required columns not found in CSV header");
                }
                continue;
            }
            
            // Parse data row
            DataRecord record = parse_csv_row(fields);
            
            // Apply date filtering
            if (p.fromdate > 0 && record.timestamp < p.fromdate) {
                continue;
            }
            if (p.todate > 0 && record.timestamp > p.todate) {
                break;
            }
            
            data_records_.push_back(record);
        }
        
        file.close();
        
        // Sort by timestamp (ascending)
        std::sort(data_records_.begin(), data_records_.end(),
                  [](const DataRecord& a, const DataRecord& b) {
                      return a.timestamp < b.timestamp;
                  });
        
        data_index_ = 0;
        
        if (p.debug) {
            std::cout << "Loaded " << data_records_.size() 
                      << " records from " << filename << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error loading from file: " << e.what() << std::endl;
        }
        return false;
    }
}

bool QuandlFeed::download_from_quandl() {
    try {
        // Build Quandl API URL
        std::string url = build_quandl_url();
        
        if (p.debug) {
            std::cout << "Downloading from Quandl: " << url << std::endl;
        }
        
        // Download data (this would use HTTP client in real implementation)
        std::string csv_data = fetch_url_content(url);
        
        if (csv_data.empty()) {
            throw std::runtime_error("No data received from Quandl");
        }
        
        // Parse CSV response
        if (!parse_csv_response(csv_data)) {
            throw std::runtime_error("Failed to parse Quandl response");
        }
        
        data_index_ = 0;
        
        if (p.debug) {
            std::cout << "Downloaded " << data_records_.size() 
                      << " records from Quandl dataset " << dataset_ << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error downloading from Quandl: " << e.what() << std::endl;
        }
        return false;
    }
}

std::string QuandlFeed::build_quandl_url() const {
    std::ostringstream url;
    url << "https://www.quandl.com/api/v3/datasets/" << dataset_ << ".csv";
    
    // Add parameters
    std::vector<std::string> params;
    
    if (!api_key_.empty()) {
        params.push_back("api_key=" + api_key_);
    }
    
    if (p.start_date > 0) {
        params.push_back("start_date=" + format_date(p.start_date));
    }
    
    if (p.end_date > 0) {
        params.push_back("end_date=" + format_date(p.end_date));
    }
    
    if (p.limit > 0) {
        params.push_back("limit=" + std::to_string(p.limit));
    }
    
    if (!p.order.empty()) {
        params.push_back("order=" + p.order);
    }
    
    if (!p.collapse.empty()) {
        params.push_back("collapse=" + p.collapse);
    }
    
    if (!p.transform.empty()) {
        params.push_back("transform=" + p.transform);
    }
    
    if (!params.empty()) {
        url << "?";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) url << "&";
            url << params[i];
        }
    }
    
    return url.str();
}

std::string QuandlFeed::fetch_url_content(const std::string& url) const {
    // This is a placeholder implementation
    // In real implementation, you would use a HTTP client library like libcurl
    
    if (p.debug) {
        std::cout << "Note: HTTP download not implemented. URL: " << url << std::endl;
    }
    
    // For demonstration, return empty string
    // In real implementation:
    // 1. Initialize libcurl
    // 2. Set URL and options
    // 3. Perform request
    // 4. Return response body
    
    return "";
}

bool QuandlFeed::parse_csv_response(const std::string& csv_data) {
    std::istringstream stream(csv_data);
    std::string line;
    bool header_read = false;
    std::vector<std::string> headers;
    
    data_records_.clear();
    
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        
        auto fields = split_csv_line(line);
        
        if (!header_read) {
            headers = fields;
            header_read = true;
            
            // Find column indices
            if (!find_column_indices(headers)) {
                return false;
            }
            continue;
        }
        
        // Parse data row
        DataRecord record = parse_csv_row(fields);
        data_records_.push_back(record);
    }
    
    // Sort by timestamp (ascending)
    std::sort(data_records_.begin(), data_records_.end(),
              [](const DataRecord& a, const DataRecord& b) {
                  return a.timestamp < b.timestamp;
              });
    
    return true;
}

std::vector<std::string> QuandlFeed::split_csv_line(const std::string& line) const {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    
    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    
    // Add the last field
    fields.push_back(field);
    
    return fields;
}

bool QuandlFeed::find_column_indices(const std::vector<std::string>& headers) {
    // Find required columns (flexible mapping)
    col_date_ = -1;
    col_open_ = -1;
    col_high_ = -1;
    col_low_ = -1;
    col_close_ = -1;
    col_volume_ = -1;
    
    for (size_t i = 0; i < headers.size(); ++i) {
        std::string header = to_lower(headers[i]);
        
        if (header == "date" || header == "timestamp") {
            col_date_ = static_cast<int>(i);
        } else if (header == "open") {
            col_open_ = static_cast<int>(i);
        } else if (header == "high") {
            col_high_ = static_cast<int>(i);
        } else if (header == "low") {
            col_low_ = static_cast<int>(i);
        } else if (header == "close" || header == "value") {
            col_close_ = static_cast<int>(i);
        } else if (header == "volume") {
            col_volume_ = static_cast<int>(i);
        }
    }
    
    // Date and at least one price column are required
    return col_date_ >= 0 && col_close_ >= 0;
}

QuandlFeed::DataRecord QuandlFeed::parse_csv_row(const std::vector<std::string>& fields) const {
    DataRecord record;
    
    // Parse date
    if (col_date_ >= 0 && col_date_ < static_cast<int>(fields.size())) {
        record.timestamp = parse_date(fields[col_date_]);
    }
    
    // Parse OHLC data
    if (col_open_ >= 0 && col_open_ < static_cast<int>(fields.size())) {
        record.open = parse_double(fields[col_open_]);
    } else {
        record.open = record.close; // Use close if open not available
    }
    
    if (col_high_ >= 0 && col_high_ < static_cast<int>(fields.size())) {
        record.high = parse_double(fields[col_high_]);
    } else {
        record.high = record.close; // Use close if high not available
    }
    
    if (col_low_ >= 0 && col_low_ < static_cast<int>(fields.size())) {
        record.low = parse_double(fields[col_low_]);
    } else {
        record.low = record.close; // Use close if low not available
    }
    
    if (col_close_ >= 0 && col_close_ < static_cast<int>(fields.size())) {
        record.close = parse_double(fields[col_close_]);
    }
    
    if (col_volume_ >= 0 && col_volume_ < static_cast<int>(fields.size())) {
        record.volume = parse_double(fields[col_volume_]);
    } else {
        record.volume = 0.0; // Default volume
    }
    
    // Ensure OHLC consistency
    if (col_open_ < 0) record.open = record.close;
    if (col_high_ < 0) record.high = std::max(record.open, record.close);
    if (col_low_ < 0) record.low = std::min(record.open, record.close);
    
    return record;
}

double QuandlFeed::parse_date(const std::string& date_str) const {
    // Parse date in format YYYY-MM-DD
    std::tm tm = {};
    std::istringstream ss(date_str);
    
    ss >> std::get_time(&tm, "%Y-%m-%d");
    
    if (ss.fail()) {
        // Try alternative format
        std::istringstream ss2(date_str);
        ss2 >> std::get_time(&tm, "%m/%d/%Y");
        
        if (ss2.fail()) {
            return 0.0; // Invalid date
        }
    }
    
    return static_cast<double>(std::mktime(&tm));
}

double QuandlFeed::parse_double(const std::string& value_str) const {
    if (value_str.empty() || value_str == "null") {
        return 0.0;
    }
    
    try {
        return std::stod(value_str);
    } catch (const std::exception&) {
        return 0.0;
    }
}

std::string QuandlFeed::to_lower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string QuandlFeed::format_date(double timestamp) const {
    std::time_t time_t_val = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time_t_val);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

bool QuandlFeed::islive() const {
    return false; // Quandl feed is always historical
}

std::string QuandlFeed::get_dataset() const {
    return dataset_;
}

size_t QuandlFeed::get_data_size() const {
    return data_records_.size();
}

double QuandlFeed::get_last_price() const {
    if (!lines.empty() && !lines[4].empty()) {
        return lines[4].back(); // Last close price
    }
    return 0.0;
}

QuandlFeed::DataRecord QuandlFeed::get_last_record() const {
    if (!data_records_.empty()) {
        return data_records_.back();
    }
    
    DataRecord record;
    record.timestamp = 0.0;
    record.open = record.high = record.low = record.close = record.volume = 0.0;
    return record;
}

std::vector<std::string> QuandlFeed::get_available_datasets(const std::string& database) const {
    // This would query Quandl API for available datasets in a database
    // For now, return some common examples
    
    std::vector<std::string> datasets;
    
    if (database == "WIKI") {
        // Some popular US stocks from WIKI database (now discontinued)
        datasets = {"WIKI/AAPL", "WIKI/GOOGL", "WIKI/MSFT", "WIKI/TSLA"};
    } else if (database == "FRED") {
        // Federal Reserve Economic Data
        datasets = {"FRED/GDP", "FRED/UNRATE", "FRED/DGS10", "FRED/CPIAUCSL"};
    } else if (database == "YAHOO") {
        // Yahoo Finance data
        datasets = {"YAHOO/AAPL", "YAHOO/SPY", "YAHOO/QQQ", "YAHOO/IWM"};
    }
    
    return datasets;
}

void QuandlFeed::set_api_key(const std::string& api_key) {
    api_key_ = api_key;
}

std::string QuandlFeed::get_api_key() const {
    return api_key_;
}

void QuandlFeed::set_dataset(const std::string& dataset) {
    dataset_ = dataset;
}

} // namespace feeds
} // namespace backtrader