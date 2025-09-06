# Backtrader C++ 关键组件实现示例

基于深度代码分析，本文档提供关键组件的详细C++实现示例，展示如何将Python的动态特性转换为高效的C++代码。

## 🏗️ 1. 元类系统模拟

### 1.1 生命周期管理器

```cpp
#pragma once
#include <memory>
#include <type_traits>
#include <any>
#include <unordered_map>
#include <string>

// 模拟Python的元类生命周期
template<typename Derived>
class MetaBase {
public:
    virtual ~MetaBase() = default;
    
    // 五阶段生命周期钩子
    virtual void doPreNew() {}
    virtual void doNew() {}
    virtual void doPreInit() {}
    virtual void doInit() {}
    virtual void doPostInit() {}
    
    // 工厂方法
    template<typename... Args>
    static std::unique_ptr<Derived> create(Args&&... args) {
        auto obj = std::make_unique<Derived>();
        
        obj->doPreNew();
        obj->doNew();
        obj->doPreInit();
        obj->doInit();
        obj->doPostInit();
        
        return obj;
    }
};

// 参数系统
class ParameterRegistry {
private:
    std::unordered_map<std::string, std::any> params_;
    
public:
    template<typename T>
    void set(const std::string& name, T&& value) {
        params_[name] = std::forward<T>(value);
    }
    
    template<typename T>
    T get(const std::string& name) const {
        try {
            return std::any_cast<T>(params_.at(name));
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Parameter type mismatch: " + name);
        }
    }
    
    template<typename T>
    T get(const std::string& name, T default_value) const {
        auto it = params_.find(name);
        if (it == params_.end()) {
            return default_value;
        }
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast& e) {
            return default_value;
        }
    }
    
    bool has(const std::string& name) const {
        return params_.find(name) != params_.end();
    }
    
    size_t hash() const {
        size_t seed = 0;
        std::hash<std::string> hasher;
        for (const auto& [key, value] : params_) {
            seed ^= hasher(key) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

// 模拟MetaParams
template<typename Derived>
class MetaParams : public MetaBase<Derived> {
protected:
    ParameterRegistry params_;
    
public:
    const ParameterRegistry& params() const { return params_; }
    ParameterRegistry& params() { return params_; }
    
    template<typename T>
    void setParam(const std::string& name, T&& value) {
        params_.set(name, std::forward<T>(value));
    }
    
    template<typename T>
    T getParam(const std::string& name) const {
        return params_.get<T>(name);
    }
    
    template<typename T>
    T getParam(const std::string& name, T default_value) const {
        return params_.get<T>(name, default_value);
    }
};
```

### 1.2 动态类创建模拟

```cpp
#pragma once
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>

// 工厂基类
template<typename Base>
class Factory {
public:
    using CreateFunc = std::function<std::unique_ptr<Base>()>;
    
private:
    std::unordered_map<std::string, CreateFunc> creators_;
    
public:
    template<typename Derived>
    void registerType(const std::string& name) {
        static_assert(std::is_base_of_v<Base, Derived>, 
                     "Derived must inherit from Base");
        
        creators_[name] = []() -> std::unique_ptr<Base> {
            return std::make_unique<Derived>();
        };
    }
    
    std::unique_ptr<Base> create(const std::string& name) const {
        auto it = creators_.find(name);
        if (it == creators_.end()) {
            throw std::runtime_error("Unknown type: " + name);
        }
        return it->second();
    }
    
    bool hasType(const std::string& name) const {
        return creators_.find(name) != creators_.end();
    }
    
    std::vector<std::string> getRegisteredTypes() const {
        std::vector<std::string> types;
        for (const auto& [name, func] : creators_) {
            types.push_back(name);
        }
        return types;
    }
};

// 自动注册宏
#define REGISTER_TYPE(factory, name, type) \
    static auto _register_##name = []() { \
        factory.registerType<type>(#name); \
        return true; \
    }();
```

## 📊 2. 数据线系统实现

### 2.1 环形缓冲区

