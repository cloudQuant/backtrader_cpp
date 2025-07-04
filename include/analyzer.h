#pragma once

#include "metabase.h"
#include "timeframe.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <any>
#include <chrono>
#include <variant>
#include <iostream>
#include <type_traits>

namespace backtrader {

// Forward declarations
class Strategy;
class Observer;
class DataSeries;
class Order;
class Trade;
class Position;
class Broker;

// TimeFrame is now defined in timeframe.h

// Analysis result type - can hold various types
using AnalysisValue = std::variant<double, int, std::string, std::map<std::string, double>>;
using AnalysisResult = std::map<std::string, AnalysisValue>;

// Utility class for ordered dict-like behavior
template<typename Key, typename Value>
class OrderedDict {
public:
    using iterator = typename std::vector<std::pair<Key, Value>>::iterator;
    using const_iterator = typename std::vector<std::pair<Key, Value>>::const_iterator;
    
    Value& operator[](const Key& key) {
        auto it = std::find_if(data_.begin(), data_.end(),
            [&key](const auto& pair) { return pair.first == key; });
        if (it != data_.end()) {
            return it->second;
        }
        data_.emplace_back(key, Value{});
        return data_.back().second;
    }
    
    const Value& at(const Key& key) const {
        auto it = std::find_if(data_.begin(), data_.end(),
            [&key](const auto& pair) { return pair.first == key; });
        if (it != data_.end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found");
    }
    
    bool contains(const Key& key) const {
        return std::find_if(data_.begin(), data_.end(),
            [&key](const auto& pair) { return pair.first == key; }) != data_.end();
    }
    
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.begin(); }
    const_iterator cend() const { return data_.end(); }
    
private:
    std::vector<std::pair<Key, Value>> data_;
};

// Base analyzer class matching Python interface
class Analyzer : public std::enable_shared_from_this<Analyzer> {
public:
    Analyzer();
    virtual ~Analyzer() = default;
    
    // Core relationships (matching Python version)
    std::shared_ptr<Strategy> strategy = nullptr;
    std::shared_ptr<Analyzer> _parent = nullptr;
    std::vector<std::shared_ptr<Analyzer>> _children;
    
    // Data access (matching Python data attributes)
    std::vector<std::shared_ptr<DataSeries>> datas;
    std::shared_ptr<DataSeries> data = nullptr;
    
    // CSV output flag
    bool csv = true;
    
    // Lifecycle methods (matching Python interface)
    virtual void create_analysis();
    virtual void start() {}
    virtual void stop() {}
    virtual void prenext() { next(); }  // Default behavior from Python
    virtual void nextstart() { next(); } // Default behavior from Python
    virtual void next() {}
    
    // Internal lifecycle methods (matching Python _methods)
    virtual void _start();
    virtual void _stop();
    virtual void _prenext();
    virtual void _nextstart();
    virtual void _next();
    
    // Notification methods (matching Python interface)
    virtual void notify_cashvalue(double cash, double value) {}
    virtual void notify_fund(double cash, double value, double fundvalue, double shares) {}
    virtual void notify_order(std::shared_ptr<Order> order) {}
    virtual void notify_trade(std::shared_ptr<Trade> trade) {}
    
    // Internal notification methods
    virtual void _notify_cashvalue(double cash, double value);
    virtual void _notify_fund(double cash, double value, double fundvalue, double shares);
    virtual void _notify_order(std::shared_ptr<Order> order);
    virtual void _notify_trade(std::shared_ptr<Trade> trade);
    
    // Child management
    void _register(std::shared_ptr<Analyzer> child);
    std::vector<std::shared_ptr<Analyzer>> get_children() const;
    
    // Analysis data management (enhanced to match Python)
    virtual AnalysisResult get_analysis() const;
    virtual void set_analysis(const AnalysisResult& analysis);
    virtual void clear_analysis();
    
