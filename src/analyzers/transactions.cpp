#include "../../include/analyzers/transactions.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {
namespace analyzers {

Transactions::Transactions(const Params& params) : p(params) {
    transactions_.clear();
}

void Transactions::start() {
    // Initialize analyzer
    transactions_.clear();
    daily_transactions_.clear();
    monthly_transactions_.clear();
}

void Transactions::stop() {
    // Calculate final statistics
    calculate_final_stats();
}

void Transactions::prenext() {
    // Called before next() during minimum period
    next();
}

void Transactions::next() {
    // Transaction tracking is done via notify_trade
}

void Transactions::notify_trade(const Trade& trade) {
    // Record transaction
    TransactionRecord record;
    record.trade_id = trade.id;
    record.order_id = trade.order_id;
    record.symbol = trade.symbol;
    record.size = trade.size;
    record.price = trade.price;
    record.value = trade.value;
    record.commission = trade.commission;
    record.datetime = trade.datetime;
    record.is_buy = trade.size > 0;
    record.gross_pnl = 0.0; // Will be calculated when position is closed
    record.net_pnl = 0.0;
    
    transactions_.push_back(record);
    
    // Group by date
    auto date = get_date_from_time(trade.datetime);
    daily_transactions_[date].push_back(record);
    
    // Group by month
    auto month = get_month_from_time(trade.datetime);
    monthly_transactions_[month].push_back(record);
}

std::map<std::string, std::any> Transactions::get_analysis() {
    calculate_final_stats();
    
    std::map<std::string, std::any> analysis;
    
    // Overall statistics
    analysis["total_transactions"] = static_cast<int>(transactions_.size());
    analysis["buy_transactions"] = buy_count_;
    analysis["sell_transactions"] = sell_count_;
    
    // Value statistics
    analysis["total_value"] = total_value_;
    analysis["total_commission"] = total_commission_;
    analysis["avg_transaction_value"] = avg_transaction_value_;
    analysis["max_transaction_value"] = max_transaction_value_;
    analysis["min_transaction_value"] = min_transaction_value_;
    
    // Commission statistics
    analysis["avg_commission"] = avg_commission_;
    analysis["commission_ratio"] = commission_ratio_;
    
    // Size statistics
    analysis["total_volume"] = total_volume_;
    analysis["avg_transaction_size"] = avg_transaction_size_;
    analysis["max_transaction_size"] = max_transaction_size_;
    
    // Daily statistics
    analysis["active_days"] = static_cast<int>(daily_transactions_.size());
    analysis["avg_daily_transactions"] = avg_daily_transactions_;
    analysis["max_daily_transactions"] = max_daily_transactions_;
    analysis["avg_daily_volume"] = avg_daily_volume_;
    analysis["max_daily_volume"] = max_daily_volume_;
    
    // Monthly statistics
    analysis["active_months"] = static_cast<int>(monthly_transactions_.size());
    analysis["avg_monthly_transactions"] = avg_monthly_transactions_;
    analysis["max_monthly_transactions"] = max_monthly_transactions_;
    
    // Round trip analysis
    analysis["round_trips_completed"] = round_trips_completed_;
    analysis["avg_round_trip_duration"] = avg_round_trip_duration_;
    
    // Transaction frequency
    analysis["transaction_frequency"] = transaction_frequency_;
    
    return analysis;
}

void Transactions::calculate_final_stats() {
    if (transactions_.empty()) {
        return;
    }
    
    // Reset statistics
    buy_count_ = 0;
    sell_count_ = 0;
    total_value_ = 0.0;
    total_commission_ = 0.0;
    total_volume_ = 0.0;
    max_transaction_value_ = std::numeric_limits<double>::lowest();
    min_transaction_value_ = std::numeric_limits<double>::max();
    max_transaction_size_ = 0.0;
    
    // Calculate basic statistics
    for (const auto& trans : transactions_) {
        if (trans.is_buy) {
            buy_count_++;
        } else {
            sell_count_++;
        }
        
        double abs_value = std::abs(trans.value);
        total_value_ += abs_value;
        total_commission_ += trans.commission;
        
        double abs_size = std::abs(trans.size);
        total_volume_ += abs_size;
        
        max_transaction_value_ = std::max(max_transaction_value_, abs_value);
        min_transaction_value_ = std::min(min_transaction_value_, abs_value);
        max_transaction_size_ = std::max(max_transaction_size_, abs_size);
    }
    
    // Calculate averages
    int total_count = transactions_.size();
    avg_transaction_value_ = total_value_ / total_count;
    avg_commission_ = total_commission_ / total_count;
    avg_transaction_size_ = total_volume_ / total_count;
    
    if (total_value_ > 0) {
        commission_ratio_ = total_commission_ / total_value_;
    }
    
    // Calculate daily statistics
    if (!daily_transactions_.empty()) {
        max_daily_transactions_ = 0;
        double total_daily_transactions = 0.0;
        double total_daily_volume_sum = 0.0;
        max_daily_volume_ = 0.0;
        
        for (const auto& daily_pair : daily_transactions_) {
            const auto& daily_trans = daily_pair.second;
            int daily_count = daily_trans.size();
            
            max_daily_transactions_ = std::max(max_daily_transactions_, daily_count);
            total_daily_transactions += daily_count;
            
            double daily_volume = 0.0;
            for (const auto& trans : daily_trans) {
                daily_volume += std::abs(trans.size);
            }
            
            total_daily_volume_sum += daily_volume;
            max_daily_volume_ = std::max(max_daily_volume_, daily_volume);
        }
        
        avg_daily_transactions_ = total_daily_transactions / daily_transactions_.size();
        avg_daily_volume_ = total_daily_volume_sum / daily_transactions_.size();
    }
    
    // Calculate monthly statistics
    if (!monthly_transactions_.empty()) {
        max_monthly_transactions_ = 0;
        double total_monthly_transactions = 0.0;
        
        for (const auto& monthly_pair : monthly_transactions_) {
            const auto& monthly_trans = monthly_pair.second;
            int monthly_count = monthly_trans.size();
            
            max_monthly_transactions_ = std::max(max_monthly_transactions_, monthly_count);
            total_monthly_transactions += monthly_count;
        }
        
        avg_monthly_transactions_ = total_monthly_transactions / monthly_transactions_.size();
    }
    
    // Calculate round trips
    calculate_round_trips();
    
    // Calculate transaction frequency
    if (transactions_.size() > 1) {
        auto first_time = transactions_.front().datetime;
        auto last_time = transactions_.back().datetime;
        auto duration = std::chrono::duration_cast<std::chrono::hours>(last_time - first_time);
        
        if (duration.count() > 0) {
            // Transactions per day
            transaction_frequency_ = (transactions_.size() * 24.0) / duration.count();
        }
    }
}

void Transactions::calculate_round_trips() {
    // Track open positions
    std::map<std::string, std::vector<TransactionRecord>> open_positions;
    std::vector<RoundTrip> round_trips;
    
    for (const auto& trans : transactions_) {
        if (trans.is_buy) {
            // Opening or adding to position
            open_positions[trans.symbol].push_back(trans);
        } else {
            // Closing or reducing position
            auto& positions = open_positions[trans.symbol];
            if (!positions.empty()) {
                // Match with open positions (FIFO)
                double remaining_size = std::abs(trans.size);
                
                while (remaining_size > 0 && !positions.empty()) {
                    auto& open_pos = positions.front();
                    double match_size = std::min(remaining_size, std::abs(open_pos.size));
                    
                    // Create round trip
                    RoundTrip rt;
                    rt.symbol = trans.symbol;
                    rt.entry_time = open_pos.datetime;
                    rt.exit_time = trans.datetime;
                    rt.entry_price = open_pos.price;
                    rt.exit_price = trans.price;
                    rt.size = match_size;
                    rt.gross_pnl = (trans.price - open_pos.price) * match_size;
                    rt.commission = open_pos.commission + trans.commission * (match_size / std::abs(trans.size));
                    rt.net_pnl = rt.gross_pnl - rt.commission;
                    
                    round_trips.push_back(rt);
                    
                    // Update remaining sizes
                    remaining_size -= match_size;
                    open_pos.size -= match_size;
                    
                    if (std::abs(open_pos.size) < 1e-8) {
                        positions.erase(positions.begin());
                    }
                }
            }
        }
    }
    
    // Calculate round trip statistics
    round_trips_completed_ = round_trips.size();
    
    if (!round_trips.empty()) {
        double total_duration = 0.0;
        
        for (const auto& rt : round_trips) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                rt.exit_time - rt.entry_time).count();
            total_duration += duration;
        }
        