```cpp
#pragma once
#include <vector>
#include <deque>
#include <limits>
#include <cmath>
#include <stdexcept>

template<typename T>
class CircularBuffer {
public:
    enum class Mode {
        UnBounded,  // 无限增长模式
        QBuffer     // 固定大小队列模式
    };
    
private:
    std::vector<T> data_;
    std::deque<T> queue_data_;  // QBuffer模式使用
    int idx_ = -1;              // 当前索引
    size_t capacity_;
    Mode mode_ = Mode::UnBounded;
    size_t extension_ = 0;
    
public:
    explicit CircularBuffer(size_t initial_capacity = 1000) 
        : capacity_(initial_capacity) {
        data_.reserve(initial_capacity);
    }
    
    // 支持负索引访问（Python风格）
    T& operator[](int ago) {
        if (mode_ == Mode::UnBounded) {
            int actual_idx = idx_ + ago;
            if (actual_idx < 0 || actual_idx >= static_cast<int>(data_.size())) {
                throw std::out_of_range("Index out of range");
            }
            return data_[actual_idx];
        } else {
            // QBuffer模式
            size_t queue_idx = queue_data_.size() - 1 - ago;
            if (queue_idx >= queue_data_.size()) {
                throw std::out_of_range("Index out of range");
            }
            return queue_data_[queue_idx];
        }
    }
    
    const T& operator[](int ago) const {
        return const_cast<CircularBuffer*>(this)->operator[](ago);
    }
    
    // 向前移动
    void forward(T value = std::numeric_limits<T>::quiet_NaN(), size_t size = 1) {
        for (size_t i = 0; i < size; ++i) {
            if (mode_ == Mode::UnBounded) {
                data_.push_back(value);
                idx_++;
            } else {
                queue_data_.push_back(value);
                idx_++;
            }
        }
    }
    
    // 向后移动
    void backward(size_t size = 1) {
        if (size > static_cast<size_t>(idx_ + 1)) {
            throw std::out_of_range("Cannot backward beyond beginning");
        }
        
        for (size_t i = 0; i < size; ++i) {
            if (mode_ == Mode::UnBounded) {
                if (!data_.empty()) {
                    data_.pop_back();
                    idx_--;
                }
            } else {
                if (!queue_data_.empty()) {
                    queue_data_.pop_back();
                    idx_--;
                }
            }
        }
    }
    
    // 回到起始位置
    void home() {
        idx_ = -1;
    }
    
    // 扩展缓冲区
    void extend(T value = std::numeric_limits<T>::quiet_NaN(), size_t size = 0) {
        extension_ += size;
        for (size_t i = 0; i < size; ++i) {
            if (mode_ == Mode::UnBounded) {
                data_.push_back(value);
            } else {
                queue_data_.push_back(value);
            }
        }
    }
    
    // 设置缓冲模式
    void setMode(Mode mode, size_t max_len = 0) {
        mode_ = mode;
        if (mode == Mode::QBuffer && max_len > 0) {
            capacity_ = max_len;
            
            // 转换现有数据
            if (mode_ == Mode::UnBounded && !data_.empty()) {
                queue_data_.clear();
                size_t start_idx = data_.size() > capacity_ ? data_.size() - capacity_ : 0;
                for (size_t i = start_idx; i < data_.size(); ++i) {
                    queue_data_.push_back(data_[i]);
                }
                data_.clear();
            }
            
            // 设置最大长度
            while (queue_data_.size() > capacity_) {
                queue_data_.pop_front();
            }
        }
    }
    
    // 获取当前长度
    size_t len() const {
        return mode_ == Mode::UnBounded ? data_.size() : queue_data_.size();
    }
    
    // 获取缓冲区长度
    size_t buflen() const {
        return len() + extension_;
    }
    
    // 当前索引
    int getIdx() const { return idx_; }
    
    // 检查是否为空
    bool empty() const {
        return len() == 0;
    }
};

// 特化double类型，支持NaN
template<>
class CircularBuffer<double> {
    // ... 相同的实现，但默认值使用std::numeric_limits<double>::quiet_NaN()
};
```

### 2.2 LineRoot基类

