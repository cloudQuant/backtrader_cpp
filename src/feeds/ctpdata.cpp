#include "../../include/feeds/ctpdata.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <regex>

namespace backtrader {
namespace feeds {

// Static field mapping initialization
const std::map<std::string, std::string> CTPData::field_mapping_ = CTPData::create_field_mapping();

// Static trading sessions initialization
const std::map<std::string, std::vector<CTPData::TradingSession>> CTPData::exchange_sessions_ = 
    CTPData::create_exchange_sessions();

CTPData::CTPData(const Params& params) : params_(params) {
    initialize_store();
    validate_instrument();
}

void CTPData::start() {
    if (!store_) {
        throw std::runtime_error("CTP store not initialized");
    }
    
    // Subscribe to market data if enabled
    if (params_.subscribe_market_data) {
        subscribe_market_data();
    }
    
    // Initialize retry logic
    reset_retry_count();
    
    std::cout << "CTPData started for instrument: " << params_.instrument_id << std::endl;
}

void CTPData::stop() {
    if (subscribed_) {
        unsubscribe_market_data();
    }
    
    if (store_) {
        store_->disconnect();
    }
    
    std::cout << "CTPData stopped for instrument: " << params_.instrument_id << std::endl;
}

bool CTPData::next() {
    if (!store_ || !subscribed_) {
        return false;
    }
    
    // Check for new market data
    if (!market_data_queue_.empty()) {
        auto market_data = market_data_queue_.front();
        market_data_queue_.pop();
        
        process_market_data(market_data);
        
        // Convert CTP data to backtrader format
        auto bar_data = convert_ctp_market_data(market_data);
        
        if (!bar_data.empty()) {
            // Update lines with new data
            update_lines(bar_data);
            return true;
        }
    }
    
    // Check for tick data if enabled
    if (params_.tick_data && !tick_queue_.empty()) {
        auto tick_data = tick_queue_.front();
        tick_queue_.pop();
        
        process_tick_data(tick_data);
        
        // Create bar from accumulated ticks if needed
        if (has_new_bar_data()) {
            create_bar_from_ticks();
            return true;
        }
    }
    
    return false;
}

void CTPData::preload() {
    // CTP is primarily for real-time data
    // Historical data preloading not typically supported
    if (params_.historical) {
        std::cout << "Warning: CTP historical data preloading not supported" << std::endl;
    }
}

void CTPData::set_instrument(const std::string& instrument_id) {
    if (subscribed_) {
        unsubscribe_market_data();
    }
    
    params_.instrument_id = instrument_id;
    validate_instrument();
    
    if (params_.subscribe_market_data) {
        subscribe_market_data();
    }
}

void CTPData::subscribe_market_data() {
    if (!store_) {
        throw std::runtime_error("CTP store not initialized");
    }
    
    if (subscribed_) {
        return;
    }
    
    try {
        store_->subscribe_market_data(params_.instrument_id);
        subscribed_ = true;
        last_data_time_ = std::chrono::system_clock::now();
        
        std::cout << "Subscribed to market data for: " << params_.instrument_id << std::endl;
    } catch (const std::exception& e) {
        handle_subscription_error();
        throw;
    }
}

void CTPData::unsubscribe_market_data() {
    if (!store_ || !subscribed_) {
        return;
    }
    
    try {
        store_->unsubscribe_market_data(params_.instrument_id);
        subscribed_ = false;
        
        std::cout << "Unsubscribed from market data for: " << params_.instrument_id << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error unsubscribing from market data: " << e.what() << std::endl;
    }
}

void CTPData::enable_tick_data() {
    params_.tick_data = true;
    if (subscribed_) {
        // Re-subscribe with tick data enabled
        unsubscribe_market_data();
        subscribe_market_data();
    }
}

void CTPData::disable_tick_data() {
    params_.tick_data = false;
    if (subscribed_) {
        // Re-subscribe with tick data disabled
        unsubscribe_market_data();
        subscribe_market_data();
    }
}

void CTPData::initialize_store() {
    if (!store_) {
        // Create CTP store instance
        stores::CTPStore::Params store_params;
        store_params.broker_id = "9999";  // Default broker ID
        store_params.user_id = "";        // To be set by user
        store_params.password = "";       // To be set by user
        store_params.app_id = "simnow_client_test";
        store_params.auth_code = "0000000000000000";
        
        store_ = std::make_shared<stores::CTPStore>(store_params);
    }
}

void CTPData::validate_instrument() {
    if (params_.instrument_id.empty()) {
        throw std::invalid_argument("Instrument ID cannot be empty");
    }
    
    if (!is_valid_instrument(params_.instrument_id)) {
        throw std::invalid_argument("Invalid instrument ID: " + params_.instrument_id);
    }
}

void CTPData::process_market_data(const std::map<std::string, std::any>& market_data) {
    if (!validate_market_data(market_data)) {
        return;
    }
    
    // Update current market data
    update_current_data(market_data);
    
    // Update timestamp
    last_data_time_ = std::chrono::system_clock::now();
    
    // Reset retry count on successful data
    reset_retry_count();
}

void CTPData::process_tick_data(const std::map<std::string, std::any>& tick_data) {
    if (!validate_market_data(tick_data)) {
        return;
    }
    
    // Process tick data for bar creation
    last_tick_time_ = std::chrono::system_clock::now();
    
    // Store tick data for bar aggregation
    current_market_data_ = tick_data;
}

std::vector<double> CTPData::convert_ctp_market_data(const std::map<std::string, std::any>& data) const {
    std::vector<double> bar_data;
    
    try {
        // Standard OHLCV format
        double open = get_open_price(data);
        double high = get_high_price(data);
        double low = get_low_price(data);
        double close = get_last_price(data);
        double volume = get_volume(data);
        double open_interest = get_open_interest(data);
        
        // Create timestamp
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        bar_data = {
            static_cast<double>(timestamp),  // datetime
            open,                           // open
            high,                           // high
            low,                            // low
            close,                          // close
            volume,                         // volume
            open_interest                   // openinterest
        };
        
    } catch (const std::exception& e) {
        std::cerr << "Error converting CTP market data: " << e.what() << std::endl;
        return {};
    }
    
    return bar_data;
}

std::chrono::system_clock::time_point CTPData::parse_ctp_time(const std::string& time_str) const {
    // Parse CTP time format: "HH:MM:SS" or "YYYYMMDD HH:MM:SS"
    std::tm tm = {};
    std::istringstream ss(time_str);
    
    if (time_str.length() > 8) {
        // Full datetime format
        ss >> std::get_time(&tm, "%Y%m%d %H:%M:%S");
    } else {
        // Time only format
        ss >> std::get_time(&tm, "%H:%M:%S");
        // Use current date
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto* local_tm = std::localtime(&time_t);
        tm.tm_year = local_tm->tm_year;
        tm.tm_mon = local_tm->tm_mon;
        tm.tm_mday = local_tm->tm_mday;
    }
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

double CTPData::get_last_price(const std::map<std::string, std::any>& data) const {
    auto it = data.find("LastPrice");
    if (it != data.end()) {
        return std::any_cast<double>(it->second);
    }
    return 0.0;
}

double CTPData::get_open_price(const std::map<std::string, std::any>& data) const {
    auto it = data.find("OpenPrice");
    if (it != data.end()) {
        return std::any_cast<double>(it->second);
    }
    return get_last_price(data);
}

double CTPData::get_high_price(const std::map<std::string, std::any>& data) const {
    auto it = data.find("HighestPrice");
    if (it != data.end()) {
        return std::any_cast<double>(it->second);
    }
    return get_last_price(data);
}

double CTPData::get_low_price(const std::map<std::string, std::any>& data) const {
    auto it = data.find("LowestPrice");
    if (it != data.end()) {
        return std::any_cast<double>(it->second);
    }
    return get_last_price(data);
}

double CTPData::get_volume(const std::map<std::string, std::any>& data) const {
    auto it = data.find("Volume");
    if (it != data.end()) {
        return std::any_cast<double>(it->second);
    }
    return 0.0;
}

double CTPData::get_open_interest(const std::map<std::string, std::any>& data) const {
    auto it = data.find("OpenInterest");
    if (it != data.end()) {
        return std::any_cast<double>(it->second);
    }
    return 0.0;
}

bool CTPData::validate_market_data(const std::map<std::string, std::any>& data) const {
    // Check if data contains required fields
    if (data.find("InstrumentID") == data.end()) {
        return false;
    }
    
    if (data.find("LastPrice") == data.end()) {
        return false;
    }
    
    // Validate instrument ID matches
    try {
        auto instrument_id = std::any_cast<std::string>(data.at("InstrumentID"));
        if (instrument_id != params_.instrument_id) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

bool CTPData::is_valid_instrument(const std::string& instrument_id) const {
    // Basic validation for Chinese futures instrument format
    // Example: rb2410, au2412, IF2410, etc.
    std::regex pattern(R"([A-Za-z]{1,4}\d{4})");
    return std::regex_match(instrument_id, pattern);
}

bool CTPData::is_trading_time() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* local_tm = std::localtime(&time_t);
    
    // Check if current time is within trading sessions
    auto sessions = get_trading_sessions();
    
    for (const auto& session : sessions) {
        // Parse session times and check if current time is within range
        // This is a simplified check - real implementation would be more complex
        return true;  // Placeholder
    }
    
    return false;
}

void CTPData::handle_ctp_error(const std::map<std::string, std::any>& error) {
    std::cerr << "CTP Error for instrument " << params_.instrument_id << ": ";
    
    auto it = error.find("ErrorMsg");
    if (it != error.end()) {
        std::cerr << std::any_cast<std::string>(it->second) << std::endl;
    } else {
        std::cerr << "Unknown error" << std::endl;
    }
    
    if (params_.auto_retry && should_retry()) {
        attempt_retry();
    }
}

void CTPData::handle_subscription_error() {
    std::cerr << "Failed to subscribe to market data for: " << params_.instrument_id << std::endl;
    
    if (params_.auto_retry && should_retry()) {
        attempt_retry();
    }
}

void CTPData::handle_connection_error() {
    std::cerr << "Connection error for CTP data feed" << std::endl;
    
    if (params_.reconnect) {
        attempt_retry();
    }
}

void CTPData::attempt_retry() {
    if (retry_count_ >= params_.max_retries) {
        std::cerr << "Maximum retries reached for instrument: " << params_.instrument_id << std::endl;
        return;
    }
    
    retry_count_++;
    last_retry_time_ = std::chrono::system_clock::now();
    
    std::cout << "Retrying connection/subscription for instrument: " << params_.instrument_id
              << " (attempt " << retry_count_ << "/" << params_.max_retries << ")" << std::endl;
    
    // Attempt to reconnect and resubscribe
    if (store_) {
        store_->disconnect();
        store_->connect();
        
        if (params_.subscribe_market_data) {
            subscribe_market_data();
        }
    }
}

bool CTPData::should_retry() const {
    if (retry_count_ >= params_.max_retries) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_since_last_retry = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_retry_time_).count();
    
    return time_since_last_retry >= params_.reconnect_timeout;
}

void CTPData::reset_retry_count() {
    retry_count_ = 0;
}

bool CTPData::is_market_session() const {
    // Check if current time is within regular trading hours
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* local_tm = std::localtime(&time_t);
    
    int hour = local_tm->tm_hour;
    
    // Simplified check for Chinese futures market hours
    // Morning: 9:00-11:30, Afternoon: 13:30-15:00
    return (hour >= 9 && hour < 12) || (hour >= 13 && hour < 15);
}

bool CTPData::is_night_session() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* local_tm = std::localtime(&time_t);
    
    int hour = local_tm->tm_hour;
    
    // Night session: 21:00-02:30 (next day)
    return hour >= 21 || hour < 3;
}

std::chrono::system_clock::time_point CTPData::get_next_trading_session() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* local_tm = std::localtime(&time_t);
    
