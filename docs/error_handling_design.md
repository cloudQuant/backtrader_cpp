# 统一错误处理和调试系统设计

本文档设计了backtrader C++版本的统一错误处理、异常管理和调试系统，确保系统的稳定性、可维护性和开发效率。

## 🎯 设计目标

### 核心目标
1. **统一性**: 全系统使用一致的错误处理机制
2. **可追踪性**: 完整的错误追踪和日志记录
3. **调试友好**: 丰富的调试信息和工具支持
4. **性能平衡**: 最小化错误处理对性能的影响
5. **Python兼容**: 与Python异常系统的无缝对接

### 设计原则
- **早期检测**: 在问题发生源头进行检测
- **明确分类**: 不同类型错误使用不同处理策略
- **优雅降级**: 系统能在错误情况下优雅处理
- **可配置性**: 错误处理级别可配置
- **向后兼容**: 保持与Python版本的行为一致

## 🏗️ 错误分类体系

### 错误类型层次结构

```cpp
// include/core/Exceptions.h
#pragma once
#include <exception>
#include <string>
#include <vector>
#include <source_location>

namespace backtrader {

// 基础异常类
class BacktraderException : public std::exception {
protected:
    std::string message_;
    std::string context_;
    std::source_location location_;
    std::vector<std::string> stack_trace_;
    
public:
    explicit BacktraderException(
        const std::string& message,
        const std::string& context = "",
        const std::source_location& location = std::source_location::current()
    );
    
    const char* what() const noexcept override;
    const std::string& getMessage() const noexcept { return message_; }
    const std::string& getContext() const noexcept { return context_; }
    const std::source_location& getLocation() const noexcept { return location_; }
    const std::vector<std::string>& getStackTrace() const noexcept { return stack_trace_; }
    
    virtual std::string getFormattedMessage() const;
    virtual int getErrorCode() const { return 0; }
};

// 数据相关异常
class DataException : public BacktraderException {
public:
    using BacktraderException::BacktraderException;
    int getErrorCode() const override { return 1000; }
};

class DataFeedException : public DataException {
public:
    using DataException::DataException;
    int getErrorCode() const override { return 1001; }
};

class DataFormatException : public DataException {
public:
    using DataException::DataException;
    int getErrorCode() const override { return 1002; }
};

class DataRangeException : public DataException {
public:
    using DataException::DataException;
    int getErrorCode() const override { return 1003; }
};

// 指标相关异常
class IndicatorException : public BacktraderException {
public:
    using BacktraderException::BacktraderException;
    int getErrorCode() const override { return 2000; }
};

class IndicatorPeriodException : public IndicatorException {
public:
    using IndicatorException::IndicatorException;
    int getErrorCode() const override { return 2001; }
};

class IndicatorCalculationException : public IndicatorException {
public:
    using IndicatorException::IndicatorException;
    int getErrorCode() const override { return 2002; }
};

// 交易相关异常
class TradingException : public BacktraderException {
public:
    using BacktraderException::BacktraderException;
    int getErrorCode() const override { return 3000; }
};

class OrderException : public TradingException {
public:
    using TradingException::TradingException;
    int getErrorCode() const override { return 3001; }
};

class BrokerException : public TradingException {
public:
    using TradingException::TradingException;
    int getErrorCode() const override { return 3002; }
};

class InsufficientFundsException : public BrokerException {
public:
    using BrokerException::BrokerException;
    int getErrorCode() const override { return 3003; }
};

// 策略相关异常
class StrategyException : public BacktraderException {
public:
    using BacktraderException::BacktraderException;
    int getErrorCode() const override { return 4000; }
};

class StrategyInitException : public StrategyException {
public:
    using StrategyException::StrategyException;
    int getErrorCode() const override { return 4001; }
};

class StrategyExecutionException : public StrategyException {
public:
    using StrategyException::StrategyException;
    int getErrorCode() const override { return 4002; }
};

// 引擎相关异常
class EngineException : public BacktraderException {
public:
    using BacktraderException::BacktraderException;
    int getErrorCode() const override { return 5000; }
};

class CerebroException : public EngineException {
public:
    using EngineException::EngineException;
    int getErrorCode() const override { return 5001; }
};

// 配置相关异常
class ConfigurationException : public BacktraderException {
public:
    using BacktraderException::BacktraderException;
    int getErrorCode() const override { return 6000; }
};

class ParameterException : public ConfigurationException {
public:
    using ConfigurationException::ConfigurationException;
    int getErrorCode() const override { return 6001; }
};

} // namespace backtrader
```