```cpp
#pragma once
#include "CircularBuffer.h"
#include <memory>
#include <functional>

class LineRoot {
protected:
    CircularBuffer<double> buffer_;
    size_t min_period_ = 1;
    
    // 操作阶段：1=构建阶段，2=运行阶段
    int op_stage_ = 1;
    
public:
    LineRoot() = default;
    virtual ~LineRoot() = default;
    
    // 索引访问
    double operator[](int ago) const { 
        return buffer_[ago]; 
    }
    
    double operator()(int ago) const { 
        return buffer_[ago]; 
    }
    
    // 获取当前值
    double get(int ago = 0) const {
        return buffer_[ago];
    }
    
    // 设置值
    void set(double value, int ago = 0) {
        // 注意：这需要特殊处理，因为CircularBuffer是只追加的
        // 在实际实现中可能需要修改CircularBuffer的设计
    }
    
    // 运算符重载 - 返回延迟计算对象
    template<typename Other>
    auto operator+(const Other& other) const;
    
    template<typename Other>
    auto operator-(const Other& other) const;
    
    template<typename Other>
    auto operator*(const Other& other) const;
    
    template<typename Other>
    auto operator/(const Other& other) const;
    
    // 比较运算符
    template<typename Other>
    auto operator>(const Other& other) const;
    
    template<typename Other>
    auto operator<(const Other& other) const;
    
    template<typename Other>
    auto operator>=(const Other& other) const;
    
    template<typename Other>
    auto operator<=(const Other& other) const;
    
    // 长度相关
    virtual size_t len() const { return buffer_.len(); }
    size_t buflen() const { return buffer_.buflen(); }
    
    // 缓冲区操作
    void forward(double value = std::numeric_limits<double>::quiet_NaN(), size_t size = 1) {
        buffer_.forward(value, size);
    }
    
    void backward(size_t size = 1) {
        buffer_.backward(size);
    }
    
    void home() {
        buffer_.home();
    }
    
    void extend(double value = std::numeric_limits<double>::quiet_NaN(), size_t size = 0) {
        buffer_.extend(value, size);
    }
    
    // 最小周期管理
    size_t getMinPeriod() const { return min_period_; }
    void updateMinPeriod(size_t period) { 
        min_period_ = std::max(min_period_, period); 
    }
    
    // 操作阶段
    int getOpStage() const { return op_stage_; }
    void setOpStage(int stage) { op_stage_ = stage; }
};

// 延迟计算操作类
template<typename Left, typename Right, typename Op>
class LazyBinaryOp : public LineRoot {
private:
    Left left_;
    Right right_;
    Op operation_;
    bool computed_ = false;
    
public:
    LazyBinaryOp(const Left& left, const Right& right, Op op)
        : left_(left), right_(right), operation_(op) {}
    
    double operator[](int ago) const override {
        if constexpr (std::is_arithmetic_v<Left> && std::is_arithmetic_v<Right>) {
            return operation_(left_, right_);
        } else if constexpr (std::is_arithmetic_v<Left>) {
            return operation_(left_, right_[ago]);
        } else if constexpr (std::is_arithmetic_v<Right>) {
            return operation_(left_[ago], right_);
        } else {
            return operation_(left_[ago], right_[ago]);
        }
    }
    
    size_t len() const override {
        if constexpr (std::is_arithmetic_v<Left> && std::is_arithmetic_v<Right>) {
            return 1;
        } else if constexpr (std::is_arithmetic_v<Left>) {
            return right_.len();
        } else if constexpr (std::is_arithmetic_v<Right>) {
            return left_.len();
        } else {
            return std::min(left_.len(), right_.len());
        }
    }
};

// 运算符重载实现
template<typename Other>
auto LineRoot::operator+(const Other& other) const {
    return LazyBinaryOp(*this, other, std::plus<double>());
}

template<typename Other>
auto LineRoot::operator-(const Other& other) const {
    return LazyBinaryOp(*this, other, std::minus<double>());
}

template<typename Other>
auto LineRoot::operator*(const Other& other) const {
    return LazyBinaryOp(*this, other, std::multiplies<double>());
}

template<typename Other>
auto LineRoot::operator/(const Other& other) const {
    return LazyBinaryOp(*this, other, std::divides<double>());
}
```

### 2.3 LineIterator类

