#pragma once

#include "lineiterator.h"
#include <vector>
#include <memory>
#include <string>

namespace backtrader {

// Forward declarations
class Analyzer;

// Observer class
class Observer : public ObserverBase {
public:
    Observer();
    virtual ~Observer() = default;
    
    // Clock settings
    bool _stclock = false;
    
    // Line type
    static constexpr int _ltype = LineIterator::ObsType;
    
    // CSV output
    bool csv = true;
    
    // Plot configuration
    struct PlotInfo {
        bool plot = false;
        bool subplot = true;
        std::string plotname = "";
        bool plotskip = false;
        bool plotabove = false;
        bool plotlinelabels = false;
        bool plotlinevalues = true;
        bool plotvaluetags = true;
        double plotymargin = 0.0;
        std::vector<double> plotyhlines;
        std::vector<double> plotyticks;
        std::vector<double> plothlines;
        bool plotforce = false;
        std::shared_ptr<LineIterator> plotmaster = nullptr;
    } plotinfo;
    
    // Lifecycle methods
    virtual void _start();
    virtual void start();
    virtual void stop() {}
    
    // Override prenext to always call next (observers always observe)
    void prenext() override;
    
    // Analyzer management
    void _register_analyzer(std::shared_ptr<Analyzer> analyzer);
    std::vector<std::shared_ptr<Analyzer>> get_analyzers() const;
    
protected:
    // Child analyzers
    std::vector<std::shared_ptr<Analyzer>> _analyzers;
};

// Observer registry
class ObserverRegistry {
public:
    using FactoryFunc = std::function<std::shared_ptr<Observer>()>;
    
    static ObserverRegistry& instance() {
        static ObserverRegistry registry;
        return registry;
    }
    
    void register_observer(const std::string& name, FactoryFunc factory) {
        observers_[name] = factory;
    }
    
    std::shared_ptr<Observer> create(const std::string& name) {
        auto it = observers_.find(name);
        if (it != observers_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    std::vector<std::string> get_names() const {
        std::vector<std::string> names;
        for (const auto& pair : observers_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    std::map<std::string, FactoryFunc> observers_;
};

// Registration macro
#define REGISTER_OBSERVER(ClassName) \
    namespace { \
        struct ClassName##Registrar { \
            ClassName##Registrar() { \
                ObserverRegistry::instance().register_observer( \
                    #ClassName, \
                    []() -> std::shared_ptr<Observer> { \
                        return std::make_shared<ClassName>(); \
                    } \
                ); \
            } \
        }; \
        static ClassName##Registrar global_##ClassName##_registrar; \
    }

} // namespace backtrader