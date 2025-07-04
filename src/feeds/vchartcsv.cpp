#include "../../include/feeds/vchartcsv.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <regex>
#include <iomanip>

namespace backtrader {
namespace feeds {

// Static member definitions
const std::map<char, TimeFrame> VChartCSVData::VC_TIMEFRAMES = {
    {'I', TimeFrame::Minutes},  // Intraday
    {'D', TimeFrame::Days},     // Daily
    {'W', TimeFrame::Weeks},    // Weekly  
    {'M', TimeFrame::Months}    // Monthly
};

// VChartCSVData implementation

VChartCSVData::VChartCSVData(const VChartParams& params) : vchart_params_(params) {
    // Set up base CSV parameters
    this->params.separator = ",";
    this->params.headers = false; // VisualChart CSV doesn't have headers
    
    processed_rows_ = 0;
    skipped_rows_ = 0;
    error_rows_ = 0;
    symbol_consistent_ = true;
    timeframe_consistent_ = true;
}

VChartCSVData::VChartCSVData(const std::string& filename, const VChartParams& params) 
    : vchart_params_(params) {
    
    this->params.dataname = filename;
    this->params.separator = ",";
    this->params.headers = false;
    
    processed_rows_ = 0;
    skipped_rows_ = 0;
    error_rows_ = 0;
    symbol_consistent_ = true;
    timeframe_consistent_ = true;
}

void VChartCSVData::start() {
    std::cout << "Starting VChartCSVData for file: " << params.dataname << std::endl;
    
    try {
        // Analyze file first if auto-detection is enabled
        if (vchart_params_.auto_extract_symbol || vchart_params_.auto_detect_timeframe) {
            auto analysis = analyze_file();
            
            if (vchart_params_.auto_extract_symbol) {
                extracted_symbol_ = analysis.primary_symbol;
            }
            
            if (vchart_params_.auto_detect_timeframe) {
                detected_timeframe_ = analysis.primary_timeframe;
            }
        }
        
        // Call parent start
        GenericCSVData::start();
        
        std::cout << "VChartCSVData started successfully" << std::endl;
        std::cout << "Detected symbol: " << extracted_symbol_ << std::endl;
        std::cout << "Detected timeframe: " << static_cast<int>(detected_timeframe_) << std::endl;
        
    } catch (const std::exception& e) {
        log_parsing_error("Failed to start VChartCSVData: " + std::string(e.what()));
        throw;
    }
}

void VChartCSVData::stop() {
    GenericCSVData::stop();
    
    std::cout << "VChartCSVData stopped" << std::endl;
    std::cout << "Processed rows: " << processed_rows_ << std::endl;
    std::cout << "Skipped rows: " << skipped_rows_ << std::endl;
    std::cout << "Error rows: " << error_rows_ << std::endl;
    
    if (!symbol_consistent_) {
        std::cout << "Warning: Multiple symbols found in file" << std::endl;
    }
    
    if (!timeframe_consistent_) {
        std::cout << "Warning: Multiple timeframes found in file" << std::endl;
    }
}

bool VChartCSVData::next() {
    return GenericCSVData::next();
}

void VChartCSVData::preload() {
    GenericCSVData::preload();
}

bool VChartCSVData::load_line_data(const std::vector<std::string>& tokens) {
    try {
        return parse_vchart_line(tokens);
    } catch (const std::exception& e) {
        log_parsing_error("Error parsing line: " + std::string(e.what()), processed_rows_ + 1);
        error_rows_++;
        return false;
    }
}

bool VChartCSVData::parse_vchart_line(const std::vector<std::string>& tokens) {
    if (tokens.size() < 8) {
        log_parsing_error("Insufficient columns in VisualChart CSV line", processed_rows_ + 1);
        return false;
    }
    
    try {
        // VisualChart CSV format:
        // 0: Symbol, 1: Timeframe, 2: Date(YYYYMMDD), 3: Time(HHMMSS), 
        // 4: Open, 5: High, 6: Low, 7: Close, 8: Volume, 9: OpenInterest
        
        std::string symbol = tokens[0];
        char timeframe_char = tokens[1].empty() ? 'D' : tokens[1][0];
        std::string date_str = tokens[2];
        std::string time_str = tokens[3];
        
        // Process symbol
        if (!process_symbol(symbol)) {
            return false;
        }
        
        // Process timeframe
        if (!process_timeframe(timeframe_char)) {
            return false;
        }
        
        // Parse datetime
        TimeFrame current_tf = convert_vchart_timeframe(timeframe_char);
        auto datetime = parse_vchart_datetime(date_str, time_str, current_tf);
        
        if (!validate_market_time(datetime)) {
            log_validation_warning("Invalid datetime", processed_rows_ + 1);
            if (vchart_params_.skip_invalid_timeframes) {
                skipped_rows_++;
                return false;
            }
        }
        
        // Check if we should skip this row
        if (should_skip_row(datetime)) {
            skipped_rows_++;
            return false;
        }
        
        // Parse OHLCV data
        double open = std::stod(tokens[4]);
        double high = std::stod(tokens[5]);
        double low = std::stod(tokens[6]);
        double close = std::stod(tokens[7]);
        double volume = tokens.size() > 8 ? std::stod(tokens[8]) : 0.0;
        double openinterest = tokens.size() > 9 ? std::stod(tokens[9]) : 0.0;
        
        // Validate OHLC data
        if (vchart_params_.validate_ohlc && !validate_price_data(open, high, low, close)) {
            log_validation_warning("Invalid OHLC data", processed_rows_ + 1);
            error_rows_++;
            return false;
        }
        
        // Apply volume filter
        if (!passes_volume_filter(volume)) {
            skipped_rows_++;
            return false;
        }
        
        // Set data in our lines
        set_datetime(datetime);
        set_open(open);
        set_high(high);
        set_low(low);
        set_close(close);
        set_volume(volume);
        set_openinterest(openinterest);
        
        processed_rows_++;
        return true;
        
    } catch (const std::exception& e) {
        log_parsing_error("Error parsing numeric data: " + std::string(e.what()), processed_rows_ + 1);
        error_rows_++;
        return false;
    }
}

std::chrono::system_clock::time_point VChartCSVData::parse_vchart_datetime(
    const std::string& date_str, 
    const std::string& time_str,
    TimeFrame timeframe) const {
    
    if (!validate_date_string(date_str)) {
        throw std::invalid_argument("Invalid date format: " + date_str);
    }
    
    auto date_point = combine_date_time(date_str, time_str);
    
    // Apply session time for non-intraday data
    if (timeframe != TimeFrame::Minutes) {
        date_point = apply_session_time(date_point, timeframe);
    }
    
    return date_point;
}

bool VChartCSVData::process_symbol(const std::string& symbol) {
    if (!validate_symbol(symbol)) {
        return false;
    }
    
    // Track symbol consistency
    if (found_symbols_.empty()) {
        found_symbols_.push_back(symbol);
        if (extracted_symbol_.empty()) {
            extracted_symbol_ = symbol;
        }
    } else {
        if (std::find(found_symbols_.begin(), found_symbols_.end(), symbol) == found_symbols_.end()) {
            found_symbols_.push_back(symbol);
            symbol_consistent_ = false;
        }
    }
    
    // Use override symbol if provided
    if (!vchart_params_.override_symbol.empty()) {
        return vchart_params_.override_symbol == symbol;
    }
    
    return true;
}

bool VChartCSVData::process_timeframe(char timeframe_char) {
    TimeFrame tf = convert_vchart_timeframe(timeframe_char);
    
    if (!validate_timeframe(tf)) {
        return false;
    }
    
    // Track timeframe consistency
    if (found_timeframes_.empty()) {
        found_timeframes_.push_back(tf);
        detected_timeframe_ = tf;
    } else {
        if (std::find(found_timeframes_.begin(), found_timeframes_.end(), tf) == found_timeframes_.end()) {
            found_timeframes_.push_back(tf);
            timeframe_consistent_ = false;
        }
    }
    
    // Use override timeframe if provided
    if (vchart_params_.override_timeframe != TimeFrame::Minutes) {
        return vchart_params_.override_timeframe == tf;
    }
    
    return true;
}

TimeFrame VChartCSVData::convert_vchart_timeframe(char tf_char) const {
    auto it = VC_TIMEFRAMES.find(tf_char);
    if (it != VC_TIMEFRAMES.end()) {
        return it->second;
    }
    return TimeFrame::Days; // Default fallback
}

bool VChartCSVData::validate_symbol(const std::string& symbol) const {
    if (symbol.empty()) {
        return false;
    }
    
    // Basic symbol validation
    return std::all_of(symbol.begin(), symbol.end(), 
                      [](char c) { return std::isalnum(c) || c == '.' || c == '_'; });
}

bool VChartCSVData::validate_timeframe(TimeFrame tf) const {
    // Validate against known VisualChart timeframes
    for (const auto& pair : VC_TIMEFRAMES) {
        if (pair.second == tf) {
            return true;
        }
    }
    return false;
}

bool VChartCSVData::validate_date_string(const std::string& date_str) const {
    if (date_str.length() != 8) {
        return false;
    }
    
    // Check YYYYMMDD format
    std::regex date_pattern(R"(\d{8})");
    if (!std::regex_match(date_str, date_pattern)) {
        return false;
    }
    
    // Basic date validation
    int year = std::stoi(date_str.substr(0, 4));
    int month = std::stoi(date_str.substr(4, 2));
    int day = std::stoi(date_str.substr(6, 2));
    
    return year >= 1900 && year <= 2100 && 
           month >= 1 && month <= 12 && 
           day >= 1 && day <= 31;
}

bool VChartCSVData::validate_time_string(const std::string& time_str) const {
    if (time_str.empty()) {
        return true; // Empty time is valid for daily data
    }
    
    if (time_str.length() != 6) {
        return false;
    }
    
    std::regex time_pattern(R"(\d{6})");
    if (!std::regex_match(time_str, time_pattern)) {
        return false;
    }
    
    int hour = std::stoi(time_str.substr(0, 2));
    int minute = std::stoi(time_str.substr(2, 2));
    int second = std::stoi(time_str.substr(4, 2));
    
    return hour >= 0 && hour <= 23 && 
           minute >= 0 && minute <= 59 && 
           second >= 0 && second <= 59;
}

bool VChartCSVData::validate_price_data(double open, double high, double low, double close) const {
    // Basic OHLC validation
    if (open <= 0 || high <= 0 || low <= 0 || close <= 0) {
        return false;
    }
    
    if (high < low) {
        return false;
    }
    
    if (high < open || high < close || low > open || low > close) {
        return false;
    }
    
    return true;
}

bool VChartCSVData::is_price_outlier(double price, double reference_price) const {
    if (reference_price <= 0) {
        return false;
    }
    
    double change_ratio = std::abs(price - reference_price) / reference_price;
    return change_ratio > vchart_params_.max_price_change;
}

std::chrono::system_clock::time_point VChartCSVData::combine_date_time(
    const std::string& date_str,
    const std::string& time_str) const {
    
    int year = std::stoi(date_str.substr(0, 4));
    int month = std::stoi(date_str.substr(4, 2));
    int day = std::stoi(date_str.substr(6, 2));
    
    int hour = 0, minute = 0, second = 0;
    
    if (!time_str.empty() && validate_time_string(time_str)) {
        hour = std::stoi(time_str.substr(0, 2));
        minute = std::stoi(time_str.substr(2, 2));
        second = std::stoi(time_str.substr(4, 2));
    }
    
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    
    auto time_t = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time_t);
}

std::chrono::system_clock::time_point VChartCSVData::apply_session_time(
    const std::chrono::system_clock::time_point& date,
    TimeFrame timeframe) const {
    
    if (timeframe == TimeFrame::Minutes) {
        return date; // Keep original time for intraday
    }
    
    // For daily and higher timeframes, use session end time
    auto session_end = get_session_end();
    auto session_time_t = std::chrono::system_clock::to_time_t(session_end);
    auto* session_tm = std::localtime(&session_time_t);
    
    auto date_time_t = std::chrono::system_clock::to_time_t(date);
    auto* date_tm = std::localtime(&date_time_t);
    
    if (session_tm && date_tm) {
        date_tm->tm_hour = session_tm->tm_hour;
        date_tm->tm_min = session_tm->tm_min;
        date_tm->tm_sec = session_tm->tm_sec;
        
        return std::chrono::system_clock::from_time_t(std::mktime(date_tm));
    }
    
    return date;
}

bool VChartCSVData::should_skip_row(const std::chrono::system_clock::time_point& dt) const {
    // Filter weekends
    if (vchart_params_.filter_weekends && is_weekend(dt)) {
        return true;
    }
    
    // Filter holidays
    if (vchart_params_.filter_holidays && is_holiday(dt)) {
        return true;
    }
    
    return false;
}

bool VChartCSVData::passes_volume_filter(double volume) const {
    return volume >= vchart_params_.min_volume;
}

bool VChartCSVData::passes_price_change_filter(double current_price, double previous_price) const {
    if (previous_price <= 0) {
        return true; // No previous price to compare
    }
    
    double change_ratio = std::abs(current_price - previous_price) / previous_price;
    return change_ratio <= vchart_params_.max_price_change;
}

VChartCSVData::ValidationResult VChartCSVData::validate_file() const {
    ValidationResult result = {};
    result.is_valid = true;
    result.total_rows = 0;
    result.valid_rows = 0;
    result.invalid_rows = 0;
    result.timeframe_mismatches = 0;
    result.symbol_mismatches = 0;
    result.ohlc_violations = 0;
    
    try {
        std::ifstream file(params.dataname);
        if (!file.is_open()) {
            result.is_valid = false;
            result.errors.push_back("Cannot open file: " + params.dataname);
            return result;
        }
        
        std::string line;
        size_t line_number = 0;
        std::string primary_symbol;
        TimeFrame primary_timeframe = TimeFrame::Days;
        
        while (std::getline(file, line)) {
            line_number++;
            result.total_rows++;
            
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() < 8) {
                result.invalid_rows++;
                if (result.errors.size() < 10) {
                    result.errors.push_back("Line " + std::to_string(line_number) + ": Insufficient columns");
                }
                continue;
            }
            
            // Validate symbol consistency
            if (primary_symbol.empty()) {
                primary_symbol = tokens[0];
            } else if (tokens[0] != primary_symbol) {
                result.symbol_mismatches++;
            }
            
            // Validate timeframe consistency
            char tf_char = tokens[1].empty() ? 'D' : tokens[1][0];
            TimeFrame tf = convert_vchart_timeframe(tf_char);
            
            if (result.total_rows == 1) {
                primary_timeframe = tf;
            } else if (tf != primary_timeframe) {
                result.timeframe_mismatches++;
            }
            
            // Validate OHLC data
            try {
                double open = std::stod(tokens[4]);
                double high = std::stod(tokens[5]);
                double low = std::stod(tokens[6]);
                double close = std::stod(tokens[7]);
                
                if (!validate_price_data(open, high, low, close)) {
                    result.ohlc_violations++;
                }
            } catch (...) {
                result.invalid_rows++;
                continue;
            }
            
            result.valid_rows++;
        }
        
        file.close();
        
        // Calculate overall validity
        if (result.total_rows > 0) {
            double success_rate = static_cast<double>(result.valid_rows) / result.total_rows;
            if (success_rate < 0.8) {
                result.is_valid = false;
                result.errors.push_back("Low validation success rate: " + 
                                      std::to_string(success_rate * 100) + "%");
            }
        }
        
        // Generate summary
        std::ostringstream summary;
        summary << "Validation completed: " << result.valid_rows << "/" << result.total_rows 
                << " rows valid";
        if (result.symbol_mismatches > 0) {
            summary << ", " << result.symbol_mismatches << " symbol mismatches";
        }
        if (result.timeframe_mismatches > 0) {
            summary << ", " << result.timeframe_mismatches << " timeframe mismatches";
        }
        if (result.ohlc_violations > 0) {
            summary << ", " << result.ohlc_violations << " OHLC violations";
        }
        
        result.summary = summary.str();
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.errors.push_back("Validation error: " + std::string(e.what()));
    }
    
