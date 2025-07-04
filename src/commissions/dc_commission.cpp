#include "../../include/commissions/dc_commission.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace commissions {

CryptoCommission::CryptoCommission(const Params& params) : p(params) {
    if (p.commission < 0.0) {
        throw std::invalid_argument("Commission rate cannot be negative");
    }
    if (p.min_commission < 0.0) {
        throw std::invalid_argument("Minimum commission cannot be negative");
    }
}

double CryptoCommission::get_commission(double size, double price) const {
    // Calculate base commission
    double trade_value = std::abs(size) * price;
    double commission = 0.0;
    
    switch (p.commission_type) {
        case CommissionType::Percentage:
            commission = trade_value * (p.commission / 100.0);
            break;
            
        case CommissionType::Fixed:
            commission = p.commission;
            break;
            
        case CommissionType::PerShare:
            commission = std::abs(size) * p.commission;
            break;
            
        case CommissionType::Tiered:
            commission = calculate_tiered_commission(trade_value);
            break;
    }
    
    // Apply minimum commission
    commission = std::max(commission, p.min_commission);
    
    // Apply maximum commission if set
    if (p.max_commission > 0.0) {
        commission = std::min(commission, p.max_commission);
    }
    
    return commission;
}

double CryptoCommission::calculate_tiered_commission(double trade_value) const {
    // Typical crypto exchange tiered structure
    // These are example tiers - real implementation would use configurable tiers
    
    if (trade_value <= 10000.0) {
        return trade_value * 0.001; // 0.1%
    } else if (trade_value <= 50000.0) {
        return trade_value * 0.0008; // 0.08%
    } else if (trade_value <= 100000.0) {
        return trade_value * 0.0006; // 0.06%
    } else if (trade_value <= 500000.0) {
        return trade_value * 0.0004; // 0.04%
    } else {
        return trade_value * 0.0002; // 0.02%
    }
}

CommissionInfo CryptoCommission::get_commission_info(double size, double price) const {
    CommissionInfo info;
    
    double trade_value = std::abs(size) * price;
    info.trade_value = trade_value;
    info.commission_amount = get_commission(size, price);
    info.commission_rate = (trade_value > 0.0) ? (info.commission_amount / trade_value) * 100.0 : 0.0;
    info.is_maker = p.is_maker;
    info.currency = p.currency;
    
    return info;
}

void CryptoCommission::set_maker_taker_rates(double maker_rate, double taker_rate) {
    p.maker_rate = maker_rate;
    p.taker_rate = taker_rate;
}

double CryptoCommission::get_maker_taker_commission(double size, double price, bool is_maker) const {
    double trade_value = std::abs(size) * price;
    double rate = is_maker ? p.maker_rate : p.taker_rate;
    
    double commission = trade_value * (rate / 100.0);
    commission = std::max(commission, p.min_commission);
    
    if (p.max_commission > 0.0) {
        commission = std::min(commission, p.max_commission);
    }
    
    return commission;
}

// Static factory methods for common crypto exchanges
CryptoCommission CryptoCommission::binance_spot() {
    Params params;
    params.commission = 0.1; // 0.1%
    params.commission_type = CommissionType::Percentage;
    params.min_commission = 0.0;
    params.maker_rate = 0.1;
    params.taker_rate = 0.1;
    params.currency = "USDT";
    
    return CryptoCommission(params);
}

CryptoCommission CryptoCommission::binance_futures() {
    Params params;
    params.commission = 0.04; // 0.04%
    params.commission_type = CommissionType::Percentage;
    params.min_commission = 0.0;
    params.maker_rate = 0.02;
    params.taker_rate = 0.04;
    params.currency = "USDT";
    
    return CryptoCommission(params);
}

CryptoCommission CryptoCommission::coinbase_pro() {
    Params params;
    params.commission = 0.5; // 0.5% for small volumes
    params.commission_type = CommissionType::Tiered;
    params.min_commission = 0.0;
    params.maker_rate = 0.5;
    params.taker_rate = 0.5;
    params.currency = "USD";
    
    return CryptoCommission(params);
}

