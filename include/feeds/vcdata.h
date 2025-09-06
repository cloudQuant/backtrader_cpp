#pragma once

#include "../feed.h"
#include "../stores/vcstore.h"
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <map>
#include <queue>
#include <atomic>
#include <mutex>

namespace backtrader {
namespace feeds {

/**
 * VCData - VisualChart Data Feed
 * 
 * VisualChart (http://www.visualchart.com) is a professional trading platform
 * that provides real-time and historical market data. This class provides
 * integration with VisualChart data feeds.
 * 
 * Features:
 * - Real-time market data streaming
 * - Historical data download
 * - Multiple timeframes support
 * - Timezone handling for global markets
 * - Market-specific time offset corrections
 * - Live/delayed data status notifications
 * - Continuous futures and tradeable contracts
 * 
 * Market Coverage:
 * - Global equities and indices
 * - Futures contracts
 * - Forex markets
 * - Commodities
 * - Cryptocurrencies
 */
class VCData : public AbstractDataBase {
public:
    // VisualChart specific parameters
    struct VCParams : public AbstractDataBase::Params {
        double qcheck = 0.5;                    // Timeout for resampling checks
        bool historical = false;                // Force historical only download
        bool millisecond = true;                // Fix HH:MM:59.999 timestamps
        std::string tradename;                  // Real asset name for trading
        bool usetimezones = true;               // Use timezone conversions
        
        // Connection parameters
        std::string host = "localhost";         // VC server host
        int port = 5555;                       // VC server port
        std::string username;                   // Username for authentication
        std::string password;                   // Password for authentication
        int timeout_ms = 5000;                 // Connection timeout
        
        // Data request parameters
        std::chrono::system_clock::time_point fromdate;  // Start date for historical data
        std::chrono::system_clock::time_point todate;    // End date for historical data
        bool backfill = true;                  // Backfill historical data
        int max_bars = 10000;                  // Maximum bars to request
        
        // Market data options
        bool include_volume = true;            // Include volume data
        bool include_openinterest = true;     // Include open interest
        bool validate_ohlc = true;             // Validate OHLC relationships
        bool filter_outliers = false;         // Filter price outliers
        double outlier_threshold = 5.0;       // Outlier detection threshold
    };
    
    // Data status states
    enum class DataStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DELAYED,
        LIVE,
        HISTORICAL,
        ERROR,
        NOTFOUND
    };
    
    // VisualChart market timezone mappings
    static const std::map<std::string, std::vector<std::string>> MARKET_TIMEZONES;
    
    // Special timezone output mappings for global indices
    static const std::map<std::string, std::string> TIMEZONE_OUTPUTS;
    
    // Markets requiring extra time offset correction
    static const std::vector<std::string> EXTRA_TIMEOFFSET_MARKETS;
    
    // Base NULL date for VB/Excel compatibility
    static const std::chrono::system_clock::time_point NULL_DATE;
    
    // Constructor variants
    VCData(const VCParams& params = VCParams{});
    VCData(const std::string& symbol, const VCParams& params = VCParams{});
    virtual ~VCData() = default;
    
    // AbstractDataBase interface
    void start() override;
    void stop() override;
    bool next() override;
    void preload() override;
    
    // Live data support
    bool is_live() const override { return true; }
    bool has_live_data() const;
    
    // VisualChart specific methods
    void set_symbol(const std::string& symbol);
    std::string get_symbol() const { return dataname_; }
    std::string get_trade_symbol() const { return tradename_; }
    
    // Market information
    std::string get_market_code() const { return market_code_; }
    std::chrono::system_clock::time_point get_time_offset() const;
    std::string get_market_timezone() const;
    
    // Connection status
    DataStatus get_data_status() const { return current_status_; }
    std::string get_status_description() const;
    bool is_connected() const;
    
    // Data statistics
    struct DataStatistics {
        size_t bars_received = 0;
        size_t ticks_received = 0;
        size_t errors_count = 0;
        std::chrono::system_clock::time_point session_start;
        std::chrono::system_clock::time_point last_update;
        double average_latency_ms = 0.0;
        std::string data_quality = "Unknown";
    };
    
    DataStatistics get_statistics() const { return statistics_; }
    
    // Market session information
    struct SessionInfo {
        std::chrono::system_clock::time_point session_start;
        std::chrono::system_clock::time_point session_end;
        bool is_trading_session = false;
        std::string session_name;
        std::vector<std::pair<std::chrono::system_clock::time_point, 
                             std::chrono::system_clock::time_point>> trading_hours;
    };
    
