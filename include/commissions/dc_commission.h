#pragma once

#include "../comminfo.h"
#include "../position.h"
#include <chrono>
#include <memory>

namespace backtrader {
namespace commissions {

/**
 * ComminfoDC - Digital Currency commission information class
 * 
 * Implements commission calculation specifically for digital currency trading.
 * Handles leveraged positions and interest calculations for borrowed funds.
 */
class ComminfoDC : public CommInfoBase {
public:
    // Parameters structure
    struct Params {
        bool stocklike = false;                           // Not stock-like trading
        CommType commtype = CommType::COMM_PERC;         // Percentage commission
        bool percabs = true;                             // Percentage absolute
        double interest = 3.0;                           // Interest rate percentage
        double credit_rate = 0.0001;                     // Daily credit rate
    };

    ComminfoDC(const Params& params = Params{});
    virtual ~ComminfoDC() = default;

    // Override commission calculation
    double _getcommission(double size, double price, bool pseudoexec = false) override;
    
    // Margin calculation
    double get_margin(double price) const;
    
    // Interest calculation for leveraged positions
    double get_credit_interest(std::shared_ptr<DataSeries> data,
                              std::shared_ptr<Position> pos,
                              const std::chrono::system_clock::time_point& dt) const;

    // Digital currency specific methods
    double calculate_position_value(double size, double price) const;
    double get_leverage_ratio(std::shared_ptr<Position> pos) const;
    bool is_leveraged_position(std::shared_ptr<Position> pos) const;
    
    // Interest rate management
    void set_interest_rate(double rate);
    void set_credit_rate(double rate);
    double get_interest_rate() const { return params_.interest; }
    double get_credit_rate() const { return params_.credit_rate; }

private:
    // Parameters
    Params params_;
    
    // Internal methods
    double calculate_long_interest(std::shared_ptr<Position> pos,
                                  double days_held,
                                  double total_value) const;
    double calculate_short_interest(std::shared_ptr<Position> pos,
                                   double days_held) const;
    
    // Time calculations
    double calculate_days_held(const std::chrono::system_clock::time_point& current_time,
                              const std::chrono::system_clock::time_point& position_time) const;
    
    // Value calculations
    double get_total_portfolio_value() const;
    double get_position_value(std::shared_ptr<Position> pos) const;
    
    // Leverage calculations
    double calculate_borrowed_amount(std::shared_ptr<Position> pos,
                                    double total_value) const;
    bool requires_borrowing(std::shared_ptr<Position> pos) const;
    
    // Validation
    bool is_valid_position(std::shared_ptr<Position> pos) const;
    bool is_valid_interest_rate(double rate) const;
};

/**
 * CryptoCommission - Specialized commission class for cryptocurrency exchanges
 * 
 * Provides typical commission structures used by major crypto exchanges.
 */
class CryptoCommission : public ComminfoDC {
public:
    // Exchange types with different fee structures
    enum class ExchangeType {
        BINANCE,
        COINBASE,
        KRAKEN,
        BITFINEX,
        HUOBI,
        OKEX,
        CUSTOM
    };

    // Tier-based fee structure
    struct FeeTier {
        double volume_threshold;      // 30-day trading volume threshold
        double maker_fee;            // Maker fee percentage
        double taker_fee;            // Taker fee percentage
    };

    CryptoCommission(ExchangeType exchange_type = ExchangeType::BINANCE);
    CryptoCommission(const std::vector<FeeTier>& custom_tiers);
    virtual ~CryptoCommission() = default;

    // Override commission calculation with tier-based fees
    double _getcommission(double size, double price, bool pseudoexec = false) override;
    
    // Fee tier management
    void set_trading_volume(double volume);
    void update_fee_tier();
    FeeTier get_current_tier() const;
    
    // Exchange-specific methods
    void set_vip_level(int level);
    void set_bnb_discount(bool enabled);  // Binance BNB discount
    void set_maker_taker_fees(double maker, double taker);

private:
    // Exchange configuration
    ExchangeType exchange_type_;
    std::vector<FeeTier> fee_tiers_;
    int current_tier_index_ = 0;
    double trading_volume_30d_ = 0.0;
    
    // Special features
    int vip_level_ = 0;
    bool bnb_discount_enabled_ = false;
    double bnb_discount_rate_ = 0.75;  // 25% discount with BNB
    
    // Pre-defined exchange fee structures
    static std::vector<FeeTier> create_binance_tiers();
    static std::vector<FeeTier> create_coinbase_tiers();
    static std::vector<FeeTier> create_kraken_tiers();
    static std::vector<FeeTier> create_custom_tiers();
    
    // Fee calculation helpers
    double calculate_maker_fee(double size, double price) const;
    double calculate_taker_fee(double size, double price) const;
    double apply_vip_discount(double fee) const;
    double apply_bnb_discount(double fee) const;
    
    // Tier determination
    int find_tier_index(double volume) const;
    void initialize_exchange_tiers(ExchangeType exchange_type);
};

} // namespace commissions
} // namespace backtrader