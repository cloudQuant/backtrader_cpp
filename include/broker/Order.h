#pragma once

#include "Common.h"
#include <memory>
#include <chrono>
#include <string>

namespace backtrader {

/**
 * @brief 订单类型枚举
 */
enum class OrderType {
    Buy,        // 买入
    Sell        // 卖出
};

/**
 * @brief 订单状态枚举
 */
enum class OrderStatus {
    Created,    // 已创建
    Submitted,  // 已提交
    Accepted,   // 已接受
    Partial,    // 部分成交
    Completed,  // 已完成
    Canceled,   // 已取消
    Cancelled,  // 已取消（别名）
    Rejected,   // 已拒绝
    Expired,    // 已过期
    Margin      // 保证金不足
};

/**
 * @brief 执行信息类
 */
class ExecutionInfo {
private:
    double price_;
    double size_;
    double datetime_;
    
public:
    ExecutionInfo() : price_(0.0), size_(0.0), datetime_(0.0) {}
    ExecutionInfo(double price, double size, double datetime)
        : price_(price), size_(size), datetime_(datetime) {}
    
    double getPrice() const { return price_; }
    double getSize() const { return size_; }
    double getDatetime() const { return datetime_; }
    
    void setPrice(double price) { price_ = price; }
    void setSize(double size) { size_ = size; }
    void setDatetime(double datetime) { datetime_ = datetime; }
};

/**
 * @brief 订单类
 */
class Order {
private:
    static size_t next_id_;
    size_t id_;
    OrderType type_;
    OrderStatus status_;
    double size_;
    double price_;
    ExecutionInfo executed_;
    std::string data_name_;
    
public:
    Order() : id_(++next_id_), type_(OrderType::Buy), status_(OrderStatus::Created),
              size_(0.0), price_(0.0), data_name_("") {}
              
    Order(OrderType type, double size, double price = 0.0, const std::string& data_name = "")
        : id_(++next_id_), type_(type), status_(OrderStatus::Created),
          size_(size), price_(price), data_name_(data_name) {}
    
    // Getters
    size_t getId() const { return id_; }
    OrderType getType() const { return type_; }
    OrderStatus getStatus() const { return status_; }
    double getSize() const { return size_; }
    double getPrice() const { return price_; }
    const ExecutionInfo& getExecuted() const { return executed_; }
    const std::string& getDataName() const { return data_name_; }
    
    // Setters
    void setStatus(OrderStatus status) { status_ = status; }
    void setExecuted(const ExecutionInfo& executed) { executed_ = executed; }
    void setExecuted(double price, double size, double datetime) {
        executed_.setPrice(price);
        executed_.setSize(size);
        executed_.setDatetime(datetime);
    }
    
    // Status checks
    bool isBuy() const { return type_ == OrderType::Buy; }
    bool isSell() const { return type_ == OrderType::Sell; }
    bool isCompleted() const { return status_ == OrderStatus::Completed; }
    bool isCancelled() const { 
        return status_ == OrderStatus::Canceled || status_ == OrderStatus::Cancelled; 
    }
    bool isRejected() const { return status_ == OrderStatus::Rejected; }
    bool isPending() const { 
        return status_ == OrderStatus::Submitted || 
               status_ == OrderStatus::Accepted; 
    }
    
    // 获取状态字符串
    std::string getStatusString() const {
        switch (status_) {
            case OrderStatus::Created: return "Created";
            case OrderStatus::Submitted: return "Submitted";
            case OrderStatus::Accepted: return "Accepted";
            case OrderStatus::Partial: return "Partial";
            case OrderStatus::Completed: return "Completed";
            case OrderStatus::Canceled:
            case OrderStatus::Cancelled: return "Cancelled";
            case OrderStatus::Rejected: return "Rejected";
            case OrderStatus::Expired: return "Expired";
            case OrderStatus::Margin: return "Margin";
            default: return "Unknown";
        }
    }
    
    // Execution
    void execute(double price, double datetime) {
        executed_.setPrice(price);
        executed_.setSize(size_);
        executed_.setDatetime(datetime);
        status_ = OrderStatus::Completed;
    }
    
    void submit() {
        if (status_ == OrderStatus::Created) {
            status_ = OrderStatus::Submitted;
        }
    }
    
    void accept() {
        if (status_ == OrderStatus::Submitted) {
            status_ = OrderStatus::Accepted;
        }
    }
    
    void cancel() {
        if (status_ != OrderStatus::Completed) {
            status_ = OrderStatus::Canceled;
        }
    }
    
    void reject() {
        status_ = OrderStatus::Rejected;
    }
};

} // namespace backtrader