    // Calculate next trading session start time
    local_tm->tm_hour = 9;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    auto next_session = std::chrono::system_clock::from_time_t(std::mktime(local_tm));
    
    // If past today's morning session, move to next day
    if (next_session <= now) {
        next_session += std::chrono::hours(24);
    }
    
    return next_session;
}

std::string CTPData::get_product_id() const {
    // Extract product ID from instrument ID
    std::regex pattern(R"([A-Za-z]+)");
    std::smatch match;
    
    if (std::regex_search(params_.instrument_id, match, pattern)) {
        return match[0];
    }
    
    return "";
}

std::string CTPData::get_contract_month() const {
    // Extract contract month from instrument ID
    std::regex pattern(R"(\d{4})");
    std::smatch match;
    
    if (std::regex_search(params_.instrument_id, match, pattern)) {
        return match[0];
    }
    
    return "";
}

bool CTPData::is_main_contract() const {
    // Logic to determine if this is a main contract
    // This would require market data analysis
    return false;  // Placeholder
}

void CTPData::update_current_data(const std::map<std::string, std::any>& data) {
    current_market_data_ = data;
}

bool CTPData::has_new_bar_data() const {
    // Check if enough time has passed or sufficient tick data accumulated
    auto now = std::chrono::system_clock::now();
    auto time_since_last_bar = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_tick_time_).count();
    
