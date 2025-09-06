# 性能基准测试框架设计

本文档设计了一个全面的性能基准测试框架，用于验证C++重构版本的性能提升效果，并提供持续的性能监控能力。

## 🎯 测试框架目标

### 核心目标
1. **准确性**: 精确测量各组件的性能差异
2. **全面性**: 覆盖所有关键性能场景
3. **可重复性**: 确保测试结果稳定可靠
4. **可比性**: Python版本与C++版本的直接对比
5. **持续性**: 支持CI/CD集成的性能回归检测

### 测试维度
- **组件级别**: 单个指标、策略、数据处理组件
- **系统级别**: 完整回测流程、多策略并行
- **场景级别**: 不同数据规模、不同复杂度策略
- **资源级别**: CPU使用率、内存占用、I/O性能

## 🏗️ 框架架构设计

### 整体架构

```
基准测试框架
├── 测试数据生成器 (DataGenerator)
├── 性能测试执行器 (BenchmarkExecutor)
├── 结果收集器 (ResultCollector)
├── 性能分析器 (PerformanceAnalyzer)
└── 报告生成器 (ReportGenerator)
```

### 核心组件设计

#### 1. 基准测试基类

```cpp
// BenchmarkBase.h
#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct BenchmarkResult {
    std::string name;
    double execution_time_ns;
    double memory_usage_mb;
    double cpu_usage_percent;
    size_t iterations;
    std::map<std::string, double> custom_metrics;
    
    // 统计信息
    double mean_time;
    double median_time;
    double std_dev_time;
    double min_time;
    double max_time;
};

struct BenchmarkConfig {
    size_t min_iterations = 10;
    size_t max_iterations = 1000;
    double min_time_seconds = 1.0;
    double max_time_seconds = 60.0;
    bool warmup_enabled = true;
    size_t warmup_iterations = 3;
    bool memory_tracking = true;
    bool cpu_tracking = true;
};

class BenchmarkBase {
protected:
    std::string name_;
    BenchmarkConfig config_;
    std::vector<double> execution_times_;
    
public:
    explicit BenchmarkBase(const std::string& name, 
                          const BenchmarkConfig& config = {})
        : name_(name), config_(config) {}
    
    virtual ~BenchmarkBase() = default;
    
    // 纯虚函数 - 子类实现具体测试逻辑
    virtual void SetUp() = 0;
    virtual void Execute() = 0;
    virtual void TearDown() = 0;
    
    // 运行基准测试
    BenchmarkResult Run();
    
    // 自定义指标
    virtual std::map<std::string, double> GetCustomMetrics() { return {}; }
    
protected:
    // 内存使用监控
    double GetMemoryUsageMB();
    
    // CPU使用率监控
    double GetCpuUsagePercent();
};

// 实现
BenchmarkResult BenchmarkBase::Run() {
    SetUp();
    
    // 预热
    if (config_.warmup_enabled) {
        for (size_t i = 0; i < config_.warmup_iterations; ++i) {
            Execute();
        }
    }
    
    execution_times_.clear();
    
    auto start_memory = GetMemoryUsageMB();
    auto start_time = std::chrono::steady_clock::now();
    
    size_t iterations = 0;
    double total_time = 0.0;
    
    // 自适应迭代次数
    while (iterations < config_.min_iterations || 
           (total_time < config_.min_time_seconds && iterations < config_.max_iterations)) {
        
        auto iter_start = std::chrono::high_resolution_clock::now();
        Execute();
        auto iter_end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            iter_end - iter_start).count();
        
        execution_times_.push_back(static_cast<double>(duration));
        total_time = std::chrono::duration<double>(iter_end - start_time).count();
        iterations++;
        
        if (total_time > config_.max_time_seconds) break;
    }
    
    auto end_memory = GetMemoryUsageMB();
    auto cpu_usage = GetCpuUsagePercent();
    
    TearDown();
    
    // 计算统计信息
    std::sort(execution_times_.begin(), execution_times_.end());
    
    double mean = std::accumulate(execution_times_.begin(), execution_times_.end(), 0.0) 
                  / execution_times_.size();
    double median = execution_times_[execution_times_.size() / 2];
    
    double variance = 0.0;
    for (double time : execution_times_) {
        variance += (time - mean) * (time - mean);
    }
    double std_dev = std::sqrt(variance / execution_times_.size());
    
    BenchmarkResult result;
    result.name = name_;
    result.execution_time_ns = mean;
    result.memory_usage_mb = end_memory - start_memory;
    result.cpu_usage_percent = cpu_usage;
    result.iterations = iterations;
    result.mean_time = mean;
    result.median_time = median;
    result.std_dev_time = std_dev;
    result.min_time = execution_times_.front();
    result.max_time = execution_times_.back();
    result.custom_metrics = GetCustomMetrics();
    
    return result;
}
```

