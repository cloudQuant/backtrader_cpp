#pragma once

#include "../feed.h"
#include "../stores/ctpstore.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <queue>

namespace backtrader {
namespace feeds {

/**
 * CTPData - CTP futures data feed
 * 
 * Provides real-time and historical futures data from CTP
 * for Chinese futures markets.
 */
class CTPData : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::string instrument_id;    // Instrument ID (e.g., "rb2410")
        std::string exchange_id;      // Exchange ID (e.g., "SHFE")
        
        // Data parameters
        bool historical = false;      // CTP primarily for real-time
        bool real_time = true;        // Real-time market data
        
        // Market data subscription
        bool subscribe_market_data = true;
        bool tick_data = false;       // Subscribe to tick data
        
        // Connection parameters
        bool reconnect = true;
        int reconnect_timeout = 5;
        bool auto_retry = true;
        int max_retries = 3;
    };

    CTPData(const Params& params);
    virtual ~CTPData() = default;

    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;

    // CTP-specific methods
    void set_instrument(const std::string& instrument_id);
    void subscribe_market_data();
    void unsubscribe_market_data();
    
    void enable_tick_data();
    void disable_tick_data();

    // Properties
    const std::string& get_instrument_id() const { return params_.instrument_id; }
    const std::string& get_exchange_id() const { return params_.exchange_id; }
    bool is_subscribed() const { return subscribed_; }
    bool is_tick_data_enabled() const { return params_.tick_data; }

private:
    // Parameters
    Params params_;
    
    // Store reference
    std::shared_ptr<stores::CTPStore> store_;
    
    // Subscription state
    bool subscribed_ = false;
    
    // Data queues
    std::queue<std::map<std::string, std::any>> market_data_queue_;
    std::queue<std::map<std::string, std::any>> tick_queue_;
    
    // Current market data
    std::map<std::string, std::any> current_market_data_;
    
    // Timing
    std::chrono::system_clock::time_point last_data_time_;
    std::chrono::system_clock::time_point last_tick_time_;
    
    // Retry logic
    int retry_count_ = 0;
    std::chrono::system_clock::time_point last_retry_time_;
    
    // Internal methods
    void initialize_store();
    void validate_instrument();
    
    // Market data handling
    void process_market_data(const std::map<std::string, std::any>& market_data);
    void process_tick_data(const std::map<std::string, std::any>& tick_data);
    
    // Data conversion
    std::vector<double> convert_ctp_market_data(const std::map<std::string, std::any>& data) const;
    std::chrono::system_clock::time_point parse_ctp_time(const std::string& time_str) const;
    
    // CTP data field extraction
    double get_last_price(const std::map<std::string, std::any>& data) const;
    double get_open_price(const std::map<std::string, std::any>& data) const;
    double get_high_price(const std::map<std::string, std::any>& data) const;
    double get_low_price(const std::map<std::string, std::any>& data) const;
    double get_volume(const std::map<std::string, std::any>& data) const;
    double get_open_interest(const std::map<std::string, std::any>& data) const;
    
    // Validation
    bool validate_market_data(const std::map<std::string, std::any>& data) const;
    bool is_valid_instrument(const std::string& instrument_id) const;
    bool is_trading_time() const;
    
    // Error handling
    void handle_ctp_error(const std::map<std::string, std::any>& error);
    void handle_subscription_error();
    void handle_connection_error();
    
    // Retry logic
    void attempt_retry();
    bool should_retry() const;
    void reset_retry_count();
    
    // Time utilities
    bool is_market_session() const;
    bool is_night_session() const;
    std::chrono::system_clock::time_point get_next_trading_session() const;
    
    // Instrument utilities
    std::string get_product_id() const;
    std::string get_contract_month() const;
    bool is_main_contract() const;
    
    // Data management
    void update_current_data(const std::map<std::string, std::any>& data);
    bool has_new_bar_data() const;
    void create_bar_from_ticks();
    
    // CTP specific field mappings
    static std::map<std::string, std::string> create_field_mapping();
    static const std::map<std::string, std::string> field_mapping_;
    
    // Trading session times for different exchanges
    struct TradingSession {
        std::string start_time;
        std::string end_time;
        bool is_night_session;
    };
    
    std::vector<TradingSession> get_trading_sessions() const;
    static std::map<std::string, std::vector<TradingSession>> create_exchange_sessions();
    static const std::map<std::string, std::vector<TradingSession>> exchange_sessions_;
};

} // namespace feeds
} // namespace backtrader