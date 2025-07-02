/**
 * @file Analyzer.h
 * @brief Analyzer Base Class - Core framework for strategy analysis
 * 
 * This file implements the Analyzer functionality from backtrader's Python codebase,
 * providing a framework for analyzing strategy performance, trades, and other metrics.
 * Analyzers operate in the frame of a strategy and provide post-processing analysis.
 */

#pragma once

#include "MetaClass.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <any>

namespace backtrader {

// Forward declarations
class Strategy;
class Trade;
class Order;
class DataFeed;

namespace analyzer {

/**
 * @brief Base class for all analyzers
 * 
 * Analyzers provide post-processing analysis of strategy performance.
 * They receive notifications from the strategy and can accumulate
 * statistics and metrics throughout the trading session.
 */
class Analyzer : public meta::MetaParams<Analyzer> {
protected:
    // Reference to the strategy being analyzed
    Strategy* strategy_;
    
    // Parent analyzer (for hierarchical analyzers)
    Analyzer* parent_;
    
    // Child analyzers
    std::vector<std::unique_ptr<Analyzer>> children_;
    
    // Analysis results storage
    std::unordered_map<std::string, std::any> rets_;
    
    // Data feeds reference
    std::vector<std::shared_ptr<DataFeed>> datas_;
    
    // Primary data feed
    std::shared_ptr<DataFeed> data_;
    
    // Whether to save to CSV
    bool csv_ = true;
    
    // Name of the analyzer
    std::string name_;
    
public:
    /**
     * @brief Constructor
     * @param name Analyzer name
     */
    explicit Analyzer(const std::string& name = "Analyzer");
    
    /**
     * @brief Destructor
     */
    virtual ~Analyzer() = default;
    
    // ========== Setup and Configuration ==========
    
    /**
     * @brief Set the strategy reference
     * @param strategy Pointer to the strategy
     */
    void setStrategy(Strategy* strategy);
    
    /**
     * @brief Set parent analyzer
     * @param parent Pointer to parent analyzer
     */
    void setParent(Analyzer* parent) { parent_ = parent; }
    
    /**
     * @brief Get analyzer name
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief Set analyzer name
     * @param name New name
     */
    void setName(const std::string& name) { name_ = name; }
    
    /**
     * @brief Register a child analyzer
     * @param child Analyzer to register
     */
    void registerChild(std::unique_ptr<Analyzer> child);
    
    // ========== Lifecycle Methods ==========
    
    /**
     * @brief Called when analysis starts
     * Override to set up needed structures
     */
    virtual void start();
    
    /**
     * @brief Called when analysis stops
     * Override to perform final calculations
     */
    virtual void stop();
    
    /**
     * @brief Create analysis structures
     * Default creates an empty results map
     */
    virtual void createAnalysis();
    
    // ========== Strategy Callback Methods ==========
    
    /**
     * @brief Called before strategy's prenext
     */
    virtual void prenext();
    
    /**
     * @brief Called on strategy's nextstart
     */
    virtual void nextstart();
    
    /**
     * @brief Called on each strategy next
     */
    virtual void next();
    
    // ========== Notification Methods ==========
    
    /**
     * @brief Notification of cash and value
     * @param cash Current cash
     * @param value Current portfolio value
     */
    virtual void notifyCashValue(double cash, double value);
    
    /**
     * @brief Notification of fund status
     * @param cash Current cash
     * @param value Current portfolio value
     * @param fundvalue Fund value
     * @param shares Number of shares
     */
    virtual void notifyFund(double cash, double value, double fundvalue, double shares);
    
    /**
     * @brief Notification of order update
     * @param order Order that was updated
     */
    virtual void notifyOrder(const Order& order);
    
    /**
     * @brief Notification of trade
     * @param trade Trade that occurred
     */
    virtual void notifyTrade(const Trade& trade);
    
    // ========== Analysis Results ==========
    
