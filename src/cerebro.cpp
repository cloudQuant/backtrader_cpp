#include "cerebro.h"
#include <algorithm>
#include <iostream>
#include <thread>

namespace backtrader {

Cerebro::Cerebro() : strategy_id_counter_(0), runonce_disabled_(false) {
    _setup_broker();
}

void Cerebro::addstrategy(std::function<std::shared_ptr<Strategy>()> strategy_factory) {
    strategy_factories_.push_back(strategy_factory);
}

void Cerebro::adddata(std::shared_ptr<DataSeries> data, const std::string& name) {
    if (!name.empty()) {
        data->_name = name;
        data->name = name;
    }
    datas_.push_back(data);
}

void Cerebro::resampledata(std::shared_ptr<DataSeries> data, int timeframe, int compression) {
    // This would implement data resampling
    // For now, just add the data as-is
    adddata(data);
}

void Cerebro::replaydata(std::shared_ptr<DataSeries> data, int timeframe, int compression) {
    // This would implement data replay
    // For now, just add the data as-is
    adddata(data);
}

void Cerebro::setbroker(std::shared_ptr<BrokerBase> broker) {
    broker_ = broker;
}

std::shared_ptr<BrokerBase> Cerebro::getbroker() const {
    return broker_;
}

void Cerebro::setcash(double cash) {
    if (broker_) {
        // broker_->setcash(cash);
    }
}

void Cerebro::setcommission(double commission, double margin, double mult) {
    if (broker_) {
        // broker_->setcommission(commission, margin, mult);
    }
}

void Cerebro::addobserver(std::function<std::shared_ptr<Observer>()> observer_factory) {
    observer_factories_.push_back(observer_factory);
}

void Cerebro::addanalyzer(std::function<std::shared_ptr<Analyzer>()> analyzer_factory) {
    analyzer_factories_.push_back(analyzer_factory);
}

void Cerebro::addwriter(std::shared_ptr<Writer> writer) {
    writers_.push_back(writer);
}

void Cerebro::addtimer(std::shared_ptr<Timer> timer) {
    timers_.push_back(timer);
}

std::vector<std::shared_ptr<Strategy>> Cerebro::run(int maxcpus, bool preload, bool runonce) {
    // Clear previous runs
    strategies_.clear();
    observers_.clear();
    analyzers_.clear();
    
    // Setup phase
    _setup_observers();
    _setup_writers();
    
    if (preload && params.preload) {
        _preload_data();
    }
    
    // Create strategies
    for (auto& factory : strategy_factories_) {
        auto strategy = factory();
        strategy->cerebro = std::shared_ptr<Cerebro>(this, [](Cerebro*) {}); // Non-owning shared_ptr
        strategy->broker = broker_;
        strategy->_id = _next_stid();
        
        // Add data to strategy
        for (auto& data : datas_) {
            strategy->datas.push_back(data);
        }
        if (!strategy->datas.empty()) {
            // Access the data member variable directly to avoid conflict with data() method
            static_cast<LineIterator*>(strategy.get())->data = strategy->datas[0];
            strategy->_clock = strategy->datas[0];
        }
        
        strategies_.push_back(strategy);
    }
    
    // Setup analyzers after strategies are created
    _setup_analyzers();
    
    // Run strategies
    if (runonce && params.runonce && !runonce_disabled_) {
        _run_once_mode();
    } else {
        _run_next_mode();
    }
    
    _cleanup();
    
    return strategies_;
}

std::vector<OptReturn> Cerebro::optstrategy(
    std::function<std::shared_ptr<Strategy>()> strategy_factory,
    const std::map<std::string, std::vector<double>>& param_ranges) {
    
    std::vector<OptReturn> results;
    auto param_combinations = _generate_param_combinations(param_ranges);
    
    for (const auto& params : param_combinations) {
        OptReturn result = _single_run(params);
        result.params = params;
        results.push_back(result);
    }
    
    return results;
}

void Cerebro::plot(const std::string& style) {
    std::cout << "Plotting not implemented yet" << std::endl;
}

size_t Cerebro::_next_stid() {
    return ++strategy_id_counter_;
}

void Cerebro::_disable_runonce() {
    runonce_disabled_ = true;
}

void Cerebro::_add_signal_strategy() {
    // This would add a signal-based strategy
    // Placeholder for now
}

void Cerebro::_setup_broker() {
    if (!broker_) {
        // Create default broker
        // broker_ = std::make_shared<BackBroker>();
    }
}

void Cerebro::_setup_observers() {
    if (params.stdstats) {
        // Add standard observers: Broker, Trades, BuySell
        // This would be implemented when observer classes are available
    }
    
    // Create observers from factories
    for (auto& factory : observer_factories_) {
        observers_.push_back(factory());
    }
}

void Cerebro::_setup_analyzers() {
    // Create analyzers from factories
    for (size_t i = 0; i < analyzer_factories_.size(); ++i) {
        auto analyzer = analyzer_factories_[i]();
        analyzers_.push_back(analyzer);
        
        // Register analyzer with strategies if we have a name
        if (i < analyzer_names_.size() && !strategies_.empty()) {
            const std::string& name = analyzer_names_[i];
            for (auto& strategy : strategies_) {
                strategy->_analyzer_instances[name] = analyzer;
            }
        }
    }
}

void Cerebro::_setup_writers() {
    // Writers are already added directly, no factory needed
}

void Cerebro::_preload_data() {
    // This would preload all data feeds
    // For now, assume data is already loaded
    for (auto& data : datas_) {
        // data->preload();
    }
}

void Cerebro::_run_strategies() {
    // This is called by both once and next modes
    for (auto& strategy : strategies_) {
        strategy->start();
    }
}

void Cerebro::_run_once_mode() {
    _run_strategies();
    
    // Run in vectorized mode
    for (auto& strategy : strategies_) {
        strategy->_once();
    }
    
    for (auto& strategy : strategies_) {
        strategy->stop();
    }
}

void Cerebro::_run_next_mode() {
    _run_strategies();
    
    // Determine the maximum length of data
    size_t max_len = 0;
    for (auto& data : datas_) {
        max_len = std::max(max_len, data->size());
    }
    
    // Run event-driven mode
    for (size_t i = 0; i < max_len; ++i) {
        // Advance all data
        for (auto& data : datas_) {
            if (i < data->size()) {
                data->forward();
            }
        }
        
        // Execute strategies
        for (auto& strategy : strategies_) {
            strategy->_next();
        }
    }
    
    for (auto& strategy : strategies_) {
        strategy->stop();
    }
}

void Cerebro::_cleanup() {
    // Cleanup after run
    for (auto& observer : observers_) {
        // observer->stop();
    }
    
    for (auto& analyzer : analyzers_) {
        // analyzer->stop();
    }
}

OptReturn Cerebro::_single_run(const std::map<std::string, double>& params) {
    // This would run a single optimization iteration
    // For now, return a dummy result
    OptReturn result;
    result.returns = 0.0;
    result.sharpe = "0.0";
    return result;
}

std::vector<std::map<std::string, double>> Cerebro::_generate_param_combinations(
    const std::map<std::string, std::vector<double>>& param_ranges) {
    
    std::vector<std::map<std::string, double>> combinations;
    
    // Simple implementation: generate all combinations
    // This is a simplified version - a full implementation would use
    // recursive generation or itertools-like functionality
    
    if (param_ranges.empty()) {
        return combinations;
    }
    
    // For now, just return the first value of each parameter
    std::map<std::string, double> single_combo;
    for (const auto& param : param_ranges) {
        if (!param.second.empty()) {
            single_combo[param.first] = param.second[0];
        }
    }
    combinations.push_back(single_combo);
    
    return combinations;
}

std::shared_ptr<Cerebro> create_cerebro() {
    return std::make_shared<Cerebro>();
}

} // namespace backtrader