```cpp
#pragma once
#include "LineRoot.h"
#include <vector>
#include <memory>

class LineIterator : public LineRoot {
protected:
    std::vector<std::shared_ptr<LineRoot>> lines_;
    std::shared_ptr<LineIterator> clock_;
    size_t current_len_ = 0;
    
public:
    LineIterator() = default;
    virtual ~LineIterator() = default;
    
    // 生命周期方法
    virtual void prenext() {}
    virtual void nextstart() {}
    virtual void next() = 0;
    virtual void stop() {}
    
    // 批量处理方法
    virtual void preonce(size_t start, size_t end) {}
    virtual void oncestart(size_t start, size_t end) {}
    virtual void once(size_t start, size_t end) {}
    
    // 执行控制
    void _next() {
        size_t clock_len = clock_ ? clock_->len() : current_len_ + 1;
        
        if (clock_len > len()) {
            forward();
        }
        
        current_len_ = clock_len;
        
        if (current_len_ < min_period_) {
            prenext();
        } else if (current_len_ == min_period_) {
            nextstart();
        } else {
            next();
        }
    }
    
    void _once() {
        if (!clock_) return;
        
        size_t total_len = clock_->buflen();
        
        // 一次性扩展到总长度
        forward(std::numeric_limits<double>::quiet_NaN(), total_len);
        home();
        
        // 批量处理
        if (min_period_ > 1) {
            preonce(0, min_period_ - 1);
        }
        
        if (min_period_ <= total_len) {
            oncestart(min_period_ - 1, min_period_);
            if (min_period_ < total_len) {
                once(min_period_, total_len);
            }
        }
    }
    
    // 时钟设置
    void setClock(std::shared_ptr<LineIterator> clock) {
        clock_ = clock;
    }
    
    std::shared_ptr<LineIterator> getClock() const {
        return clock_;
    }
    
    // 添加输入线
    void addInput(std::shared_ptr<LineRoot> line) {
        lines_.push_back(line);
        
        // 更新最小周期
        if (auto li = std::dynamic_pointer_cast<LineIterator>(line)) {
            updateMinPeriod(li->getMinPeriod());
        }
    }
    
    // 获取输入线
    std::shared_ptr<LineRoot> getInput(size_t index) const {
        if (index >= lines_.size()) {
            throw std::out_of_range("Input index out of range");
        }
        return lines_[index];
    }
    
    size_t getInputCount() const {
        return lines_.size();
    }
};
```

## 📈 3. 指标系统实现

### 3.1 指标基类

```cpp
#pragma once
#include "LineIterator.h"
#include <unordered_map>
#include <string>

class IndicatorBase : public LineIterator {
protected:
    std::vector<CircularBuffer<double>> output_lines_;
    static std::unordered_map<size_t, std::shared_ptr<IndicatorBase>> cache_;
    
public:
    IndicatorBase() = default;
    virtual ~IndicatorBase() = default;
    
    // 添加输出线
    void addOutputLine() {
        output_lines_.emplace_back();
    }
    
    // 获取输出线
    CircularBuffer<double>& getLine(size_t index = 0) {
        if (index >= output_lines_.size()) {
            throw std::out_of_range("Output line index out of range");
        }
        return output_lines_[index];
    }
    
    const CircularBuffer<double>& getLine(size_t index = 0) const {
        if (index >= output_lines_.size()) {
            throw std::out_of_range("Output line index out of range");
        }
        return output_lines_[index];
    }
    
    // 重载[]运算符访问主线
    double operator[](int ago) const override {
        if (output_lines_.empty()) {
            throw std::runtime_error("No output lines available");
        }
        return output_lines_[0][ago];
    }
    
    // 获取输出线数量
    size_t getLineCount() const {
        return output_lines_.size();
    }
    
    // 向输出线写入数据
    void setOutput(size_t line_index, double value) {
        if (line_index >= output_lines_.size()) {
            throw std::out_of_range("Output line index out of range");
        }
        output_lines_[line_index].forward(value);
    }
    
    // 缓存键生成
    virtual size_t getCacheKey() const {
        std::hash<std::string> hasher;
        size_t seed = hasher(typeid(*this).name());
        
        // 输入数据哈希
        for (const auto& input : lines_) {
            seed ^= reinterpret_cast<size_t>(input.get()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        
        // 参数哈希
        seed ^= params().hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        
        return seed;
    }
    
    // 工厂方法带缓存
    template<typename T, typename... Args>
    static std::shared_ptr<T> create(Args&&... args) {
        auto temp = std::make_shared<T>(std::forward<Args>(args)...);
        size_t key = temp->getCacheKey();
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (auto cached = std::dynamic_pointer_cast<T>(it->second.lock())) {
                return cached;
            }
        }
        
        cache_[key] = temp;
        return temp;
    }
    
    // 清理缓存
    static void clearCache() {
        cache_.clear();
    }
};

// 静态成员定义
template<typename T>
std::unordered_map<size_t, std::shared_ptr<IndicatorBase>> IndicatorBase::cache_;
```