#### 2. 指标性能测试

```cpp
// IndicatorBenchmarks.h
#pragma once
#include "BenchmarkBase.h"
#include "../indicators/SMA.h"
#include "../indicators/EMA.h"
#include "../indicators/RSI.h"
#include "../core/LineRoot.h"

// SMA基准测试
class SMABenchmark : public BenchmarkBase {
private:
    std::vector<double> test_data_;
    std::shared_ptr<LineRoot> data_line_;
    std::unique_ptr<SMA> sma_;
    size_t period_;
    size_t data_size_;
    
public:
    SMABenchmark(size_t period, size_t data_size)
        : BenchmarkBase("SMA_" + std::to_string(period) + "_" + std::to_string(data_size))
        , period_(period), data_size_(data_size) {}
    
    void SetUp() override {
        // 生成测试数据
        test_data_ = GenerateRandomWalk(data_size_, 100.0, 0.02);
        
        // 创建数据线
        data_line_ = std::make_shared<LineRoot>();
        for (double value : test_data_) {
            data_line_->forward(value);
        }
        
        // 创建SMA指标
        sma_ = std::make_unique<SMA>(data_line_, period_);
    }
    
    void Execute() override {
        sma_->calculate();
    }
    
    void TearDown() override {
        sma_.reset();
        data_line_.reset();
        test_data_.clear();
    }
    
    std::map<std::string, double> GetCustomMetrics() override {
        return {
            {"throughput_bars_per_second", static_cast<double>(data_size_) / (execution_times_.back() / 1e9)},
            {"period", static_cast<double>(period_)},
            {"data_size", static_cast<double>(data_size_)}
        };
    }
    
private:
    std::vector<double> GenerateRandomWalk(size_t size, double initial, double volatility) {
        std::vector<double> data;
        data.reserve(size);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> dist(0.0, volatility);
        
        double current = initial;
        for (size_t i = 0; i < size; ++i) {
            current *= (1.0 + dist(gen));
            data.push_back(current);
        }
        
        return data;
    }
};

// EMA基准测试
class EMABenchmark : public BenchmarkBase {
private:
    std::vector<double> test_data_;
    std::shared_ptr<LineRoot> data_line_;
    std::unique_ptr<EMA> ema_;
    size_t period_;
    size_t data_size_;
    
public:
    EMABenchmark(size_t period, size_t data_size)
        : BenchmarkBase("EMA_" + std::to_string(period) + "_" + std::to_string(data_size))
        , period_(period), data_size_(data_size) {}
    
    void SetUp() override {
        test_data_ = GenerateRandomWalk(data_size_, 100.0, 0.02);
        
        data_line_ = std::make_shared<LineRoot>();
        for (double value : test_data_) {
            data_line_->forward(value);
        }
        
        ema_ = std::make_unique<EMA>(data_line_, period_);
    }
    
    void Execute() override {
        ema_->calculate();
    }
    
    void TearDown() override {
        ema_.reset();
        data_line_.reset();
        test_data_.clear();
    }
    
    std::map<std::string, double> GetCustomMetrics() override {
        return {
            {"throughput_bars_per_second", static_cast<double>(data_size_) / (execution_times_.back() / 1e9)},
            {"period", static_cast<double>(period_)},
            {"alpha", ema_->getAlpha()}
        };
    }
};

// 复杂指标基准测试
class ComplexIndicatorBenchmark : public BenchmarkBase {
private:
    std::vector<double> test_data_;
    std::shared_ptr<LineRoot> data_line_;
    std::vector<std::unique_ptr<IndicatorBase>> indicators_;
    size_t data_size_;
    
public:
    explicit ComplexIndicatorBenchmark(size_t data_size)
        : BenchmarkBase("ComplexIndicators_" + std::to_string(data_size))
        , data_size_(data_size) {}
    
    void SetUp() override {
        test_data_ = GenerateRandomWalk(data_size_, 100.0, 0.02);
        
        data_line_ = std::make_shared<LineRoot>();
        for (double value : test_data_) {
            data_line_->forward(value);
        }
        
        // 创建多个指标
        indicators_.push_back(std::make_unique<SMA>(data_line_, 20));
        indicators_.push_back(std::make_unique<EMA>(data_line_, 20));
        indicators_.push_back(std::make_unique<RSI>(data_line_, 14));
        indicators_.push_back(std::make_unique<BollingerBands>(data_line_, 20, 2.0));
        indicators_.push_back(std::make_unique<MACD>(data_line_, 12, 26, 9));
    }
    
    void Execute() override {
        // 并行计算所有指标
        #pragma omp parallel for
        for (auto& indicator : indicators_) {
            indicator->calculate();
        }
    }
    
    void TearDown() override {
        indicators_.clear();
        data_line_.reset();
        test_data_.clear();
    }
    
    std::map<std::string, double> GetCustomMetrics() override {
        return {
            {"indicator_count", static_cast<double>(indicators_.size())},
            {"total_throughput", static_cast<double>(data_size_ * indicators_.size()) / (execution_times_.back() / 1e9)}
        };
    }
};
```

