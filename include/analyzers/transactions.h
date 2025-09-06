#pragma once

#include "../analyzer.h"
#include "../order.h"
#include "../trade.h"
#include <vector>
#include <map>
#include <memory>

namespace backtrader {
namespace analyzers {

/**
 * Transactions - Transaction analyzer
 * 
 * Records and analyzes all transactions (orders and trades) during strategy execution.
 * Provides detailed transaction history and statistics.
 */
class Transactions : public Analyzer {
public:
    // Transaction record structure
    struct TransactionRecord {
        std::string type;  // "order" or "trade"
        std::chrono::system_clock::time_point datetime;
        std::string symbol;
        std::string action;  // "buy", "sell", "close"
        double size;
        double price;
        double value;
        double commission;
        std::string order_type;  // "market", "limit", "stop", etc.
        std::string status;  // "submitted", "executed", "canceled", etc.
        std::string ref_id;  // Order/trade reference ID
        std::map<std::string, std::any> metadata;
    };

    Transactions();
    virtual ~Transactions() = default;

    // Analyzer interface
    void start() override;
    void next() override;
    void stop() override;
    AnalysisResult get_analysis() override;
    
    // Notification handlers
    void notify_order(std::shared_ptr<Order> order) override;
    void notify_trade(std::shared_ptr<Trade> trade) override;

    // Transaction access
    const std::vector<TransactionRecord>& get_all_transactions() const;
    std::vector<TransactionRecord> get_orders() const;
    std::vector<TransactionRecord> get_trades() const;
    std::vector<TransactionRecord> get_transactions_for_symbol(const std::string& symbol) const;
    
    // Transaction statistics
    int get_total_transactions() const;
    int get_total_orders() const;
    int get_total_trades() const;
    int get_executed_orders() const;
    int get_canceled_orders() const;
    
    // Volume statistics
    double get_total_volume() const;
    double get_total_value() const;
    double get_total_commission() const;
    double get_average_trade_size() const;
    double get_average_trade_value() const;
    
    // Transaction filtering
    std::vector<TransactionRecord> filter_by_date_range(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;
    std::vector<TransactionRecord> filter_by_action(const std::string& action) const;
    std::vector<TransactionRecord> filter_by_order_type(const std::string& order_type) const;

private:
    // Transaction storage
    std::vector<TransactionRecord> all_transactions_;
    
    // Statistics
    int total_orders_ = 0;
    int total_trades_ = 0;
    int executed_orders_ = 0;
    int canceled_orders_ = 0;
    double total_volume_ = 0.0;
    double total_value_ = 0.0;
    double total_commission_ = 0.0;
    
    // Internal methods
    void record_order_transaction(std::shared_ptr<Order> order);
    void record_trade_transaction(std::shared_ptr<Trade> trade);
    
    // Transaction creation
    TransactionRecord create_order_record(std::shared_ptr<Order> order) const;
    TransactionRecord create_trade_record(std::shared_ptr<Trade> trade) const;
    
    // Order/Trade information extraction
    std::string get_order_action(std::shared_ptr<Order> order) const;
    std::string get_order_type_string(std::shared_ptr<Order> order) const;
    std::string get_order_status_string(std::shared_ptr<Order> order) const;
    std::string get_symbol_name(std::shared_ptr<DataSeries> data) const;
    
    // Statistics updates
    void update_order_statistics(std::shared_ptr<Order> order);
    void update_trade_statistics(std::shared_ptr<Trade> trade);
    
    // Utility methods
    double calculate_transaction_value(double size, double price) const;
    std::string generate_transaction_id() const;
    
    // Export functionality
    std::string export_to_csv() const;
    std::string export_to_json() const;
    void save_to_file(const std::string& filename, const std::string& format) const;
};

} // namespace analyzers
} // namespace backtrader