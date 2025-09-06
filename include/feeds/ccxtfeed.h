#pragma once

#include "../feed.h"
#include "../stores/ccxtstore.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace backtrader {
namespace feeds {

/**
 * CCXTFeed - CCXT exchange data feed
 * 
 * Provides real-time and historical data from cryptocurrency exchanges
 * through the CCXT library integration.
 */
class CCXTFeed : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::string symbol;
        std::string exchange;
        std::string timeframe = "1m";
        std::chrono::milliseconds since = std::chrono::milliseconds{0};
        int limit = 0;
        std::map<std::string, std::string> config;
        bool sandbox = false;
        bool debug = false;
        bool drop_newest = false;
        std::string ohlcv_limit = "none";
        bool historical = false;
    };

    CCXTFeed(const Params& params);
    virtual ~CCXTFeed() = default;

    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;

    // CCXT-specific methods
    void set_symbol(const std::string& symbol);
    void set_timeframe(const std::string& timeframe);
    void set_since(std::chrono::milliseconds since);
    void set_limit(int limit);
    
    // Data retrieval
    std::vector<std::vector<double>> fetch_ohlcv(int limit = 0);
    void enable_live_data();
    void disable_live_data();

    // Properties
    const std::string& get_symbol() const { return params_.symbol; }
    const std::string& get_timeframe() const { return params_.timeframe; }
    bool is_historical() const { return params_.historical; }

private:
    // Parameters
    Params params_;
    
    // Store reference
    std::shared_ptr<stores::CCXTStore> store_;
    
    // Data state
    std::vector<std::vector<double>> ohlcv_data_;
    size_t current_index_ = 0;
    bool live_data_ = false;
    
    // Timing
    std::chrono::system_clock::time_point last_fetch_;
    std::chrono::milliseconds fetch_interval_;
    
    // Internal methods
    void initialize_store();
    bool load_historical_data();
    bool fetch_new_data();
    void process_ohlcv_bar(const std::vector<double>& bar);
    
    // Data validation
    bool validate_ohlcv_data(const std::vector<double>& bar) const;
    void fill_data_lines(const std::vector<double>& bar);
    
    // Timeframe conversion
    std::chrono::milliseconds get_timeframe_duration() const;
    std::chrono::system_clock::time_point parse_timestamp(double timestamp) const;
    
    // Error handling
    void handle_fetch_error(const std::exception& e);
    
    // Data management
    void cleanup_old_data();
    bool should_fetch_new_data() const;
    void update_fetch_timing();
};

} // namespace feeds
} // namespace backtrader