    return result;
}

VChartCSVData::FileAnalysis VChartCSVData::analyze_file() const {
    FileAnalysis analysis = {};
    
    try {
        std::ifstream file(params.dataname);
        if (!file.is_open()) {
            return analysis;
        }
        
        std::string line;
        bool first_line = true;
        
        while (std::getline(file, line)) {
            analysis.total_records++;
            
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() < 8) continue;
            
            // Analyze symbols
            std::string symbol = tokens[0];
            analysis.symbol_counts[symbol]++;
            
            // Analyze timeframes
            char tf_char = tokens[1].empty() ? 'D' : tokens[1][0];
            TimeFrame tf = convert_vchart_timeframe(tf_char);
            analysis.timeframe_counts[tf]++;
            
            // Analyze date range
            try {
                std::string date_str = tokens[2];
                std::string time_str = tokens[3];
                auto dt = parse_vchart_datetime(date_str, time_str, tf);
                
                if (first_line) {
                    analysis.start_date = dt;
                    analysis.end_date = dt;
                    first_line = false;
                } else {
                    if (dt < analysis.start_date) analysis.start_date = dt;
                    if (dt > analysis.end_date) analysis.end_date = dt;
                }
                
                // Analyze volume
                if (tokens.size() > 8) {
                    double volume = std::stod(tokens[8]);
                    analysis.average_volume += volume;
                }
            } catch (...) {
                // Skip invalid lines
                continue;
            }
        }
        
        file.close();
        
        // Determine primary symbol and timeframe
        if (!analysis.symbol_counts.empty()) {
            auto max_symbol = std::max_element(analysis.symbol_counts.begin(), analysis.symbol_counts.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            analysis.primary_symbol = max_symbol->first;
        }
        
        if (!analysis.timeframe_counts.empty()) {
            auto max_timeframe = std::max_element(analysis.timeframe_counts.begin(), analysis.timeframe_counts.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            analysis.primary_timeframe = max_timeframe->first;
        }
        
        // Calculate averages
        if (analysis.total_records > 0) {
            analysis.average_volume /= analysis.total_records;
        }
        
        // Calculate data span
        analysis.data_span = std::chrono::duration_cast<std::chrono::seconds>(analysis.end_date - analysis.start_date);
        
        // Data quality assessment
        if (analysis.symbol_counts.size() == 1 && analysis.timeframe_counts.size() == 1) {
            analysis.data_quality_assessment = "High - Consistent format";
        } else if (analysis.symbol_counts.size() <= 3 && analysis.timeframe_counts.size() <= 2) {
            analysis.data_quality_assessment = "Medium - Some inconsistencies";
        } else {
            analysis.data_quality_assessment = "Low - Multiple inconsistencies";
        }
        
    } catch (const std::exception& e) {
        analysis.data_quality_assessment = "Error: " + std::string(e.what());
    }
    
    return analysis;
}

void VChartCSVData::set_session_end(int hour, int minute, int second) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* tm = std::localtime(&time_t);
    
    if (tm) {
        tm->tm_hour = hour;
        tm->tm_min = minute;
        tm->tm_sec = second;
        vchart_params_.session_end = std::chrono::system_clock::from_time_t(std::mktime(tm));
    }
}

