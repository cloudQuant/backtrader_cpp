#include "cerebro.h"
#include "writer.h"
#include "feed.h"
#include "lineiterator.h"
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

void Cerebro::adddata(std::shared_ptr<LineSeries> data, const std::string& name) {
    // Try to set name if it's a DataSeries
    if (!name.empty()) {
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data)) {
            data_series->_name = name;
            data_series->name = name;
        }
    }
    
    std::cerr << "Cerebro::adddata(LineSeries) - received data, buflen=" << data->buflen() 
              << ", size=" << data->size() << std::endl;
    
    datas_.push_back(data);
    std::cerr << "Cerebro::adddata - datas_ now has " << datas_.size() << " data feeds" << std::endl;
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
    std::cerr << "Cerebro::run() - entry" << std::endl;
    // Clear previous runs
    strategies_.clear();
    observers_.clear();
    analyzers_.clear();
    
    // Reset global bar count for fresh start
    LineIterator::reset_global_bar_count();
    
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
        
        // Initialize the strategy
        strategy->init();
        
        // For strategies, call _periodset to calculate per-data minimum periods
        if (auto strat = std::dynamic_pointer_cast<Strategy>(strategy)) {
            strat->_periodset();
        }
        
        // After init, recalculate the minimum period to consider all indicators
        strategy->_periodrecalc();
        
        strategies_.push_back(strategy);
    }
    
    // Setup analyzers after strategies are created
    std::cerr << "Cerebro: Calling _setup_analyzers()" << std::endl;
    _setup_analyzers();
    std::cerr << "Cerebro: _setup_analyzers() completed" << std::endl;
    
    // Register strategies with writers
    for (auto& writer : writers_) {
        if (auto file_writer = std::dynamic_pointer_cast<WriterFile>(writer)) {
            for (auto& strategy : strategies_) {
                file_writer->register_strategy(strategy);
            }
        }
    }
    
    // Start writers
    for (auto& writer : writers_) {
        if (auto file_writer = std::dynamic_pointer_cast<WriterFile>(writer)) {
            file_writer->start();
        }
    }
    
    // Run strategies
    if (runonce && params.runonce && !runonce_disabled_) {
        _run_once_mode();
    } else {
        _run_next_mode();
    }
    
    // Stop writers
    for (auto& writer : writers_) {
        if (auto file_writer = std::dynamic_pointer_cast<WriterFile>(writer)) {
            file_writer->stop();
        }
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
        broker_ = std::make_shared<BackBroker>();
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
    std::cerr << "Cerebro::_setup_analyzers() - strategies.size()=" << strategies_.size() 
              << ", analyzer_factories.size()=" << analyzer_factories_.size() << std::endl;
    
    // Create analyzers for each strategy
    for (auto& strategy : strategies_) {
        for (size_t i = 0; i < analyzer_factories_.size(); ++i) {
            std::cerr << "Cerebro::_setup_analyzers() - Creating analyzer " << i << std::endl;
            // Create a new analyzer instance for this strategy
            auto analyzer = analyzer_factories_[i]();
            std::cerr << "Cerebro::_setup_analyzers() - Analyzer created" << std::endl;
            analyzers_.push_back(analyzer);
            
            // Set up analyzer relationships
            analyzer->strategy = strategy;
            // Convert LineSeries to DataSeries
            analyzer->datas.clear();
            for (auto& line_series : strategy->datas) {
                if (auto data_series = std::dynamic_pointer_cast<DataSeries>(line_series)) {
                    analyzer->datas.push_back(data_series);
                }
            }
            if (!analyzer->datas.empty()) {
                analyzer->data = analyzer->datas[0];
            }
            
            // Register analyzer with strategy if we have a name
            if (i < analyzer_names_.size()) {
                const std::string& name = analyzer_names_[i];
                strategy->_analyzer_instances[name] = analyzer;
                // std::cout << "Cerebro: Registered analyzer '" << name << "' with strategy" << std::endl;
            }
            
            // Start the analyzer
            std::cerr << "Cerebro::_setup_analyzers() - Starting analyzer" << std::endl;
            analyzer->_start();
            std::cerr << "Cerebro::_setup_analyzers() - Started analyzer" << std::endl;
        }
    }
}

void Cerebro::_setup_writers() {
    // Writers are already added directly, no factory needed
    // Register data feeds with writers
    for (auto& writer : writers_) {
        // Try to cast to WriterFile to register data
        if (auto file_writer = std::dynamic_pointer_cast<WriterFile>(writer)) {
            for (auto& data : datas_) {
                // Try to cast to DataSeries for WriterFile
            if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data)) {
                file_writer->register_data(data_series);
            }
            }
            // Register strategies will be done after they are created
        }
    }
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
    // Start broker first
    if (broker_) {
        std::cerr << "Cerebro::_run_strategies() - Starting broker" << std::endl;
        broker_->start();
    }
    
    // Start data feeds
    std::cerr << "Cerebro::_run_strategies() - Starting " << datas_.size() << " data feeds" << std::endl;
    for (auto& data : datas_) {
        // Check for AbstractDataBase first (includes DataReplay, CSVDataBase, etc.)
        // This must come before DataSeries check because AbstractDataBase inherits from DataSeries
        if (auto abstract_data = std::dynamic_pointer_cast<AbstractDataBase>(data)) {
            std::cerr << "Cerebro::_run_strategies() - Found AbstractDataBase, starting it" << std::endl;
            abstract_data->start();
        } else if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data)) {
            std::cerr << "Cerebro::_run_strategies() - Found DataSeries (no start method)" << std::endl;
            // Pure DataSeries doesn't have start() method
        } else {
            std::cerr << "Cerebro::_run_strategies() - Unknown data type!" << std::endl;
        }
    }
    
    // Then start strategies
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
    
    // In once mode, we need to still iterate through the data to write outputs
    if (!writers_.empty()) {
        // Get the number of bars to iterate
        size_t max_len = 0;
        for (const auto& data : datas_) {
            max_len = std::max(max_len, data->buflen());
        }
        
        // Reset data positions to beginning
        for (auto& data : datas_) {
            data->home();
        }
        
        // Iterate through each bar and call writers
        for (size_t i = 0; i < max_len; ++i) {
            // Call writers with current data position
            for (auto& writer : writers_) {
                if (auto file_writer = std::dynamic_pointer_cast<WriterFile>(writer)) {
                    file_writer->next();
                }
            }
            
            // Advance data feeds for next iteration
            for (auto& data : datas_) {
                data->advance(1);
            }
        }
    }
    
    for (auto& strategy : strategies_) {
        strategy->stop();
    }
}

