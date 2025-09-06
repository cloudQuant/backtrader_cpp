#pragma once

#include "../store.h"
#include <string>
#include <chrono>
#include <random>
#include <memory>

namespace backtrader {
namespace stores {

/**
 * IBStore - Interactive Brokers store implementation
 * 
 * Singleton class wrapping an IB connection interface.
 * Manages connection to IB TWS or IB Gateway for data and broker operations.
 */
class IBStore : public Store {
public:
    // Parameters structure matching Python version
    struct Params {
        std::string host = "127.0.0.1";
        int port = 7496;
        int clientId = 0;  // 0 means generate random
        bool notifyall = false;
        bool _debug = false;
        int reconnect = 3;
        double timeout = 3.0;
        bool timeoffset = true;
        double timerefresh = 60.0;
        bool indcash = true;
    };

    // Request ID base to distinguish from order IDs
    static const int REQIDBASE = 0x01000000;

    // Static factory method for singleton pattern
    static std::shared_ptr<IBStore> getInstance(const Params& params = Params{});
    
    virtual ~IBStore() = default;

    // Store interface implementation
    std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                       const std::map<std::string, std::any>& kwargs = {}) override;
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // Lifecycle management
    void start(std::shared_ptr<DataSeries> data = nullptr, 
              std::shared_ptr<Broker> broker = nullptr) override;
    void stop() override;

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;

    // Time management
    std::chrono::system_clock::time_point getCurrentTime();
    std::chrono::milliseconds getTimeOffset() const;
    void refreshTimeOffset();

    // Request management
    int getNextReqId();
    void cancelHistoricalData(int reqId);
    void cancelRealTimeData(int reqId);

    // Parameters access
    const Params& getParams() const { return params_; }

private:
    IBStore(const Params& params);
    
    // Singleton management
    static std::shared_ptr<IBStore> instance_;
    static std::mutex instance_mutex_;
    
    // Connection state
    bool connected_ = false;
    std::mutex connection_mutex_;
    
    // Parameters
    Params params_;
    
    // Request ID management
    std::atomic<int> reqId_;
    
    // Time offset management
    std::chrono::milliseconds timeOffset_{0};
    std::chrono::system_clock::time_point lastTimeRefresh_;
    
    // Random number generator for client ID
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> clientIdDist_;
    
    // Generate random client ID if not specified
    int generateClientId();
    
    // Internal connection management
    bool doConnect();
    void doDisconnect();
    
    // Time offset calculation
    void calculateTimeOffset();
};

/**
 * RTVolume - Real-time volume data parser
 * 
 * Parses IB RTVolume tick data into structured format
 */
struct RTVolume {
    double price = 0.0;
    int size = 0;
    std::chrono::system_clock::time_point datetime;
    int volume = 0;
    double vwap = 0.0;
    bool single = false;
    
    RTVolume() = default;
    RTVolume(const std::string& rtvol, double price_override = 0.0, 
             std::chrono::milliseconds tmoffset = std::chrono::milliseconds{0});
    
private:
    void parseRTVolString(const std::string& rtvol);
    std::chrono::system_clock::time_point timestampToDateTime(const std::string& timestamp);
};

} // namespace stores
} // namespace backtrader