#### 3. 策略性能测试

```cpp
// StrategyBenchmarks.h
#pragma once
#include "BenchmarkBase.h"
#include "../strategy/StrategyBase.h"
#include "../engine/Cerebro.h"

class SimpleStrategyBenchmark : public BenchmarkBase {
private:
    std::unique_ptr<Cerebro> cerebro_;
    std::vector<OHLCV> test_data_;
    size_t data_size_;
    
public:
    explicit SimpleStrategyBenchmark(size_t data_size)
        : BenchmarkBase("SimpleStrategy_" + std::to_string(data_size))
        , data_size_(data_size) {}
    
    void SetUp() override {
        // 生成OHLCV测试数据
        test_data_ = GenerateOHLCVData(data_size_);
        
        // 创建Cerebro实例
        cerebro_ = std::make_unique<Cerebro>();
        
        // 添加数据
        auto data_feed = std::make_shared<VectorDataFeed>(test_data_);
        cerebro_->addData(data_feed);
        
        // 添加简单策略
        cerebro_->addStrategy<SimpleSMAStrategy>();
    }
    
    void Execute() override {
        cerebro_->run();
        cerebro_->reset();  // 重置以便下次运行
    }
    
    void TearDown() override {
        cerebro_.reset();
        test_data_.clear();
    }
    
    std::map<std::string, double> GetCustomMetrics() override {
        return {
            {"bars_per_second", static_cast<double>(data_size_) / (execution_times_.back() / 1e9)},
            {"data_size", static_cast<double>(data_size_)}
        };
    }
    
private:
    std::vector<OHLCV> GenerateOHLCVData(size_t size) {
        std::vector<OHLCV> data;
        data.reserve(size);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> price_change(0.0, 0.02);
        std::uniform_real_distribution<double> volume_dist(1000, 10000);
        
        double base_price = 100.0;
        auto start_time = std::chrono::system_clock::now();
        
        for (size_t i = 0; i < size; ++i) {
            OHLCV bar;
            bar.datetime = start_time + std::chrono::hours(24 * i);
            
            double change = price_change(gen);
            double open = base_price * (1.0 + change);
            double high = open * (1.0 + std::abs(price_change(gen)));
            double low = open * (1.0 - std::abs(price_change(gen)));
            double close = open + (high - low) * (gen() % 1000) / 1000.0;
            
            bar.open = open;
            bar.high = high;
            bar.low = low;
            bar.close = close;
            bar.volume = volume_dist(gen);
            
            data.push_back(bar);
            base_price = close;
        }
        
        return data;
    }
};

// 复杂策略基准测试
class ComplexStrategyBenchmark : public BenchmarkBase {
private:
    std::unique_ptr<Cerebro> cerebro_;
    std::vector<OHLCV> test_data_;
    size_t data_size_;
    size_t strategy_count_;
    
public:
    ComplexStrategyBenchmark(size_t data_size, size_t strategy_count)
        : BenchmarkBase("ComplexStrategy_" + std::to_string(data_size) + "_" + std::to_string(strategy_count))
        , data_size_(data_size), strategy_count_(strategy_count) {}
    
    void SetUp() override {
        test_data_ = GenerateOHLCVData(data_size_);
        
        cerebro_ = std::make_unique<Cerebro>();
        
        auto data_feed = std::make_shared<VectorDataFeed>(test_data_);
        cerebro_->addData(data_feed);
        
        // 添加多个复杂策略
        for (size_t i = 0; i < strategy_count_; ++i) {
            cerebro_->addStrategy<ComplexMultiIndicatorStrategy>();
        }
    }
    
    void Execute() override {
        cerebro_->run();
        cerebro_->reset();
    }
    
    void TearDown() override {
        cerebro_.reset();
        test_data_.clear();
    }
    
    std::map<std::string, double> GetCustomMetrics() override {
        return {
            {"strategy_count", static_cast<double>(strategy_count_)},
            {"total_bars_processed", static_cast<double>(data_size_ * strategy_count_)},
            {"throughput", static_cast<double>(data_size_ * strategy_count_) / (execution_times_.back() / 1e9)}
        };
    }
};
```