    // Printing and output (matching Python interface)
    virtual void print() const;
    virtual void pprint() const;
    virtual std::string to_string() const;
    
    // Length support (matching Python __len__)
    virtual size_t size() const;
    
    // Factory method
    template<typename T, typename... Args>
    static std::shared_ptr<T> create(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
protected:
    // Analysis storage (flexible type matching Python OrderedDict)
    AnalysisResult rets;
    
    // Internal state
    bool _started = false;
    
    // Helper methods for data access
    void _setup_data_aliases();
    
    // Helper to convert analysis values to string
    std::string _analysis_value_to_string(const AnalysisValue& value) const;
};

// Time frame analyzer base (matching Python TimeFrameAnalyzerBase)
class TimeFrameAnalyzerBase : public Analyzer {
public:
    // Parameters matching Python version
    struct Params {
        TimeFrame timeframe;
        int compression;
        bool _doprenext;
        
        Params() : timeframe(TimeFrame::NoTimeFrame), compression(1), _doprenext(true) {}
    };
    
    TimeFrameAnalyzerBase();
    TimeFrameAnalyzerBase(const Params& params);
    virtual ~TimeFrameAnalyzerBase() = default;
    
    // Override lifecycle methods
    void _start() override;
    void _prenext() override;
    void _nextstart() override;
    void _next() override;
    
    // Time frame specific method (matching Python on_dt_over)
    virtual void on_dt_over() {}
    
protected:
    Params p;  // Parameters
    TimeFrame timeframe;
    int compression;
    
    // Date tracking (matching Python implementation)
    std::chrono::system_clock::time_point dtkey, dtkey1;
    int64_t dtcmp = 0, dtcmp1 = 0;
    
    // Time frame management methods (matching Python)
    virtual bool _dt_over();
    virtual std::pair<int64_t, std::chrono::system_clock::time_point> _get_dt_cmpkey(
        std::chrono::system_clock::time_point dt);
    virtual std::pair<int64_t, std::chrono::system_clock::time_point> _get_subday_cmpkey(
        std::chrono::system_clock::time_point dt);
    
    // Helper methods for time calculations
    static std::tm to_tm(std::chrono::system_clock::time_point tp);
    static std::chrono::system_clock::time_point from_tm(const std::tm& tm);
};

// Registry for analyzers (enhanced)
class AnalyzerRegistry {
public:
    using FactoryFunc = std::function<std::shared_ptr<Analyzer>()>;
    
    static AnalyzerRegistry& instance() {
        static AnalyzerRegistry registry;
        return registry;
    }
    
    void register_analyzer(const std::string& name, FactoryFunc factory) {
        analyzers_[name] = factory;
    }
    
    std::shared_ptr<Analyzer> create(const std::string& name) {
        auto it = analyzers_.find(name);
        if (it != analyzers_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    std::vector<std::string> get_names() const {
        std::vector<std::string> names;
        names.reserve(analyzers_.size());
        for (const auto& pair : analyzers_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    bool is_registered(const std::string& name) const {
        return analyzers_.find(name) != analyzers_.end();
    }
    
private:
    std::map<std::string, FactoryFunc> analyzers_;
};

// Registration macro with better error handling
#define REGISTER_ANALYZER(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                try { \
                    AnalyzerRegistry::instance().register_analyzer( \
                        #ClassName, \
                        []() -> std::shared_ptr<Analyzer> { \
                            return std::make_shared<ClassName>(); \
                        } \
                    ); \
                } catch (const std::exception& e) { \
                    std::cerr << "Failed to register analyzer " << #ClassName << ": " << e.what() << std::endl; \
                } \
            } \
        }; \
        static ClassName##Registrar global_##ClassName##_registrar; \
    }

// Helper macros for analyzer parameters (matching Python params pattern)
#define ANALYZER_PARAMS(...) \
    struct Params { \
        __VA_ARGS__ \
    }; \
    Params p;

} // namespace backtrader