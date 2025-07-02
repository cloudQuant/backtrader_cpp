#pragma once

#include "core/LineRoot.h"
#include "core/Common.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace backtrader {

/**
 * @brief 指标基类，模拟Python版本的IndicatorBase
 * 
 * 提供指标计算的基础框架，支持多输入、多输出
 */
class IndicatorBase : public LineRoot {
protected:
    std::vector<std::shared_ptr<LineRoot>> inputs_;    // 输入数据线
    std::vector<std::shared_ptr<LineRoot>> outputs_;   // 输出数据线
    std::unordered_map<std::string, double> params_;   // 参数
    bool initialized_;
    
public:
    /**
     * @brief 构造函数
     * @param input 主输入数据线
     * @param name 指标名称
     */
    explicit IndicatorBase(std::shared_ptr<LineRoot> input = nullptr, 
                          const std::string& name = "indicator")
        : LineRoot(1000, name), initialized_(false) {
        if (input) {
            inputs_.push_back(input);
        }
        
        // 默认创建一个输出线（自身）
        outputs_.push_back(std::shared_ptr<LineRoot>(this, [](LineRoot*){}));
    }
    
    virtual ~IndicatorBase() = default;
    
    // 输入输出管理
    
    /**
     * @brief 添加输入数据线
     * @param input 输入数据线
     */
    void addInput(std::shared_ptr<LineRoot> input) {
        inputs_.push_back(input);
    }
    
    /**
     * @brief 获取输入数据线
     * @param index 索引，默认0
     * @return 输入数据线指针
     */
    std::shared_ptr<LineRoot> getInput(size_t index = 0) const {
        if (index >= inputs_.size()) {
            return nullptr;
        }
        return inputs_[index];
    }
    
    /**
     * @brief 获取输入数据线数量
     * @return 输入数量
     */
    size_t getInputCount() const { return inputs_.size(); }
    
    /**
     * @brief 添加输出数据线
     * @param output 输出数据线
     */
    void addOutput(std::shared_ptr<LineRoot> output) {
        outputs_.push_back(output);
    }
    
    /**
     * @brief 获取输出数据线
     * @param index 索引，默认0
     * @return 输出数据线指针
     */
    std::shared_ptr<LineRoot> getOutput(size_t index = 0) const {
        if (index >= outputs_.size()) {
            return nullptr;
        }
        return outputs_[index];
    }
    
    /**
     * @brief 获取输出数据线数量
     * @return 输出数量
     */
    size_t getOutputCount() const { return outputs_.size(); }
    
    // 参数管理
    
    /**
     * @brief 设置参数
     * @param name 参数名
     * @param value 参数值
     */
    void setParam(const std::string& name, double value) {
        params_[name] = value;
    }
    
    /**
     * @brief 获取参数
     * @param name 参数名
     * @param default_value 默认值
     * @return 参数值
     */
    double getParam(const std::string& name, double default_value = 0.0) const {
        auto it = params_.find(name);
        return (it != params_.end()) ? it->second : default_value;
    }
    
    /**
     * @brief 检查参数是否存在
     * @param name 参数名
     * @return true if exists
     */
    bool hasParam(const std::string& name) const {
        return params_.find(name) != params_.end();
    }
    
    /**
     * @brief 获取所有参数
     * @return 参数映射
     */
    const std::unordered_map<std::string, double>& getParams() const {
        return params_;
    }
    
    // 计算接口
    
    /**
     * @brief 初始化指标
     * 子类可重写此方法进行初始化
     */
    virtual void initialize() {
        initialized_ = true;
    }
    
    /**
     * @brief 检查是否已初始化
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief 单步计算
     * 子类必须实现此方法
     */
    virtual void calculate() = 0;
    
    /**
     * @brief 批量计算（向量化）
     * 子类可重写此方法以提供更高效的批量计算
     * @param start 起始位置
     * @param end 结束位置
     */
    virtual void calculateBatch(size_t start, size_t end) {
        // 默认实现：逐个调用calculate
        for (size_t i = start; i < end; ++i) {
            calculate();
            if (!inputs_.empty()) {
                inputs_[0]->forward();
            }
        }
    }
    
