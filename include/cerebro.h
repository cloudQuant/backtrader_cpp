#pragma once

#include "strategy.h"
#include "dataseries.h"
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>

namespace backtrader {

// Forward declarations
class AbstractDataBase;
class BrokerBase;
class Observer;
class Analyzer;
class WriterBase;
using Writer = WriterBase;
class Timer;

class OptReturn {
public:
    OptReturn() = default;
    virtual ~OptReturn() = default;
    
    std::map<std::string, double> params;
    double returns = 0.0;
    std::string sharpe = "";
    // Additional optimization results can be added here
};

class Cerebro {
public:
    // Parameters structure
    struct Params {
        bool preload = true;
        bool runonce = true;
        bool live = false;
        int maxcpus = 0; // 0 means all available cores
        bool stdstats = true;
        bool oldbuysell = false;
        bool oldtrades = false;
        int exactbars = 0; // 0=False, 1=True, -1=automatic
        bool optdatas = true;
        bool optreturn = true;
        std::string objcache = "";
        bool lookahead = false;
        bool tz = false;
        std::string cheat_on_open = "";
        std::string writer_csv = "";
    } params;
    
    Cerebro();
    virtual ~Cerebro() = default;
    
    // Strategy management
    void addstrategy(std::function<std::shared_ptr<Strategy>()> strategy_factory);
    template<typename StrategyType, typename... Args>
    void addstrategy(Args&&... args) {
        addstrategy([args...]() -> std::shared_ptr<Strategy> {
            return std::make_shared<StrategyType>(args...);
        });
    }
    
    // Data management
    void adddata(std::shared_ptr<LineSeries> data, const std::string& name = "");
    void resampledata(std::shared_ptr<DataSeries> data, int timeframe, int compression = 1);
    void replaydata(std::shared_ptr<DataSeries> data, int timeframe, int compression = 1);
    
    // Broker management
    void setbroker(std::shared_ptr<BrokerBase> broker);
    std::shared_ptr<BrokerBase> getbroker() const;
    void setcash(double cash);
    void setcommission(double commission, double margin = 0.0, double mult = 1.0);
    
    // Configuration setters
    void setRunOnce(bool runonce) { params.runonce = runonce; }
    void setPreload(bool preload) { params.preload = preload; }
    
    // Observer management
    void addobserver(std::function<std::shared_ptr<Observer>()> observer_factory);
    template<typename ObserverType, typename... Args>
    void addobserver(Args&&... args) {
        addobserver([args...]() -> std::shared_ptr<Observer> {
            return std::make_shared<ObserverType>(args...);
        });
    }
    
    // Analyzer management
    void addanalyzer(std::function<std::shared_ptr<Analyzer>()> analyzer_factory);
    template<typename AnalyzerType, typename... Args>
    void addanalyzer(const std::string& name, Args&&... args) {
        addanalyzer([name, args...]() -> std::shared_ptr<Analyzer> {
            auto analyzer = std::make_shared<AnalyzerType>(name, args...);
            // Store the analyzer with its name for later retrieval
            return analyzer;
        });
        // Store the name for tracking
        analyzer_names_.push_back(name);
    }
    
    template<typename AnalyzerType, typename... Args>
    void addanalyzer(Args&&... args) {
        addanalyzer([args...]() -> std::shared_ptr<Analyzer> {
            return std::make_shared<AnalyzerType>(args...);
        });
    }
    
    // Writer management
    void addwriter(std::shared_ptr<Writer> writer);
    
    // Timer management
    void addtimer(std::shared_ptr<Timer> timer);
    
    // Execution
    std::vector<std::shared_ptr<Strategy>> run(int maxcpus = 0, bool preload = true, bool runonce = true);
    std::vector<OptReturn> optstrategy(
        std::function<std::shared_ptr<Strategy>()> strategy_factory,
        const std::map<std::string, std::vector<double>>& param_ranges
    );
    
    // Plotting (placeholder)
    void plot(const std::string& style = "default");
    
    // Internal methods
    size_t _next_stid();
    void _disable_runonce();
    void _add_signal_strategy();
    
    // Getters
    std::vector<std::shared_ptr<LineSeries>> getdatafeeds() const { return datas_; }
    std::vector<std::shared_ptr<Strategy>> getstrategies() const { return strategies_; }
    std::vector<std::shared_ptr<Writer>> getWriters() const { return writers_; }
    
private:
    // Data storage
    std::vector<std::shared_ptr<LineSeries>> datas_;
    std::vector<std::function<std::shared_ptr<Strategy>()>> strategy_factories_;
    std::vector<std::shared_ptr<Strategy>> strategies_;
    std::vector<std::function<std::shared_ptr<Observer>()>> observer_factories_;
    std::vector<std::shared_ptr<Observer>> observers_;
    std::vector<std::function<std::shared_ptr<Analyzer>()>> analyzer_factories_;
    std::vector<std::shared_ptr<Analyzer>> analyzers_;
    std::vector<std::string> analyzer_names_;
    std::vector<std::shared_ptr<Writer>> writers_;
    std::vector<std::shared_ptr<Timer>> timers_;
    
    // Broker
    std::shared_ptr<BrokerBase> broker_;
    
    // State
    size_t strategy_id_counter_;
    bool runonce_disabled_;
    
    // Internal execution methods
    void _setup_broker();
    void _setup_observers();
    void _setup_analyzers();
    void _setup_writers();
    void _preload_data();
    void _run_strategies();
    void _run_once_mode();
    void _run_next_mode();
    void _cleanup();
    void _brokernotify();
    
    // Optimization helpers
    OptReturn _single_run(const std::map<std::string, double>& params);
    std::vector<std::map<std::string, double>> _generate_param_combinations(
        const std::map<std::string, std::vector<double>>& param_ranges
    );
};

// Convenience functions
std::shared_ptr<Cerebro> create_cerebro();

} // namespace backtrader