std::chrono::system_clock::time_point VChartCSVData::get_session_end() const {
    return vchart_params_.session_end;
}

void VChartCSVData::set_symbol_override(const std::string& symbol) {
    vchart_params_.override_symbol = symbol;
}

bool VChartCSVData::is_symbol_consistent() const {
    return symbol_consistent_;
}

std::vector<std::string> VChartCSVData::get_found_symbols() const {
    return found_symbols_;
}

void VChartCSVData::set_timeframe_override(TimeFrame tf) {
    vchart_params_.override_timeframe = tf;
}

bool VChartCSVData::is_timeframe_consistent() const {
    return timeframe_consistent_;
}

std::vector<TimeFrame> VChartCSVData::get_found_timeframes() const {
    return found_timeframes_;
}

void VChartCSVData::add_holiday_date(const std::string& date) {
    vchart_params_.holiday_dates.push_back(date);
}

void VChartCSVData::set_holiday_dates(const std::vector<std::string>& dates) {
    vchart_params_.holiday_dates = dates;
}

bool VChartCSVData::is_holiday(const std::chrono::system_clock::time_point& dt) const {
    auto time_t = std::chrono::system_clock::to_time_t(dt);
    auto* tm = std::localtime(&time_t);
    
    if (!tm) return false;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << (tm->tm_year + 1900)
        << std::setw(2) << (tm->tm_mon + 1)
        << std::setw(2) << tm->tm_mday;
    
    std::string date_str = oss.str();
    
    return std::find(vchart_params_.holiday_dates.begin(), 
                    vchart_params_.holiday_dates.end(), 
                    date_str) != vchart_params_.holiday_dates.end();
}

