#pragma once

#include "OrderMatchingEngine.h"
#include "strategy/StrategyBase.h"
#include "feeds/DataFeed.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace backtrader {
namespace broker {

using namespace strategy;
using namespace data;

/**
 * @brief 账户信息
 */
struct AccountInfo {
    double cash;                    // 现金余额
    double equity;                  // 账户净值
    double margin_used;             // 已用保证金
    double margin_available;        // 可用保证金
    double unrealized_pnl;          // 未实现盈亏
    double realized_pnl;            // 已实现盈亏
    double total_commission;        // 总手续费
    
    // 风险指标
    double max_drawdown;            // 最大回撤
    double current_drawdown;        // 当前回撤
    double peak_equity;             // 历史最高净值
    
    AccountInfo() : cash(100000.0), equity(100000.0), margin_used(0.0),
                   margin_available(100000.0), unrealized_pnl(0.0),
                   realized_pnl(0.0), total_commission(0.0),
                   max_drawdown(0.0), current_drawdown(0.0),
                   peak_equity(100000.0) {}
};

/**
 * @brief 手续费模型
 */
class CommissionModel {
public:
    virtual ~CommissionModel() = default;
    
    /**
     * @brief 计算手续费
     * @param order 订单
     * @param executed_price 执行价格
     * @param executed_size 执行数量
     * @return 手续费金额
     */
    virtual double calculateCommission(const Order& order, 
                                     double executed_price, 
                                     double executed_size) = 0;
};

/**
 * @brief 固定手续费模型
 */
class FixedCommissionModel : public CommissionModel {
private:
    double commission_per_trade_;
    
public:
    explicit FixedCommissionModel(double commission_per_trade = 5.0)
        : commission_per_trade_(commission_per_trade) {}
    
    double calculateCommission(const Order& order, 
                             double executed_price, 
                             double executed_size) override {
        return commission_per_trade_;
    }
};

/**
 * @brief 百分比手续费模型
 */
class PercentageCommissionModel : public CommissionModel {
private:
    double commission_percentage_;
    double min_commission_;
    
public:
    explicit PercentageCommissionModel(double commission_percentage = 0.001,
                                     double min_commission = 1.0)
        : commission_percentage_(commission_percentage),
          min_commission_(min_commission) {}
    
    double calculateCommission(const Order& order, 
                             double executed_price, 
                             double executed_size) override {
        double commission = executed_price * executed_size * commission_percentage_;
        return std::max(commission, min_commission_);
    }
};

/**
 * @brief 分层手续费模型
 */
class TieredCommissionModel : public CommissionModel {
private:
    struct Tier {
        double volume_threshold;
        double commission_rate;
    };
    
    std::vector<Tier> tiers_;
    double base_commission_;
    
public:
    explicit TieredCommissionModel(double base_commission = 0.002)
        : base_commission_(base_commission) {
        // 默认分层结构
        tiers_.push_back({0.0, 0.002});       // 0-10万：0.2%
        tiers_.push_back({100000.0, 0.0015}); // 10万-50万：0.15%
        tiers_.push_back({500000.0, 0.001});  // 50万以上：0.1%
    }
    
    double calculateCommission(const Order& order, 
                             double executed_price, 
                             double executed_size) override {
        double trade_value = executed_price * executed_size;
        
        for (auto it = tiers_.rbegin(); it != tiers_.rend(); ++it) {
            if (trade_value >= it->volume_threshold) {
                return trade_value * it->commission_rate;
            }
        }
        
        return trade_value * base_commission_;
    }
};

/**
 * @brief 经纪商（Broker）
 * 
 * 负责订单执行、资金管理、风险控制和账户管理
 * 连接策略和市场，提供完整的交易环境
 */
class Broker {
private:
    // 核心组件
    std::unique_ptr<OrderMatchingEngine> matching_engine_;
    std::unique_ptr<CommissionModel> commission_model_;
    
    // 数据源
    std::shared_ptr<DataFeed> data_feed_;
    
    // 账户管理
    AccountInfo account_;
    std::unordered_map<std::string, Position> positions_;
    std::vector<Order> order_history_;
    std::unordered_map<size_t, Order*> active_orders_;
    
    // 风险控制参数
    double max_position_size_;
    double max_daily_loss_;
    double margin_requirement_;
    bool risk_management_enabled_;
    
    // 统计数据
    std::vector<double> equity_curve_;
    std::vector<double> cash_curve_;
    size_t total_trades_;
    size_t winning_trades_;
    size_t losing_trades_;
    
