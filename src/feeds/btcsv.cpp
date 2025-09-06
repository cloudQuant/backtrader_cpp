#include "feeds/btcsv.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>

namespace backtrader {

BacktraderCSVData::BacktraderCSVData() : CSVDataBase() {
    // Initialize with default parameters
}

BacktraderCSVData::~BacktraderCSVData() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool BacktraderCSVData::start() {
    if (!CSVDataBase::start()) {
        return false;
    }
    
    // Open the CSV file
    file_.open(params.dataname);
    if (!file_.is_open()) {
        std::cerr << "Error: Cannot open CSV file: " << params.dataname << std::endl;
        return false;
    }
    
    // Skip header if present
    if (params.headers) {
        std::string header_line;
        if (!std::getline(file_, header_line)) {
            std::cerr << "Error: Cannot read header from CSV file" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool BacktraderCSVData::stop() {
    if (file_.is_open()) {
        file_.close();
    }
    
    return CSVDataBase::stop();
}

bool BacktraderCSVData::_load() {
    std::string line;
    
    // Read next line from file
    if (!std::getline(file_, line)) {
        return false; // End of file or error
    }
    
    // Skip empty lines
    if (line.empty()) {
        return _load(); // Recursively try next line
    }
    
    // Split line into tokens
    current_line_tokens_ = split_line(line);
    
    // Load the line data
    return load_line(current_line_tokens_);
}

bool BacktraderCSVData::load_line(const std::vector<std::string>& line_tokens) {
    if (line_tokens.empty()) {
        return false;
    }
    
    try {
        // Parse datetime
        std::string date_str = line_tokens[params.datetime_col];
        std::string time_str;
        
        // Check if we have time column or use session end
        if (params.time_col >= 0 && params.time_col < line_tokens.size()) {
            time_str = line_tokens[params.time_col];
        } else {
            time_str = params.sessionend;
        }
        
        // Parse date and time
        int year, month, day, hour, minute, second;
        if (!parse_date_time(date_str, time_str, year, month, day, hour, minute, second)) {
            return false;
        }
        
        // Convert to timestamp (days since epoch)
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        
        std::time_t time_t = std::mktime(&tm);
        double datetime_value = static_cast<double>(time_t) / 86400.0 + 719163.0; // Excel epoch adjustment
        
        // Set datetime
        if (lines && lines->getline(0)) {
            (*lines->getline(0))[0] = datetime_value;
        }
        
        // Parse OHLCV data
        if (params.open_col < line_tokens.size() && lines && lines->getline(1)) {
            (*lines->getline(1))[0] = std::stod(line_tokens[params.open_col]);
        }
        
        if (params.high_col < line_tokens.size() && lines && lines->getline(2)) {
            (*lines->getline(2))[0] = std::stod(line_tokens[params.high_col]);
        }
        
        if (params.low_col < line_tokens.size() && lines && lines->getline(3)) {
            (*lines->getline(3))[0] = std::stod(line_tokens[params.low_col]);
        }
        
        if (params.close_col < line_tokens.size() && lines && lines->getline(4)) {
            (*lines->getline(4))[0] = std::stod(line_tokens[params.close_col]);
        }
        
        if (params.volume_col < line_tokens.size() && lines && lines->getline(5)) {
            (*lines->getline(5))[0] = std::stod(line_tokens[params.volume_col]);
        }
        
        if (params.openinterest_col < line_tokens.size() && lines && lines->getline(6)) {
            (*lines->getline(6))[0] = std::stod(line_tokens[params.openinterest_col]);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CSV line: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> BacktraderCSVData::split_line(const std::string& line) {
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

bool BacktraderCSVData::parse_date_time(const std::string& date_str, const std::string& time_str,
                                        int& year, int& month, int& day, 
                                        int& hour, int& minute, int& second) {
    try {
        // Parse date (assuming YYYY-MM-DD format)
        if (date_str.length() >= 10) {
            year = std::stoi(date_str.substr(0, 4));
            month = std::stoi(date_str.substr(5, 2));
            day = std::stoi(date_str.substr(8, 2));
        } else {
            return false;
        }
        
        // Parse time (assuming HH:MM:SS format)
        if (time_str.length() >= 8) {
            hour = std::stoi(time_str.substr(0, 2));
            minute = std::stoi(time_str.substr(3, 2));
            second = std::stoi(time_str.substr(6, 2));
        } else if (time_str.length() >= 5) {
            // HH:MM format
            hour = std::stoi(time_str.substr(0, 2));
            minute = std::stoi(time_str.substr(3, 2));
            second = 0;
        } else {
            // Default to end of day
            hour = 17;
            minute = 0;
            second = 0;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing date/time: " << e.what() << std::endl;
        return false;
    }
}

} // namespace backtrader