#### 4. 内存和缓存性能测试

```cpp
// MemoryBenchmarks.h
#pragma once
#include "BenchmarkBase.h"

class MemoryAccessBenchmark : public BenchmarkBase {
private:
    std::vector<double> sequential_data_;
    std::vector<double> random_data_;
    std::vector<size_t> random_indices_;
    size_t data_size_;
    
public:
    explicit MemoryAccessBenchmark(size_t data_size)
        : BenchmarkBase("MemoryAccess_" + std::to_string(data_size))
        , data_size_(data_size) {}
    
    void SetUp() override {
        sequential_data_.resize(data_size_);
        random_data_.resize(data_size_);
        random_indices_.resize(data_size_);
        
        // 初始化数据
        std::iota(sequential_data_.begin(), sequential_data_.end(), 0);
        std::copy(sequential_data_.begin(), sequential_data_.end(), random_data_.begin());
        std::iota(random_indices_.begin(), random_indices_.end(), 0);
        
        // 随机打乱
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(random_data_.begin(), random_data_.end(), gen);
        std::shuffle(random_indices_.begin(), random_indices_.end(), gen);
    }
    
    void Execute() override {
        // 测试顺序访问vs随机访问
        double sequential_sum = 0.0;
        double random_sum = 0.0;
        
        // 顺序访问
        for (size_t i = 0; i < data_size_; ++i) {
            sequential_sum += sequential_data_[i];
        }
        
        // 随机访问
        for (size_t i = 0; i < data_size_; ++i) {
            random_sum += random_data_[random_indices_[i]];
        }
        
        // 防止编译器优化
        volatile double result = sequential_sum + random_sum;
        (void)result;
    }
    
    void TearDown() override {
        sequential_data_.clear();
        random_data_.clear();
        random_indices_.clear();
    }
};

class CacheEfficiencyBenchmark : public BenchmarkBase {
private:
    struct AoS {  // Array of Structures
        double open, high, low, close, volume;
    };
    
    struct SoA {  // Structure of Arrays
        std::vector<double> open, high, low, close, volume;
    };
    
    std::vector<AoS> aos_data_;
    SoA soa_data_;
    size_t data_size_;
    
public:
    explicit CacheEfficiencyBenchmark(size_t data_size)
        : BenchmarkBase("CacheEfficiency_" + std::to_string(data_size))
        , data_size_(data_size) {}
    
    void SetUp() override {
        aos_data_.resize(data_size_);
        soa_data_.open.resize(data_size_);
        soa_data_.high.resize(data_size_);
        soa_data_.low.resize(data_size_);
        soa_data_.close.resize(data_size_);
        soa_data_.volume.resize(data_size_);
        
        // 初始化数据
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(100.0, 200.0);
        
        for (size_t i = 0; i < data_size_; ++i) {
            double price = dist(gen);
            
            aos_data_[i] = {price, price * 1.01, price * 0.99, price * 1.005, 1000.0};
            
            soa_data_.open[i] = price;
            soa_data_.high[i] = price * 1.01;
            soa_data_.low[i] = price * 0.99;
            soa_data_.close[i] = price * 1.005;
            soa_data_.volume[i] = 1000.0;
        }
    }
    
    void Execute() override {
        // 测试AoS vs SoA的缓存效率
        
        // AoS访问模式 - 只处理close价格
        double aos_sum = 0.0;
        for (size_t i = 0; i < data_size_; ++i) {
            aos_sum += aos_data_[i].close;  // 缓存不友好，每次加载整个结构
        }
        
        // SoA访问模式 - 只处理close价格
        double soa_sum = 0.0;
        for (size_t i = 0; i < data_size_; ++i) {
            soa_sum += soa_data_.close[i];  // 缓存友好，连续内存访问
        }
        
        volatile double result = aos_sum + soa_sum;
        (void)result;
    }
    
    void TearDown() override {
        aos_data_.clear();
        soa_data_.open.clear();
        soa_data_.high.clear();
        soa_data_.low.clear();
        soa_data_.close.clear();
        soa_data_.volume.clear();
    }
};
```

