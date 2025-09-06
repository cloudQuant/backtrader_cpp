#include "analyzers/sqn.h"
#include "trade.h"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace backtrader {

SQN::SQN() : Analyzer(), sqn_value_(0.0), trade_count_(0) {
    // Initialize with default values
}

SQN::SQN(const std::string& name) : Analyzer(), sqn_value_(0.0), trade_count_(0) {
    // Initialize with default values, name parameter is for compatibility
    (void)name; // Suppress unused parameter warning
}

void SQN::start() {
    Analyzer::start();
    
    // Reset state
    pnl_list_.clear();
    sqn_value_ = 0.0;
    trade_count_ = 0;
}

void SQN::stop() {
    // Calculate SQN if we have sufficient trades
    if (trade_count_ > 1) {
        double pnl_average = calculate_average(pnl_list_);
        double pnl_stddev = calculate_standard_deviation(pnl_list_, pnl_average);
        
        if (pnl_stddev > 0.0) {
            // SQN = sqrt(N) * Average(PnL) / StdDev(PnL)
            sqn_value_ = std::sqrt(static_cast<double>(pnl_list_.size())) * pnl_average / pnl_stddev;
        } else {
            // Standard deviation is zero - all trades had same PnL
            sqn_value_ = 0.0;
        }
    } else {
        // Insufficient trades for meaningful SQN calculation
        sqn_value_ = 0.0;
    }
    
    Analyzer::stop();
}

void SQN::notify_trade(std::shared_ptr<Trade> trade) {
    std::cerr << "SQN::notify_trade called, trade=" << trade.get() << std::endl;
    
    // Only consider closed trades
    if (trade) {
        std::cerr << "SQN::notify_trade - trade status=" << static_cast<int>(trade->status) 
                  << ", pnlcomm=" << trade->pnlcomm << std::endl;
                  
        if (trade->status == TradeStatus::Closed) {
            std::cerr << "SQN::notify_trade - Adding closed trade with pnl=" << trade->pnlcomm << std::endl;
            pnl_list_.push_back(trade->pnlcomm);
            trade_count_++;
        }
    }
}

AnalysisResult SQN::get_analysis() const {
    AnalysisResult result;
    
    result["sqn"] = sqn_value_;
    result["trades"] = trade_count_;
    
    return result;
}

SQN::Quality SQN::get_quality_category() const {
    if (sqn_value_ >= 7.0) {
        return Quality::HolyGrail;
    } else if (sqn_value_ >= 5.1) {
        return Quality::Superb;
    } else if (sqn_value_ >= 3.0) {
        return Quality::Excellent;
    } else if (sqn_value_ >= 2.5) {
        return Quality::Good;
    } else if (sqn_value_ >= 2.0) {
        return Quality::Average;
    } else {
        return Quality::BelowAverage;
    }
}

std::string SQN::get_quality_description() const {
    switch (get_quality_category()) {
        case Quality::HolyGrail:
            return "Holy Grail (7.0+)";
        case Quality::Superb:
            return "Superb (5.1-6.9)";
        case Quality::Excellent:
            return "Excellent (3.0-5.0)";
        case Quality::Good:
            return "Good (2.5-2.9)";
        case Quality::Average:
            return "Average (2.0-2.4)";
        case Quality::BelowAverage:
        default:
            return "Below Average (1.6-1.9)";
    }
}

double SQN::calculate_average(const std::vector<double>& values) const {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

double SQN::calculate_standard_deviation(const std::vector<double>& values, double mean) const {
    if (values.size() <= 1) {
        return 0.0;
    }
    
    double variance = 0.0;
    for (double value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    
    variance /= values.size(); // Use population standard deviation (N) to match Python
    return std::sqrt(variance);
}

} // namespace backtrader