#pragma once

#include "../feed.h"
#include "../stores/ibstore.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <queue>

namespace backtrader {
namespace feeds {

/**
 * IBData - Interactive Brokers data feed
 * 
 * Provides real-time and historical data from Interactive Brokers
 * through the IB TWS/Gateway API.
 */
class IBData : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::string symbol;
        std::string sectype = "STK";  // Security type (STK, FUT, OPT, etc.)
        std::string exchange = "SMART";
        std::string currency = "USD";
        std::string expiry;           // For futures/options
        double strike = 0.0;          // For options
        std::string right;            // For options (C/P)
        std::string multiplier;       // Contract multiplier
        
        // Data parameters
        bool historical = true;
        std::string what = "TRADES";  // What to show (TRADES, MIDPOINT, BID, ASK)
        bool use_rth = true;          // Use regular trading hours
        std::string format_date = "1"; // Date format
        bool keep_up_to_date = false; // Keep historical data up to date
        
        // Historical data parameters
        std::string duration = "1 D";  // Duration (e.g., "1 D", "1 W", "1 M")
        std::string bar_size = "1 min"; // Bar size
        std::chrono::system_clock::time_point end_datetime; // End date/time
        
        // Real-time parameters
        bool real_time = false;
        int tick_type = 1;            // RT Volume tick type
        
        // Connection parameters
        bool reconnect = true;
        int reconnect_timeout = 5;
    };

    IBData(const Params& params);
    virtual ~IBData() = default;

    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;

    // IB-specific methods
    void set_contract_details(const std::string& symbol,
                             const std::string& sectype,
                             const std::string& exchange,
                             const std::string& currency);
    
    void enable_real_time();
    void disable_real_time();
    void request_historical_data();
    void cancel_historical_data();
    void cancel_real_time_data();

    // Contract management
    std::map<std::string, std::any> get_contract() const;
    void set_contract(const std::map<std::string, std::any>& contract);

    // Properties
    const std::string& get_symbol() const { return params_.symbol; }
    const std::string& get_sectype() const { return params_.sectype; }
    bool is_historical() const { return params_.historical; }
    bool is_real_time() const { return params_.real_time; }

private:
    // Parameters
    Params params_;
    
    // Store reference
    std::shared_ptr<stores::IBStore> store_;
    
    // Request IDs
    int historical_req_id_ = -1;
    int real_time_req_id_ = -1;
    
    // Data state
    std::queue<std::vector<double>> data_queue_;
    bool historical_complete_ = false;
    bool real_time_active_ = false;
    
    // Contract details
    std::map<std::string, std::any> contract_;
    
    // Timing
    std::chrono::system_clock::time_point last_data_time_;
    
    // Internal methods
    void initialize_store();
    void create_contract();
    void validate_contract();
    
    // Historical data handling
    void process_historical_bar(const std::map<std::string, std::any>& bar);
    void on_historical_data_end();
    void on_historical_data_error(const std::string& error);
    
    // Real-time data handling
    void process_real_time_bar(const std::map<std::string, std::any>& bar);
    void process_tick_data(const std::map<std::string, std::any>& tick);
    
    // Data conversion
    std::vector<double> convert_ib_bar(const std::map<std::string, std::any>& bar) const;
    std::chrono::system_clock::time_point parse_ib_datetime(const std::string& datetime_str) const;
    
    // Contract validation
    bool validate_security_type() const;
    bool validate_exchange() const;
    bool validate_currency() const;
    
    // Request management
    int get_next_request_id();
    void cleanup_requests();
    
    // Error handling
    void handle_ib_error(const std::map<std::string, std::any>& error);
    void handle_connection_error();
    
    // Reconnection logic
    void attempt_reconnection();
    bool should_reconnect() const;
    
    // Data validation
    bool validate_bar_data(const std::vector<double>& bar) const;
    void fill_data_gap(const std::chrono::system_clock::time_point& gap_start,
                      const std::chrono::system_clock::time_point& gap_end);
    
    // Utility methods
    std::string build_contract_string() const;
    std::string get_ib_bar_size() const;
    std::string get_ib_duration() const;
    std::string format_ib_datetime(const std::chrono::system_clock::time_point& dt) const;
};

} // namespace feeds
} // namespace backtrader