#pragma once

#include "cerebro/Cerebro.h"
#include "strategy/StrategyBase.h"
#include <vector>
#include <functional>
#include <map>
#include <string>
#include <future>
#include <thread>
#include <atomic>

namespace backtrader {
namespace optimization {

using namespace cerebro;
using namespace strategy;

/**
 * @brief 参数范围定义
 */
struct ParameterRange {
    std::string name;
    double min_value;
    double max_value;
    double step;
    
    ParameterRange(const std::string& n, double min_val, double max_val, double s = 1.0)
        : name(n), min_value(min_val), max_value(max_val), step(s) {}
    
    /**
     * @brief 生成参数值序列
     */
    std::vector<double> generateValues() const {
        std::vector<double> values;
        for (double value = min_value; value <= max_value; value += step) {
            values.push_back(value);
        }
        return values;
    }
    
    /**
     * @brief 获取参数组合数量
     */
    size_t getCount() const {
        if (step <= 0) return 0;
        return static_cast<size_t>((max_value - min_value) / step) + 1;
    }
};

/**
 * @brief 参数组合
 */
using ParameterSet = std::map<std::string, double>;

/**
 * @brief 优化结果
 */
struct OptimizationResult {
    ParameterSet parameters;
    BacktestResult backtest_result;
    double objective_value;
    
    OptimizationResult() : objective_value(0.0) {}
};

/**
 * @brief 目标函数类型
 */
enum class ObjectiveType {
    TOTAL_RETURN,        // 总收益率
    SHARPE_RATIO,        // 夏普比率
    MAX_DRAWDOWN,        // 最大回撤 (负优化)
    PROFIT_FACTOR,       // 盈亏比
    WIN_RATE,            // 胜率
    CALMAR_RATIO,        // 卡玛比率
    CUSTOM              // 自定义函数
};

/**
 * @brief 策略工厂函数类型
 */
using StrategyFactory = std::function<std::shared_ptr<StrategyBase>(const ParameterSet&)>;

/**
 * @brief 自定义目标函数类型
 */
using CustomObjectiveFunction = std::function<double(const BacktestResult&)>;

/**
 * @brief 策略优化器
 * 
 * 支持多参数网格搜索优化，并行计算，多种目标函数
 */
class StrategyOptimizer {
private:
    std::vector<ParameterRange> parameter_ranges_;
    StrategyFactory strategy_factory_;
    ObjectiveType objective_type_;
    CustomObjectiveFunction custom_objective_;
    
    // 数据和配置
    std::vector<std::shared_ptr<data::DataFeed>> data_feeds_;
    double initial_cash_;
    double commission_;
    double slippage_;
    
    // 并行执行
    size_t max_workers_;
    std::atomic<bool> stop_optimization_;
    std::atomic<size_t> completed_runs_;
    size_t total_runs_;
    
    // 进度回调
    std::function<void(size_t, size_t, const OptimizationResult&)> progress_callback_;
    std::function<void(const std::string&)> log_callback_;
    
public:
    /**
     * @brief 构造函数
     * @param strategy_factory 策略工厂函数
     * @param initial_cash 初始资金
     */
    explicit StrategyOptimizer(StrategyFactory strategy_factory,
                              double initial_cash = 100000.0)
        : strategy_factory_(strategy_factory),
          objective_type_(ObjectiveType::TOTAL_RETURN),
          initial_cash_(initial_cash),
          commission_(0.001),
          slippage_(0.0),
          max_workers_(std::thread::hardware_concurrency()),
          stop_optimization_(false),
          completed_runs_(0),
          total_runs_(0) {
        
        if (max_workers_ == 0) max_workers_ = 1;
    }
    
    /**
     * @brief 添加参数范围
     * @param range 参数范围
     */
    void addParameterRange(const ParameterRange& range) {
        parameter_ranges_.push_back(range);
    }
    
    /**
     * @brief 添加数据源
     * @param data_feed 数据源
     */
    void addDataFeed(std::shared_ptr<data::DataFeed> data_feed) {
        data_feeds_.push_back(data_feed);
    }
    
    /**
     * @brief 设置目标函数
     * @param objective_type 目标函数类型
     */
    void setObjectiveFunction(ObjectiveType objective_type) {
        objective_type_ = objective_type;
    }
    
    /**
     * @brief 设置自定义目标函数
     * @param custom_func 自定义目标函数
     */
    void setCustomObjectiveFunction(CustomObjectiveFunction custom_func) {
        objective_type_ = ObjectiveType::CUSTOM;
        custom_objective_ = custom_func;
    }
    
    /**
     * @brief 设置交易成本
     * @param commission 手续费率
     * @param slippage 滑点
     */
    void setTradingCosts(double commission, double slippage = 0.0) {
        commission_ = commission;
        slippage_ = slippage;
    }
    