        avg_round_trip_duration_ = total_duration / round_trips.size();
    }
}

std::string Transactions::get_date_from_time(const std::chrono::system_clock::time_point& tp) const {
    std::time_t time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&time_t_val);
    
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm);
    return std::string(buffer);
}

std::string Transactions::get_month_from_time(const std::chrono::system_clock::time_point& tp) const {
    std::time_t time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&time_t_val);
    
    char buffer[8];
    std::strftime(buffer, sizeof(buffer), "%Y-%m", tm);
    return std::string(buffer);
}

std::vector<Transactions::TransactionRecord> Transactions::get_all_transactions() const {
    return transactions_;
}

std::vector<Transactions::TransactionRecord> Transactions::get_buy_transactions() const {
    std::vector<TransactionRecord> buys;
    
    for (const auto& trans : transactions_) {
        if (trans.is_buy) {
            buys.push_back(trans);
        }
    }
    
    return buys;
}

std::vector<Transactions::TransactionRecord> Transactions::get_sell_transactions() const {
    std::vector<TransactionRecord> sells;
    
    for (const auto& trans : transactions_) {
        if (!trans.is_buy) {
            sells.push_back(trans);
        }
    }
    
    return sells;
}

std::map<std::string, std::vector<Transactions::TransactionRecord>> 
Transactions::get_transactions_by_symbol() const {
    std::map<std::string, std::vector<TransactionRecord>> by_symbol;
    
    for (const auto& trans : transactions_) {
        by_symbol[trans.symbol].push_back(trans);
    }
    
    return by_symbol;
}