## 🛠️ 错误处理基础设施

### 错误处理器和日志系统

```cpp
// include/core/ErrorHandler.h
#pragma once
#include "Exceptions.h"
#include "Logger.h"
#include <functional>
#include <unordered_map>
#include <memory>

namespace backtrader {

enum class ErrorLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

enum class ErrorAction {
    IGNORE,      // 忽略错误
    LOG,         // 仅记录日志
    WARN,        // 发出警告
    THROW,       // 抛出异常
    ABORT        // 终止程序
};

struct ErrorPolicy {
    ErrorAction action = ErrorAction::THROW;
    ErrorLevel log_level = ErrorLevel::ERROR;
    bool collect_stack_trace = true;
    bool notify_handlers = true;
};

class ErrorHandler {
private:
    static std::unique_ptr<ErrorHandler> instance_;
    std::unordered_map<int, ErrorPolicy> policies_;
    std::vector<std::function<void(const BacktraderException&)>> error_handlers_;
    std::shared_ptr<Logger> logger_;
    
public:
    static ErrorHandler& getInstance();
    
    // 配置错误策略
    void setErrorPolicy(int error_code, const ErrorPolicy& policy);
    void setDefaultPolicy(const ErrorPolicy& policy);
    
    // 注册错误处理器
    void addErrorHandler(std::function<void(const BacktraderException&)> handler);
    
    // 处理错误
    template<typename ExceptionType>
    void handleError(
        const std::string& message,
        const std::string& context = "",
        const std::source_location& location = std::source_location::current()
    );
    
    // 检查和验证
    template<typename T>
    void checkCondition(
        bool condition,
        const std::string& message,
        const std::string& context = "",
        const std::source_location& location = std::source_location::current()
    );
    
    void setLogger(std::shared_ptr<Logger> logger) { logger_ = logger; }
    
private:
    ErrorHandler() = default;
    void executePolicy(const BacktraderException& exception, const ErrorPolicy& policy);
    void notifyHandlers(const BacktraderException& exception);
    std::vector<std::string> collectStackTrace();
};

// 实现模板方法
template<typename ExceptionType>
void ErrorHandler::handleError(
    const std::string& message,
    const std::string& context,
    const std::source_location& location
) {
    static_assert(std::is_base_of_v<BacktraderException, ExceptionType>,
                  "ExceptionType must derive from BacktraderException");
    
    ExceptionType exception(message, context, location);
    
    auto it = policies_.find(exception.getErrorCode());
    const auto& policy = (it != policies_.end()) ? it->second : policies_[0];
    
    executePolicy(exception, policy);
}

template<typename T>
void ErrorHandler::checkCondition(
    bool condition,
    const std::string& message,
    const std::string& context,
    const std::source_location& location
) {
    if (!condition) {
        handleError<T>(message, context, location);
    }
}

} // namespace backtrader
```

### 高级日志系统