    // Create new bar every minute (simplified)
    return time_since_last_bar >= 60;
}

void CTPData::create_bar_from_ticks() {
    // Aggregate tick data into bar data
    // This is a simplified implementation
    if (current_market_data_.empty()) {
        return;
    }
    
    // Use current market data as bar data
    auto bar_data = convert_ctp_market_data(current_market_data_);
    if (!bar_data.empty()) {
        update_lines(bar_data);
    }
}

std::map<std::string, std::string> CTPData::create_field_mapping() {
    return {
        {"InstrumentID", "symbol"},
        {"LastPrice", "close"},
        {"OpenPrice", "open"},
        {"HighestPrice", "high"},
        {"LowestPrice", "low"},
        {"Volume", "volume"},
        {"OpenInterest", "openinterest"},
        {"UpdateTime", "datetime"},
        {"BidPrice1", "bid"},
        {"AskPrice1", "ask"},
        {"BidVolume1", "bidsize"},
        {"AskVolume1", "asksize"}
    };
}

std::vector<CTPData::TradingSession> CTPData::get_trading_sessions() const {
    auto it = exchange_sessions_.find(params_.exchange_id);
    if (it != exchange_sessions_.end()) {
        return it->second;
    }
    
    // Default sessions if exchange not found
    return {
        {"09:00:00", "11:30:00", false},
        {"13:30:00", "15:00:00", false},
        {"21:00:00", "02:30:00", true}
    };
}