std::map<std::string, std::vector<Transactions::TransactionRecord>> 
Transactions::get_daily_transactions() const {
    return daily_transactions_;
}

std::map<std::string, std::vector<Transactions::TransactionRecord>> 
Transactions::get_monthly_transactions() const {
    return monthly_transactions_;
}

std::vector<Transactions::TransactionRecord> 
Transactions::get_transactions_for_date(const std::string& date) const {
    auto it = daily_transactions_.find(date);
    if (it != daily_transactions_.end()) {
        return it->second;
    }
    return {};
}

double Transactions::get_total_volume_for_symbol(const std::string& symbol) const {
    double total = 0.0;
    
    for (const auto& trans : transactions_) {
        if (trans.symbol == symbol) {
            total += std::abs(trans.size);
        }
    }
    
    return total;
}

double Transactions::get_total_commission_for_symbol(const std::string& symbol) const {
    double total = 0.0;
    
    for (const auto& trans : transactions_) {
        if (trans.symbol == symbol) {
            total += trans.commission;
        }
    }
    
    return total;
}

std::map<std::string, double> Transactions::get_commission_by_symbol() const {
    std::map<std::string, double> commission_map;
    
    for (const auto& trans : transactions_) {
        commission_map[trans.symbol] += trans.commission;
    }
    
    return commission_map;
}

std::map<std::string, double> Transactions::get_volume_by_symbol() const {
    std::map<std::string, double> volume_map;
    
    for (const auto& trans : transactions_) {
        volume_map[trans.symbol] += std::abs(trans.size);
    }
    
    return volume_map;
}

void Transactions::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

} // namespace analyzers
} // namespace backtrader