#include "../../include/btrun/btrun.h"
#include <iostream>
#include <thread>
#include <future>
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace btrun {

BTRun::BTRun(const Params& params) : p(params) {
    results_.clear();
}

void BTRun::add_strategy(std::shared_ptr<Strategy> strategy, 
                        const std::map<std::string, std::any>& params) {
    StrategyConfig config;
    config.strategy = strategy;
    config.params = params;
    strategy_configs_.push_back(config);
}

void BTRun::add_data(std::shared_ptr<DataSeries> data) {
    data_feeds_.push_back(data);
}

void BTRun::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

void BTRun::add_analyzer(std::shared_ptr<Analyzer> analyzer) {
    analyzers_.push_back(analyzer);
}

std::vector<BTRun::RunResult> BTRun::run() {
    if (strategy_configs_.empty()) {
        throw std::runtime_error("No strategies configured for backtesting");
    }
    
    if (data_feeds_.empty()) {
        throw std::runtime_error("No data feeds configured for backtesting");
    }
    
    results_.clear();
    
    if (p.optimization_mode) {
        return run_optimization();
    } else {
        return run_single();
    }
}

std::vector<BTRun::RunResult> BTRun::run_single() {
    std::vector<RunResult> run_results;
    
    for (const auto& config : strategy_configs_) {
        if (p.debug) {
            std::cout << "Running strategy: " << config.strategy->get_name() << std::endl;
        }
        
        RunResult result = execute_single_run(config);
        run_results.push_back(result);
        results_.push_back(result);
    }
    
    return run_results;
}

std::vector<BTRun::RunResult> BTRun::run_optimization() {
    std::vector<RunResult> optimization_results;
    
    // Generate parameter combinations
    auto param_combinations = generate_parameter_combinations();
    
    if (p.debug) {
        std::cout << "Running optimization with " << param_combinations.size() 
                  << " parameter combinations" << std::endl;
    }
    
    if (p.parallel_execution && param_combinations.size() > 1) {
        optimization_results = run_parallel_optimization(param_combinations);
    } else {
        optimization_results = run_sequential_optimization(param_combinations);
    }
    
    // Sort results by optimization metric
    sort_optimization_results(optimization_results);
    
    return optimization_results;
}

BTRun::RunResult BTRun::execute_single_run(const StrategyConfig& config) {
    RunResult result;
    result.strategy_name = config.strategy->get_name();
    result.parameters = config.params;
    result.start_time = std::chrono::system_clock::now();
    
    try {
        // Create cerebro instance for this run
        auto cerebro = create_cerebro_instance(config);
        
        // Run the backtest
        auto strategies = cerebro->run();
        
        if (!strategies.empty()) {
            auto strategy = strategies[0];
            
            // Collect results
            result.final_value = cerebro->get_broker()->get_value();
            result.total_return = calculate_total_return(cerebro->get_broker());
            
            // Collect analyzer results
            result.analyzer_results = collect_analyzer_results(strategy);
            
            result.success = true;
        } else {
            result.success = false;
            result.error_message = "No strategy results returned";
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    result.end_time = std::chrono::system_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        result.end_time - result.start_time).count();
    
    return result;
}

std::shared_ptr<Cerebro> BTRun::create_cerebro_instance(const StrategyConfig& config) {
    auto cerebro = std::make_shared<Cerebro>();
    
    // Add strategy with parameters
    cerebro->add_strategy(config.strategy, config.params);
    
    // Add data feeds
    for (auto& data : data_feeds_) {
        cerebro->add_data(data);
    }
    
    // Set broker
    if (broker_) {
        cerebro->set_broker(broker_);
    }
    
    // Add analyzers
    for (auto& analyzer : analyzers_) {
        cerebro->add_analyzer(analyzer);
    }
    
    // Set cerebro parameters
    if (p.cash > 0) {
        cerebro->set_cash(p.cash);
    }
    
    return cerebro;
}

double BTRun::calculate_total_return(std::shared_ptr<Broker> broker) {
    double initial_value = p.cash > 0 ? p.cash : 100000.0; // Default
    double final_value = broker->get_value();
    
    return (final_value - initial_value) / initial_value;
}

std::map<std::string, std::any> BTRun::collect_analyzer_results(std::shared_ptr<Strategy> strategy) {
    std::map<std::string, std::any> results;
    
    // This would collect results from all analyzers attached to the strategy
    // For now, return empty map as placeholder
    
    return results;
}

std::vector<std::map<std::string, std::any>> BTRun::generate_parameter_combinations() {
    std::vector<std::map<std::string, std::any>> combinations;
    
    // This would generate all combinations of optimization parameters
    // For now, return single combination as placeholder
    if (!strategy_configs_.empty()) {
        combinations.push_back(strategy_configs_[0].params);
    }
    
    return combinations;
}

std::vector<BTRun::RunResult> BTRun::run_parallel_optimization(
    const std::vector<std::map<std::string, std::any>>& param_combinations) {
    
    std::vector<std::future<RunResult>> futures;
    std::vector<RunResult> results;
    
    // Limit concurrent executions
    int max_threads = std::min(static_cast<int>(param_combinations.size()), 
                              static_cast<int>(std::thread::hardware_concurrency()));
    
    for (size_t i = 0; i < param_combinations.size(); ++i) {
        // Wait if we've reached the thread limit
        if (futures.size() >= static_cast<size_t>(max_threads)) {
            // Wait for one to complete
            for (auto it = futures.begin(); it != futures.end(); ++it) {
                if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    results.push_back(it->get());
                    futures.erase(it);
                    break;
                }
            }
        }
        
        // Launch new task
        auto future = std::async(std::launch::async, [this, &param_combinations, i]() {
            StrategyConfig config = strategy_configs_[0]; // Use first strategy as template
            config.params = param_combinations[i];
            return execute_single_run(config);
        });
        
        futures.push_back(std::move(future));
    }
    
    // Wait for remaining futures
    for (auto& future : futures) {
        results.push_back(future.get());
    }
    
    return results;
}

std::vector<BTRun::RunResult> BTRun::run_sequential_optimization(
    const std::vector<std::map<std::string, std::any>>& param_combinations) {
    
    std::vector<RunResult> results;
    
    for (size_t i = 0; i < param_combinations.size(); ++i) {
        if (p.debug && i % 100 == 0) {
            std::cout << "Optimization progress: " << i << "/" << param_combinations.size() << std::endl;
        }
        
        StrategyConfig config = strategy_configs_[0]; // Use first strategy as template
        config.params = param_combinations[i];
        
        RunResult result = execute_single_run(config);
        results.push_back(result);
    }
    
    return results;
}

void BTRun::sort_optimization_results(std::vector<RunResult>& results) {
    std::sort(results.begin(), results.end(), [this](const RunResult& a, const RunResult& b) {
        return get_optimization_metric(a) > get_optimization_metric(b);
    });
}

double BTRun::get_optimization_metric(const RunResult& result) {
    switch (p.optimization_metric) {
        case OptimizationMetric::TotalReturn:
            return result.total_return;
            
        case OptimizationMetric::SharpeRatio:
            // Would extract Sharpe ratio from analyzer results
            return 0.0; // Placeholder
            
        case OptimizationMetric::MaxDrawdown:
            // Would extract max drawdown from analyzer results (note: negative for sorting)
            return 0.0; // Placeholder
            
        case OptimizationMetric::ProfitFactor:
            // Would extract profit factor from analyzer results
            return 0.0; // Placeholder
            
        case OptimizationMetric::WinRate:
            // Would extract win rate from analyzer results
            return 0.0; // Placeholder
            
        default:
            return result.total_return;
    }
}

std::vector<BTRun::RunResult> BTRun::get_results() const {
    return results_;
}

BTRun::RunResult BTRun::get_best_result() const {
    if (results_.empty()) {
        return RunResult{};
    }
    
    auto best_it = std::max_element(results_.begin(), results_.end(),
                                    [this](const RunResult& a, const RunResult& b) {
                                        return get_optimization_metric(a) < get_optimization_metric(b);
                                    });
    
    return *best_it;
}

std::map<std::string, double> BTRun::get_summary_statistics() const {
    std::map<std::string, double> stats;
    
    if (results_.empty()) {
        return stats;
    }
    
    // Calculate summary statistics
    std::vector<double> returns;
    std::vector<double> final_values;
    std::vector<double> execution_times;
    int successful_runs = 0;
    
    for (const auto& result : results_) {
        if (result.success) {
            returns.push_back(result.total_return);
            final_values.push_back(result.final_value);
            successful_runs++;
        }
        execution_times.push_back(static_cast<double>(result.execution_time));
    }
    
    stats["total_runs"] = static_cast<double>(results_.size());
    stats["successful_runs"] = static_cast<double>(successful_runs);
    stats["success_rate"] = static_cast<double>(successful_runs) / results_.size();
    
    if (!returns.empty()) {
        stats["mean_return"] = calculate_mean(returns);
        stats["std_return"] = calculate_std_dev(returns);
        stats["min_return"] = *std::min_element(returns.begin(), returns.end());
        stats["max_return"] = *std::max_element(returns.begin(), returns.end());
        
        stats["mean_final_value"] = calculate_mean(final_values);
        stats["min_final_value"] = *std::min_element(final_values.begin(), final_values.end());
        stats["max_final_value"] = *std::max_element(final_values.begin(), final_values.end());
    }
    
    if (!execution_times.empty()) {
        stats["mean_execution_time"] = calculate_mean(execution_times);
        stats["total_execution_time"] = std::accumulate(execution_times.begin(), execution_times.end(), 0.0);
    }
    
    return stats;
}

double BTRun::calculate_mean(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double BTRun::calculate_std_dev(const std::vector<double>& values) const {
    if (values.size() < 2) return 0.0;
    
    double mean = calculate_mean(values);
    double sum_squared_diff = 0.0;
    
    for (double value : values) {
        double diff = value - mean;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / (values.size() - 1));
}

// ParameterOptimizer implementation
ParameterOptimizer::ParameterOptimizer() {
    ranges_.clear();
}

void ParameterOptimizer::add_parameter_range(const std::string& name, double min_val, 
                                           double max_val, double step) {
    ParameterRange range;
    range.name = name;
    range.min_value = min_val;
    range.max_value = max_val;
    range.step = step;
    range.type = ParameterType::Continuous;
    
    ranges_[name] = range;
}

void ParameterOptimizer::add_parameter_list(const std::string& name, 
                                           const std::vector<std::any>& values) {
    ParameterRange range;
    range.name = name;
    range.discrete_values = values;
    range.type = ParameterType::Discrete;
    
    ranges_[name] = range;
}

std::vector<std::map<std::string, std::any>> ParameterOptimizer::generate_combinations() {
    std::vector<std::map<std::string, std::any>> combinations;
    
    if (ranges_.empty()) {
        return combinations;
    }
    
    // Generate all parameter combinations
    std::vector<std::string> param_names;
    std::vector<std::vector<std::any>> param_values;
    
    for (const auto& range_pair : ranges_) {
        const auto& range = range_pair.second;
        param_names.push_back(range.name);
        
        std::vector<std::any> values;
        
        if (range.type == ParameterType::Continuous) {
            for (double val = range.min_value; val <= range.max_value; val += range.step) {
                values.push_back(val);
            }
        } else {
            values = range.discrete_values;
        }
        
        param_values.push_back(values);
    }
    
    // Generate cartesian product
    combinations = generate_cartesian_product(param_names, param_values);
    
    return combinations;
}

std::vector<std::map<std::string, std::any>> ParameterOptimizer::generate_cartesian_product(
    const std::vector<std::string>& names,
    const std::vector<std::vector<std::any>>& values) {
    
    std::vector<std::map<std::string, std::any>> result;
    
    if (names.empty() || values.empty()) {
        return result;
    }
    
    // Calculate total combinations
    size_t total_combinations = 1;
    for (const auto& value_list : values) {
        total_combinations *= value_list.size();
    }
    
    result.reserve(total_combinations);
    
    // Generate combinations
    std::vector<size_t> indices(values.size(), 0);
    
    do {
        std::map<std::string, std::any> combination;
        
        for (size_t i = 0; i < names.size(); ++i) {
            combination[names[i]] = values[i][indices[i]];
        }
        
        result.push_back(combination);
        
    } while (increment_indices(indices, values));
    
    return result;
}

bool ParameterOptimizer::increment_indices(std::vector<size_t>& indices,
                                         const std::vector<std::vector<std::any>>& values) {
    for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
        indices[i]++;
        if (indices[i] < values[i].size()) {
            return true;
        }
        indices[i] = 0;
    }
    return false;
}

size_t ParameterOptimizer::get_total_combinations() const {
    size_t total = 1;
    
    for (const auto& range_pair : ranges_) {
        const auto& range = range_pair.second;
        
        if (range.type == ParameterType::Continuous) {
            size_t count = static_cast<size_t>((range.max_value - range.min_value) / range.step) + 1;
            total *= count;
        } else {
            total *= range.discrete_values.size();
        }
    }
    
    return total;
}

} // namespace btrun
} // namespace backtrader