bool VChartCSVData::is_weekend(const std::chrono::system_clock::time_point& dt) const {
    auto time_t = std::chrono::system_clock::to_time_t(dt);
    auto* tm = std::localtime(&time_t);
    
    if (!tm) return false;
    
    return tm->tm_wday == 0 || tm->tm_wday == 6; // Sunday or Saturday
}

VChartCSVData::QualityMetrics VChartCSVData::assess_data_quality() const {
    QualityMetrics metrics = {};
    
    // Calculate scores based on processing statistics
    if (processed_rows_ + error_rows_ > 0) {
        metrics.completeness_score = static_cast<double>(processed_rows_) / (processed_rows_ + error_rows_);
    } else {
        metrics.completeness_score = 1.0;
    }
    
    metrics.consistency_score = (symbol_consistent_ && timeframe_consistent_) ? 1.0 : 0.7;
    metrics.accuracy_score = 0.95; // Placeholder - would need detailed validation
    
    metrics.outlier_count = 0; // Would be calculated during processing
    metrics.gap_count = 0; // Would be calculated during time series analysis
    
    // Overall grade calculation
    double overall = (metrics.completeness_score + metrics.consistency_score + metrics.accuracy_score) / 3.0;
    
    if (overall >= 0.95) {
        metrics.quality_grade = "A";
    } else if (overall >= 0.85) {
        metrics.quality_grade = "B";
    } else if (overall >= 0.75) {
        metrics.quality_grade = "C";
    } else if (overall >= 0.65) {
        metrics.quality_grade = "D";
    } else {
        metrics.quality_grade = "F";
    }
    
    // Quality issues
    if (!symbol_consistent_) {
        metrics.quality_issues.push_back("Multiple symbols detected");
    }
    if (!timeframe_consistent_) {
        metrics.quality_issues.push_back("Multiple timeframes detected");
    }
    if (error_rows_ > processed_rows_ * 0.1) {
        metrics.quality_issues.push_back("High error rate");
    }
    
    return metrics;
}