```cpp
// include/core/Logger.h
#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <source_location>

namespace backtrader {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

struct LogEntry {
    LogLevel level;
    std::string message;
    std::string context;
    std::source_location location;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
};

class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
};

class ConsoleSink : public LogSink {
public:
    void write(const LogEntry& entry) override;
    void flush() override;
};

class FileSink : public LogSink {
private:
    std::ofstream file_;
    std::string filename_;
    
public:
    explicit FileSink(const std::string& filename);
    ~FileSink();
    
    void write(const LogEntry& entry) override;
    void flush() override;
};

class AsyncLogger {
private:
    std::vector<std::unique_ptr<LogSink>> sinks_;
    std::queue<LogEntry> log_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_{true};
    LogLevel min_level_{LogLevel::INFO};
    
public:
    AsyncLogger();
    ~AsyncLogger();
    
    void addSink(std::unique_ptr<LogSink> sink);
    void setMinLevel(LogLevel level) { min_level_ = level; }
    
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args);
    
    void log(LogLevel level, 
             const std::string& message,
             const std::string& context = "",
             const std::source_location& location = std::source_location::current());
    
    void flush();
    
private:
    void workerThread();
    std::string formatMessage(const LogEntry& entry);
};

using Logger = AsyncLogger;

// 便利的日志宏
#define BT_LOG_TRACE(logger, msg, ...) \
    (logger)->log(LogLevel::TRACE, msg, ##__VA_ARGS__)

#define BT_LOG_DEBUG(logger, msg, ...) \
    (logger)->log(LogLevel::DEBUG, msg, ##__VA_ARGS__)

#define BT_LOG_INFO(logger, msg, ...) \
    (logger)->log(LogLevel::INFO, msg, ##__VA_ARGS__)

#define BT_LOG_WARNING(logger, msg, ...) \
    (logger)->log(LogLevel::WARNING, msg, ##__VA_ARGS__)

#define BT_LOG_ERROR(logger, msg, ...) \
    (logger)->log(LogLevel::ERROR, msg, ##__VA_ARGS__)

#define BT_LOG_CRITICAL(logger, msg, ...) \
    (logger)->log(LogLevel::CRITICAL, msg, ##__VA_ARGS__)

} // namespace backtrader
```

## 🔍 调试系统设计

### 调试信息收集器

```cpp
// include/debug/DebugInfo.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <memory>

namespace backtrader {
namespace debug {

struct PerformanceMetrics {
    std::chrono::nanoseconds execution_time{0};
    size_t memory_usage = 0;
    size_t function_calls = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
};

struct ComponentState {
    std::string component_name;
    std::unordered_map<std::string, std::string> properties;
    std::chrono::system_clock::time_point timestamp;
    PerformanceMetrics metrics;
};

class DebugCollector {
private:
    static std::unique_ptr<DebugCollector> instance_;
    std::vector<ComponentState> state_history_;
    std::unordered_map<std::string, PerformanceMetrics> component_metrics_;
    mutable std::mutex mutex_;
    bool enabled_ = false;
    
public:
    static DebugCollector& getInstance();
    
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }
    
    void recordState(const ComponentState& state);
    void updateMetrics(const std::string& component, const PerformanceMetrics& metrics);
    
    std::vector<ComponentState> getStateHistory(const std::string& component = "") const;
    PerformanceMetrics getMetrics(const std::string& component) const;
    
    void clear();
    void exportToJson(const std::string& filename) const;
    void exportToCSV(const std::string& filename) const;
};

// 性能计时器
class ScopedTimer {
private:
    std::string component_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    explicit ScopedTimer(const std::string& component_name);
    ~ScopedTimer();
};

// 便利宏
#define BT_DEBUG_TIMER(component) \
    ScopedTimer timer(component)

#define BT_DEBUG_STATE(component, ...) \
    do { \
        if (DebugCollector::getInstance().isEnabled()) { \
            ComponentState state; \
            state.component_name = component; \
            state.timestamp = std::chrono::system_clock::now(); \
            __VA_ARGS__ \
            DebugCollector::getInstance().recordState(state); \
        } \
    } while(0)

} // namespace debug
} // namespace backtrader
```

### 内存和资源监控