    // 回调函数
    std::function<void(const Order&)> order_callback_;
    std::function<void(const Order&)> trade_callback_;
    
public:
    /**
     * @brief 构造函数
     * @param data_feed 数据源
     * @param initial_cash 初始资金
     */
    explicit Broker(std::shared_ptr<DataFeed> data_feed, 
                   double initial_cash = 100000.0)
        : data_feed_(data_feed),
          max_position_size_(1000000.0),
          max_daily_loss_(10000.0),
          margin_requirement_(0.1),
          risk_management_enabled_(true),
          total_trades_(0),
          winning_trades_(0),
          losing_trades_(0) {
        
        // 初始化账户
        account_.cash = initial_cash;
        account_.equity = initial_cash;
        account_.margin_available = initial_cash;
        account_.peak_equity = initial_cash;
        
        // 初始化订单匹配引擎
        matching_engine_ = std::make_unique<OrderMatchingEngine>(data_feed);
        
        // 默认手续费模型
        commission_model_ = std::make_unique<PercentageCommissionModel>(0.001, 1.0);
    }
    
    /**
     * @brief 设置手续费模型
     * @param model 手续费模型
     */
    void setCommissionModel(std::unique_ptr<CommissionModel> model) {
        commission_model_ = std::move(model);
    }
    
    /**
     * @brief 设置滑点模型
     * @param model 滑点模型
     */
    void setSlippageModel(std::unique_ptr<SlippageModel> model) {
        if (matching_engine_) {
            matching_engine_->setSlippageModel(std::move(model));
        }
    }
    
    /**
     * @brief 设置风险控制参数
     */
    void setRiskParameters(double max_position_size = 1000000.0,
                          double max_daily_loss = 10000.0,
                          double margin_requirement = 0.1,
                          bool enabled = true) {
        max_position_size_ = max_position_size;
        max_daily_loss_ = max_daily_loss;
        margin_requirement_ = margin_requirement;
        risk_management_enabled_ = enabled;
    }
    
    /**
     * @brief 设置回调函数
     */
    void setOrderCallback(std::function<void(const Order&)> callback) {
        order_callback_ = callback;
    }
    
    void setTradeCallback(std::function<void(const Order&)> callback) {
        trade_callback_ = callback;
    }
    
    /**
     * @brief 提交订单
     * @param order 订单
     * @return 订单ID，0表示失败
     */
    size_t submitOrder(Order order) {
        // 风险检查
        if (risk_management_enabled_ && !passRiskCheck(order)) {
            order.status = OrderStatus::REJECTED;
            if (order_callback_) order_callback_(order);
            return 0;
        }
        
        // 检查资金充足性
        if (!checkFunding(order)) {
            order.status = OrderStatus::REJECTED;
            if (order_callback_) order_callback_(order);
            return 0;
        }
        
        // 提交到匹配引擎
        auto result = matching_engine_->submitOrder(order);
        
        // 处理执行结果
        if (result.new_status == OrderStatus::COMPLETED) {
            processTradeExecution(order, result);
        } else if (result.new_status == OrderStatus::SUBMITTED) {
            active_orders_[order.id] = &order_history_.back();
        }
        
        // 保存订单历史
        order_history_.push_back(order);
        
        // 触发回调
        if (order_callback_) order_callback_(order);
        if (result.new_status == OrderStatus::COMPLETED && trade_callback_) {
            trade_callback_(order);
        }
        
        return order.id;
    }
    
    /**
     * @brief 取消订单
     * @param order_id 订单ID
     * @return true if successful
     */
    bool cancelOrder(size_t order_id) {
        auto it = active_orders_.find(order_id);
        if (it != active_orders_.end()) {
            it->second->status = OrderStatus::CANCELED;
            it->second->updated_time = std::chrono::system_clock::now();
            
            active_orders_.erase(it);
            
            if (order_callback_) order_callback_(*it->second);
            return true;
        }
        
        return matching_engine_->cancelOrder(order_id);
    }
    
    /**
     * @brief 处理市场数据更新
     * 在每个时间步调用
     */
    void updateMarket() {
        // 处理待处理订单
        if (matching_engine_) {
            matching_engine_->processPendingOrders();
        }
        
        // 更新持仓盈亏
        updatePositions();
        
        // 更新账户信息
        updateAccount();
        
        // 记录权益曲线
        equity_curve_.push_back(account_.equity);
        cash_curve_.push_back(account_.cash);
        
        // 更新风险指标
        updateRiskMetrics();
    }
    
