#pragma once

#include "../feed.h"
#include "../stores/oandastore.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <queue>

namespace backtrader {
namespace feeds {

/**
 * OandaData - OANDA forex data feed
 * 
 * Provides real-time and historical forex data from OANDA
 * through their REST API and streaming endpoints.
 */
class OandaData : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::string instrument;       // Currency pair (e.g., "EUR_USD")
        std::string granularity = "M1"; // Candle granularity
        std::string price = "M";      // Price type (M=mid, B=bid, A=ask)
        
        // Historical data parameters
        bool historical = true;
        int count = 500;              // Number of candles
        std::string from_time;        // Start time (RFC3339)
        std::string to_time;          // End time (RFC3339)
        bool smooth = false;          // Smooth prices
        bool include_first = true;    // Include first candle
        
        // Real-time parameters
        bool real_time = false;
        bool stream_prices = false;   // Stream pricing data
        
        // Connection parameters
        bool reconnect = true;
        int reconnect_timeout = 5;
        double heartbeat = 10.0;      // Heartbeat interval for streaming
    };

    OandaData(const Params& params);
    virtual ~OandaData() = default;

    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;

    // OANDA-specific methods
    void set_instrument(const std::string& instrument);
    void set_granularity(const std::string& granularity);
    void set_price_type(const std::string& price);
    
    void enable_streaming();
    void disable_streaming();
    void request_historical_data();
    
    // Data range methods
    void set_count(int count);
    void set_time_range(const std::string& from_time, const std::string& to_time);

    // Properties
    const std::string& get_instrument() const { return params_.instrument; }
    const std::string& get_granularity() const { return params_.granularity; }
    bool is_historical() const { return params_.historical; }
    bool is_streaming() const { return params_.real_time; }

private:
    // Parameters
    Params params_;
    
    // Store reference
    std::shared_ptr<stores::OandaStore> store_;
    
    // Data state
    std::queue<std::map<std::string, std::any>> candle_queue_;
    std::queue<std::map<std::string, std::any>> price_queue_;
    bool historical_complete_ = false;
    bool streaming_active_ = false;
    
    // Timing
    std::chrono::system_clock::time_point last_candle_time_;
    std::chrono::system_clock::time_point last_heartbeat_;
    
    // Internal methods
    void initialize_store();
    void validate_instrument();
    void validate_granularity();
    
    // Historical data handling
    void fetch_historical_candles();
    void process_historical_candle(const std::map<std::string, std::any>& candle);
    void on_historical_data_complete();
    
    // Streaming data handling
    void start_price_streaming();
    void stop_price_streaming();
    void process_streaming_price(const std::map<std::string, std::any>& price);
    void process_heartbeat();
    
    // Data conversion
    std::vector<double> convert_oanda_candle(const std::map<std::string, std::any>& candle) const;
    std::chrono::system_clock::time_point parse_oanda_time(const std::string& time_str) const;
    
    // Price extraction based on price type
    double extract_price(const std::map<std::string, std::any>& price_data, 
                        const std::string& component) const;
    
    // Validation
    bool validate_candle_data(const std::map<std::string, std::any>& candle) const;
    bool is_valid_instrument(const std::string& instrument) const;
    bool is_valid_granularity(const std::string& granularity) const;
    
    // Error handling
    void handle_oanda_error(const std::map<std::string, std::any>& error);
    void handle_streaming_error(const std::exception& e);
    void handle_connection_loss();
    
    // Reconnection logic
    void attempt_reconnection();
    bool should_reconnect() const;
    
    // Time utilities
    std::string format_oanda_time(const std::chrono::system_clock::time_point& time) const;
    bool is_market_open() const;
    std::chrono::system_clock::time_point get_next_market_open() const;
    
    // Data management
    void cleanup_old_data();
    bool should_request_new_data() const;
    void update_last_data_time();
    
    // Granularity mapping
    std::chrono::seconds get_granularity_duration() const;
    static std::map<std::string, std::chrono::seconds> create_granularity_map();
    static const std::map<std::string, std::chrono::seconds> granularity_map_;
};

} // namespace feeds
} // namespace backtrader