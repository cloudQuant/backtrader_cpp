#pragma once

#include "metabase.h"
#include <memory>

namespace backtrader {

// Forward declarations
class Strategy;
class Broker;
class DataSeries;
class CommInfo;

// Base sizer class
class Sizer {
public:
    Sizer() = default;
    virtual ~Sizer() = default;
    
    // References to strategy and broker
    std::shared_ptr<Strategy> strategy = nullptr;
    std::shared_ptr<Broker> broker = nullptr;
    
    // Main sizing method
    virtual double getsizing(std::shared_ptr<DataSeries> data, bool isbuy);
    
    // Set strategy and broker
    virtual void set(std::shared_ptr<Strategy> strategy, std::shared_ptr<Broker> broker);
    
protected:
    // Override this method in derived classes
    virtual double _getsizing(std::shared_ptr<CommInfo> comminfo,
                             double cash,
                             std::shared_ptr<DataSeries> data,
                             bool isbuy) = 0;
};

// Fixed size sizer
class FixedSize : public Sizer {
public:
    explicit FixedSize(double size = 1.0);
    virtual ~FixedSize() = default;
    
    // Parameters
    double stake = 1.0;
    
protected:
    double _getsizing(std::shared_ptr<CommInfo> comminfo,
                     double cash,
                     std::shared_ptr<DataSeries> data,
                     bool isbuy) override;
};

// All-in sizer (uses all available cash)
class AllInSizer : public Sizer {
public:
    AllInSizer() = default;
    virtual ~AllInSizer() = default;
    
protected:
    double _getsizing(std::shared_ptr<CommInfo> comminfo,
                     double cash,
                     std::shared_ptr<DataSeries> data,
                     bool isbuy) override;
};

// Percentage of portfolio sizer
class PercentSizer : public Sizer {
public:
    explicit PercentSizer(double percent = 95.0);
    virtual ~PercentSizer() = default;
    
    // Parameters
    double percents = 95.0; // Percentage of available cash to use
    double retint = false;  // Return integer size
    
protected:
    double _getsizing(std::shared_ptr<CommInfo> comminfo,
                     double cash,
                     std::shared_ptr<DataSeries> data,
                     bool isbuy) override;
};

// Sizer registry
class SizerRegistry {
public:
    using FactoryFunc = std::function<std::shared_ptr<Sizer>()>;
    
    static SizerRegistry& instance() {
        static SizerRegistry registry;
        return registry;
    }
    
    void register_sizer(const std::string& name, FactoryFunc factory) {
        sizers_[name] = factory;
    }
    
    std::shared_ptr<Sizer> create(const std::string& name) {
        auto it = sizers_.find(name);
        if (it != sizers_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    std::vector<std::string> get_names() const {
        std::vector<std::string> names;
        for (const auto& pair : sizers_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    std::map<std::string, FactoryFunc> sizers_;
};

// Registration macro
#define REGISTER_SIZER(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                SizerRegistry::instance().register_sizer( \
                    #ClassName, \
                    []() -> std::shared_ptr<Sizer> { \
                        return std::make_shared<ClassName>(); \
                    } \
                ); \
            } \
        }; \
        static ClassName##Registrar global_##ClassName##_registrar; \
    }

// Alias for backward compatibility
using SizerBase = Sizer;

} // namespace backtrader