    /**
     * @brief 设置并行度
     * @param max_workers 最大工作线程数
     */
    void setMaxWorkers(size_t max_workers) {
        max_workers_ = std::max(1UL, max_workers);
    }
    
    /**
     * @brief 设置进度回调
     * @param callback 进度回调函数
     */
    void setProgressCallback(std::function<void(size_t, size_t, const OptimizationResult&)> callback) {
        progress_callback_ = callback;
    }
    
    /**
     * @brief 设置日志回调
     * @param callback 日志回调函数
     */
    void setLogCallback(std::function<void(const std::string&)> callback) {
        log_callback_ = callback;
    }
    
    /**
     * @brief 运行优化
     * @return 优化结果列表（按目标函数值降序排列）
     */
    std::vector<OptimizationResult> optimize() {
        logMessage("Starting strategy optimization");
        
        // 生成所有参数组合
        auto parameter_combinations = generateParameterCombinations();
        total_runs_ = parameter_combinations.size();
        completed_runs_ = 0;
        stop_optimization_ = false;
        
        logMessage("Generated " + std::to_string(total_runs_) + " parameter combinations");
        
        std::vector<OptimizationResult> results;
        results.reserve(total_runs_);
        
        if (max_workers_ == 1) {
            // 单线程执行
            for (const auto& params : parameter_combinations) {
                if (stop_optimization_) break;
                
                auto result = runSingleOptimization(params);
                results.push_back(result);
                
                completed_runs_++;
                if (progress_callback_) {
                    progress_callback_(completed_runs_, total_runs_, result);
                }
            }
        } else {
            // 多线程并行执行
            results = runParallelOptimization(parameter_combinations);
        }
        
        // 按目标函数值排序
        std::sort(results.begin(), results.end(),
                 [](const OptimizationResult& a, const OptimizationResult& b) {
                     return a.objective_value > b.objective_value;
                 });
        
        logMessage("Optimization completed. Best objective value: " + 
                  std::to_string(results.empty() ? 0.0 : results[0].objective_value));
        
        return results;
    }
    
    /**
     * @brief 停止优化
     */
    void stop() {
        stop_optimization_ = true;
        logMessage("Optimization stop requested");
    }
    
    /**
     * @brief 获取进度
     * @return 进度百分比 (0-100)
     */
    double getProgress() const {
        if (total_runs_ == 0) return 0.0;
        return (static_cast<double>(completed_runs_) / total_runs_) * 100.0;
    }
    
    /**
     * @brief 估算总运行时间
     * @param sample_size 采样大小
     * @return 预估时间（秒）
     */
    double estimateRunTime(size_t sample_size = 10) {
        if (parameter_ranges_.empty()) return 0.0;
        
        auto parameter_combinations = generateParameterCombinations();
        size_t sample_count = std::min(sample_size, parameter_combinations.size());
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < sample_count; ++i) {
            runSingleOptimization(parameter_combinations[i]);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        double avg_time_per_run = static_cast<double>(duration.count()) / sample_count;
        double total_estimated_time = (avg_time_per_run * parameter_combinations.size()) / 1000.0; // 转换为秒
        
        // 考虑并行执行
        if (max_workers_ > 1) {
            total_estimated_time /= max_workers_;
        }
        
        return total_estimated_time;
    }
    
    /**
     * @brief 生成优化报告
     * @param results 优化结果
     * @param top_n 显示前N个结果
     * @return 报告字符串
     */
    std::string generateOptimizationReport(const std::vector<OptimizationResult>& results,
                                          size_t top_n = 10) const {
        std::ostringstream report;
        report << "=== Strategy Optimization Report ===\n";
        report << "Total parameter combinations tested: " << results.size() << "\n";
        report << "Objective function: " << objectiveTypeToString(objective_type_) << "\n\n";
        
        size_t count = std::min(top_n, results.size());
        report << "Top " << count << " results:\n";
        report << std::string(80, '-') << "\n";
        
        for (size_t i = 0; i < count; ++i) {
            const auto& result = results[i];
            report << "Rank " << (i + 1) << ":\n";
            report << "  Objective Value: " << result.objective_value << "\n";
            report << "  Parameters: ";
            
            for (const auto& [name, value] : result.parameters) {
                report << name << "=" << value << " ";
            }
            report << "\n";
            
            report << "  Total Return: " << result.backtest_result.total_return << "%\n";
            report << "  Max Drawdown: " << result.backtest_result.max_drawdown << "%\n";
            report << "  Total Trades: " << result.backtest_result.total_trades << "\n";
            report << "  Win Rate: " << result.backtest_result.win_rate << "%\n";
            report << "\n";
        }
        
        return report.str();
    }
    
private:
    /**
     * @brief 生成所有参数组合
     */
    std::vector<ParameterSet> generateParameterCombinations() const {
        std::vector<ParameterSet> combinations;
        
        if (parameter_ranges_.empty()) {
            combinations.push_back(ParameterSet());
            return combinations;
        }
        
        // 递归生成所有组合
        ParameterSet current_params;
        generateCombinationsRecursive(0, current_params, combinations);
        
        return combinations;
    }
    