#### 5. Python vs C++ 对比测试

```cpp
// ComparisonBenchmarks.h
#pragma once
#include "BenchmarkBase.h"

class PythonVsCppBenchmark : public BenchmarkBase {
private:
    std::string python_script_;
    std::string test_data_file_;
    size_t data_size_;
    
public:
    PythonVsCppBenchmark(const std::string& test_name, size_t data_size)
        : BenchmarkBase("PythonVsCpp_" + test_name + "_" + std::to_string(data_size))
        , data_size_(data_size) {}
    
    void SetUp() override {
        // 生成测试数据文件
        test_data_file_ = "test_data_" + std::to_string(data_size_) + ".csv";
        GenerateTestDataFile(test_data_file_, data_size_);
        
        // 准备Python测试脚本
        python_script_ = R"(
import backtrader as bt
import pandas as pd
import time

class TestStrategy(bt.Strategy):
    def __init__(self):
        self.sma = bt.indicators.SMA(period=20)
        self.rsi = bt.indicators.RSI(period=14)
    
    def next(self):
        if not self.position:
            if self.data.close[0] > self.sma[0] and self.rsi[0] < 30:
                self.buy()
        else:
            if self.rsi[0] > 70:
                self.sell()

cerebro = bt.Cerebro()
data = bt.feeds.GenericCSVData(dataname=')" + test_data_file_ + R"(')
cerebro.adddata(data)
cerebro.addstrategy(TestStrategy)

start_time = time.time()
cerebro.run()
end_time = time.time()

print(f"Python execution time: {(end_time - start_time) * 1000:.2f}ms")
)";
    }
    
    void Execute() override {
        // 运行C++版本
        auto start_cpp = std::chrono::high_resolution_clock::now();
        RunCppVersion();
        auto end_cpp = std::chrono::high_resolution_clock::now();
        
        // 运行Python版本
        auto start_python = std::chrono::high_resolution_clock::now();
        RunPythonVersion();
        auto end_python = std::chrono::high_resolution_clock::now();
        
        // 记录时间差异
        cpp_time_ = std::chrono::duration<double, std::milli>(end_cpp - start_cpp).count();
        python_time_ = std::chrono::duration<double, std::milli>(end_python - start_python).count();
    }
    
    void TearDown() override {
        std::remove(test_data_file_.c_str());
    }
    
    std::map<std::string, double> GetCustomMetrics() override {
        return {
            {"cpp_time_ms", cpp_time_},
            {"python_time_ms", python_time_},
            {"speedup_factor", python_time_ / cpp_time_},
            {"data_size", static_cast<double>(data_size_)}
        };
    }
    
private:
    double cpp_time_;
    double python_time_;
    
    void RunCppVersion() {
        // 实现C++版本的相同逻辑
        auto cerebro = std::make_unique<Cerebro>();
        auto data = std::make_shared<CSVDataFeed>(test_data_file_);
        cerebro->addData(data);
        cerebro->addStrategy<TestStrategy>();
        cerebro->run();
    }
    
    void RunPythonVersion() {
        // 执行Python脚本
        std::string command = "python3 -c \"" + python_script_ + "\"";
        std::system(command.c_str());
    }
    
    void GenerateTestDataFile(const std::string& filename, size_t size) {
        std::ofstream file(filename);
        file << "Date,Open,High,Low,Close,Volume\n";
        
        auto start_date = std::chrono::system_clock::now();
        double price = 100.0;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> price_change(0.0, 0.02);
        
        for (size_t i = 0; i < size; ++i) {
            auto date = start_date + std::chrono::hours(24 * i);
            auto time_t_date = std::chrono::system_clock::to_time_t(date);
            
            double change = price_change(gen);
            double open = price * (1.0 + change);
            double high = open * (1.0 + std::abs(change));
            double low = open * (1.0 - std::abs(change));
            double close = open + (high - low) * 0.5;
            
            file << std::put_time(std::gmtime(&time_t_date), "%Y-%m-%d") << ","
                 << open << "," << high << "," << low << "," << close << ",1000\n";
            
            price = close;
        }
    }
};
```