std::map<std::string, std::vector<CTPData::TradingSession>> CTPData::create_exchange_sessions() {
    std::map<std::string, std::vector<TradingSession>> sessions;
    
    // SHFE (Shanghai Futures Exchange)
    sessions["SHFE"] = {
        {"09:00:00", "10:15:00", false},
        {"10:30:00", "11:30:00", false},
        {"13:30:00", "15:00:00", false},
        {"21:00:00", "02:30:00", true}
    };
    
    // DCE (Dalian Commodity Exchange)
    sessions["DCE"] = {
        {"09:00:00", "10:15:00", false},
        {"10:30:00", "11:30:00", false},
        {"13:30:00", "15:00:00", false},
        {"21:00:00", "23:00:00", true}
    };
    
    // CZCE (Zhengzhou Commodity Exchange)
    sessions["CZCE"] = {
        {"09:00:00", "10:15:00", false},
        {"10:30:00", "11:30:00", false},
        {"13:30:00", "15:00:00", false},
        {"21:00:00", "23:30:00", true}
    };
    
    // CFFEX (China Financial Futures Exchange)
    sessions["CFFEX"] = {
        {"09:30:00", "11:30:00", false},
        {"13:00:00", "15:00:00", false}
    };
    
    return sessions;
}

void CTPData::update_lines(const std::vector<double>& bar_data) {
    // Update the data lines with new bar data
    // This method would be implemented in the base class
    // For now, we'll just store the data
    if (bar_data.size() >= 7) {
        // Standard OHLCV format with datetime and openinterest
        // The base class should handle line updates
        std::cout << "Updated bar data for " << params_.instrument_id 
                  << " - Close: " << bar_data[4] << ", Volume: " << bar_data[5] << std::endl;
    }
}

} // namespace feeds
} // namespace backtrader