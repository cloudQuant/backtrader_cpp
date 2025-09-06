#pragma once

#include "../cerebro.h"
#include "../strategy.h"
#include "../feed.h"
#include "../timeframe.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace backtrader {
namespace btrun {

/**
 * BTRun - Batch execution and command-line interface for backtrader
 * 
 * Provides utilities for running backtests from command line or programmatically
 * with multiple parameter combinations and configurations.
 */
class BTRun {
public:
    // Data format registry
    using DataFormatRegistry = std::map<std::string, std::function<std::shared_ptr<AbstractDataBase>()>>;
    
    // Timeframe registry
    using TimeFrameRegistry = std::map<std::string, TimeFrame>;
    
    // Run configuration
    struct RunConfig {
        // Cerebro configuration
        double initial_cash = 100000.0;
        double commission = 0.001;
        bool stdstats = true;
        bool plot = false;
        std::string plot_file;
        
        // Data configuration
        std::string data_format = "csv";
        std::vector<std::string> data_files;
        std::string fromdate;
        std::string todate;
        
        // Resampling/Replay
        std::string resample;  // format: "timeframe:compression"
        std::string replay;    // format: "timeframe:compression"
        
        // Strategy configuration
        std::string strategy_module;
        std::string strategy_class;
        std::map<std::string, std::string> strategy_params;
        
        // Analyzer configuration
        std::vector<std::string> analyzers;
        std::map<std::string, std::map<std::string, std::string>> analyzer_params;
        
        // Observer configuration
        std::vector<std::string> observers;
        
        // Output configuration
        std::string output_file;
        std::string output_format = "json";  // json, csv, xml
        bool verbose = false;
        bool quiet = false;
        
        // Optimization configuration
        bool optimize = false;
        std::map<std::string, std::vector<std::any>> optimization_params;
        int max_cpus = 1;
        
        // Advanced options
        bool flush_output = false;
        std::string timezone;
        bool preload = true;
        bool runonce = true;
        bool live = false;
    };

    BTRun();
    virtual ~BTRun() = default;

    // Main execution methods
    int run(const RunConfig& config);
    int run(int argc, char* argv[]);
    int run(const std::vector<std::string>& args);
    
    // Configuration methods
    void set_config(const RunConfig& config);
    RunConfig& get_config();
    void load_config_file(const std::string& filename);
    void save_config_file(const std::string& filename) const;
    
    // Data format registration
    void register_data_format(const std::string& name,
                             std::function<std::shared_ptr<AbstractDataBase>()> creator);
    void register_standard_data_formats();
    
    // Strategy registration
    void register_strategy(const std::string& name,
                          std::function<std::shared_ptr<Strategy>()> creator);
    
    // Analyzer registration
    void register_analyzer(const std::string& name,
                          std::function<std::shared_ptr<Analyzer>()> creator);

    // Utility methods
    static RunConfig parse_command_line(const std::vector<std::string>& args);
    static std::string format_results(const std::vector<std::any>& results,
                                     const std::string& format = "json");

private:
    // Configuration
    RunConfig config_;
    
    // Registries
    DataFormatRegistry data_formats_;
    TimeFrameRegistry timeframes_;
    std::map<std::string, std::function<std::shared_ptr<Strategy>()>> strategies_;
    std::map<std::string, std::function<std::shared_ptr<Analyzer>()>> analyzers_;
    
    // Internal execution methods
    std::shared_ptr<Cerebro> create_cerebro();
    void add_data_feeds(std::shared_ptr<Cerebro> cerebro);
    void add_strategy(std::shared_ptr<Cerebro> cerebro);
    void add_analyzers(std::shared_ptr<Cerebro> cerebro);
    void add_observers(std::shared_ptr<Cerebro> cerebro);
    
    // Data handling
    std::shared_ptr<AbstractDataBase> create_data_feed(const std::string& filename);
    void apply_resampling(std::shared_ptr<Cerebro> cerebro,
                         std::shared_ptr<AbstractDataBase> data);
    void apply_replay(std::shared_ptr<Cerebro> cerebro,
                     std::shared_ptr<AbstractDataBase> data);
    
    // Parameter parsing
    std::pair<TimeFrame, int> parse_timeframe_compression(const std::string& spec);
    std::map<std::string, std::any> parse_parameters(const std::string& param_str);
    std::chrono::system_clock::time_point parse_date(const std::string& date_str);
    
    // Output methods
    void save_results(const std::vector<std::any>& results);
    void print_results(const std::vector<std::any>& results);
    std::string format_json_results(const std::vector<std::any>& results);
    std::string format_csv_results(const std::vector<std::any>& results);
    std::string format_xml_results(const std::vector<std::any>& results);
    
    // Optimization methods
    std::vector<std::any> run_optimization();
    std::vector<std::map<std::string, std::any>> generate_parameter_combinations();
    
    // Error handling
    void handle_error(const std::exception& e);
    void validate_config();
    
    // Utility methods
    void setup_logging();
    void initialize_standard_registries();
    bool file_exists(const std::string& filename);
    std::string get_file_extension(const std::string& filename);
};

/**
 * ParameterOptimizer - Parameter optimization utility
 * 
 * Provides grid search and other optimization methods for strategy parameters.
 */
class ParameterOptimizer {
public:
    // Optimization methods
    enum class Method {
        GRID_SEARCH,
        RANDOM_SEARCH,
        GENETIC_ALGORITHM,
        BAYESIAN_OPTIMIZATION
    };
    
    // Parameter range specification
    struct ParameterRange {
        std::string name;
        std::vector<std::any> values;  // For discrete values
        std::any min_value;            // For continuous ranges
        std::any max_value;
        std::any step;
        bool is_discrete = true;
    };
    
    // Optimization configuration
    struct OptimizationConfig {
        Method method = Method::GRID_SEARCH;
        std::vector<ParameterRange> parameters;
        std::string objective = "total_return";  // Metric to optimize
        bool maximize = true;
        int max_iterations = 1000;
        int population_size = 50;  // For genetic algorithm
        double mutation_rate = 0.1;
        int random_seed = 42;
        int max_cpus = 1;
    };

    ParameterOptimizer(const OptimizationConfig& config);
    virtual ~ParameterOptimizer() = default;

    // Optimization execution
    std::vector<std::map<std::string, std::any>> optimize(
        std::function<double(const std::map<std::string, std::any>&)> objective_function);
    
    // Result analysis
    std::map<std::string, std::any> get_best_parameters() const;
    double get_best_score() const;
    std::vector<std::map<std::string, std::any>> get_all_results() const;

private:
    // Configuration
    OptimizationConfig config_;
    
    // Results storage
    std::vector<std::pair<std::map<std::string, std::any>, double>> results_;
    
    // Optimization methods
    std::vector<std::map<std::string, std::any>> grid_search();
    std::vector<std::map<std::string, std::any>> random_search();
    std::vector<std::map<std::string, std::any>> genetic_algorithm();
    
    // Parameter generation
    std::vector<std::map<std::string, std::any>> generate_grid_combinations();
    std::map<std::string, std::any> generate_random_parameters();
    
    // Utility methods
    void validate_parameters();
    std::vector<std::any> expand_parameter_range(const ParameterRange& range);
};

} // namespace btrun
} // namespace backtrader