### 3.2 SMA指标实现

```cpp
#pragma once
#include "IndicatorBase.h"

class SMA : public IndicatorBase {
private:
    size_t period_;
    double sum_ = 0.0;
    bool is_primed_ = false;
    
public:
    explicit SMA(std::shared_ptr<LineRoot> data, size_t period = 30) 
        : period_(period) {
        addInput(data);
        addOutputLine();
        updateMinPeriod(period);
        setParam("period", period);
    }
    
    void prenext() override {
        double current = getInput(0)->get(0);
        if (!std::isnan(current)) {
            sum_ += current;
        }
    }
    
    void nextstart() override {
        prenext();  // 累积最后一个值
        double avg = sum_ / period_;
        setOutput(0, avg);
        is_primed_ = true;
    }
    
    void next() override {
        double current = getInput(0)->get(0);
        double old = getInput(0)->get(-(static_cast<int>(period_)));
        
        if (!std::isnan(current)) {
            sum_ += current;
        }
        if (!std::isnan(old)) {
            sum_ -= old;
        }
        
        double avg = sum_ / period_;
        setOutput(0, avg);
    }
    
    void once(size_t start, size_t end) override {
        auto input = getInput(0);
        
        for (size_t i = start; i < end; ++i) {
            double sum = 0.0;
            size_t count = 0;
            
            // 计算当前位置的SMA
            for (size_t j = 0; j < period_ && j <= i; ++j) {
                double val = input->get(-(static_cast<int>(j)));
                if (!std::isnan(val)) {
                    sum += val;
                    count++;
                }
            }
            
            double avg = count > 0 ? sum / count : std::numeric_limits<double>::quiet_NaN();
            getLine(0).forward(avg);
        }
    }
    
    // 访问器
    size_t getPeriod() const { return period_; }
    double getSum() const { return sum_; }
};
```

### 3.3 EMA指标实现

```cpp
#pragma once
#include "IndicatorBase.h"

class EMA : public IndicatorBase {
private:
    size_t period_;
    double alpha_;
    double previous_ema_ = std::numeric_limits<double>::quiet_NaN();
    bool is_primed_ = false;
    
public:
    explicit EMA(std::shared_ptr<LineRoot> data, size_t period = 30) 
        : period_(period), alpha_(2.0 / (period + 1)) {
        addInput(data);
        addOutputLine();
        updateMinPeriod(1);  // EMA can start from first data point
        setParam("period", period);
        setParam("alpha", alpha_);
    }
    
    void nextstart() override {
        double current = getInput(0)->get(0);
        if (!std::isnan(current)) {
            previous_ema_ = current;
            setOutput(0, previous_ema_);
            is_primed_ = true;
        }
    }
    
    void next() override {
        double current = getInput(0)->get(0);
        
        if (!std::isnan(current)) {
            if (std::isnan(previous_ema_)) {
                previous_ema_ = current;
            } else {
                previous_ema_ = alpha_ * current + (1.0 - alpha_) * previous_ema_;
            }
            setOutput(0, previous_ema_);
        } else {
            setOutput(0, previous_ema_);
        }
    }
    
    void once(size_t start, size_t end) override {
        auto input = getInput(0);
        double ema = std::numeric_limits<double>::quiet_NaN();
        
        for (size_t i = start; i < end; ++i) {
            double current = input->get(-(static_cast<int>(i)));
            
            if (!std::isnan(current)) {
                if (std::isnan(ema)) {
                    ema = current;
                } else {
                    ema = alpha_ * current + (1.0 - alpha_) * ema;
                }
            }
            
            getLine(0).forward(ema);
        }
    }
    
    // 访问器
    size_t getPeriod() const { return period_; }
    double getAlpha() const { return alpha_; }
    double getCurrentEMA() const { return previous_ema_; }
};
```

### 3.4 复杂指标：布林带

