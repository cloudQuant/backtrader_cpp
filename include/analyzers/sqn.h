#pragma once

#include "../analyzer.h"
#include <vector>
#include <map>
#include <string>

namespace backtrader {

// SQN (System Quality Number) Analyzer - categorizes trading systems using Van K. Tharp's method
class SQN : public Analyzer {
public:
    // SQN Quality Categories (Van K. Tharp)
    enum Quality {
        BelowAverage,    // 1.6 - 1.9
        Average,         // 2.0 - 2.4
        Good,           // 2.5 - 2.9
        Excellent,      // 3.0 - 5.0
        Superb,         // 5.1 - 6.9
        HolyGrail       // 7.0+
    };
    
    SQN();
    explicit SQN(const std::string& name);
    virtual ~SQN() = default;
    
    // Analyzer interface
    void start() override;
    void stop() override;
    void notify_trade(std::shared_ptr<Trade> trade) override;
    
    // Get analysis data
    AnalysisResult get_analysis() const override;
    
    // Get SQN value and trade count
    double get_sqn() const { return sqn_value_; }
    int get_trade_count() const { return trade_count_; }
    
    // Get quality category
    Quality get_quality_category() const;
    std::string get_quality_description() const;
    
    // Check if SQN is reliable (>= 30 trades)
    bool is_reliable() const { return trade_count_ >= 30; }
    
private:
    // Trade PnL storage
    std::vector<double> pnl_list_;
    
    // Statistics
    double sqn_value_;
    int trade_count_;
    
    // Helper methods
    double calculate_average(const std::vector<double>& values) const;
    double calculate_standard_deviation(const std::vector<double>& values, double mean) const;
};

// Alias for SystemQualityNumber
using SystemQualityNumber = SQN;

} // namespace backtrader