    /**
     * @brief 获取账户信息
     * @return 账户信息
     */
    const AccountInfo& getAccountInfo() const {
        return account_;
    }
    
    /**
     * @brief 获取持仓信息
     * @param symbol 品种名称
     * @return 持仓信息
     */
    const Position& getPosition(const std::string& symbol = "default") const {
        static Position empty_position;
        auto it = positions_.find(symbol);
        return (it != positions_.end()) ? it->second : empty_position;
    }
    
    /**
     * @brief 获取所有持仓
     * @return 持仓映射
     */
    const std::unordered_map<std::string, Position>& getAllPositions() const {
        return positions_;
    }
    
    /**
     * @brief 获取订单历史
     * @return 订单列表
     */
    const std::vector<Order>& getOrderHistory() const {
        return order_history_;
    }
    
    /**
     * @brief 获取活跃订单
     * @return 活跃订单映射
     */
    const std::unordered_map<size_t, Order*>& getActiveOrders() const {
        return active_orders_;
    }
    
    /**
     * @brief 获取权益曲线
     * @return 权益历史
     */
    const std::vector<double>& getEquityCurve() const {
        return equity_curve_;
    }
    
    /**
     * @brief 获取交易统计
     */
    struct TradingStatistics {
        size_t total_trades;
        size_t winning_trades;
        size_t losing_trades;
        double win_rate;
        double total_pnl;
        double avg_win;
        double avg_loss;
        double profit_factor;
        double sharpe_ratio;
        double max_drawdown;
        
        TradingStatistics() : total_trades(0), winning_trades(0), losing_trades(0),
                             win_rate(0.0), total_pnl(0.0), avg_win(0.0),
                             avg_loss(0.0), profit_factor(0.0), sharpe_ratio(0.0),
                             max_drawdown(0.0) {}
    };
    
    TradingStatistics getTradingStatistics() const {
        TradingStatistics stats;
        stats.total_trades = total_trades_;
        stats.winning_trades = winning_trades_;
        stats.losing_trades = losing_trades_;
        stats.win_rate = (total_trades_ > 0) ? 
                        static_cast<double>(winning_trades_) / total_trades_ : 0.0;
        stats.total_pnl = account_.realized_pnl;
        stats.max_drawdown = account_.max_drawdown;
        
        // 计算平均盈亏
        double total_wins = 0.0, total_losses = 0.0;
        for (const auto& order : order_history_) {
            if (order.status == OrderStatus::COMPLETED) {
                // 简化计算，实际需要考虑持仓成本
                double pnl = 0.0; // 需要从持仓变化计算
                if (pnl > 0) total_wins += pnl;
                else if (pnl < 0) total_losses += std::abs(pnl);
            }
        }
        
        stats.avg_win = (winning_trades_ > 0) ? total_wins / winning_trades_ : 0.0;
        stats.avg_loss = (losing_trades_ > 0) ? total_losses / losing_trades_ : 0.0;
        stats.profit_factor = (stats.avg_loss > 0) ? stats.avg_win / stats.avg_loss : 0.0;
        
        return stats;
    }
    