    SessionInfo get_session_info() const;
    bool is_in_trading_session() const;
    
    // Timezone handling
    void set_timezone(const std::string& tz);
    std::chrono::system_clock::time_point convert_market_time(
        const std::chrono::system_clock::time_point& vc_time) const;
    
    // Error handling
    struct ErrorInfo {
        int error_code;
        std::string error_message;
        std::chrono::system_clock::time_point timestamp;
        std::string context;
    };
    
    std::vector<ErrorInfo> get_recent_errors() const { return recent_errors_; }
    void clear_errors() { recent_errors_.clear(); }
    
protected:
    VCParams params_;
    std::shared_ptr<stores::VCStore> store_;
    
    // Symbol information
    std::string dataname_;              // VC data symbol (e.g., "010ESPZ21")
    std::string tradename_;             // Trading symbol (e.g., "ESPZ21")
    std::string market_code_;           // Market code (first 3 digits)
    
    // Connection state
    std::atomic<DataStatus> current_status_{DataStatus::DISCONNECTED};
    DataStatus last_status_ = DataStatus::DISCONNECTED;
    
    // Data processing
    std::queue<std::shared_ptr<MarketData>> data_queue_;
    std::mutex queue_mutex_;
    bool feeding_started_ = false;
    size_t bar_index_ = 1;              // VC uses 1-based indexing
    
    // Time management
    std::chrono::milliseconds market_offset_{0};     // Market time offset
    std::chrono::milliseconds market_offset1_{0};    // Primary offset
    std::chrono::milliseconds offset_diff_{0};       // Extra offset for special markets
    std::chrono::system_clock::time_point time_offset_;  // Local to server offset
    
    // Statistics and monitoring
    mutable DataStatistics statistics_;
    std::vector<ErrorInfo> recent_errors_;
    std::chrono::system_clock::time_point ping_timeout_;
    
    // Market data structure
    struct MarketData {
        std::chrono::system_clock::time_point timestamp;
        double open;
        double high;
        double low;
        double close;
        double volume;
        double openinterest;
        bool is_tick = false;
        int tick_count = 0;
    };
    
private:
    // Initialization helpers
    void initialize_symbol_info();
    void calculate_time_offsets();
    void setup_market_timezone();
    
    // Data processing
    bool process_market_data();
    void handle_bar_data(const std::shared_ptr<MarketData>& bar);
    void handle_tick_data(const std::shared_ptr<MarketData>& tick);
    
    // Status management
    void update_status(DataStatus new_status);
    void handle_connection_event(bool connected);
    void handle_data_quality_event(const std::string& quality);
    
    // Time utilities
    std::chrono::system_clock::time_point parse_vc_datetime(double vc_date) const;
    double convert_to_vc_date(const std::chrono::system_clock::time_point& dt) const;
    bool validate_market_time(const std::chrono::system_clock::time_point& dt) const;
    
    // Error handling
    void log_error(int code, const std::string& message, const std::string& context = "");
    void cleanup_old_errors();
    
    // Market session helpers
    bool is_market_open() const;
    std::chrono::system_clock::time_point get_next_session_start() const;
    std::chrono::system_clock::time_point get_session_end() const;
    
    // Data validation
    bool validate_bar_data(const MarketData& data) const;
    bool is_data_outlier(double price) const;
    void update_data_quality_metrics(const MarketData& data);
    
    // Threading support
    std::atomic<bool> should_stop_{false};
    std::mutex status_mutex_;
    
    // Backfill management
    void request_historical_data();
    void process_historical_response();
    
    // Symbol name processing
    std::string normalize_symbol_name(const std::string& symbol) const;
    bool is_continuous_future(const std::string& symbol) const;
    std::string extract_market_code(const std::string& symbol) const;
};

/**
 * Market Data Event Handlers
 * 
 * These are callback interfaces for handling VisualChart COM events
 */
class VCDataEventHandler {
public:
    virtual ~VCDataEventHandler() = default;
    
    // Data series events
    virtual void on_new_bar(const std::shared_ptr<VCData::MarketData>& bar) = 0;
    virtual void on_bar_update(const std::shared_ptr<VCData::MarketData>& bar) = 0;
    
