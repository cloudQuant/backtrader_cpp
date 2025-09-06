#pragma once

#include "lineiterator.h"
#include <map>
#include <memory>
#include <functional>

namespace backtrader {

// Meta information for indicators
template<typename T>
class MetaIndicator {
public:
    static std::map<std::string, std::function<std::shared_ptr<T>()>> _indcol;
    static std::map<size_t, std::shared_ptr<T>> _icache;
    static bool _icacheuse;
    
    static void cleancache() {
        _icache.clear();
    }
    
    static void usecache(bool onoff) {
        _icacheuse = onoff;
    }
    
    static void register_indicator(const std::string& name, std::function<std::shared_ptr<T>()> factory) {
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
std::map<std::string, std::function<std::shared_ptr<T>()>> MetaIndicator<T>::_indcol;

template<typename T>
std::map<size_t, std::shared_ptr<T>> MetaIndicator<T>::_icache;

template<typename T>
bool MetaIndicator<T>::_icacheuse = false;

class Indicator : public IndicatorBase {
public:
    Indicator();
    virtual ~Indicator() = default;
    
    // CSV output flag
    bool csv = false;
    
    // Advance operation for different timeframes
    void advance(size_t advance_size = 1) override;
    
    // Once mode implementations
    void preonce_via_prenext(int start, int end);
    void oncestart_via_nextstart(int start, int end);
    void once_via_next(int start, int end);
    
    // Factory method for creating indicators
    template<typename T, typename... Args>
    static std::shared_ptr<T> create(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    // Get a specific line by index - override base class method
    std::shared_ptr<LineSingle> getLine(size_t idx = 0) const override;
    
protected:
    // Override these methods to implement the indicator logic
    virtual void prenext() override {}
    virtual void nextstart() override { next(); }
    virtual void next() override {}
    virtual void preonce(int start, int end) override {}
    virtual void oncestart(int start, int end) override { once(start, end); }
    virtual void once(int start, int end) override {}
};

// Base template for creating indicators with specific line configurations
template<typename LineNames>
class IndicatorTemplate : public Indicator {
public:
    IndicatorTemplate() : Indicator() {
        _init_template_lines();
    }
    
protected:
    virtual void _init_template_lines() = 0;
};

// Multi-line plotter indicator (placeholder)
class MtLinePlotterIndicator {
public:
    // This would be a complex meta-programming implementation
    // For now, we'll keep it as a placeholder
};

class LinePlotterIndicator : public Indicator {
public:
    LinePlotterIndicator() : Indicator() {}
    virtual ~LinePlotterIndicator() = default;
};

// Registry for auto-registration of indicators
class IndicatorRegistry {
public:
    using FactoryFunc = std::function<std::shared_ptr<Indicator>()>;
    
    static IndicatorRegistry& instance() {
        static IndicatorRegistry registry;
        return registry;
    }
    
    void register_indicator(const std::string& name, FactoryFunc factory) {
        indicators_[name] = factory;
    }
    
    std::shared_ptr<Indicator> create(const std::string& name) {
        auto it = indicators_.find(name);
        if (it != indicators_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    std::vector<std::string> get_names() const {
        std::vector<std::string> names;
        for (const auto& pair : indicators_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    std::map<std::string, FactoryFunc> indicators_;
};

// Macro for auto-registration of indicators
#define REGISTER_INDICATOR(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                IndicatorRegistry::instance().register_indicator( \
                    #ClassName, \
                    []() -> std::shared_ptr<Indicator> { \
                        return std::make_shared<ClassName>(); \
                    } \
                ); \
            } \
        }; \
        static ClassName##Registrar global_##ClassName##_registrar; \
    }

} // namespace backtrader