### 6. 基准测试套件管理器

```cpp
// BenchmarkSuite.h
#pragma once
#include "BenchmarkBase.h"
#include <vector>
#include <memory>
#include <fstream>
#include <iomanip>

class BenchmarkSuite {
private:
    std::vector<std::unique_ptr<BenchmarkBase>> benchmarks_;
    std::string output_file_;
    
public:
    explicit BenchmarkSuite(const std::string& output_file = "benchmark_results.json")
        : output_file_(output_file) {}
    
    // 添加基准测试
    template<typename T, typename... Args>
    void AddBenchmark(Args&&... args) {
        benchmarks_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
    
    // 运行所有基准测试
    std::vector<BenchmarkResult> RunAll() {
        std::vector<BenchmarkResult> results;
        results.reserve(benchmarks_.size());
        
        std::cout << "Running " << benchmarks_.size() << " benchmarks...\n";
        
        for (size_t i = 0; i < benchmarks_.size(); ++i) {
            std::cout << "[" << (i + 1) << "/" << benchmarks_.size() << "] "
                      << "Running " << benchmarks_[i]->GetName() << "..." << std::flush;
            
            auto result = benchmarks_[i]->Run();
            results.push_back(result);
            
            std::cout << " Done (" << std::fixed << std::setprecision(2)
                      << result.mean_time / 1e6 << "ms avg)\n";
        }
        
        // 保存结果
        SaveResults(results);
        
        return results;
    }
    
    // 保存结果到JSON文件
    void SaveResults(const std::vector<BenchmarkResult>& results) {
        std::ofstream file(output_file_);
        file << "{\n";
        file << "  \"benchmark_results\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            
            file << "    {\n";
            file << "      \"name\": \"" << result.name << "\",\n";
            file << "      \"mean_time_ns\": " << result.mean_time << ",\n";
            file << "      \"median_time_ns\": " << result.median_time << ",\n";
            file << "      \"std_dev_ns\": " << result.std_dev_time << ",\n";
            file << "      \"min_time_ns\": " << result.min_time << ",\n";
            file << "      \"max_time_ns\": " << result.max_time << ",\n";
            file << "      \"iterations\": " << result.iterations << ",\n";
            file << "      \"memory_usage_mb\": " << result.memory_usage_mb << ",\n";
            file << "      \"cpu_usage_percent\": " << result.cpu_usage_percent << ",\n";
            file << "      \"custom_metrics\": {\n";
            
            size_t metric_count = 0;
            for (const auto& [key, value] : result.custom_metrics) {
                file << "        \"" << key << "\": " << value;
                if (++metric_count < result.custom_metrics.size()) {
                    file << ",";
                }
                file << "\n";
            }
            
            file << "      }\n";
            file << "    }";
            if (i < results.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ],\n";
        file << "  \"timestamp\": \"" << GetCurrentTimestamp() << "\",\n";
        file << "  \"total_benchmarks\": " << results.size() << "\n";
        file << "}\n";
    }
    
    // 生成性能报告
    void GenerateReport(const std::vector<BenchmarkResult>& results) {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "BENCHMARK RESULTS SUMMARY\n";
        std::cout << std::string(80, '=') << "\n";
        
        std::cout << std::left << std::setw(40) << "Benchmark"
                  << std::setw(15) << "Mean (ms)"
                  << std::setw(15) << "Std Dev (ms)"
                  << std::setw(10) << "Iterations" << "\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(40) << result.name
                      << std::setw(15) << std::fixed << std::setprecision(3) << (result.mean_time / 1e6)
                      << std::setw(15) << std::fixed << std::setprecision(3) << (result.std_dev_time / 1e6)
                      << std::setw(10) << result.iterations << "\n";
        }
        
        std::cout << std::string(80, '=') << "\n";
    }
    
private:
    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
        return ss.str();
    }
};
```