void Cerebro::_run_next_mode() {
    std::cerr << "Cerebro::_run_next_mode() - entry" << std::endl;
    _run_strategies();
    
    // Determine the maximum length of data
    size_t max_len = 0;
    std::cerr << "Cerebro: Number of data feeds: " << datas_.size() << std::endl;
    for (size_t i = 0; i < datas_.size(); ++i) {
        // Use buflen() to get the total data available, not size() which is current position
        size_t data_size = datas_[i]->buflen();
        std::cerr << "Cerebro: Data feed " << i << " buflen: " << data_size << std::endl;
        max_len = std::max(max_len, data_size);
    }
    
    // Debug
    // std::cout << "Cerebro::_run_next_mode() - max_len = " << max_len << std::endl;
    // std::cout << "Number of strategies: " << strategies_.size() << std::endl;
    // for (size_t i = 0; i < strategies_.size(); ++i) {
    //     std::cout << "Strategy " << i << " has " << strategies_[i]->_analyzer_instances.size() << " analyzers" << std::endl;
    // }
    if (max_len == 0) {
        std::cerr << "Warning: max_len is 0 - no data to process" << std::endl;
    }
    
    // Run event-driven mode
    std::cerr << "Cerebro::_run_next_mode() - Starting event loop, max_len = " << max_len << std::endl;
    for (size_t i = 0; i < max_len; ++i) {
        // Debug output every 50 bars
        if (i % 50 == 0 || i == max_len - 1) {
            std::cerr << "Cerebro: Processing bar " << i << " of " << max_len << std::endl;
        }
        
        // Advance all data
        for (auto& data : datas_) {
            // Check if we have more data to process
            // We need to check against buflen() (total data) not size() (current position)
            if (i < data->buflen()) {
                data->forward();
            } else {
                if (i == data->buflen()) {
                    std::cerr << "Cerebro: Data exhausted at bar " << i << ", buflen=" << data->buflen() << std::endl;
                }
            }
        }
        
        // Increment global bar counter for LineIterator tracking
        LineIterator::increment_global_bar();
        
        // CRITICAL FIX: Process broker first to execute pending orders from previous bar
        // This must happen BEFORE strategy->_notify() so order_id can be cleared
        _brokernotify();
        
        // Execute strategies (notify first, then next)
        for (auto& strategy : strategies_) {
            strategy->_notify();
            
            // Always call _next() to update indicators, but strategy logic should 
            // handle minperiod internally by checking if indicators are ready
            strategy->_next();
            
            // Notify analyzers of fund/cash values after strategy execution
            double cash = broker_->getcash();
            double value = broker_->getvalue();
            
            double fundvalue = value;  // For now, same as value
            double shares = 1.0;  // Placeholder
            
            // Call analyzers attached to this strategy
            // if (i == 0) {  // Only print for first iteration
            //     std::cout << "Cerebro: Strategy has " << strategy->_analyzer_instances.size() << " analyzers" << std::endl;
            // }
            for (auto& [name, analyzer] : strategy->_analyzer_instances) {
                if (analyzer) {
                    if (i == 0) {  // Only print for first iteration
                        std::cerr << "Cerebro: Calling analyzer '" << name << "', ptr=" 
                                  << analyzer.get() << std::endl;
                    }
                    // Notify fund values
                    analyzer->_notify_fund(cash, value, fundvalue, shares);
                    
                    // Call next on analyzer
                    analyzer->_next();
                }
            }
        }
        
        // Execute writers
        for (auto& writer : writers_) {
            if (auto file_writer = std::dynamic_pointer_cast<WriterFile>(writer)) {
                file_writer->next();
            }
        }
    }
    
    // Stop strategies and their analyzers
    for (auto& strategy : strategies_) {
        // Stop analyzers first
        for (auto& [name, analyzer] : strategy->_analyzer_instances) {
            if (analyzer) {
                analyzer->_stop();
            }
        }
        
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

void Cerebro::_brokernotify() {
    if (!broker_) {
        std::cerr << "Cerebro::_brokernotify() - broker_ is NULL!" << std::endl;
        return;
    }
    
    std::cerr << "Cerebro::_brokernotify() called, broker_=" << broker_.get() << std::endl;
    
    // Process broker notifications
    broker_->next();
    
    // Get notifications and pass them to strategies
    while (broker_->has_notifications()) {
        std::cerr << "Cerebro: Found broker notification" << std::endl;
        auto order = broker_->get_notification();
        if (order) {
            std::cerr << "Cerebro: Passing order notification to strategies" << std::endl;
            // Find the strategy that owns this order
            for (auto& strategy : strategies_) {
                // Pass the order notification to the strategy
                strategy->_addnotification(order);
            }
        }
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