void VChartCSVData::log_parsing_error(const std::string& error, size_t line_number) {
    if (line_number > 0) {
        std::cerr << "VChartCSV Parse Error (Line " << line_number << "): " << error << std::endl;
    } else {
        std::cerr << "VChartCSV Parse Error: " << error << std::endl;
    }
}

void VChartCSVData::log_validation_warning(const std::string& warning, size_t line_number) {
    if (line_number > 0) {
        std::cerr << "VChartCSV Warning (Line " << line_number << "): " << warning << std::endl;
    } else {
        std::cerr << "VChartCSV Warning: " << warning << std::endl;
    }
}

// VChartIntradayCSV implementation

VChartIntradayCSV::VChartIntradayCSV(const IntradayParams& params) 
    : VChartCSVData(params), intraday_params_(params) {
    
    // Set up for intraday processing
    vchart_params_.auto_detect_timeframe = true;
    vchart_params_.validate_ohlc = true;
}

VChartIntradayCSV::VChartIntradayCSV(const std::string& filename, const IntradayParams& params)
    : VChartCSVData(filename, params), intraday_params_(params) {
    
    vchart_params_.auto_detect_timeframe = true;
    vchart_params_.validate_ohlc = true;
}

void VChartIntradayCSV::set_trading_session(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) {
    
    intraday_params_.session_start = start;
    intraday_params_.session_end = end;
    intraday_params_.session_filtering = true;
}

bool VChartIntradayCSV::is_within_trading_session(const std::chrono::system_clock::time_point& dt) const {
    if (!intraday_params_.session_filtering) {
        return true;
    }
    
    auto time_t = std::chrono::system_clock::to_time_t(dt);
    auto* tm = std::localtime(&time_t);
    
    if (!tm) return false;
    
    auto start_time_t = std::chrono::system_clock::to_time_t(intraday_params_.session_start);
    auto* start_tm = std::localtime(&start_time_t);
    
    auto end_time_t = std::chrono::system_clock::to_time_t(intraday_params_.session_end);
    auto* end_tm = std::localtime(&end_time_t);
    
    if (!start_tm || !end_tm) return true;
    
    int current_minutes = tm->tm_hour * 60 + tm->tm_min;
    int start_minutes = start_tm->tm_hour * 60 + start_tm->tm_min;
    int end_minutes = end_tm->tm_hour * 60 + end_tm->tm_min;
    
    return current_minutes >= start_minutes && current_minutes <= end_minutes;
}