### 7. 主基准测试程序

```cpp
// main_benchmark.cpp
#include "BenchmarkSuite.h"
#include "IndicatorBenchmarks.h"
#include "StrategyBenchmarks.h"
#include "MemoryBenchmarks.h"
#include "ComparisonBenchmarks.h"

int main(int argc, char* argv[]) {
    BenchmarkSuite suite("backtrader_cpp_benchmarks.json");
    
    // 指标性能测试
    std::cout << "Adding indicator benchmarks...\n";
    
    // 不同数据规模的SMA测试
    for (size_t data_size : {1000, 10000, 100000, 1000000}) {
        for (size_t period : {10, 20, 50}) {
            suite.AddBenchmark<SMABenchmark>(period, data_size);
            suite.AddBenchmark<EMABenchmark>(period, data_size);
        }
    }
    
    // 复杂指标测试
    for (size_t data_size : {10000, 100000, 1000000}) {
        suite.AddBenchmark<ComplexIndicatorBenchmark>(data_size);
    }
    
    // 策略性能测试
    std::cout << "Adding strategy benchmarks...\n";
    
    for (size_t data_size : {1000, 10000, 100000}) {
        suite.AddBenchmark<SimpleStrategyBenchmark>(data_size);
        
        for (size_t strategy_count : {1, 5, 10}) {
            suite.AddBenchmark<ComplexStrategyBenchmark>(data_size, strategy_count);
        }
    }
    
    // 内存和缓存测试
    std::cout << "Adding memory benchmarks...\n";
    
    for (size_t data_size : {10000, 100000, 1000000, 10000000}) {
        suite.AddBenchmark<MemoryAccessBenchmark>(data_size);
        suite.AddBenchmark<CacheEfficiencyBenchmark>(data_size);
    }
    
    // Python vs C++ 对比测试
    std::cout << "Adding comparison benchmarks...\n";
    
    for (size_t data_size : {1000, 10000, 50000}) {
        suite.AddBenchmark<PythonVsCppBenchmark>("SMAStrategy", data_size);
    }
    
    // 运行所有基准测试
    auto results = suite.RunAll();
    
    // 生成报告
    suite.GenerateReport(results);
    
    std::cout << "\nBenchmark results saved to: backtrader_cpp_benchmarks.json\n";
    
    return 0;
}
```

### 8. CI/CD集成

#### GitHub Actions基准测试工作流

```yaml
# .github/workflows/benchmarks.yml
name: Performance Benchmarks

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 2 * * 0'  # 每周日运行

jobs:
  benchmark:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake ninja-build libbenchmark-dev python3 python3-pip
        pip3 install backtrader pandas numpy
    
    - name: Configure CMake
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON -G Ninja
    
    - name: Build
      run: cmake --build build --config Release
    
    - name: Run benchmarks
      run: |
        cd build
        ./benchmarks/backtrader_benchmarks --benchmark_format=json > benchmark_results.json
    
    - name: Upload benchmark results
      uses: actions/upload-artifact@v3
      with:
        name: benchmark-results
        path: build/benchmark_results.json
    
    - name: Compare with baseline
      if: github.event_name == 'pull_request'
      run: |
        python3 scripts/compare_benchmarks.py \
          --baseline baseline_benchmarks.json \
          --current build/benchmark_results.json \
          --output comparison.md
    
    - name: Comment PR with results
      if: github.event_name == 'pull_request'
      uses: actions/github-script@v6
      with:
        script: |
          const fs = require('fs');
          const comparison = fs.readFileSync('comparison.md', 'utf8');
          github.rest.issues.createComment({
            issue_number: context.issue.number,
            owner: context.repo.owner,
            repo: context.repo.repo,
            body: comparison
          });
```

这个基准测试框架提供了：

1. **全面的性能测试**: 覆盖所有关键组件和场景
2. **统计学准确性**: 自适应迭代次数和详细统计信息
3. **资源监控**: CPU和内存使用率跟踪
4. **对比测试**: Python vs C++性能对比
5. **CI/CD集成**: 自动化性能回归检测
6. **详细报告**: JSON和文本格式的结果输出

通过这个框架，可以精确验证C++重构版本是否达到了8-15倍的性能提升目标。