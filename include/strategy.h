#pragma once

#include "lineiterator.h"
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace backtrader {

// Forward declarations
class Cerebro;
class Broker;
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
    
    // Line type
    static constexpr int _ltype = LineIterator::StratType;
    
    // Strategy identification
    size_t _id = 0;
    
    // Environment references
    std::shared_ptr<Cerebro> env = nullptr;
    std::shared_ptr<Cerebro> cerebro = nullptr;
    std::shared_ptr<Broker> broker = nullptr;
    
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
    
    // Trade history
    bool _tradehistoryon = false;
    
    // Notification methods
    virtual void notify_order(std::shared_ptr<Order> order) {}
    virtual void notify_trade(std::shared_ptr<Trade> trade) {}
    virtual void notify_cashvalue(double cash, double value) {}
    virtual void notify_fund(double cash, double value, double fundvalue, double shares) {}
    virtual void notify_store(int status, double data) {}
    
    // Strategy lifecycle methods
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
    
    // Account information
    double getcash() const;
    double getvalue() const;
    
    // Logging
    virtual void log(const std::string& message, bool doprint = true);
    
    // Internal methods
    void _addnotification(const std::string& type, const std::string& msg) override;
    void _notify() override;
    
    // Factory method
    template<typename T, typename... Args>
    static std::shared_ptr<T> create(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
protected:
    std::vector<std::string> _notifications;
    
private:
    void _setup_default_sizer();
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