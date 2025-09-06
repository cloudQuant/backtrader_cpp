#pragma once

#include "../store.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

namespace backtrader {
namespace stores {

/**
 * CTPStore - CTP (Comprehensive Transaction Platform) store implementation
 * 
 * Store provider for CTP futures trading platform.
 * Handles connections to CTP front-end servers for Chinese futures markets.
 */
class CTPStore : public Store {
public:
    // Parameters structure
    struct Params {
        std::string trader_server;      // Trading server address
        std::string md_server;          // Market data server address
        std::string broker_id;          // Broker ID
        std::string user_id;            // User ID
        std::string password;           // Password
        std::string auth_code;          // Authentication code
        std::string app_id;             // Application ID
        std::string user_product_info;  // User product info
        bool auto_login = true;         // Auto login flag
        int request_timeout = 30;       // Request timeout in seconds
        int reconnect_interval = 5;     // Reconnect interval in seconds
        int max_reconnect_attempts = 10; // Maximum reconnect attempts
    };

    // CTP connection state
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        LOGGED_IN,
        ERROR
    };

    // Static factory method for singleton pattern
    static std::shared_ptr<CTPStore> getInstance(const Params& params);
    
    virtual ~CTPStore() = default;

    // Store interface implementation
    std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                       const std::map<std::string, std::any>& kwargs = {}) override;
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // CTP specific methods
    bool connect();
    void disconnect();
    bool is_connected() const;
    bool is_logged_in() const;
    
    // Login/logout
    bool login();
    bool logout();
    
    // Market data operations
    bool subscribe_market_data(const std::vector<std::string>& instruments);
    bool unsubscribe_market_data(const std::vector<std::string>& instruments);
    
    // Trading operations
    std::string insert_order(const std::map<std::string, std::any>& order_data);
    bool cancel_order(const std::string& order_ref);
    
    // Query operations
    std::vector<std::map<std::string, std::any>> query_instruments();
    std::map<std::string, std::any> query_account();
    std::vector<std::map<std::string, std::any>> query_positions();
    std::vector<std::map<std::string, std::any>> query_orders();
    std::vector<std::map<std::string, std::any>> query_trades();
    
    // Settlement operations
    bool confirm_settlement_info();
    
    // Properties
    ConnectionState get_connection_state() const { return connection_state_; }
    const std::string& get_trading_day() const { return trading_day_; }
    const std::string& get_user_id() const { return user_id_; }
    const std::string& get_broker_id() const { return broker_id_; }

private:
    CTPStore(const Params& params);
    
    // Singleton management
    static std::shared_ptr<CTPStore> instance_;
    static std::mutex instance_mutex_;
    
    // Parameters
    Params params_;
    
    // Connection details
    std::string trader_server_;
    std::string md_server_;
    std::string broker_id_;
    std::string user_id_;
    std::string password_;
    std::string auth_code_;
    std::string app_id_;
    std::string user_product_info_;
    
    // Connection state
    std::atomic<ConnectionState> connection_state_{ConnectionState::DISCONNECTED};
    std::string trading_day_;
    
    // Session information
    int front_id_ = 0;
    int session_id_ = 0;
    std::string max_order_ref_ = "0";
    
    // Request management
    std::atomic<int> request_id_{0};
    std::map<int, std::string> pending_requests_;
    std::mutex request_mutex_;
    
    // Data queues
    std::queue<std::map<std::string, std::any>> tick_queue_;
    std::queue<std::map<std::string, std::any>> trade_queue_;
    std::queue<std::map<std::string, std::any>> order_queue_;
    std::mutex data_mutex_;
    
    // Threading
    std::thread trader_thread_;
    std::thread md_thread_;
    std::atomic<bool> should_stop_{false};
    
    // CTP API interfaces (forward declarations)
    void* trader_api_ = nullptr;
    void* md_api_ = nullptr;
    void* trader_spi_ = nullptr;
    void* md_spi_ = nullptr;
    
    // Internal methods
    void initialize_apis();
    void cleanup_apis();
    
    // Request ID management
    int get_next_request_id();
    
    // Threading workers
    void trader_worker();
    void md_worker();
    
    // Callback handlers (to be implemented with actual CTP SPI)
    void on_front_connected();
    void on_front_disconnected(int reason);
    void on_rsp_authenticate(const std::map<std::string, std::any>& response, 
                            const std::map<std::string, std::any>& error, 
                            int request_id, bool is_last);
    void on_rsp_user_login(const std::map<std::string, std::any>& response, 
                          const std::map<std::string, std::any>& error, 
                          int request_id, bool is_last);
    void on_rsp_user_logout(const std::map<std::string, std::any>& response, 
                           const std::map<std::string, std::any>& error, 
                           int request_id, bool is_last);
    
    // Market data callbacks
    void on_rtn_depth_market_data(const std::map<std::string, std::any>& market_data);
    
    // Trading callbacks
    void on_rtn_order(const std::map<std::string, std::any>& order);
    void on_rtn_trade(const std::map<std::string, std::any>& trade);
    void on_err_rtn_order_insert(const std::map<std::string, std::any>& order, 
                                const std::map<std::string, std::any>& error);
    
    // Utility methods
    std::string generate_order_ref();
    std::map<std::string, std::any> create_instrument_id(const std::string& symbol);
};

} // namespace stores
} // namespace backtrader