    // Tick events
    virtual void on_new_tick(const std::shared_ptr<VCData::MarketData>& tick) = 0;
    virtual void on_tick_array(const std::vector<std::shared_ptr<VCData::MarketData>>& ticks) = 0;
    
    // Connection events
    virtual void on_connection_status(VCData::DataStatus status) = 0;
    virtual void on_symbol_found(bool found, const std::string& symbol) = 0;
    virtual void on_error(int code, const std::string& message) = 0;
    
    // Market events
    virtual void on_market_status(const std::string& status) = 0;
    virtual void on_trading_session_change(bool is_trading) = 0;
};

/**
 * Specialized VisualChart data feeds for different use cases
 */

/**
 * Real-time VisualChart data feed
 * Optimized for live trading and real-time analysis
 */
class VCLiveData : public VCData {
public:
    struct LiveParams : public VCParams {
        bool tick_data = false;             // Enable tick-by-tick data
        int tick_buffer_size = 1000;        // Tick buffer size
        bool auto_reconnect = true;         // Auto-reconnect on disconnect
        int reconnect_delay_ms = 5000;      // Delay between reconnect attempts
        int max_reconnect_attempts = 10;    // Maximum reconnect attempts
        bool prioritize_speed = true;       // Prioritize speed over accuracy
        double latency_threshold_ms = 100.0; // Latency alert threshold
    };
    
    VCLiveData(const LiveParams& params = LiveParams{});
    VCLiveData(const std::string& symbol, const LiveParams& params = LiveParams{});
    
    // Real-time specific methods
    void enable_tick_data(bool enable = true);
    void set_latency_monitoring(bool enable, double threshold_ms = 100.0);
    
    // Performance metrics
    double get_current_latency_ms() const;
    size_t get_tick_rate_per_second() const;
    
protected:
    LiveParams live_params_;
    
    // Real-time processing
    std::atomic<size_t> tick_count_{0};
    std::chrono::system_clock::time_point last_tick_time_;
    
private:
    void handle_live_tick(const std::shared_ptr<MarketData>& tick);
    void monitor_latency(const std::shared_ptr<MarketData>& data);
    void attempt_reconnection();
};

/**
 * Historical VisualChart data feed
 * Optimized for backtesting and historical analysis
 */
class VCHistoricalData : public VCData {
public:
    struct HistoricalParams : public VCParams {
        bool cache_data = true;             // Cache historical data
        size_t cache_size_mb = 100;         // Cache size limit
        bool validate_continuity = true;    // Validate data continuity
        bool fill_gaps = false;             // Fill data gaps
        std::string gap_fill_method = "forward"; // "forward", "backward", "interpolate"
        bool adjust_for_splits = true;      // Adjust for stock splits
        bool adjust_for_dividends = false;  // Adjust for dividends
    };
    
    VCHistoricalData(const HistoricalParams& params = HistoricalParams{});
    VCHistoricalData(const std::string& symbol, 
                     const std::chrono::system_clock::time_point& start_date,
                     const std::chrono::system_clock::time_point& end_date,
                     const HistoricalParams& params = HistoricalParams{});
    
    // Historical data methods
    void set_date_range(const std::chrono::system_clock::time_point& start,
                       const std::chrono::system_clock::time_point& end);
    void enable_data_adjustments(bool splits = true, bool dividends = false);
    
    // Data continuity
    struct ContinuityReport {
        bool is_continuous;
        size_t gap_count;
        std::chrono::seconds largest_gap;
        std::vector<std::pair<std::chrono::system_clock::time_point, 
                             std::chrono::seconds>> gaps;
        double completeness_percentage;
    };
    
    ContinuityReport analyze_continuity() const;
    bool fill_data_gaps();
    
protected:
    HistoricalParams historical_params_;
    