    /**
     * @brief 重置指标状态
     */
    virtual void reset() {
        buffer_.home();
        for (auto& output : outputs_) {
            if (output.get() != this) {
                output->home();
            }
        }
        initialized_ = false;
    }
    
    /**
     * @brief 检查输入数据是否有效
     * @return true if valid
     */
    bool hasValidInput() const {
        if (inputs_.empty()) {
            return false;
        }
        
        for (const auto& input : inputs_) {
            if (!input || input->empty()) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief 获取当前所需的最小数据量
     * @return 最小数据量
     */
    virtual size_t getRequiredDataCount() const {
        return getMinPeriod();
    }
    
    /**
     * @brief 检查是否有足够的数据进行计算
     * @return true if enough data
     */
    bool hasEnoughData() const {
        if (!hasValidInput()) {
            return false;
        }
        
        size_t min_len = SIZE_MAX;
        for (const auto& input : inputs_) {
            min_len = std::min(min_len, input->len());
        }
        
        return min_len >= getRequiredDataCount();
    }
    
protected:
    /**
     * @brief 设置输出值
     * @param index 输出索引
     * @param value 值
     */
    void setOutput(size_t index, double value) {
        if (index == 0) {
            // 主输出就是自身
            this->forward(value);
        } else if (index < outputs_.size()) {
            outputs_[index]->forward(value);
        }
    }
    
    /**
     * @brief 获取输出值
     * @param index 输出索引
     * @param ago 偏移量
     * @return 输出值
     */
    double getOutputValue(size_t index, int ago = 0) const {
        if (index == 0) {
            return this->get(ago);
        } else if (index < outputs_.size()) {
            return outputs_[index]->get(ago);
        }
        return NaN;
    }
    
    /**
     * @brief 更新最小周期
     * @param period 新的最小周期
     */
    void updateMinPeriod(size_t period) {
        setMinPeriod(std::max(getMinPeriod(), period));
    }
};

/**
 * @brief 多线指标基类
 * 
 * 用于支持多个输出线的指标（如布林带）
 */
class MultiLineIndicator : public IndicatorBase {
protected:
    std::vector<std::string> line_names_;
    
public:
    /**
     * @brief 构造函数
     * @param input 输入数据线
     * @param line_names 输出线名称列表
     * @param name 指标名称
     */
    explicit MultiLineIndicator(std::shared_ptr<LineRoot> input,
                               const std::vector<std::string>& line_names,
                               const std::string& name = "multi_indicator")
        : IndicatorBase(input, name), line_names_(line_names) {
        
        // 清除默认的单一输出
        outputs_.clear();
        
        // 为每个输出线创建独立的LineRoot
        for (size_t i = 0; i < line_names_.size(); ++i) {
            if (i == 0) {
                // 第一个输出线使用自身
                outputs_.push_back(std::shared_ptr<LineRoot>(this, [](LineRoot*){}));
            } else {
                // 其他输出线创建新的LineRoot
                auto line = std::make_shared<LineRoot>(1000, line_names[i]);
                outputs_.push_back(line);
            }
        }
    }
    
    /**
     * @brief 获取输出线名称
     * @param index 索引
     * @return 线名称
     */
    const std::string& getLineName(size_t index) const {
        if (index < line_names_.size()) {
            return line_names_[index];
        }
        static const std::string empty;
        return empty;
    }
    
    /**
     * @brief 通过名称获取输出线
     * @param name 线名称
     * @return 输出线指针
     */
    std::shared_ptr<LineRoot> getLineByName(const std::string& name) const {
        auto it = std::find(line_names_.begin(), line_names_.end(), name);
        if (it != line_names_.end()) {
            size_t index = std::distance(line_names_.begin(), it);
            return getOutput(index);
        }
        return nullptr;
    }
    
    /**
     * @brief 获取所有线名称
     * @return 线名称列表
     */
    const std::vector<std::string>& getLineNames() const {
        return line_names_;
    }
};

} // namespace backtrader