#include "../../include/feeds/mt4csv.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace backtrader {
namespace feeds {

MT4CSVFeed::MT4CSVFeed(const Params& params) : p(params) {
    if (p.dataname.empty()) {
        throw std::invalid_argument("Dataname (file path) is required");
    }
    
    filename_ = p.dataname;
    
    // Initialize data lines (datetime, open, high, low, close, volume)
    lines.resize(6);
    
    // Set up parameters
    params["dataname"] = std::any(filename_);
    params["separator"] = std::any(p.separator);
}

bool MT4CSVFeed::start() {
    try {
        return load_from_file();
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to start MT4 CSV feed: " << e.what() << std::endl;
        }
        return false;
    }
}

void MT4CSVFeed::stop() {
    // Nothing to stop for file-based feed
}

bool MT4CSVFeed::load() {
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

bool MT4CSVFeed::load_from_file() {
    try {
        std::ifstream file(filename_);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename_);
        }
        
        std::string line;
        data_records_.clear();
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            // Parse MT4 CSV line
            DataRecord record = parse_mt4_line(line);
            
            // Skip invalid records
            if (record.timestamp == 0.0) {
                continue;
            }
            
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
                      << " records from " << filename_ << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error loading MT4 CSV file: " << e.what() << std::endl;
        }
        return false;
    }
}

MT4CSVFeed::DataRecord MT4CSVFeed::parse_mt4_line(const std::string& line) const {
    DataRecord record;
    
    // Split by separator
    auto fields = split_line(line, p.separator);
    
    if (fields.size() < 6) {
        // Invalid line
        record.timestamp = 0.0;
        return record;
    }
    
    try {
        // MT4 CSV format: Date,Time,Open,High,Low,Close,Volume
        // or combined: DateTime,Open,High,Low,Close,Volume
        
        if (fields.size() >= 7) {
            // Separate date and time fields
            std::string date_str = trim(fields[0]);
            std::string time_str = trim(fields[1]);
            record.timestamp = parse_mt4_datetime(date_str, time_str);
            
            record.open = parse_double(fields[2]);
            record.high = parse_double(fields[3]);
            record.low = parse_double(fields[4]);
            record.close = parse_double(fields[5]);
            record.volume = parse_double(fields[6]);
        } else {
            // Combined datetime field
            std::string datetime_str = trim(fields[0]);
            record.timestamp = parse_mt4_datetime_combined(datetime_str);
            
            record.open = parse_double(fields[1]);
            record.high = parse_double(fields[2]);
            record.low = parse_double(fields[3]);
            record.close = parse_double(fields[4]);
            record.volume = parse_double(fields[5]);
        }
        
        // Validate OHLC data
        if (!validate_ohlc(record)) {
            if (p.debug) {
                std::cout << "Invalid OHLC data at timestamp " << record.timestamp << std::endl;
            }
            
            if (p.fix_ohlc) {
                fix_ohlc(record);
            }
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
        }
        record.timestamp = 0.0;
    }
    
    return record;
}

std::vector<std::string> MT4CSVFeed::split_line(const std::string& line, char separator) const {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    
    while (std::getline(ss, field, separator)) {
        fields.push_back(field);
    }
    
    return fields;
}

