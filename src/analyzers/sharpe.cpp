#include "analyzers/sharpe.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace backtrader {

// Rate conversion factors
const std::map<SharpeRatio::TimeFrame, double> SharpeRatio::RATE_FACTORS = {
    {TimeFrame::Days, 252.0},
    {TimeFrame::Weeks, 52.0},
    {TimeFrame::Months, 12.0},
    {TimeFrame::Years, 1.0}
};

SharpeRatio::SharpeRatio() : Analyzer(), sharpe_ratio_(0.0) {
    // Initialize with default parameters
}

void SharpeRatio::start() {
    Analyzer::start();
    
    // Auto-detect fund mode if needed
    if (params.auto_fund && strategy_) {
        params.fund = strategy_->broker->get_fundmode();
    }
    
    // Initialize child analyzers based on parameters
    if (params.legacyannual) {
        annualreturn_ = std::make_unique<AnnualReturn>();
        if (strategy_) {
            annualreturn_->set_strategy(strategy_);
        }
        annualreturn_->start();
    } else {
        timereturn_ = std::make_unique<TimeReturn>();
        if (timereturn_) {
            timereturn_->params.timeframe = static_cast<int>(params.timeframe);
            timereturn_->params.compression = params.compression;
            timereturn_->params.fund = params.fund;
            
            if (strategy_) {
                timereturn_->set_strategy(strategy_);
            }
            timereturn_->start();
        }
    }
    
    sharpe_ratio_ = 0.0;
}

void SharpeRatio::stop() {
    // Stop child analyzers
    if (timereturn_) {
        timereturn_->stop();
    }
    if (annualreturn_) {
        annualreturn_->stop();
    }
    
    double ratio = 0.0;
    
    if (params.legacyannual) {
        // Legacy annual calculation
        if (annualreturn_) {
            auto annual_returns = annualreturn_->get_returns();
            if (!annual_returns.empty()) {
                double rate = params.riskfreerate;
                
                // Calculate excess returns
                std::vector<double> excess_returns;
                for (double r : annual_returns) {
                    excess_returns.push_back(r - rate);
                }
                
                // Calculate average excess return
                double avg_excess = calculate_average(excess_returns);
                
                // Calculate standard deviation of annual returns
                double annual_avg = calculate_average(annual_returns);
                double stddev = calculate_standard_deviation(annual_returns, annual_avg, params.stddev_sample);
                
                if (stddev > 0.0) {
                    ratio = avg_excess / stddev;
                }
            }
        }
    } else {
        // Modern timeframe-based calculation
        if (timereturn_) {
            auto time_returns = timereturn_->get_returns();
            if (!time_returns.empty()) {
                ratio = calculate_sharpe_ratio(time_returns, params.riskfreerate);
            }
        }
    }
    
    sharpe_ratio_ = ratio;
    
    Analyzer::stop();
}

std::map<std::string, double> SharpeRatio::get_analysis() {
    std::map<std::string, double> analysis;
    analysis["sharperatio"] = sharpe_ratio_;
    return analysis;
}

double SharpeRatio::get_conversion_factor() const {
    // Handle legacy daysfactor parameter
    if (params.timeframe == TimeFrame::Days && params.daysfactor > 0.0) {
        return params.daysfactor;
    }
    
    // Use explicit factor if provided
    if (params.factor > 0.0) {
        return params.factor;
    }
    
    // Use default factor from table
    auto it = RATE_FACTORS.find(params.timeframe);
    if (it != RATE_FACTORS.end()) {
        return it->second;
    }
    
    return 252.0; // Default to daily
}

double SharpeRatio::calculate_sharpe_ratio(const std::vector<double>& returns, double risk_free_rate) {
    if (returns.empty()) {
        return 0.0;
    }
    
    double rate = risk_free_rate;
    std::vector<double> adjusted_returns = returns;
    
    // Get conversion factor
    double factor = get_conversion_factor();
    
    if (factor > 0.0) {
        if (params.convertrate) {
            // Convert annual risk-free rate to timeframe rate
            rate = std::pow(1.0 + rate, 1.0 / factor) - 1.0;
        } else {
            // Convert returns to annual returns
            for (double& r : adjusted_returns) {
                r = std::pow(1.0 + r, factor) - 1.0;
            }
        }
    }
    
    // Calculate number of valid returns
    int valid_returns = static_cast<int>(adjusted_returns.size());
    if (params.stddev_sample) {
        valid_returns -= 1;
    }
    
    if (valid_returns <= 0) {
        return 0.0;
    }
    
    // Calculate excess returns
    std::vector<double> excess_returns = calculate_excess_returns(adjusted_returns, rate);
    
    // Calculate average excess return
    double avg_excess = calculate_average(excess_returns);
    
    // Calculate standard deviation of excess returns
    double stddev = calculate_standard_deviation(excess_returns, avg_excess, params.stddev_sample);
    
    if (stddev <= 0.0) {
        return 0.0;
    }
    
    double ratio = avg_excess / stddev;
    
    // Apply annualization if needed
    if (factor > 0.0 && params.convertrate && params.annualize) {
        ratio = std::sqrt(factor) * ratio;
    }
    
    return ratio;
}

std::vector<double> SharpeRatio::calculate_excess_returns(const std::vector<double>& returns, double risk_free_rate) {
    std::vector<double> excess_returns;
    excess_returns.reserve(returns.size());
    
    for (double r : returns) {
        excess_returns.push_back(r - risk_free_rate);
    }
    
    return excess_returns;
}

double SharpeRatio::calculate_average(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

double SharpeRatio::calculate_standard_deviation(const std::vector<double>& values, double mean, bool bessel) {
    if (values.empty()) {
        return 0.0;
    }
    
    double variance = 0.0;
    for (double value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    
    // Apply Bessel's correction if requested
    size_t denominator = values.size();
    if (bessel && denominator > 1) {
        denominator -= 1;
    }
    
    if (denominator == 0) {
        return 0.0;
    }
    
    variance /= denominator;
    return std::sqrt(variance);
}

} // namespace backtrader