```cpp
#pragma once
#include "IndicatorBase.h"
#include "SMA.h"
#include <cmath>

class StdDev : public IndicatorBase {
private:
    size_t period_;
    std::shared_ptr<SMA> sma_;
    
public:
    explicit StdDev(std::shared_ptr<LineRoot> data, size_t period = 30) 
        : period_(period) {
        addInput(data);
        addOutputLine();
        updateMinPeriod(period);
        setParam("period", period);
        
        // 内部SMA用于计算均值
        sma_ = std::make_shared<SMA>(data, period);
    }
    
    void next() override {
        sma_->_next();  // 更新SMA
        
        double mean = sma_->get(0);
        double sum_sq_diff = 0.0;
        size_t count = 0;
        
        for (size_t i = 0; i < period_; ++i) {
            double val = getInput(0)->get(-(static_cast<int>(i)));
            if (!std::isnan(val)) {
                double diff = val - mean;
                sum_sq_diff += diff * diff;
                count++;
            }
        }
        
        double variance = count > 1 ? sum_sq_diff / (count - 1) : 0.0;
        double stddev = std::sqrt(variance);
        setOutput(0, stddev);
    }
    
    void once(size_t start, size_t end) override {
        // 先计算SMA
        sma_->_once();
        
        auto input = getInput(0);
        
        for (size_t i = start; i < end; ++i) {
            double mean = sma_->get(-(static_cast<int>(i)));
            double sum_sq_diff = 0.0;
            size_t count = 0;
            
            for (size_t j = 0; j < period_ && j <= i; ++j) {
                double val = input->get(-(static_cast<int>(i - j)));
                if (!std::isnan(val) && !std::isnan(mean)) {
                    double diff = val - mean;
                    sum_sq_diff += diff * diff;
                    count++;
                }
            }
            
            double variance = count > 1 ? sum_sq_diff / (count - 1) : 0.0;
            double stddev = std::sqrt(variance);
            getLine(0).forward(stddev);
        }
    }
    
    std::shared_ptr<SMA> getSMA() const { return sma_; }
};

class BollingerBands : public IndicatorBase {
private:
    size_t period_;
    double dev_factor_;
    std::shared_ptr<SMA> sma_;
    std::shared_ptr<StdDev> stddev_;
    
public:
    explicit BollingerBands(std::shared_ptr<LineRoot> data, 
                           size_t period = 20, 
                           double devfactor = 2.0) 
        : period_(period), dev_factor_(devfactor) {
        addInput(data);
        
        // 三条输出线：中轨、上轨、下轨
        addOutputLine();  // 中轨
        addOutputLine();  // 上轨
        addOutputLine();  // 下轨
        
        updateMinPeriod(period);
        setParam("period", period);
        setParam("devfactor", devfactor);
        
        // 创建内部指标
        sma_ = std::make_shared<SMA>(data, period);
        stddev_ = std::make_shared<StdDev>(data, period);
    }
    
    void next() override {
        sma_->_next();
        stddev_->_next();
        
        double mid = sma_->get(0);
        double std = stddev_->get(0);
        double deviation = dev_factor_ * std;
        
        setOutput(0, mid);              // 中轨
        setOutput(1, mid + deviation);  // 上轨
        setOutput(2, mid - deviation);  // 下轨
    }
    
    void once(size_t start, size_t end) override {
        sma_->_once();
        stddev_->_once();
        
        for (size_t i = start; i < end; ++i) {
            double mid = sma_->get(-(static_cast<int>(i)));
            double std = stddev_->get(-(static_cast<int>(i)));
            double deviation = dev_factor_ * std;
            
            getLine(0).forward(mid);
            getLine(1).forward(mid + deviation);
            getLine(2).forward(mid - deviation);
        }
    }
    
    // 便捷访问方法
    const CircularBuffer<double>& mid() const { return getLine(0); }
    const CircularBuffer<double>& top() const { return getLine(1); }
    const CircularBuffer<double>& bot() const { return getLine(2); }
    
    std::shared_ptr<SMA> getSMA() const { return sma_; }
    std::shared_ptr<StdDev> getStdDev() const { return stddev_; }
};
```

## 🏭 4. 订单和Broker系统

### 4.1 订单系统

