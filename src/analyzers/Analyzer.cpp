/**
 * @file Analyzer.cpp
 * @brief Implementation of the Analyzer base class
 */

#include "analyzers/Analyzer.h"
#include "StrategyBase.h"
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <algorithm>

namespace backtrader {
namespace analyzer {

// ========== Constructor/Destructor ==========

Analyzer::Analyzer(const std::string& name) 
    : meta::MetaParams<Analyzer>(),
      strategy_(nullptr),
      parent_(nullptr),
      name_(name) {
    initializeParams();
}

// ========== Setup and Configuration ==========

void Analyzer::setStrategy(Strategy* strategy) {
    strategy_ = strategy;
    if (strategy_) {
        // Get data feeds from strategy
        // Note: This assumes Strategy class has getDatas() method
        // datas_ = strategy_->getDatas();
        // if (!datas_.empty()) {
        //     data_ = datas_[0];
        // }
    }
}

void Analyzer::registerChild(std::unique_ptr<Analyzer> child) {
    child->setParent(this);
    children_.push_back(std::move(child));
}

// ========== Lifecycle Methods ==========

void Analyzer::start() {
    // Default implementation - do nothing
}

void Analyzer::stop() {
    // Default implementation - do nothing
}

void Analyzer::createAnalysis() {
    // Default creates empty results map
    rets_.clear();
}

// ========== Strategy Callback Methods ==========

void Analyzer::prenext() {
    // Default calls next()
    next();
}

void Analyzer::nextstart() {
    // Default calls next()
    next();
}

void Analyzer::next() {
    // Default implementation - do nothing
}

// ========== Notification Methods ==========

void Analyzer::notifyCashValue(double cash, double value) {
    // Default implementation - do nothing
}

void Analyzer::notifyFund(double cash, double value, double fundvalue, double shares) {
    // Default implementation - do nothing
}

void Analyzer::notifyOrder(const Order& order) {
    // Default implementation - do nothing
}

void Analyzer::notifyTrade(const Trade& trade) {
    // Default implementation - do nothing
}

// ========== Analysis Results ==========

std::unordered_map<std::string, std::any> Analyzer::getAnalysis() const {
    return rets_;
}

// ========== Utility Methods ==========

int Analyzer::len() const {
    if (strategy_) {
        // Assuming Strategy has a len() method
        // return strategy_->len();
    }
    return 0;
}

void Analyzer::print(std::ostream& stream) const {
    stream << "=== " << name_ << " Analysis Results ===" << std::endl;
    
    auto analysis = getAnalysis();
    for (const auto& [key, value] : analysis) {
        stream << key << ": ";
        
        // Try to print common types
        try {
            if (value.type() == typeid(double)) {
                stream << std::any_cast<double>(value);
            } else if (value.type() == typeid(int)) {
                stream << std::any_cast<int>(value);
            } else if (value.type() == typeid(std::string)) {
                stream << std::any_cast<std::string>(value);
            } else {
                stream << "<complex type>";
            }
        } catch (const std::bad_any_cast& e) {
            stream << "<error reading value>";
        }
        
        stream << std::endl;
    }
}

void Analyzer::pprint() const {
    print(std::cout);
}

// ========== Internal Methods ==========

void Analyzer::_prenext() {
    // Call on children first
    for (auto& child : children_) {
        child->_prenext();
    }
    prenext();
}

void Analyzer::_nextstart() {
    for (auto& child : children_) {
        child->_nextstart();
    }
    nextstart();
}

void Analyzer::_next() {
    for (auto& child : children_) {
        child->_next();
    }
    next();
}

void Analyzer::_start() {
    for (auto& child : children_) {
        child->_start();
    }
    start();
}

void Analyzer::_stop() {
    for (auto& child : children_) {
        child->_stop();
    }
    stop();
}

void Analyzer::_notifyCashValue(double cash, double value) {
    for (auto& child : children_) {
        child->_notifyCashValue(cash, value);
    }
    notifyCashValue(cash, value);
}

void Analyzer::_notifyFund(double cash, double value, double fundvalue, double shares) {
    for (auto& child : children_) {
        child->_notifyFund(cash, value, fundvalue, shares);
    }
    notifyFund(cash, value, fundvalue, shares);
}

void Analyzer::_notifyOrder(const Order& order) {
    for (auto& child : children_) {
        child->_notifyOrder(order);
    }
    notifyOrder(order);
}

void Analyzer::_notifyTrade(const Trade& trade) {
    for (auto& child : children_) {
        child->_notifyTrade(trade);
    }
    notifyTrade(trade);
}

void Analyzer::_register(Analyzer* child) {
    // This is called by the metaclass system
    // In C++, we handle this through registerChild
}

void Analyzer::initializeParams() {
    // Initialize default parameters - parameters are handled by the MetaParams base class
    // For now, just store the csv flag directly
    csv_ = true;
}

// ========== Common Analysis Utilities ==========

std::vector<double> calculateReturns(const std::vector<double>& values) {
    std::vector<double> returns;
    if (values.size() < 2) {
        return returns;
    }
    
    returns.reserve(values.size() - 1);
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i-1] != 0) {
            returns.push_back((values[i] - values[i-1]) / values[i-1]);
        } else {
            returns.push_back(0.0);
        }
    }
    
    return returns;
}