    /**
     * @brief Get analysis results
     * @return Map of analysis results
     */
    virtual std::unordered_map<std::string, std::any> getAnalysis() const;
    
    /**
     * @brief Set analysis result
     * @param key Result key
     * @param value Result value
     */
    template<typename T>
    void setAnalysisValue(const std::string& key, const T& value) {
        rets_[key] = value;
    }
    
    /**
     * @brief Get analysis result
     * @param key Result key
     * @return Result value
     */
    template<typename T>
    T getAnalysisValue(const std::string& key) const {
        auto it = rets_.find(key);
        if (it != rets_.end()) {
            return std::any_cast<T>(it->second);
        }
        throw std::runtime_error("Analysis key not found: " + key);
    }
    
    /**
     * @brief Check if analysis key exists
     * @param key Result key
     * @return true if key exists
     */
    bool hasAnalysisValue(const std::string& key) const {
        return rets_.find(key) != rets_.end();
    }
    
    // ========== Utility Methods ==========
    
    /**
     * @brief Get strategy length
     * @return Current bar count
     */
    int len() const;
    
    /**
     * @brief Print analysis results
     * @param stream Output stream
     */
    virtual void print(std::ostream& stream = std::cout) const;
    
    /**
     * @brief Pretty print analysis results
     */
    virtual void pprint() const;
    
    /**
     * @brief Enable/disable CSV output
     * @param enable Whether to enable CSV
     */
    void setCSV(bool enable) { csv_ = enable; }
    
    /**
     * @brief Check if CSV output is enabled
     */
    bool isCSVEnabled() const { return csv_; }
    
protected:
    // ========== Internal Methods ==========
    
    /**
     * @brief Internal prenext handler
     */
    void _prenext();
    
    /**
     * @brief Internal nextstart handler
     */
    void _nextstart();
    
    /**
     * @brief Internal next handler
     */
    void _next();
    
    /**
     * @brief Internal start handler
     */
    void _start();
    
    /**
     * @brief Internal stop handler
     */
    void _stop();
    
    /**
     * @brief Internal cash/value notification
     */
    void _notifyCashValue(double cash, double value);
    
    /**
     * @brief Internal fund notification
     */
    void _notifyFund(double cash, double value, double fundvalue, double shares);
    
    /**
     * @brief Internal order notification
     */
    void _notifyOrder(const Order& order);
    
    /**
     * @brief Internal trade notification
     */
    void _notifyTrade(const Trade& trade);
    
    /**
     * @brief Register with parent analyzer
     */
    void _register(Analyzer* child);
    
    /**
     * @brief Initialize parameters
     */
    virtual void initializeParams() override;
};

// ========== Common Analysis Utilities ==========

/**
 * @brief Calculate returns from values
 * @param values Vector of portfolio values
 * @return Vector of returns
 */
std::vector<double> calculateReturns(const std::vector<double>& values);

/**
 * @brief Calculate cumulative returns
 * @param returns Vector of returns
 * @return Vector of cumulative returns
 */
std::vector<double> calculateCumulativeReturns(const std::vector<double>& returns);

/**
 * @brief Calculate drawdown from values
 * @param values Vector of portfolio values
 * @return Pair of max drawdown and max drawdown duration
 */
std::pair<double, int> calculateDrawdown(const std::vector<double>& values);

/**
 * @brief Calculate Sharpe ratio
 * @param returns Vector of returns
 * @param riskFreeRate Risk-free rate (annualized)
 * @param periods Periods per year (252 for daily)
 * @return Sharpe ratio
 */
double calculateSharpeRatio(const std::vector<double>& returns, 
                           double riskFreeRate = 0.0, 
                           int periods = 252);

/**
 * @brief Calculate Sortino ratio
 * @param returns Vector of returns
 * @param targetReturn Target return
 * @param periods Periods per year
 * @return Sortino ratio
 */
double calculateSortinoRatio(const std::vector<double>& returns,
                            double targetReturn = 0.0,
                            int periods = 252);

} // namespace analyzer
} // namespace backtrader