```cpp
#pragma once
#include <chrono>
#include <string>
#include <memory>

using DateTime = std::chrono::system_clock::time_point;
using OrderId = uint64_t;

enum class OrderType {
    Market,
    Limit,
    Stop,
    StopLimit
};

enum class OrderStatus {
    Created,
    Submitted,
    Accepted,
    Partial,
    Completed,
    Canceled,
    Expired,
    Margin,
    Rejected
};

enum class OrderSide {
    Buy,
    Sell
};

class Order {
private:
    OrderId id_;
    OrderType type_;
    OrderStatus status_;
    OrderSide side_;
    double size_;
    double price_;
    double stop_price_;  // 用于止损单
    double executed_size_ = 0.0;
    double executed_price_ = 0.0;
    DateTime created_time_;
    DateTime submitted_time_;
    DateTime executed_time_;
    std::string symbol_;
    
public:
    Order(OrderId id, OrderType type, OrderSide side, 
          double size, double price = 0.0, double stop_price = 0.0,
          const std::string& symbol = "")
        : id_(id), type_(type), status_(OrderStatus::Created),
          side_(side), size_(size), price_(price), stop_price_(stop_price),
          symbol_(symbol), created_time_(std::chrono::system_clock::now()) {}
    
    // 访问器
    OrderId getId() const { return id_; }
    OrderType getType() const { return type_; }
    OrderStatus getStatus() const { return status_; }
    OrderSide getSide() const { return side_; }
    double getSize() const { return size_; }
    double getPrice() const { return price_; }
    double getStopPrice() const { return stop_price_; }
    double getExecutedSize() const { return executed_size_; }
    double getExecutedPrice() const { return executed_price_; }
    double getRemainingSize() const { return size_ - executed_size_; }
    const std::string& getSymbol() const { return symbol_; }
    
    // 状态管理
    void setStatus(OrderStatus status) { 
        status_ = status;
        if (status == OrderStatus::Submitted) {
            submitted_time_ = std::chrono::system_clock::now();
        }
    }
    
    // 执行相关
    void addExecution(double size, double price) {
        executed_size_ += size;
        executed_price_ = ((executed_price_ * (executed_size_ - size)) + (price * size)) / executed_size_;
        executed_time_ = std::chrono::system_clock::now();
        
        if (executed_size_ >= size_) {
            status_ = OrderStatus::Completed;
        } else {
            status_ = OrderStatus::Partial;
        }
    }
    
    // 状态检查
    bool isActive() const {
        return status_ == OrderStatus::Submitted || 
               status_ == OrderStatus::Accepted || 
               status_ == OrderStatus::Partial;
    }
    
    bool isCompleted() const {
        return status_ == OrderStatus::Completed;
    }
    
    bool isCanceled() const {
        return status_ == OrderStatus::Canceled || 
               status_ == OrderStatus::Expired;
    }
    
    bool isFilled() const {
        return executed_size_ > 0;
    }
    
    // 用于止损单检查
    bool isTriggered(double current_price) const {
        if (type_ != OrderType::Stop && type_ != OrderType::StopLimit) {
            return true;  // 非止损单始终触发
        }
        
        if (side_ == OrderSide::Buy) {
            return current_price >= stop_price_;
        } else {
            return current_price <= stop_price_;
        }
    }
    
    // 用于限价单检查
    bool canExecuteAtPrice(double market_price) const {
        switch (type_) {
            case OrderType::Market:
                return true;
                
            case OrderType::Limit:
                if (side_ == OrderSide::Buy) {
                    return market_price <= price_;
                } else {
                    return market_price >= price_;
                }
                
            case OrderType::Stop:
                return isTriggered(market_price);
                
            case OrderType::StopLimit:
                if (!isTriggered(market_price)) {
                    return false;
                }
                // 触发后按限价单处理
                if (side_ == OrderSide::Buy) {
                    return market_price <= price_;
                } else {
                    return market_price >= price_;
                }
                
            default:
                return false;
        }
    }
};
```

### 4.2 持仓管理

