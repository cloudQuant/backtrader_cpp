#include "feeds/csvgeneric.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace backtrader {

GenericCSVData::GenericCSVData() : CSVDataBase(), current_line_index_(0), use_reverse_mode_(false) {
    // Initialize with default parameters
}

GenericCSVData::~GenericCSVData() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool GenericCSVData::start() {
    if (!CSVDataBase::start()) {
        return false;
    }
    
    // Open the CSV file
    file_.open(params.dataname);
    if (!file_.is_open()) {
        std::cerr << "Error: Cannot open CSV file: " << params.dataname << std::endl;
        return false;
    }
    
    // Handle headers and column mapping
    if (params.headers) {
        std::string header_line;
        if (!std::getline(file_, header_line)) {
            std::cerr << "Error: Cannot read header from CSV file" << std::endl;
            return false;
        }
        
        // Map header names to column indices
        std::vector<std::string> headers = split_line(header_line);
        map_headers_to_indices(headers);
    }
    
    // If reverse mode, read all lines into memory
    if (params.reverse) {
        use_reverse_mode_ = true;
        std::string line;
        while (std::getline(file_, line)) {
            if (!line.empty()) {
                all_lines_.push_back(split_line(line));
            }
        }
        
        // Reverse the order
        std::reverse(all_lines_.begin(), all_lines_.end());
        current_line_index_ = 0;
        
        file_.close(); // Don't need file handle anymore in reverse mode
    }
    
    return true;
}

bool GenericCSVData::stop() {
    if (file_.is_open()) {
        file_.close();
    }
    
    all_lines_.clear();
    current_line_index_ = 0;
    
    return CSVDataBase::stop();
}

bool GenericCSVData::_load() {
    std::vector<std::string> line_tokens;
    
    if (use_reverse_mode_) {
        // Use pre-loaded lines in reverse order
        if (current_line_index_ >= all_lines_.size()) {
            return false; // End of data
        }
        
        line_tokens = all_lines_[current_line_index_++];
    } else {
        // Read line from file
        std::string line;
        if (!std::getline(file_, line)) {
            return false; // End of file or error
        }
        
        // Skip empty lines
        if (line.empty()) {
            return _load(); // Recursively try next line
        }
        
        line_tokens = split_line(line);
    }
    
    // Load the line data
    return load_line(line_tokens);
}

bool GenericCSVData::load_line(const std::vector<std::string>& line_tokens) {
    if (line_tokens.empty()) {
        return false;
    }
    
    try {
        // Parse datetime
        double datetime_value = 0.0;
        
        if (params.datetime_idx >= 0 && params.datetime_idx < line_tokens.size()) {
            if (params.unix_timestamp > 0) {
                datetime_value = parse_unix_timestamp(line_tokens[params.datetime_idx]);
            } else {
                std::string date_str = line_tokens[params.datetime_idx];
                std::string time_str;
                
                // Check if we have separate time column
                if (params.time_idx >= 0 && params.time_idx < line_tokens.size()) {
                    time_str = line_tokens[params.time_idx];
                }
                
                datetime_value = parse_datetime(date_str, time_str);
            }
        }
        
        // Set datetime
        if (lines && lines->getline(0)) {
            (*lines->getline(0))[0] = datetime_value;
        }
        
        // Parse OHLCV data
        double value;
        
        // Open
        if (parse_field(line_tokens, params.open_idx, value) && lines && lines->getline(1)) {
            (*lines->getline(1))[0] = value;
        }
        
        // High
        if (parse_field(line_tokens, params.high_idx, value) && lines && lines->getline(2)) {
            (*lines->getline(2))[0] = value;
        }
        
        // Low
        if (parse_field(line_tokens, params.low_idx, value) && lines && lines->getline(3)) {
            (*lines->getline(3))[0] = value;
        }
        
        // Close
        if (parse_field(line_tokens, params.close_idx, value) && lines && lines->getline(4)) {
            (*lines->getline(4))[0] = value;
        }
        
        // Volume
        if (parse_field(line_tokens, params.volume_idx, value) && lines && lines->getline(5)) {
            (*lines->getline(5))[0] = value;
        }
        
        // Open Interest
        if (parse_field(line_tokens, params.openinterest_idx, value) && lines && lines->getline(6)) {
            (*lines->getline(6))[0] = value;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CSV line: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> GenericCSVData::split_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, params.separator[0])) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        tokens.push_back(token);
    }
    
    return tokens;
}

