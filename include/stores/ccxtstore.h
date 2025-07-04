#pragma once

#include "../store.h"
#include "../timeframe.h"
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace backtrader {
namespace stores {

/**
 * CCXTStore - CCXT exchange store implementation
 * 
 * API provider for CCXT feed and broker classes.
 * Supports multiple cryptocurrency exchanges through CCXT library.
 */
class CCXTStore : public Store {
public:
    // Supported granularities mapping
    using GranularityMap = std::map<std::pair<TimeFrame, int>, std::string>;
    
    // Parameters structure
    struct Params {
        std::string exchange;
        std::string currency;
        std::map<std::string, std::string> config;
        int retries = 3;
        bool debug = false;
        bool sandbox = false;
    };

    // Static factory method for singleton pattern
    static std::shared_ptr<CCXTStore> getInstance(const Params& params);
    
    virtual ~CCXTStore() = default;

    // Store interface implementation
    std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                       const std::map<std::string, std::any>& kwargs = {}) override;
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // CCXT specific methods
    std::string get_granularity(TimeFrame timeframe, int compression);
    std::map<std::string, double> get_wallet_balance(const std::map<std::string, std::any>& params = {});
    
    // Exchange operations
    std::vector<std::vector<double>> fetch_ohlcv(const std::string& symbol, 
                                                 const std::string& timeframe, 
                                                 int64_t since = 0, 
                                                 int limit = 0);
    
    std::map<std::string, double> fetch_ticker(const std::string& symbol);
    std::map<std::string, std::any> fetch_balance();
    
    // Order operations
    std::map<std::string, std::any> create_order(const std::string& symbol, 
                                                const std::string& type, 
                                                const std::string& side, 
                                                double amount, 
                                                double price = 0.0);
    
    std::map<std::string, std::any> fetch_order(const std::string& id, 
                                               const std::string& symbol = "");
    
    std::vector<std::map<std::string, std::any>> fetch_orders(const std::string& symbol = "");
    std::map<std::string, std::any> cancel_order(const std::string& id, 
                                                 const std::string& symbol = "");

    // Properties
    double get_cash() const { return cash_; }
    double get_value() const { return value_; }
    const std::string& get_exchange_name() const { return exchange_name_; }
    const std::string& get_currency() const { return currency_; }
    bool is_sandbox() const { return sandbox_; }

private:
    CCXTStore(const Params& params);
    
    // Singleton management
    static std::shared_ptr<CCXTStore> instance_;
    static std::mutex instance_mutex_;
    
    // Parameters
    Params params_;
    
    // Exchange state
    std::string exchange_name_;
    std::string currency_;
    bool sandbox_ = false;
    double cash_ = 0.0;
    double value_ = 0.0;
    
    // Granularity mapping
    static const GranularityMap granularities_;
    
    // Rate limiting
    std::chrono::milliseconds rate_limit_{1000};
    std::chrono::system_clock::time_point last_request_;
    
    // Initialize granularity mapping
    static GranularityMap initGranularities();
    
    // Retry mechanism
    template<typename Func>
    auto retry(Func&& func) -> decltype(func());
    
    // Rate limiting
    void throttle();
    
    // Exchange initialization
    void initialize_exchange();
    void fetch_initial_balance();
};

} // namespace stores
} // namespace backtrader