std::vector<double> calculateCumulativeReturns(const std::vector<double>& returns) {
    std::vector<double> cumReturns;
    cumReturns.reserve(returns.size());
    
    double cumProduct = 1.0;
    for (double ret : returns) {
        cumProduct *= (1.0 + ret);
        cumReturns.push_back(cumProduct - 1.0);
    }
    
    return cumReturns;
}

std::pair<double, int> calculateDrawdown(const std::vector<double>& values) {
    if (values.empty()) {
        return {0.0, 0};
    }
    
    double maxValue = values[0];
    double maxDrawdown = 0.0;
    int maxDuration = 0;
    int currentDuration = 0;
    
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i] > maxValue) {
            maxValue = values[i];
            currentDuration = 0;
        } else {
            currentDuration++;
            double drawdown = (maxValue - values[i]) / maxValue;
            if (drawdown > maxDrawdown) {
                maxDrawdown = drawdown;
            }
            if (currentDuration > maxDuration) {
                maxDuration = currentDuration;
            }
        }
    }
    
    return {maxDrawdown, maxDuration};
}

double calculateSharpeRatio(const std::vector<double>& returns, 
                           double riskFreeRate, 
                           int periods) {
    if (returns.empty()) {
        return 0.0;
    }
    
    // Calculate mean return
    double meanReturn = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double ret : returns) {
        variance += std::pow(ret - meanReturn, 2);
    }
    variance /= returns.size();
    double stdDev = std::sqrt(variance);
    
    if (stdDev == 0) {
        return 0.0;
    }
    
    // Annualize
    double annualizedReturn = meanReturn * periods;
    double annualizedStdDev = stdDev * std::sqrt(periods);
    
    return (annualizedReturn - riskFreeRate) / annualizedStdDev;
}

double calculateSortinoRatio(const std::vector<double>& returns,
                            double targetReturn,
                            int periods) {
    if (returns.empty()) {
        return 0.0;
    }
    
    // Calculate mean return
    double meanReturn = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    // Calculate downside deviation
    double downsideVariance = 0.0;
    int downsideCount = 0;
    for (double ret : returns) {
        if (ret < targetReturn) {
            downsideVariance += std::pow(ret - targetReturn, 2);
            downsideCount++;
        }
    }
    
    if (downsideCount == 0) {
        return 0.0;
    }
    
    downsideVariance /= downsideCount;
    double downsideDeviation = std::sqrt(downsideVariance);
    
    if (downsideDeviation == 0) {
        return 0.0;
    }
    
    // Annualize
    double annualizedReturn = meanReturn * periods;
    double annualizedDownsideDev = downsideDeviation * std::sqrt(periods);
    double annualizedTarget = targetReturn * periods;
    
    return (annualizedReturn - annualizedTarget) / annualizedDownsideDev;
}

} // namespace analyzer
} // namespace backtrader