    /**
     * @brief 获取匹配引擎统计
     * @return 匹配引擎统计
     */
    OrderMatchingEngine::Statistics getMatchingEngineStatistics() const {
        return matching_engine_ ? matching_engine_->getStatistics() : 
               OrderMatchingEngine::Statistics();
    }
    
private:
    /**
     * @brief 风险检查
     * @param order 订单
     * @return true if passed
     */
    bool passRiskCheck(const Order& order) {
        // 检查持仓限制
        auto it = positions_.find("default");
        double current_position = (it != positions_.end()) ? it->second.size : 0.0;
        
        double new_position = current_position;
        if (order.side == OrderSide::BUY) {
            new_position += order.size;
        } else {
            new_position -= order.size;
        }
        
        if (std::abs(new_position) > max_position_size_) {
            return false;
        }
        
        // 检查日损失限制
        if (account_.current_drawdown > max_daily_loss_) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 检查资金充足性
     * @param order 订单
     * @return true if sufficient
     */
    bool checkFunding(const Order& order) {
        if (order.side == OrderSide::BUY) {
            double required_margin = 0.0;
            
            if (order.type == OrderType::MARKET) {
                double price = data_feed_->close()->get(0);
                required_margin = price * order.size * margin_requirement_;
            } else if (order.type == OrderType::LIMIT) {
                required_margin = order.price * order.size * margin_requirement_;
            }
            
            return account_.margin_available >= required_margin;
        }
        
        return true; // 卖单通常不需要额外资金
    }
    
    /**
     * @brief 处理交易执行
     * @param order 订单
     * @param result 执行结果
     */
    void processTradeExecution(Order& order, const ExecutionResult& result) {
        // 计算手续费
        double commission = 0.0;
        if (commission_model_) {
            commission = commission_model_->calculateCommission(
                order, result.executed_price, result.executed_size);
        }
        
        // 更新持仓
        updatePositionForTrade(order, result, commission);
        
        // 更新账户现金
        if (order.side == OrderSide::BUY) {
            account_.cash -= result.executed_price * result.executed_size + commission;
        } else {
            account_.cash += result.executed_price * result.executed_size - commission;
        }
        
        account_.total_commission += commission;
        total_trades_++;
    }
    
    /**
     * @brief 更新持仓（基于交易）
     * @param order 订单
     * @param result 执行结果
     * @param commission 手续费
     */
    void updatePositionForTrade(const Order& order, 
                               const ExecutionResult& result, 
                               double commission) {
        Position& position = positions_["default"];
        
        double trade_value = result.executed_price * result.executed_size;
        
        if (order.side == OrderSide::BUY) {
            if (position.size >= 0) {
                // 增加多头持仓
                double total_cost = position.size * position.price + trade_value + commission;
                position.size += result.executed_size;
                position.price = (position.size > 0) ? total_cost / position.size : 0.0;
            } else {
                // 平空头持仓
                double close_size = std::min(result.executed_size, -position.size);
                double pnl = close_size * (position.price - result.executed_price) - commission;
                position.realized_pnl += pnl;
                account_.realized_pnl += pnl;
                
                if (pnl > 0) winning_trades_++;
                else if (pnl < 0) losing_trades_++;
                
                position.size += result.executed_size;
                if (position.size > 0) {
                    position.price = result.executed_price;
                }
            }
        } else { // SELL
            if (position.size <= 0) {
                // 增加空头持仓
                double total_cost = -position.size * position.price + trade_value + commission;
                position.size -= result.executed_size;
                position.price = (position.size < 0) ? total_cost / (-position.size) : 0.0;
            } else {
                // 平多头持仓
                double close_size = std::min(result.executed_size, position.size);
                double pnl = close_size * (result.executed_price - position.price) - commission;
                position.realized_pnl += pnl;
                account_.realized_pnl += pnl;
                
                if (pnl > 0) winning_trades_++;
                else if (pnl < 0) losing_trades_++;
                
                position.size -= result.executed_size;
                if (position.size < 0) {
                    position.price = result.executed_price;
                }
            }
        }
    }
    
    /**
     * @brief 更新持仓（未实现盈亏）
     */
    void updatePositions() {
        if (!data_feed_ || data_feed_->len() == 0) return;
        
        double current_price = data_feed_->close()->get(0);
        if (isNaN(current_price)) return;
        
        account_.unrealized_pnl = 0.0;
        
        for (auto& [symbol, position] : positions_) {
            if (position.size != 0.0) {
                if (position.size > 0) {
                    // 多头未实现盈亏
                    position.unrealized_pnl = position.size * (current_price - position.price);
                } else {
                    // 空头未实现盈亏
                    position.unrealized_pnl = -position.size * (position.price - current_price);
                }
                account_.unrealized_pnl += position.unrealized_pnl;
            } else {
                position.unrealized_pnl = 0.0;
            }
        }
    }
    
    /**
     * @brief 更新账户信息
     */
    void updateAccount() {
        account_.equity = account_.cash + account_.unrealized_pnl;
        
        // 更新保证金
        account_.margin_used = 0.0;
        for (const auto& [symbol, position] : positions_) {
            if (position.size != 0.0) {
                double position_value = std::abs(position.size) * position.price;
                account_.margin_used += position_value * margin_requirement_;
            }
        }
        account_.margin_available = account_.cash - account_.margin_used;
    }
    
    /**
     * @brief 更新风险指标
     */
    void updateRiskMetrics() {
        // 更新最高净值
        if (account_.equity > account_.peak_equity) {
            account_.peak_equity = account_.equity;
        }
        
        // 计算当前回撤
        account_.current_drawdown = account_.peak_equity - account_.equity;
        
        // 更新最大回撤
        if (account_.current_drawdown > account_.max_drawdown) {
            account_.max_drawdown = account_.current_drawdown;
        }
    }
};

} // namespace broker
} // namespace backtrader