std::string MT4CSVFeed::trim(const std::string& str) const {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

double MT4CSVFeed::parse_mt4_datetime(const std::string& date_str, const std::string& time_str) const {
    // Parse MT4 date format: YYYY.MM.DD or YYYY-MM-DD or DD.MM.YYYY
    // Parse MT4 time format: HH:MM or HH:MM:SS
    
    std::tm tm = {};
    
    // Parse date part
    if (!parse_mt4_date(date_str, tm)) {
        return 0.0;
    }
    
    // Parse time part
    if (!parse_mt4_time(time_str, tm)) {
        return 0.0;
    }
    
    return static_cast<double>(std::mktime(&tm));
}

double MT4CSVFeed::parse_mt4_datetime_combined(const std::string& datetime_str) const {
    // Parse combined datetime: "YYYY.MM.DD HH:MM" or "YYYY-MM-DD HH:MM:SS"
    
    size_t space_pos = datetime_str.find(' ');
    if (space_pos == std::string::npos) {
        return 0.0;
    }
    
    std::string date_part = datetime_str.substr(0, space_pos);
    std::string time_part = datetime_str.substr(space_pos + 1);
    
    return parse_mt4_datetime(date_part, time_part);
}

bool MT4CSVFeed::parse_mt4_date(const std::string& date_str, std::tm& tm) const {
    // Try different date formats
    std::istringstream ss(date_str);
    
    // Format 1: YYYY.MM.DD
    if (date_str.find('.') != std::string::npos) {
        ss >> std::get_time(&tm, "%Y.%m.%d");
        if (!ss.fail()) return true;
        
        // Try DD.MM.YYYY
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%d.%m.%Y");
        if (!ss.fail()) return true;
    }
    
    // Format 2: YYYY-MM-DD
    if (date_str.find('-') != std::string::npos) {
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (!ss.fail()) return true;
        
        // Try DD-MM-YYYY
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%d-%m-%Y");
        if (!ss.fail()) return true;
    }
    
    // Format 3: YYYY/MM/DD
    if (date_str.find('/') != std::string::npos) {
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%Y/%m/%d");
        if (!ss.fail()) return true;
        
        // Try MM/DD/YYYY
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%m/%d/%Y");
        if (!ss.fail()) return true;
    }
    
    return false;
}

bool MT4CSVFeed::parse_mt4_time(const std::string& time_str, std::tm& tm) const {
    std::istringstream ss(time_str);
    
    // Try HH:MM:SS
    ss >> std::get_time(&tm, "%H:%M:%S");
    if (!ss.fail()) return true;
    
    // Try HH:MM
    ss.clear();
    ss.str(time_str);
    ss >> std::get_time(&tm, "%H:%M");
    if (!ss.fail()) {
        tm.tm_sec = 0; // Set seconds to 0
        return true;
    }
    
    return false;
}

double MT4CSVFeed::parse_double(const std::string& value_str) const {
    if (value_str.empty()) {
        return 0.0;
    }
    
    try {
        return std::stod(value_str);
    } catch (const std::exception&) {
        return 0.0;
    }
}

bool MT4CSVFeed::validate_ohlc(const DataRecord& record) const {
    // Basic OHLC validation
    if (record.high < record.low) {
        return false;
    }
    
    if (record.open < record.low || record.open > record.high) {
        return false;
    }
    
    if (record.close < record.low || record.close > record.high) {
        return false;
    }
    
    if (record.volume < 0) {
        return false;
    }
    
    return true;
}

void MT4CSVFeed::fix_ohlc(DataRecord& record) const {
    // Fix invalid OHLC data
    
    // Ensure high >= low
    if (record.high < record.low) {
        std::swap(record.high, record.low);
    }
    
    // Ensure open is within high-low range
    if (record.open > record.high) {
        record.open = record.high;
    } else if (record.open < record.low) {
        record.open = record.low;
    }
    
    // Ensure close is within high-low range
    if (record.close > record.high) {
        record.close = record.high;
    } else if (record.close < record.low) {
        record.close = record.low;
    }
    
    // Ensure volume is non-negative
    if (record.volume < 0) {
        record.volume = 0.0;
    }
}

bool MT4CSVFeed::islive() const {
    return false; // MT4 CSV feed is always historical
}

std::string MT4CSVFeed::get_filename() const {
    return filename_;
}

size_t MT4CSVFeed::get_data_size() const {
    return data_records_.size();
}

double MT4CSVFeed::get_last_price() const {
    if (!lines.empty() && !lines[4].empty()) {
        return lines[4].back(); // Last close price
    }
    return 0.0;
}

MT4CSVFeed::DataRecord MT4CSVFeed::get_last_record() const {
    if (!data_records_.empty()) {
        return data_records_.back();
    }
    
    DataRecord record;
    record.timestamp = 0.0;
    record.open = record.high = record.low = record.close = record.volume = 0.0;
    return record;
}

std::vector<MT4CSVFeed::DataRecord> MT4CSVFeed::get_records_range(size_t start, size_t end) const {
    std::vector<DataRecord> range;
    
    if (start >= data_records_.size()) {
        return range;
    }
    
    if (end > data_records_.size()) {
        end = data_records_.size();
    }
    
    range.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        range.push_back(data_records_[i]);
    }
    
    return range;
}

void MT4CSVFeed::reset() {
    data_index_ = 0;
}

bool MT4CSVFeed::seek_to_timestamp(double timestamp) {
    // Binary search for timestamp
    auto it = std::lower_bound(data_records_.begin(), data_records_.end(), timestamp,
                              [](const DataRecord& record, double ts) {
                                  return record.timestamp < ts;
                              });
    
    if (it != data_records_.end()) {
        data_index_ = std::distance(data_records_.begin(), it);
        return true;
    }
    
    return false;
}

std::map<std::string, std::any> MT4CSVFeed::get_metadata() const {
    std::map<std::string, std::any> metadata;
    
    metadata["filename"] = filename_;
    metadata["total_records"] = static_cast<int>(data_records_.size());
    metadata["separator"] = std::string(1, p.separator);
    metadata["fix_ohlc"] = p.fix_ohlc;
    
    if (!data_records_.empty()) {
        metadata["start_timestamp"] = data_records_.front().timestamp;
        metadata["end_timestamp"] = data_records_.back().timestamp;
        metadata["start_date"] = format_timestamp(data_records_.front().timestamp);
        metadata["end_date"] = format_timestamp(data_records_.back().timestamp);
    }
    
    return metadata;
}

std::string MT4CSVFeed::format_timestamp(double timestamp) const {
    std::time_t time_t_val = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time_t_val);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace feeds
} // namespace backtrader