CryptoCommission CryptoCommission::kraken() {
    Params params;
    params.commission = 0.26; // 0.26%
    params.commission_type = CommissionType::Tiered;
    params.min_commission = 0.0;
    params.maker_rate = 0.16;
    params.taker_rate = 0.26;
    params.currency = "USD";
    
    return CryptoCommission(params);
}

CryptoCommission CryptoCommission::ftx() {
    Params params;
    params.commission = 0.07; // 0.07%
    params.commission_type = CommissionType::Percentage;
    params.min_commission = 0.0;
    params.maker_rate = 0.02;
    params.taker_rate = 0.07;
    params.currency = "USD";
    
    return CryptoCommission(params);
}

CryptoCommission CryptoCommission::bybit() {
    Params params;
    params.commission = 0.075; // 0.075%
    params.commission_type = CommissionType::Percentage;
    params.min_commission = 0.0;
    params.maker_rate = -0.025; // Maker rebate
    params.taker_rate = 0.075;
    params.currency = "USDT";
    
    return CryptoCommission(params);
}

// Custom commission calculator
double CryptoCommission::calculate_custom_commission(double size, double price,
                                                   const std::vector<TierInfo>& tiers) const {
    double trade_value = std::abs(size) * price;
    
    for (const auto& tier : tiers) {
        if (trade_value <= tier.volume_threshold) {
            return trade_value * (tier.commission_rate / 100.0);
        }
    }
    
    // If no tier matches, use the last tier's rate
    if (!tiers.empty()) {
        return trade_value * (tiers.back().commission_rate / 100.0);
    }
    
    return 0.0;
}

// Volume-based discount calculation
double CryptoCommission::apply_volume_discount(double base_commission, double monthly_volume) const {
    if (monthly_volume <= 0.0) {
        return base_commission;
    }
    
    // Example volume discount structure
    double discount_rate = 0.0;
    
    if (monthly_volume >= 10000000.0) {      // $10M+
        discount_rate = 0.5; // 50% discount
    } else if (monthly_volume >= 5000000.0) { // $5M+
        discount_rate = 0.4; // 40% discount
    } else if (monthly_volume >= 1000000.0) { // $1M+
        discount_rate = 0.3; // 30% discount
    } else if (monthly_volume >= 500000.0) {  // $500K+
        discount_rate = 0.2; // 20% discount
    } else if (monthly_volume >= 100000.0) {  // $100K+
        discount_rate = 0.1; // 10% discount
    }
    
    return base_commission * (1.0 - discount_rate);
}

// Fee structure for specific order types
double CryptoCommission::get_limit_order_commission(double size, double price) const {
    return get_maker_taker_commission(size, price, true); // Limit orders are typically maker
}

double CryptoCommission::get_market_order_commission(double size, double price) const {
    return get_maker_taker_commission(size, price, false); // Market orders are typically taker
}

double CryptoCommission::get_stop_order_commission(double size, double price) const {
    return get_maker_taker_commission(size, price, false); // Stop orders are typically taker
}

// Commission summary for reporting
std::map<std::string, double> CryptoCommission::get_commission_summary(
    const std::vector<std::pair<double, double>>& trades) const {
    
    std::map<std::string, double> summary;
    
    double total_commission = 0.0;
    double total_volume = 0.0;
    double min_commission = std::numeric_limits<double>::max();
    double max_commission = 0.0;
    
    for (const auto& trade : trades) {
        double size = trade.first;
        double price = trade.second;
        double commission = get_commission(size, price);
        double volume = std::abs(size) * price;
        
        total_commission += commission;
        total_volume += volume;
        min_commission = std::min(min_commission, commission);
        max_commission = std::max(max_commission, commission);
    }
    
    summary["total_commission"] = total_commission;
    summary["total_volume"] = total_volume;
    summary["avg_commission_rate"] = (total_volume > 0.0) ? (total_commission / total_volume) * 100.0 : 0.0;
    summary["min_commission"] = (trades.empty()) ? 0.0 : min_commission;
    summary["max_commission"] = max_commission;
    summary["trade_count"] = static_cast<double>(trades.size());
    
    if (!trades.empty()) {
        summary["avg_commission"] = total_commission / trades.size();
    }
    
    return summary;
}

} // namespace commissions
} // namespace backtrader