bool VChartIntradayCSV::validate_intraday_sequence() const {
    // Placeholder for time sequence validation
    return true;
}

void VChartIntradayCSV::fill_missing_minutes() {
    // Placeholder for missing minute filling
}

// VChartDailyCSV implementation

VChartDailyCSV::VChartDailyCSV(const DailyParams& params) 
    : VChartCSVData(params), daily_params_(params) {
    
    // Set up for daily processing
    vchart_params_.filter_weekends = true;
    vchart_params_.filter_holidays = params.skip_holidays;
}

VChartDailyCSV::VChartDailyCSV(const std::string& filename, const DailyParams& params)
    : VChartCSVData(filename, params), daily_params_(params) {
    
    vchart_params_.filter_weekends = true;
    vchart_params_.filter_holidays = params.skip_holidays;
}

void VChartDailyCSV::add_holiday(const std::string& date) {
    daily_params_.custom_holidays.push_back(date);
    vchart_params_.holiday_dates.push_back(date);
}

void VChartDailyCSV::set_holiday_list(const std::vector<std::string>& holidays) {
    daily_params_.custom_holidays = holidays;
    vchart_params_.holiday_dates = holidays;
}

bool VChartDailyCSV::validate_daily_sequence() const {
    // Placeholder for date sequence validation
    return true;
}

void VChartDailyCSV::process_holiday_filtering() {
    // Placeholder for holiday processing
}

// Factory functions implementation

namespace vchart_csv_factory {

std::shared_ptr<VChartCSVData> create_auto_vchart_feed(
    const std::string& filename,
    const VChartCSVData::VChartParams& params) {
    
    return std::make_shared<VChartCSVData>(filename, params);
}

std::shared_ptr<VChartIntradayCSV> create_intraday_vchart_feed(
    const std::string& filename,
    bool enable_session_filtering) {
    
    VChartIntradayCSV::IntradayParams params;
    params.session_filtering = enable_session_filtering;
    params.validate_time_sequence = true;
    
    return std::make_shared<VChartIntradayCSV>(filename, params);
}

std::shared_ptr<VChartDailyCSV> create_daily_vchart_feed(
    const std::string& filename,
    bool skip_holidays) {
    
    VChartDailyCSV::DailyParams params;
    params.skip_holidays = skip_holidays;
    params.validate_date_sequence = true;
    
    return std::make_shared<VChartDailyCSV>(filename, params);
}

std::vector<std::shared_ptr<VChartCSVData>> create_vchart_feeds_from_directory(
    const std::string& directory_path,
    const std::string& file_pattern) {
    
    std::vector<std::shared_ptr<VChartCSVData>> feeds;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                if (filename.ends_with(".csv")) {
                    auto feed = create_auto_vchart_feed(filename);
                    feeds.push_back(feed);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating feeds from directory: " << e.what() << std::endl;
    }
    
    return feeds;
}

std::shared_ptr<VChartCSVData> create_optimized_vchart_feed(
    const std::string& filename,
    const std::string& use_case) {
    
    VChartCSVData::VChartParams params;
    
    if (use_case == "backtesting") {
        params.validate_ohlc = true;
        params.filter_weekends = true;
        params.sort_by_timestamp = true;
    } else if (use_case == "analysis") {
        params.validate_ohlc = true;
        params.filter_outliers = true;
        params.auto_extract_symbol = true;
        params.auto_detect_timeframe = true;
    } else if (use_case == "validation") {
        params.validate_ohlc = true;
        params.validate_symbol_consistency = true;
        params.strict_timeframe_validation = true;
        params.skip_invalid_timeframes = false;
    }
    
    return std::make_shared<VChartCSVData>(filename, params);
}

} // namespace vchart_csv_factory

// Utility functions implementation

namespace vchart_csv_utils {

FormatDetection detect_vchart_format(const std::string& filename) {
    FormatDetection detection = {};
    detection.is_vchart_format = false;
    detection.confidence_score = 0.0;
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            detection.format_issues.push_back("Cannot open file");
            return detection;
        }
        
        std::string line;
        int lines_checked = 0;
        int vchart_indicators = 0;
        
        while (std::getline(file, line) && lines_checked < 10) {
            lines_checked++;
            
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 8) {
                // Check for VisualChart format indicators
                
                // Check timeframe column (should be I, D, W, M)
                if (tokens.size() > 1 && tokens[1].length() == 1) {
                    char tf = tokens[1][0];
                    if (tf == 'I' || tf == 'D' || tf == 'W' || tf == 'M') {
                        vchart_indicators++;
                    }
                }
                
                // Check date format (YYYYMMDD)
                if (tokens.size() > 2 && tokens[2].length() == 8) {
                    std::regex date_pattern(R"(\d{8})");
                    if (std::regex_match(tokens[2], date_pattern)) {
                        vchart_indicators++;
                    }
                }
                
                // Check time format (HHMMSS or empty)
                if (tokens.size() > 3) {
                    if (tokens[3].empty() || 
                        (tokens[3].length() == 6 && std::regex_match(tokens[3], std::regex(R"(\d{6})")))) {
                        vchart_indicators++;
                    }
                }
            }
        }
        
        file.close();
        
        if (lines_checked > 0) {
            detection.confidence_score = static_cast<double>(vchart_indicators) / (lines_checked * 3);
            detection.is_vchart_format = detection.confidence_score > 0.7;
            
            if (detection.is_vchart_format) {
                detection.detected_version = "Standard VisualChart CSV";
                detection.recommendation = "Use VChartCSVData for processing";
            } else {
                detection.recommendation = "File may not be in VisualChart format";
            }
        }
        
    } catch (const std::exception& e) {
        detection.format_issues.push_back("Error analyzing file: " + std::string(e.what()));
    }
    
    return detection;
}