```cpp
// include/debug/ResourceMonitor.h
#pragma once
#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>

namespace backtrader {
namespace debug {

struct MemoryInfo {
    size_t total_allocated = 0;
    size_t total_freed = 0;
    size_t current_usage = 0;
    size_t peak_usage = 0;
    size_t allocation_count = 0;
    size_t deallocation_count = 0;
};

struct ResourceInfo {
    size_t file_handles = 0;
    size_t thread_count = 0;
    size_t socket_count = 0;
    double cpu_usage = 0.0;
};

class ResourceMonitor {
private:
    static std::unique_ptr<ResourceMonitor> instance_;
    std::atomic<size_t> current_memory_{0};
    std::atomic<size_t> peak_memory_{0};
    std::atomic<size_t> allocation_count_{0};
    std::unordered_map<void*, size_t> allocations_;
    mutable std::mutex allocations_mutex_;
    bool tracking_enabled_ = false;
    
public:
    static ResourceMonitor& getInstance();
    
    void enableTracking() { tracking_enabled_ = true; }
    void disableTracking() { tracking_enabled_ = false; }
    
    void recordAllocation(void* ptr, size_t size);
    void recordDeallocation(void* ptr);
    
    MemoryInfo getMemoryInfo() const;
    ResourceInfo getResourceInfo() const;
    
    void printMemoryReport() const;
    void detectMemoryLeaks() const;
};

// 自定义内存分配器（调试版本）
template<typename T>
class DebugAllocator {
public:
    using value_type = T;
    
    DebugAllocator() = default;
    
    template<typename U>
    DebugAllocator(const DebugAllocator<U>&) noexcept {}
    
    T* allocate(size_t n) {
        size_t size = n * sizeof(T);
        T* ptr = static_cast<T*>(std::malloc(size));
        
        if (!ptr) {
            throw std::bad_alloc();
        }
        
        ResourceMonitor::getInstance().recordAllocation(ptr, size);
        return ptr;
    }
    
    void deallocate(T* ptr, size_t) noexcept {
        ResourceMonitor::getInstance().recordDeallocation(ptr);
        std::free(ptr);
    }
    
    template<typename U>
    bool operator==(const DebugAllocator<U>&) const noexcept {
        return true;
    }
    
    template<typename U>
    bool operator!=(const DebugAllocator<U>&) const noexcept {
        return false;
    }
};

} // namespace debug
} // namespace backtrader
```

## 🐍 Python异常绑定

### Python异常映射

```cpp
// python/bindings/exception_bindings.cpp
#include <pybind11/pybind11.h>
#include "core/Exceptions.h"

namespace py = pybind11;

void bind_exceptions(py::module& m) {
    // 注册异常层次结构
    py::register_exception<backtrader::BacktraderException>(m, "BacktraderError");
    
    py::register_exception<backtrader::DataException>(m, "DataError", 
                          py::base<backtrader::BacktraderException>());
    py::register_exception<backtrader::DataFeedException>(m, "DataFeedError",
                          py::base<backtrader::DataException>());
    py::register_exception<backtrader::DataFormatException>(m, "DataFormatError",
                          py::base<backtrader::DataException>());
    py::register_exception<backtrader::DataRangeException>(m, "DataRangeError",
                          py::base<backtrader::DataException>());
    
    py::register_exception<backtrader::IndicatorException>(m, "IndicatorError",
                          py::base<backtrader::BacktraderException>());
    py::register_exception<backtrader::IndicatorPeriodException>(m, "IndicatorPeriodError",
                          py::base<backtrader::IndicatorException>());
    py::register_exception<backtrader::IndicatorCalculationException>(m, "IndicatorCalculationError",
                          py::base<backtrader::IndicatorException>());
    
    py::register_exception<backtrader::TradingException>(m, "TradingError",
                          py::base<backtrader::BacktraderException>());
    py::register_exception<backtrader::OrderException>(m, "OrderError",
                          py::base<backtrader::TradingException>());
    py::register_exception<backtrader::BrokerException>(m, "BrokerError",
                          py::base<backtrader::TradingException>());
    py::register_exception<backtrader::InsufficientFundsException>(m, "InsufficientFundsError",
                          py::base<backtrader::BrokerException>());
    
    py::register_exception<backtrader::StrategyException>(m, "StrategyError",
                          py::base<backtrader::BacktraderException>());
    py::register_exception<backtrader::StrategyInitException>(m, "StrategyInitError",
                          py::base<backtrader::StrategyException>());
    py::register_exception<backtrader::StrategyExecutionException>(m, "StrategyExecutionError",
                          py::base<backtrader::StrategyException>());
    
    py::register_exception<backtrader::EngineException>(m, "EngineError",
                          py::base<backtrader::BacktraderException>());
    py::register_exception<backtrader::CerebroException>(m, "CerebroError",
                          py::base<backtrader::EngineException>());
    
    py::register_exception<backtrader::ConfigurationException>(m, "ConfigurationError",
                          py::base<backtrader::BacktraderException>());
    py::register_exception<backtrader::ParameterException>(m, "ParameterError",
                          py::base<backtrader::ConfigurationException>());
}
```