double GenericCSVData::parse_datetime(const std::string& date_str, const std::string& time_str) {
    try {
        std::string combined_str = date_str;
        std::string format_str = params.dtformat;
        
        if (!time_str.empty()) {
            combined_str += " " + time_str;
            format_str += " " + params.tmformat;
        }
        
        // Simple parsing for common formats
        // This is a simplified implementation - in practice would need full strptime
        std::tm tm = {};
        
        if (params.dtformat == "%Y-%m-%d") {
            // Parse YYYY-MM-DD format
            if (date_str.length() >= 10) {
                tm.tm_year = std::stoi(date_str.substr(0, 4)) - 1900;
                tm.tm_mon = std::stoi(date_str.substr(5, 2)) - 1;
                tm.tm_mday = std::stoi(date_str.substr(8, 2));
            }
        } else if (params.dtformat == "%m/%d/%Y") {
            // Parse MM/DD/YYYY format
            size_t first_slash = date_str.find('/');
            size_t second_slash = date_str.find('/', first_slash + 1);
            if (first_slash != std::string::npos && second_slash != std::string::npos) {
                tm.tm_mon = std::stoi(date_str.substr(0, first_slash)) - 1;
                tm.tm_mday = std::stoi(date_str.substr(first_slash + 1, second_slash - first_slash - 1));
                tm.tm_year = std::stoi(date_str.substr(second_slash + 1)) - 1900;
            }
        }
        
        // Parse time if present
        if (!time_str.empty() && time_str.length() >= 8) {
            tm.tm_hour = std::stoi(time_str.substr(0, 2));
            tm.tm_min = std::stoi(time_str.substr(3, 2));
            tm.tm_sec = std::stoi(time_str.substr(6, 2));
        }
        
        std::time_t time_t = std::mktime(&tm);
        return static_cast<double>(time_t) / 86400.0 + 719163.0; // Excel epoch adjustment
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing datetime: " << e.what() << std::endl;
        return 0.0;
    }
}

double GenericCSVData::parse_unix_timestamp(const std::string& timestamp_str) {
    try {
        if (params.unix_timestamp == 1) {
            // Integer seconds since epoch
            long timestamp = std::stol(timestamp_str);
            return static_cast<double>(timestamp) / 86400.0 + 719163.0;
        } else if (params.unix_timestamp == 2) {
            // Float seconds since epoch
            double timestamp = std::stod(timestamp_str);
            return timestamp / 86400.0 + 719163.0;
        }
        
        return 0.0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Unix timestamp: " << e.what() << std::endl;
        return 0.0;
    }
}

bool GenericCSVData::parse_field(const std::vector<std::string>& tokens, int field_idx, double& value) {
    if (field_idx < 0 || field_idx >= tokens.size()) {
        value = params.nullvalue;
        return field_idx >= 0; // Only return true if field was expected
    }
    
    const std::string& token = tokens[field_idx];
    if (token.empty()) {
        value = params.nullvalue;
        return true;
    }
    
    try {
        value = std::stod(token);
        return true;
    } catch (const std::exception& e) {
        value = params.nullvalue;
        return true;
    }
}

void GenericCSVData::map_headers_to_indices(const std::vector<std::string>& headers) {
    // Map header names to column indices if name-based mapping is enabled
    for (size_t i = 0; i < headers.size(); ++i) {
        std::string header = headers[i];
        
        // Convert to lowercase for case-insensitive matching
        std::transform(header.begin(), header.end(), header.begin(), ::tolower);
        
        if (header == params.datetime_name || header == "date") {
            params.datetime_idx = static_cast<int>(i);
        } else if (header == "time") {
            params.time_idx = static_cast<int>(i);
        } else if (header == params.open_name) {
            params.open_idx = static_cast<int>(i);
        } else if (header == params.high_name) {
            params.high_idx = static_cast<int>(i);
        } else if (header == params.low_name) {
            params.low_idx = static_cast<int>(i);
        } else if (header == params.close_name) {
            params.close_idx = static_cast<int>(i);
        } else if (header == params.volume_name || header == "vol") {
            params.volume_idx = static_cast<int>(i);
        } else if (header == params.openinterest_name || header == "oi") {
            params.openinterest_idx = static_cast<int>(i);
        }
    }
}

} // namespace backtrader