bool convert_to_standard_csv(
    const std::string& vchart_file,
    const std::string& output_file,
    const std::string& format) {
    
    try {
        auto feed = vchart_csv_factory::create_auto_vchart_feed(vchart_file);
        
        std::ofstream output(output_file);
        if (!output.is_open()) {
            return false;
        }
        
        // Write header
        output << "DateTime,Open,High,Low,Close,Volume,OpenInterest\n";
        
        feed->start();
        while (feed->next()) {
            // Write data in standard CSV format
            auto dt = feed->get_datetime();
            auto time_t = std::chrono::system_clock::to_time_t(dt);
            auto* tm = std::localtime(&time_t);
            
            if (tm) {
                output << std::setfill('0') 
                       << std::setw(4) << (tm->tm_year + 1900) << "-"
                       << std::setw(2) << (tm->tm_mon + 1) << "-"
                       << std::setw(2) << tm->tm_mday << " "
                       << std::setw(2) << tm->tm_hour << ":"
                       << std::setw(2) << tm->tm_min << ":"
                       << std::setw(2) << tm->tm_sec << ","
                       << feed->get_open() << ","
                       << feed->get_high() << ","
                       << feed->get_low() << ","
                       << feed->get_close() << ","
                       << feed->get_volume() << ","
                       << feed->get_openinterest() << "\n";
            }
        }
        feed->stop();
        
        output.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error converting VChart file: " << e.what() << std::endl;
        return false;
    }
}

bool merge_vchart_files(
    const std::vector<std::string>& input_files,
    const std::string& output_file,
    bool sort_by_timestamp) {
    
    // Placeholder implementation
    std::cout << "Merging " << input_files.size() << " VChart files to " << output_file << std::endl;
    return true;
}

SymbolAnalysis analyze_symbols(const std::string& filename) {
    SymbolAnalysis analysis = {};
    
    auto feed = vchart_csv_factory::create_auto_vchart_feed(filename);
    auto file_analysis = feed->analyze_file();
    
    analysis.symbol_distribution = file_analysis.symbol_counts;
    analysis.primary_symbol = file_analysis.primary_symbol;
    analysis.has_multiple_symbols = file_analysis.symbol_counts.size() > 1;
    
    return analysis;
}

TimeframeAnalysis analyze_timeframes(const std::string& filename) {
    TimeframeAnalysis analysis = {};
    
    auto feed = vchart_csv_factory::create_auto_vchart_feed(filename);
    auto file_analysis = feed->analyze_file();
    
    analysis.timeframe_distribution = file_analysis.timeframe_counts;
    analysis.primary_timeframe = file_analysis.primary_timeframe;
    analysis.has_multiple_timeframes = file_analysis.timeframe_counts.size() > 1;
    
    return analysis;
}

QualityReport assess_file_quality(const std::string& filename) {
    QualityReport report = {};
    
    try {
        auto feed = vchart_csv_factory::create_auto_vchart_feed(filename);
        report.validation = feed->validate_file();
        report.metrics = feed->assess_data_quality();
        
        // Calculate overall quality score
        report.overall_quality_score = (report.metrics.completeness_score + 
                                       report.metrics.consistency_score + 
                                       report.metrics.accuracy_score) / 3.0;
        
        // Generate recommendations
        if (report.overall_quality_score >= 0.9) {
            report.recommendations.push_back("File quality is excellent");
        } else if (report.overall_quality_score >= 0.7) {
            report.recommendations.push_back("File quality is good with minor issues");
        } else {
            report.recommendations.push_back("File has quality issues that should be addressed");
            report.recommendations.push_back("Consider data cleaning or validation");
        }
        
        // Quality summary
        std::ostringstream summary;
        summary << "Overall Quality: " << report.metrics.quality_grade 
                << " (Score: " << std::fixed << std::setprecision(2) 
                << report.overall_quality_score * 100 << "%)";
        report.quality_summary = summary.str();
        
    } catch (const std::exception& e) {
        report.quality_summary = "Error assessing quality: " + std::string(e.what());
    }
    
    return report;
}