### Python调试接口

```python
# python/backtrader_cpp/debug.py
"""
Python调试接口
"""
import backtrader_cpp._core as _core
import json
from typing import Dict, List, Optional, Any

class DebugManager:
    """调试管理器"""
    
    def __init__(self):
        self._collector = _core.debug.DebugCollector.getInstance()
        self._resource_monitor = _core.debug.ResourceMonitor.getInstance()
    
    def enable_debug(self):
        """启用调试模式"""
        self._collector.enable()
        self._resource_monitor.enableTracking()
    
    def disable_debug(self):
        """禁用调试模式"""
        self._collector.disable()
        self._resource_monitor.disableTracking()
    
    def get_component_state(self, component: Optional[str] = None) -> List[Dict]:
        """获取组件状态历史"""
        states = self._collector.getStateHistory(component or "")
        return [self._convert_state(state) for state in states]
    
    def get_performance_metrics(self, component: str) -> Dict[str, Any]:
        """获取性能指标"""
        metrics = self._collector.getMetrics(component)
        return {
            'execution_time_ns': metrics.execution_time.count(),
            'memory_usage': metrics.memory_usage,
            'function_calls': metrics.function_calls,
            'cache_hits': metrics.cache_hits,
            'cache_misses': metrics.cache_misses,
            'cache_hit_rate': metrics.cache_hits / (metrics.cache_hits + metrics.cache_misses) 
                              if (metrics.cache_hits + metrics.cache_misses) > 0 else 0.0
        }
    
    def get_memory_info(self) -> Dict[str, int]:
        """获取内存信息"""
        info = self._resource_monitor.getMemoryInfo()
        return {
            'total_allocated': info.total_allocated,
            'total_freed': info.total_freed,
            'current_usage': info.current_usage,
            'peak_usage': info.peak_usage,
            'allocation_count': info.allocation_count,
            'deallocation_count': info.deallocation_count
        }
    
    def export_debug_data(self, filename: str, format: str = 'json'):
        """导出调试数据"""
        if format.lower() == 'json':
            self._collector.exportToJson(filename)
        elif format.lower() == 'csv':
            self._collector.exportToCSV(filename)
        else:
            raise ValueError(f"Unsupported format: {format}")
    
    def check_memory_leaks(self):
        """检查内存泄漏"""
        self._resource_monitor.detectMemoryLeaks()
    
    def _convert_state(self, state) -> Dict:
        """转换状态对象为字典"""
        return {
            'component_name': state.component_name,
            'properties': dict(state.properties),
            'timestamp': state.timestamp.isoformat(),
            'metrics': {
                'execution_time_ns': state.metrics.execution_time.count(),
                'memory_usage': state.metrics.memory_usage,
                'function_calls': state.metrics.function_calls,
                'cache_hits': state.metrics.cache_hits,
                'cache_misses': state.metrics.cache_misses
            }
        }

# 全局调试管理器实例
debug_manager = DebugManager()

# 便利函数
def enable_debug():
    """启用调试模式"""
    debug_manager.enable_debug()

def disable_debug():
    """禁用调试模式"""
    debug_manager.disable_debug()

def get_debug_info(component: Optional[str] = None) -> Dict:
    """获取调试信息"""
    return {
        'component_states': debug_manager.get_component_state(component),
        'memory_info': debug_manager.get_memory_info()
    }
```

## 🔧 实际应用示例

### 在指标中使用错误处理

