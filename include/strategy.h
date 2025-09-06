#pragma once

#include "lineiterator.h"
#include "broker.h"
#include "analyzers.h"
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace backtrader {

// Forward declarations
class Cerebro;
class Sizer;
class Trade;
class Order;

// Meta information for strategies
template<typename T>
class MetaStrategy {
public:
    static std::map<std::string, std::function<std::shared_ptr<T>()>> _indcol;
    
    static void register_strategy(const std::string& name, std::function<std::shared_ptr<T>()> factory) {
        _indcol[name] = factory;
    }
    
    static std::shared_ptr<T> create(const std::string& name) {
        auto it = _indcol.find(name);
        if (it != _indcol.end()) {
            return it->second();
        }
        return nullptr;
    }
};

template<typename T>
std::map<std::string, std::function<std::shared_ptr<T>()>> MetaStrategy<T>::_indcol;

class Strategy : public StrategyBase {
public:
    Strategy();
    virtual ~Strategy() = default;
    
    // Line type is set in StrategyBase constructor
    
    // Strategy identification
    size_t _id = 0;
    
    // Environment references
    std::shared_ptr<Cerebro> env = nullptr;
    std::shared_ptr<Cerebro> cerebro = nullptr;
    std::shared_ptr<BrokerBase> broker = nullptr;
    
    // Position sizing
    std::shared_ptr<Sizer> _sizer = nullptr;
    
    // Order and trade management
    std::vector<std::shared_ptr<Order>> _orders;
    std::vector<std::shared_ptr<Order>> _orderspending;
    std::map<std::string, std::vector<std::shared_ptr<Trade>>> _trades;
    std::vector<std::shared_ptr<Trade>> _tradespending;
    
    // Statistics and analysis
    struct Stats {
        // Placeholder for statistics
    } stats;
    
    struct Observers {
        // Placeholder for observers
    } observers;
    
    struct Analyzers {
        // Placeholder for analyzers
    } analyzers;
    
    std::vector<std::string> writers;
    
    // Slave analyzers
    std::vector<std::shared_ptr<void>> _slave_analyzers;
    
    // Analyzer instance mapping
    std::map<std::string, std::shared_ptr<Analyzer>> _analyzer_instances;
    
    // Trade history
    bool _tradehistoryon = false;
    
    // Notification methods
    virtual void notify_order(std::shared_ptr<Order> order) {}
    virtual void notify_trade(std::shared_ptr<Trade> trade) {}
    virtual void notify_cashvalue(double cash, double value) {}
    virtual void notify_fund(double cash, double value, double fundvalue, double shares) {}
    virtual void notify_store(int status, double data) {}
    
    // Overloaded notification methods for const reference compatibility
    virtual void notify_order(const Order& order) {}
    virtual void notify_trade(const Trade& trade) {}
    
    // Strategy lifecycle methods
    virtual void init() {}
    virtual void start() {}
    virtual void stop() {}
    virtual void prenext() override {}
    virtual void nextstart() override { next(); }
    virtual void next() override {}
    
    // Order methods
    std::shared_ptr<Order> buy(std::shared_ptr<DataSeries> data = nullptr,
                              double size = 0.0,
                              double price = 0.0,
                              std::string order_type = "Market");
    
    std::shared_ptr<Order> sell(std::shared_ptr<DataSeries> data = nullptr,
                               double size = 0.0,
                               double price = 0.0,
                               std::string order_type = "Market");
    
    std::shared_ptr<Order> close(std::shared_ptr<DataSeries> data = nullptr,
                                double size = 0.0);
    
    std::shared_ptr<Order> cancel(std::shared_ptr<Order> order);
    
    // Position methods
    double getposition(std::shared_ptr<DataSeries> data = nullptr) const;
    double getpositionbyname(const std::string& name) const;
    std::shared_ptr<Position> position(std::shared_ptr<DataSeries> data = nullptr) const;
    
    // Account information
    double getcash() const;
    double getvalue() const;
    
    // Broker access
    std::shared_ptr<BrokerBase> broker_ptr() const { return broker; }
    
    // Data access methods
    std::shared_ptr<LineSeries> data(int idx = 0) const;
    size_t len() const;
    size_t datas_count() const;
    
    // Analyzer access methods
    template<typename AnalyzerType>
    std::shared_ptr<AnalyzerType> getanalyzer(const std::string& name) const {
        auto it = _analyzer_instances.find(name);
        if (it != _analyzer_instances.end()) {
            return std::dynamic_pointer_cast<AnalyzerType>(it->second);
        }
        return nullptr;
    }
    std::shared_ptr<void> getanalyzer(const std::string& name) const;
    
    // Logging
    virtual void log(const std::string& message, bool doprint = true);
    
    // Internal methods
    void _addnotification(const std::string& type, const std::string& msg) override;
    void _addnotification(std::shared_ptr<Order> order);
    void _notify() override;
    
    // Override _next to properly track current bar for len()
    void _next() override;
    
    // Override _getminperstatus for multi-timeframe support
    int _getminperstatus() override;
    
    // Override _once to use data's buflen instead of strategy's buflen
    void _once() override;
    
    // Override once to call next() for each bar in runonce mode
    void once(int start, int end) override;
    
    // Calculate minimum periods for multi-timeframe synchronization
    void _periodset();
    
    // Factory method
    template<typename T, typename... Args>
    static std::shared_ptr<T> create(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
protected:
    std::vector<std::string> _notifications;
    
private:
    void _setup_default_sizer();
    
    // Current bar counter for proper len() implementation
    size_t current_bar_ = 0;
    
    // Minimum periods for each data source
    std::vector<size_t> _minperiods;
};

// Registry for auto-registration of strategies
class StrategyRegistry {
public:
    using FactoryFunc = std::function<std::shared_ptr<Strategy>()>;
    
    static StrategyRegistry& instance() {
        static StrategyRegistry registry;
        return registry;
    }
    
    void register_strategy(const std::string& name, FactoryFunc factory) {
        strategies_[name] = factory;
    }
    
    std::shared_ptr<Strategy> create(const std::string& name) {
        auto it = strategies_.find(name);
        if (it != strategies_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    std::vector<std::string> get_names() const {
        std::vector<std::string> names;
        for (const auto& pair : strategies_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    std::map<std::string, FactoryFunc> strategies_;
};

// Macro for auto-registration of strategies
#define REGISTER_STRATEGY(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                StrategyRegistry::instance().register_strategy( \
                    #ClassName, \
                    []() -> std::shared_ptr<Strategy> { \
                        return std::make_shared<ClassName>(); \
                    } \
                ); \
            } \
        }; \
        static ClassName##Registrar global_##ClassName##_registrar; \
    }

// Basic strategy template for common patterns
class BasicStrategy : public Strategy {
public:
    BasicStrategy() : Strategy() {}
    virtual ~BasicStrategy() = default;
    
    // Common strategy patterns
    virtual void buy_signal() {}
    virtual void sell_signal() {}
    virtual void exit_signal() {}
    
    // Default next implementation with signal checking
    void next() override {
        if (should_buy()) {
            buy_signal();
        }
        if (should_sell()) {
            sell_signal();
        }
        if (should_exit()) {
            exit_signal();
        }
    }
    
protected:
    virtual bool should_buy() { return false; }
    virtual bool should_sell() { return false; }
    virtual bool should_exit() { return false; }
};

} // namespace backtrader