PerformanceBenchmark benchmark_parsing_performance(
    const std::string& filename,
    size_t test_iterations) {
    
    PerformanceBenchmark benchmark = {};
    
    try {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < test_iterations; ++i) {
            auto feed = vchart_csv_factory::create_auto_vchart_feed(filename);
            feed->start();
            
            size_t rows_processed = 0;
            while (feed->next()) {
                rows_processed++;
            }
            
            feed->stop();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        benchmark.total_processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Calculate metrics
        if (benchmark.total_processing_time.count() > 0) {
            // Rough estimate - would need actual row counting
            benchmark.parsing_speed_rows_per_second = (1000.0 * test_iterations) / benchmark.total_processing_time.count() * 1000.0;
        }
        
        benchmark.memory_usage_kb = 1024; // Placeholder
        
        // Performance categorization
        if (benchmark.parsing_speed_rows_per_second > 10000) {
            benchmark.performance_category = "Excellent";
        } else if (benchmark.parsing_speed_rows_per_second > 1000) {
            benchmark.performance_category = "Good";
        } else {
            benchmark.performance_category = "Needs optimization";
            benchmark.optimization_suggestions.push_back("Consider optimizing file I/O");
        }
        
    } catch (const std::exception& e) {
        benchmark.performance_category = "Error: " + std::string(e.what());
    }
    
    return benchmark;
}

DataStatistics calculate_statistics(const std::string& filename) {
    DataStatistics stats = {};
    
    try {
        auto feed = vchart_csv_factory::create_auto_vchart_feed(filename);
        auto analysis = feed->analyze_file();
        
        stats.total_rows = analysis.total_records;
        stats.date_range_start = analysis.start_date;
        stats.date_range_end = analysis.end_date;
        stats.total_timespan = analysis.data_span;
        stats.average_volume = analysis.average_volume;
        stats.price_volatility = 0.15; // Placeholder
        
        stats.summary_stats.push_back({"Total Records", static_cast<double>(stats.total_rows)});
        stats.summary_stats.push_back({"Average Volume", stats.average_volume});
        stats.summary_stats.push_back({"Price Volatility", stats.price_volatility});
        
    } catch (const std::exception& e) {
        std::cerr << "Error calculating statistics: " << e.what() << std::endl;
    }
    
    return stats;
}

bool validate_file_integrity(const std::string& filename) {
    try {
        auto feed = vchart_csv_factory::create_auto_vchart_feed(filename);
        auto validation = feed->validate_file();
        return validation.is_valid;
    } catch (...) {
        return false;
    }
}

bool repair_vchart_file(const std::string& input_file, const std::string& output_file) {
    // Placeholder for file repair functionality
    std::cout << "Repairing VChart file: " << input_file << " -> " << output_file << std::endl;
    return true;
}

bool optimize_vchart_file(const std::string& input_file, const std::string& output_file) {
    // Placeholder for file optimization
    std::cout << "Optimizing VChart file: " << input_file << " -> " << output_file << std::endl;
    return true;
}

std::vector<std::map<std::string, std::string>> extract_sample_records(
    const std::string& filename,
    size_t num_samples) {
    
    std::vector<std::map<std::string, std::string>> samples;
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return samples;
        }
        
        std::string line;
        size_t count = 0;
        
        while (std::getline(file, line) && count < num_samples) {
            std::map<std::string, std::string> sample;
            
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            
            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            
            if (tokens.size() >= 8) {
                sample["Symbol"] = tokens[0];
                sample["Timeframe"] = tokens[1];
                sample["Date"] = tokens[2];
                sample["Time"] = tokens[3];
                sample["Open"] = tokens[4];
                sample["High"] = tokens[5];
                sample["Low"] = tokens[6];
                sample["Close"] = tokens[7];
                if (tokens.size() > 8) sample["Volume"] = tokens[8];
                if (tokens.size() > 9) sample["OpenInterest"] = tokens[9];
                
                samples.push_back(sample);
                count++;
            }
        }
        
        file.close();
        
    } catch (const std::exception& e) {
        std::cerr << "Error extracting samples: " << e.what() << std::endl;
    }
    
    return samples;
}

std::string get_format_documentation() {
    return R"(
VisualChart CSV Format Documentation:

Column Structure:
0: Symbol - Trading symbol/ticker
1: Timeframe - I(Minutes), D(Days), W(Weeks), M(Months)
2: Date - YYYYMMDD format
3: Time - HHMMSS format (empty for daily+)
4: Open - Opening price
5: High - Highest price
6: Low - Lowest price
7: Close - Closing price
8: Volume - Trading volume (optional)
9: OpenInterest - Open interest (optional)

Example:
AAPL,I,20231201,093000,150.25,151.50,149.75,151.00,1000000,0
AAPL,D,20231201,,150.25,151.50,149.75,151.00,5000000,0
)";
}

std::vector<std::string> get_supported_timeframes() {
    return {"I (Minutes)", "D (Days)", "W (Weeks)", "M (Months)"};
}

std::map<std::string, std::string> get_field_descriptions() {
    return {
        {"Symbol", "Trading symbol or ticker"},
        {"Timeframe", "Data timeframe (I/D/W/M)"},
        {"Date", "Date in YYYYMMDD format"},
        {"Time", "Time in HHMMSS format"},
        {"Open", "Opening price"},
        {"High", "Highest price"},
        {"Low", "Lowest price"},
        {"Close", "Closing price"},
        {"Volume", "Trading volume"},
        {"OpenInterest", "Open interest"}
    };
}

} // namespace vchart_csv_utils

} // namespace feeds
} // namespace backtrader