    /**
     * @brief 递归生成参数组合
     */
    void generateCombinationsRecursive(size_t param_index,
                                     ParameterSet current_params,
                                     std::vector<ParameterSet>& combinations) const {
        if (param_index >= parameter_ranges_.size()) {
            combinations.push_back(current_params);
            return;
        }
        
        const auto& range = parameter_ranges_[param_index];
        auto values = range.generateValues();
        
        for (double value : values) {
            current_params[range.name] = value;
            generateCombinationsRecursive(param_index + 1, current_params, combinations);
        }
    }
    
    /**
     * @brief 运行单次优化
     */
    OptimizationResult runSingleOptimization(const ParameterSet& parameters) {
        OptimizationResult result;
        result.parameters = parameters;
        
        try {
            // 创建策略
            auto strategy = strategy_factory_(parameters);
            if (!strategy) {
                return result;
            }
            
            // 创建Cerebro并配置
            Cerebro cerebro(initial_cash_);
            cerebro.addStrategy(strategy);
            
            for (auto& data_feed : data_feeds_) {
                cerebro.addDataFeed(data_feed);
            }
            
            cerebro.setCommission(commission_, true);
            if (slippage_ > 0) {
                cerebro.setSlippage(slippage_, true);
            }
            
            // 运行回测
            result.backtest_result = cerebro.run();
            
            // 计算目标函数值
            result.objective_value = calculateObjectiveValue(result.backtest_result);
            
        } catch (const std::exception& e) {
            logMessage("Error in optimization run: " + std::string(e.what()));
            result.objective_value = -std::numeric_limits<double>::max();
        }
        
        return result;
    }
    
    /**
     * @brief 并行运行优化
     */
    std::vector<OptimizationResult> runParallelOptimization(
        const std::vector<ParameterSet>& parameter_combinations) {
        
        std::vector<OptimizationResult> results;
        results.resize(parameter_combinations.size());
        
        std::vector<std::future<void>> futures;
        std::atomic<size_t> next_index(0);
        
        // 启动工作线程
        for (size_t i = 0; i < max_workers_; ++i) {
            futures.emplace_back(std::async(std::launch::async, [&]() {
                size_t index;
                while ((index = next_index.fetch_add(1)) < parameter_combinations.size() && 
                       !stop_optimization_) {
                    
                    results[index] = runSingleOptimization(parameter_combinations[index]);
                    
                    completed_runs_++;
                    if (progress_callback_) {
                        progress_callback_(completed_runs_, total_runs_, results[index]);
                    }
                }
            }));
        }
        
        // 等待所有线程完成
        for (auto& future : futures) {
            future.wait();
        }
        
        return results;
    }
    
    /**
     * @brief 计算目标函数值
     */
    double calculateObjectiveValue(const BacktestResult& result) const {
        switch (objective_type_) {
            case ObjectiveType::TOTAL_RETURN:
                return result.total_return;
                
            case ObjectiveType::SHARPE_RATIO:
                return result.sharpe_ratio;
                
            case ObjectiveType::MAX_DRAWDOWN:
                return -result.max_drawdown; // 负值优化
                
            case ObjectiveType::WIN_RATE:
                return result.win_rate;
                
            case ObjectiveType::CALMAR_RATIO:
                return (result.max_drawdown > 0) ? 
                       result.annualized_return / result.max_drawdown : 0.0;
                
            case ObjectiveType::CUSTOM:
                return custom_objective_ ? custom_objective_(result) : 0.0;
                
            default:
                return result.total_return;
        }
    }
    
    /**
     * @brief 目标函数类型转字符串
     */
    std::string objectiveTypeToString(ObjectiveType type) const {
        switch (type) {
            case ObjectiveType::TOTAL_RETURN: return "Total Return";
            case ObjectiveType::SHARPE_RATIO: return "Sharpe Ratio";
            case ObjectiveType::MAX_DRAWDOWN: return "Max Drawdown (minimized)";
            case ObjectiveType::WIN_RATE: return "Win Rate";
            case ObjectiveType::CALMAR_RATIO: return "Calmar Ratio";
            case ObjectiveType::CUSTOM: return "Custom Function";
            default: return "Unknown";
        }
    }
    
    /**
     * @brief 记录日志
     */
    void logMessage(const std::string& message) {
        if (log_callback_) {
            log_callback_(message);
        }
    }
};

} // namespace optimization
} // namespace backtrader