```cpp
// src/indicators/SMA.cpp
#include "indicators/SMA.h"
#include "core/ErrorHandler.h"

namespace backtrader {

SMA::SMA(std::shared_ptr<LineRoot> data, size_t period) 
    : IndicatorBase(data), period_(period) {
    
    // 参数验证
    ErrorHandler::getInstance().checkCondition<IndicatorPeriodException>(
        period > 0,
        "SMA period must be greater than 0",
        "SMA constructor"
    );
    
    ErrorHandler::getInstance().checkCondition<IndicatorPeriodException>(
        period <= 1000,
        "SMA period too large (>1000), might cause performance issues",
        "SMA constructor"
    );
    
    BT_DEBUG_STATE("SMA",
        state.properties["period"] = std::to_string(period_);
        state.properties["data_source"] = data ? "valid" : "null";
    );
}

void SMA::calculate() {
    BT_DEBUG_TIMER("SMA::calculate");
    
    try {
        if (!hasValidInput()) {
            ErrorHandler::getInstance().handleError<IndicatorCalculationException>(
                "SMA calculation failed: invalid input data",
                "SMA::calculate"
            );
            return;
        }
        
        // 执行计算...
        double sum = 0.0;
        for (size_t i = 0; i < period_; ++i) {
            double value = getInput(0)->get(-static_cast<int>(i));
            
            if (std::isnan(value)) {
                ErrorHandler::getInstance().handleError<DataException>(
                    "NaN value encountered in SMA calculation",
                    "SMA::calculate at index " + std::to_string(i)
                );
                return;
            }
            
            sum += value;
        }
        
        double result = sum / period_;
        setOutput(0, result);
        
        BT_DEBUG_STATE("SMA",
            state.properties["last_value"] = std::to_string(result);
            state.properties["input_sum"] = std::to_string(sum);
        );
        
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError<IndicatorCalculationException>(
            "Unexpected error in SMA calculation: " + std::string(e.what()),
            "SMA::calculate"
        );
    }
}

} // namespace backtrader
```

### 策略中的错误处理

```cpp
// src/strategy/StrategyBase.cpp
#include "strategy/StrategyBase.h"
#include "core/ErrorHandler.h"

namespace backtrader {

void StrategyBase::next() {
    BT_DEBUG_TIMER("Strategy::next");
    
    try {
        // 检查数据有效性
        if (!validateMarketData()) {
            BT_LOG_WARNING(logger_, "Invalid market data, skipping strategy execution");
            return;
        }
        
        // 执行用户策略逻辑
        executeUserStrategy();
        
        // 处理订单
        processOrders();
        
        BT_DEBUG_STATE("Strategy",
            state.properties["position_size"] = std::to_string(getPosition().getSize());
            state.properties["cash"] = std::to_string(getBroker().getCash());
            state.properties["orders_count"] = std::to_string(pending_orders_.size());
        );
        
    } catch (const StrategyException& e) {
        BT_LOG_ERROR(logger_, "Strategy error: {}", e.getFormattedMessage());
        // 策略错误不应该中断回测，记录并继续
        
    } catch (const BrokerException& e) {
        BT_LOG_ERROR(logger_, "Broker error: {}", e.getFormattedMessage());
        // 经纪商错误可能需要特殊处理
        handleBrokerError(e);
        
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().handleError<StrategyExecutionException>(
            "Unexpected error in strategy execution: " + std::string(e.what()),
            "StrategyBase::next"
        );
    }
}

bool StrategyBase::validateMarketData() const {
    if (!data_feeds_.empty()) {
        for (const auto& feed : data_feeds_) {
            if (!feed->isValid()) {
                return false;
            }
            
            double close = feed->close()[0];
            if (std::isnan(close) || close <= 0.0) {
                BT_LOG_WARNING(logger_, "Invalid close price: {}", close);
                return false;
            }
        }
    }
    return true;
}

} // namespace backtrader
```

### Python层的错误处理