    // Historical data cache
    std::vector<std::shared_ptr<MarketData>> cached_data_;
    bool cache_loaded_ = false;
    
private:
    void load_historical_data();
    void validate_data_sequence();
    void apply_adjustments();
    std::shared_ptr<MarketData> interpolate_gap(
        const std::shared_ptr<MarketData>& before,
        const std::shared_ptr<MarketData>& after,
        const std::chrono::system_clock::time_point& target_time) const;
};

/**
 * Factory functions for VisualChart data feeds
 */
namespace vc_factory {

/**
 * Create a standard VisualChart data feed
 */
std::shared_ptr<VCData> create_vc_feed(
    const std::string& symbol,
    const VCData::VCParams& params = VCData::VCParams{}
);

/**
 * Create a live VisualChart data feed
 */
std::shared_ptr<VCLiveData> create_live_vc_feed(
    const std::string& symbol,
    bool enable_ticks = false
);

/**
 * Create a historical VisualChart data feed
 */
std::shared_ptr<VCHistoricalData> create_historical_vc_feed(
    const std::string& symbol,
    const std::chrono::system_clock::time_point& start_date,
    const std::chrono::system_clock::time_point& end_date
);

/**
 * Create multiple VisualChart feeds for a portfolio
 */
std::vector<std::shared_ptr<VCData>> create_portfolio_feeds(
    const std::vector<std::string>& symbols,
    const VCData::VCParams& params = VCData::VCParams{}
);

/**
 * Create a VisualChart feed with automatic symbol detection
 */
std::shared_ptr<VCData> create_auto_vc_feed(
    const std::string& partial_symbol,
    const VCData::VCParams& params = VCData::VCParams{}
);

} // namespace vc_factory

/**
 * Utility functions for VisualChart integration
 */
namespace vc_utils {

/**
 * Symbol utilities
 */
struct SymbolInfo {
    std::string full_symbol;        // Full VC symbol (e.g., "010ESPZ21")
    std::string base_symbol;        // Base symbol (e.g., "ESP")
    std::string market_code;        // Market code (e.g., "010")
    std::string contract_month;     // Contract month (e.g., "Z21")
    bool is_continuous;             // Is continuous contract
    std::string description;        // Symbol description
    std::string currency;           // Trading currency
    double tick_size;               // Minimum price increment
    double point_value;             // Point value in currency
};

SymbolInfo parse_vc_symbol(const std::string& symbol);
std::string build_vc_symbol(const std::string& market_code, 
                           const std::string& base_symbol,
                           const std::string& contract = "");

/**
 * Market information
 */
struct MarketInfo {
    std::string market_code;
    std::string market_name;
    std::string timezone;
    std::vector<std::pair<std::chrono::system_clock::time_point, 
                         std::chrono::system_clock::time_point>> trading_sessions;
    std::vector<std::chrono::system_clock::time_point> holidays;
    bool supports_tick_data;
    std::vector<TimeFrame> available_timeframes;
};

MarketInfo get_market_info(const std::string& market_code);
std::vector<std::string> get_available_markets();
bool is_market_open(const std::string& market_code);

/**
 * Data quality assessment
 */
struct QualityMetrics {
    double completeness_score;      // 0.0 to 1.0
    double timeliness_score;        // 0.0 to 1.0  
    double accuracy_score;          // 0.0 to 1.0
    size_t outlier_count;
    size_t gap_count;
    std::chrono::milliseconds average_latency;
    std::string quality_grade;      // A, B, C, D, F
};

QualityMetrics assess_data_quality(const VCData& feed);

/**
 * Performance monitoring
 */
struct PerformanceMetrics {
    double throughput_bars_per_second;
    double throughput_ticks_per_second;
    std::chrono::milliseconds average_processing_time;
    size_t memory_usage_mb;
    double cpu_utilization_percent;
    std::string performance_category;
};

PerformanceMetrics measure_performance(const VCData& feed);

/**
 * Connection diagnostics
 */
struct DiagnosticReport {
    bool connection_stable;
    std::chrono::milliseconds ping_time;
    double packet_loss_percent;
    size_t reconnection_count;
    std::vector<std::string> issues;
    std::vector<std::string> recommendations;
};

DiagnosticReport run_diagnostics(const VCData& feed);

/**
 * Configuration helpers
 */
VCData::VCParams create_optimal_config(
    const std::string& use_case,        // "live_trading", "backtesting", "research"
    const std::string& market_code = ""
);

std::map<std::string, std::string> get_recommended_settings(
    const std::string& symbol
);

/**
 * Data conversion utilities
 */
bool export_to_csv(const VCData& feed, 
                  const std::string& filename,
                  const std::string& format = "standard");

bool import_from_vc_file(const std::string& vc_filename,
                        VCData& feed);

/**
 * Error analysis
 */
struct ErrorAnalysis {
    std::map<int, size_t> error_frequency;
    std::vector<std::string> common_issues;
    std::vector<std::string> solutions;
    double error_rate_percent;
};

ErrorAnalysis analyze_errors(const VCData& feed);

} // namespace vc_utils

} // namespace feeds
} // namespace backtrader