```cpp
#pragma once
#include <string>
#include <chrono>

class Position {
private:
    std::string symbol_;
    double size_ = 0.0;          // 持仓大小（正数=多头，负数=空头）
    double price_ = 0.0;         // 平均开仓价格
    double unrealized_price_ = 0.0;  // 当前市价
    DateTime datetime_;
    
public:
    explicit Position(const std::string& symbol = "") 
        : symbol_(symbol), datetime_(std::chrono::system_clock::now()) {}
    
    // 更新持仓
    void update(double size, double price, 
                const std::chrono::system_clock::time_point& dt = std::chrono::system_clock::now()) {
        if (size_ == 0) {
            // 新开仓
            size_ = size;
            price_ = price;
        } else if ((size_ > 0 && size > 0) || (size_ < 0 && size < 0)) {
            // 加仓
            double total_value = size_ * price_ + size * price;
            size_ += size;
            price_ = total_value / size_;
        } else {
            // 减仓或反向
            double close_size = std::min(std::abs(size), std::abs(size_));
            size_ += size;  // 可能变为0或反向
            
            if (std::abs(size_) < 1e-8) {
                // 完全平仓
                size_ = 0.0;
                price_ = 0.0;
            } else if ((size_ > 0 && size > 0) || (size_ < 0 && size < 0)) {
                // 反向开仓
                price_ = price;
            }
            // 如果只是减仓，价格保持不变
        }
        
        datetime_ = dt;
    }
    
    // 更新市价
    void setUnrealizedPrice(double price) {
        unrealized_price_ = price;
    }
    
    // 计算未实现盈亏
    double getUnrealizedPnL() const {
        if (size_ == 0 || unrealized_price_ == 0) {
            return 0.0;
        }
        
        if (size_ > 0) {
            // 多头
            return size_ * (unrealized_price_ - price_);
        } else {
            // 空头
            return -size_ * (unrealized_price_ - price_);
        }
    }
    
    // 计算已实现盈亏（需要交易记录支持）
    double getRealizedPnL() const {
        // 这里需要维护交易历史记录
        return 0.0;  // 简化实现
    }
    
    // 状态检查
    bool isLong() const { return size_ > 0; }
    bool isShort() const { return size_ < 0; }
    bool isFlat() const { return std::abs(size_) < 1e-8; }
    
    // 访问器
    const std::string& getSymbol() const { return symbol_; }
    double getSize() const { return size_; }
    double getPrice() const { return price_; }
    double getUnrealizedPrice() const { return unrealized_price_; }
    const DateTime& getDateTime() const { return datetime_; }
    
    // 市值计算
    double getMarketValue() const {
        return size_ * unrealized_price_;
    }
    
    double getCostBasis() const {
        return size_ * price_;
    }
};
```

这些实现示例展示了如何将Python的动态特性转换为高效的C++代码，同时保持功能的完整性和性能的优越性。关键技术包括：

1. **模板元编程**：模拟Python的运行时灵活性
2. **RAII和智能指针**：自动内存管理
3. **延迟计算**：高效的运算符重载
4. **缓存机制**：避免重复计算
5. **类型安全**：编译期错误检查

这些设计为后续的完整系统实现奠定了坚实的基础。

<function_calls>
<invoke name="TodoWrite">
<parameter name="todos">[{"id": "analyze_specific_indicators", "content": "\u6df1\u5165\u5206\u6790\u590d\u6742\u6307\u6807\u7684\u5b9e\u73b0\u7ec6\u8282\uff08\u5982Ichimoku\u3001Stochastic\uff09", "status": "completed", "priority": "high"}, {"id": "study_cerebro_execution", "content": "\u5206\u6790Cerebro\u4e3b\u5f15\u64ce\u7684\u6267\u884c\u6d41\u7a0b\u548c\u4f18\u5316\u673a\u5236", "status": "completed", "priority": "high"}, {"id": "analyze_broker_matching", "content": "\u7814\u7a76Broker\u8ba2\u5355\u64ae\u5408\u5f15\u64ce\u7684\u5b9e\u73b0", "status": "completed", "priority": "high"}, {"id": "create_implementation_examples", "content": "\u521b\u5efa\u5173\u952e\u7ec4\u4ef6\u7684C++\u5b9e\u73b0\u793a\u4f8b", "status": "completed", "priority": "high"}, {"id": "design_migration_guide", "content": "\u8bbe\u8ba1Python\u5230C++\u7684\u8be6\u7ec6\u8fc1\u79fb\u6307\u5357", "status": "in_progress", "priority": "high"}]