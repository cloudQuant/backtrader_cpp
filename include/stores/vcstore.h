#pragma once

#include "../store.h"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace backtrader {
namespace stores {

/**
 * VCStore - Visual Chart store implementation
 * 
 * Store provider for Visual Chart real-time data and trading.
 * Handles connections to Visual Chart servers for market data and order execution.
 */
class VCStore : public Store {
public:
    // Parameters structure
    struct Params {
        std::string host = "127.0.0.1";     // Server host
        int port = 5555;                    // Server port
        std::string username;               // Username
        std::string password;               // Password
        std::string client_name = "backtrader"; // Client name
        bool compression = true;            // Use compression
        int timeout = 30;                   // Connection timeout
        int reconnect_attempts = 3;         // Reconnection attempts
        int reconnect_delay = 5;            // Reconnection delay in seconds
        bool debug = false;                 // Debug mode
    };

    // Connection state
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        AUTHENTICATED,
        ERROR
    };

    // Static factory method for singleton pattern
    static std::shared_ptr<VCStore> getInstance(const Params& params);
    
    virtual ~VCStore() = default;

    // Store interface implementation
    std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                       const std::map<std::string, std::any>& kwargs = {}) override;
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // Visual Chart specific methods
    bool connect();
    void disconnect();
    bool is_connected() const;
    bool is_authenticated() const;
    
    // Authentication
    bool authenticate();
    
    // Market data operations
    bool subscribe_symbol(const std::string& symbol);
    bool unsubscribe_symbol(const std::string& symbol);
    bool subscribe_ticks(const std::string& symbol);
    bool unsubscribe_ticks(const std::string& symbol);
    
    // Historical data
    std::vector<std::map<std::string, std::any>> get_historical_data(
        const std::string& symbol,
        const std::string& period,
        int bars_count = 0,
        const std::string& from_date = "",
        const std::string& to_date = ""
    );
    
    // Symbol information
    std::vector<std::map<std::string, std::any>> get_symbols();
    std::map<std::string, std::any> get_symbol_info(const std::string& symbol);
    
    // Order operations
    std::string place_order(const std::map<std::string, std::any>& order_data);
    bool cancel_order(const std::string& order_id);
    bool modify_order(const std::string& order_id, 
                     const std::map<std::string, std::any>& modifications);
    
    // Portfolio operations
    std::map<std::string, std::any> get_account_info();
    std::vector<std::map<std::string, std::any>> get_positions();
    std::vector<std::map<std::string, std::any>> get_orders();
    std::vector<std::map<std::string, std::any>> get_trades();
    
    // Real-time data access
    std::map<std::string, std::any> get_last_quote(const std::string& symbol);
    std::map<std::string, std::any> get_last_tick(const std::string& symbol);
    
    // Properties
    ConnectionState get_connection_state() const { return connection_state_; }
    const std::string& get_host() const { return host_; }
    int get_port() const { return port_; }
    const std::string& get_username() const { return username_; }

private:
    VCStore(const Params& params);
    
    // Singleton management
    static std::shared_ptr<VCStore> instance_;
    static std::mutex instance_mutex_;
    
    // Parameters
    Params params_;
    
    // Connection details
    std::string host_;
    int port_;
    std::string username_;
    std::string password_;
    std::string client_name_;
    
    // Connection state
    std::atomic<ConnectionState> connection_state_{ConnectionState::DISCONNECTED};
    
    // Network handling
    int socket_fd_ = -1;
    std::thread network_thread_;
    std::atomic<bool> should_stop_{false};
    
    // Message queues
    std::queue<std::string> outgoing_messages_;
    std::queue<std::string> incoming_messages_;
    std::mutex message_mutex_;
    
    // Subscriptions
    std::set<std::string> subscribed_symbols_;
    std::set<std::string> subscribed_ticks_;
    std::mutex subscription_mutex_;
    
    // Data storage
    std::map<std::string, std::map<std::string, std::any>> last_quotes_;
    std::map<std::string, std::map<std::string, std::any>> last_ticks_;
    std::mutex data_mutex_;
    
    // Request management
    std::atomic<int> request_id_{0};
    std::map<int, std::string> pending_requests_;
    std::mutex request_mutex_;
    
    // Internal methods
    bool create_socket();
    void close_socket();
    bool send_message(const std::string& message);
    std::string receive_message();
    
    // Protocol handling
    std::string build_auth_message();
    std::string build_subscribe_message(const std::string& symbol, bool ticks = false);
    std::string build_unsubscribe_message(const std::string& symbol, bool ticks = false);
    std::string build_historical_request(const std::string& symbol, 
                                        const std::string& period,
                                        int bars_count,
                                        const std::string& from_date,
                                        const std::string& to_date);
    
    // Message parsing
    void parse_incoming_message(const std::string& message);
    void handle_authentication_response(const std::map<std::string, std::any>& response);
    void handle_quote_update(const std::map<std::string, std::any>& quote);
    void handle_tick_update(const std::map<std::string, std::any>& tick);
    void handle_historical_data(const std::map<std::string, std::any>& data);
    
    // Network worker
    void network_worker();
    
    // Utility methods
    int get_next_request_id();
    std::string format_date(const std::string& date);
    std::map<std::string, std::any> parse_json(const std::string& json_str);
    std::string to_json(const std::map<std::string, std::any>& data);
};

} // namespace stores
} // namespace backtrader