```python
# 使用示例
import backtrader_cpp as btcpp
from backtrader_cpp.debug import enable_debug, get_debug_info

try:
    # 启用调试模式
    enable_debug()
    
    # 创建策略
    cerebro = btcpp.Cerebro()
    
    # 配置错误处理策略
    error_handler = btcpp.ErrorHandler.getInstance()
    
    # 对于数据错误，只记录日志不抛异常
    data_policy = btcpp.ErrorPolicy()
    data_policy.action = btcpp.ErrorAction.LOG
    data_policy.log_level = btcpp.ErrorLevel.WARNING
    error_handler.setErrorPolicy(1000, data_policy)  # DataException
    
    # 添加数据
    data = btcpp.feeds.CSVDataFeed(dataname='invalid_data.csv')
    cerebro.adddata(data)
    
    # 运行回测
    results = cerebro.run()
    
except btcpp.DataError as e:
    print(f"Data error: {e}")
    print(f"Error code: {e.getErrorCode()}")
    print(f"Context: {e.getContext()}")
    
except btcpp.StrategyError as e:
    print(f"Strategy error: {e}")
    
except btcpp.BacktraderError as e:
    print(f"General backtrader error: {e}")
    
finally:
    # 获取调试信息
    debug_info = get_debug_info()
    print("Memory usage:", debug_info['memory_info'])
    
    # 导出调试数据
    btcpp.debug.debug_manager.export_debug_data("debug_report.json")
```

## 📊 配置和部署

### 错误处理配置文件

```json
{
  "error_handling": {
    "default_policy": {
      "action": "THROW",
      "log_level": "ERROR",
      "collect_stack_trace": true,
      "notify_handlers": true
    },
    "specific_policies": {
      "1000": {
        "action": "LOG",
        "log_level": "WARNING",
        "collect_stack_trace": false,
        "notify_handlers": false
      },
      "2001": {
        "action": "WARN",
        "log_level": "WARNING",
        "collect_stack_trace": true,
        "notify_handlers": true
      }
    }
  },
  "logging": {
    "min_level": "INFO",
    "sinks": [
      {
        "type": "console",
        "level": "INFO"
      },
      {
        "type": "file",
        "level": "DEBUG",
        "filename": "backtrader.log"
      }
    ]
  },
  "debug": {
    "enabled": false,
    "memory_tracking": false,
    "performance_tracking": true,
    "export_format": "json"
  }
}
```

这个统一的错误处理和调试系统设计提供了：

1. **完整的异常体系**: 分层的异常类型覆盖所有组件
2. **灵活的错误策略**: 可配置的错误处理行为
3. **强大的调试工具**: 性能监控、状态跟踪、内存分析
4. **Python兼容性**: 无缝的Python异常映射
5. **生产环境支持**: 高性能的异步日志和资源监控

这确保了C++重构版本在保持高性能的同时，提供了优秀的开发和调试体验。

<function_calls>
<invoke name="TodoWrite">
<parameter name="todos">[{"content": "\u8bbe\u8ba1\u5b8c\u6574\u7684CMake\u6784\u5efa\u7cfb\u7edf\u548cCI/CD\u6d41\u6c34\u7ebf", "status": "completed", "priority": "high", "id": "create_build_system"}, {"content": "\u6df1\u5165\u5206\u6790Python\u7248\u672c\u7684\u6027\u80fd\u74f6\u9888\u548c\u4f18\u5316\u673a\u4f1a", "status": "completed", "priority": "high", "id": "analyze_performance_bottlenecks"}, {"content": "\u8bbe\u8ba1Python\u7ed1\u5b9a\u5c42\u4fdd\u8bc1\u5411\u540e\u517c\u5bb9", "status": "completed", "priority": "medium", "id": "design_python_bindings"}, {"content": "\u5efa\u7acb\u6027\u80fd\u57fa\u51c6\u6d4b\u8bd5\u6846\u67b6", "status": "completed", "priority": "high", "id": "create_benchmarking_framework"}, {"content": "\u8bbe\u8ba1\u7edf\u4e00\u7684\u9519\u8bef\u5904\u7406\u548c\u8c03\u8bd5\u7cfb\u7edf", "status": "completed", "priority": "medium", "id": "design_error_handling"}]