#pragma once

#include "../store.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <chrono>

namespace backtrader {
namespace stores {

/**
 * OandaStore - OANDA broker store implementation
 * 
 * Store provider for OANDA forex broker.
 * Handles REST API connections and real-time data streaming.
 */
class OandaStore : public Store {
public:
    // Parameters structure
    struct Params {
        std::string token;           // OANDA API token
        std::string account;         // Account ID
        std::string practice = "false"; // Practice/demo account flag
        std::string account_tmout = "10.0"; // Account timeout
        std::string candle_tmout = "10.0";  // Candle timeout
        std::string ohlc_tmout = "10.0";    // OHLC timeout
        std::string stream_tmout = "10.0";  // Stream timeout
        std::string stream_chunk = "512";   // Stream chunk size
        std::string stream_nr = "1";        // Stream number
        std::string stream_timeout = "10.0"; // Stream timeout
    };

    // OANDA environment URLs
    struct Environment {
        std::string api_url;
        std::string streaming_url;
    };

    // Static factory method for singleton pattern
    static std::shared_ptr<OandaStore> getInstance(const Params& params);
    
    virtual ~OandaStore() = default;

    // Store interface implementation
    std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                       const std::map<std::string, std::any>& kwargs = {}) override;
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // OANDA specific methods
    std::map<std::string, std::any> get_account();
    std::map<std::string, std::any> get_instruments();
    std::map<std::string, std::any> get_pricing(const std::vector<std::string>& instruments);
    
    // Historical data
    std::vector<std::map<std::string, std::any>> get_history(const std::string& instrument,
                                                           const std::string& granularity,
                                                           int count = 0,
                                                           const std::string& from_time = "",
                                                           const std::string& to_time = "");
    
    // Order operations
    std::map<std::string, std::any> create_order(const std::map<std::string, std::any>& order_data);
    std::map<std::string, std::any> get_order(const std::string& order_id);
    std::vector<std::map<std::string, std::any>> get_orders();
    std::map<std::string, std::any> cancel_order(const std::string& order_id);
    
    // Position operations
    std::map<std::string, std::any> get_position(const std::string& instrument);
    std::vector<std::map<std::string, std::any>> get_positions();
    
    // Trade operations
    std::map<std::string, std::any> get_trade(const std::string& trade_id);
    std::vector<std::map<std::string, std::any>> get_trades();
    
    // Streaming
    bool start_streaming(const std::vector<std::string>& instruments);
    void stop_streaming();
    bool is_streaming() const;

    // Properties
    const std::string& get_account_id() const { return account_id_; }
    const std::string& get_token() const { return token_; }
    bool is_practice() const { return practice_; }
    const Environment& get_environment() const { return environment_; }

private:
    OandaStore(const Params& params);
    
    // Singleton management
    static std::shared_ptr<OandaStore> instance_;
    static std::mutex instance_mutex_;
    
    // Parameters
    Params params_;
    
    // Connection details
    std::string token_;
    std::string account_id_;
    bool practice_ = false;
    Environment environment_;
    
    // Streaming state
    bool streaming_ = false;
    std::thread streaming_thread_;
    std::atomic<bool> should_stop_streaming_{false};
    
    // Timeouts
    std::chrono::seconds account_timeout_;
    std::chrono::seconds candle_timeout_;
    std::chrono::seconds ohlc_timeout_;
    std::chrono::seconds stream_timeout_;
    
    // Initialize environment URLs
    void initialize_environment();
    
    // HTTP client methods
    std::string make_request(const std::string& method, 
                           const std::string& endpoint,
                           const std::map<std::string, std::string>& params = {},
                           const std::string& body = "");
    
    std::map<std::string, std::string> get_headers();
    
    // Streaming methods
    void streaming_worker(const std::vector<std::string>& instruments);
    void process_streaming_data(const std::string& data);
    
    // Utility methods
    std::string build_url(const std::string& endpoint, 
                         const std::map<std::string, std::string>& params = {});
    
    std::chrono::seconds parse_timeout(const std::string& timeout_str);
};

} // namespace stores
} // namespace backtrader