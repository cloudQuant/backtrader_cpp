#pragma once

#include "IndicatorBase.h"
#include <deque>
#include <iostream>

namespace backtrader {

/**
 * @brief 简单移动平均线（Simple Moving Average）
 * 
 * 计算指定周期内的算术平均值
 * 使用增量计算方法提高性能
 */
class SMA : public IndicatorBase {
private:
    size_t period_;
    double sum_;
    std::deque<double> window_;  // 滑动窗口，用于增量计算
    bool use_incremental_;       // 是否使用增量计算
    
public:
    /**
     * @brief 构造函数
     * @param input 输入数据线
     * @param period 计算周期
     * @param use_incremental 是否使用增量计算（默认true）
     */
    explicit SMA(std::shared_ptr<LineRoot> input, 
                 size_t period = 30,
                 bool use_incremental = false)  // Use direct mode for higher precision
        : IndicatorBase(input, "SMA"), 
          period_(period), 
          sum_(0.0), 
          use_incremental_(use_incremental) {
        
        if (period == 0) {
            throw std::invalid_argument("SMA period must be greater than 0");
        }
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        
        // 设置最小周期
        setMinPeriod(period);
        
        // 注意：std::deque没有reserve方法，不需要预分配
    }
    
    /**
     * @brief 获取周期
     * @return 计算周期
     */
    size_t getPeriod() const { return period_; }
    
    /**
     * @brief 设置周期
     * @param period 新周期
     */
    void setPeriod(size_t period) {
        if (period == 0) {
            throw std::invalid_argument("SMA period must be greater than 0");
        }
        
        period_ = period;
        setParam("period", static_cast<double>(period));
        setMinPeriod(period);
        
        // 重置状态
        reset();
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        IndicatorBase::reset();
        sum_ = 0.0;
        window_.clear();
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput()) {
            // Debug
            static int debug_count = 0;
            if (debug_count < 5) {
                auto input = getInput(0);
                std::cout << "*** SMA calculate: no valid input, inputs_empty=" << (inputs_.empty() ? "true" : "false")
                         << ", input_null=" << (input ? "false" : "true")
                         << ", input_empty=" << (input ? (input->empty() ? "true" : "false") : "null")
                         << ", input_len=" << (input ? input->len() : -1)
                         << " (count=" << debug_count << ")" << std::endl;
                debug_count++;
            }
            setOutput(0, NaN);
            return;
        }
        
        auto input = getInput(0);
        double current_value = input->get(0);
        
        // Debug
        static int debug_count2 = 0;
        if (debug_count2 < 5) {
            std::cout << "*** SMA calculate: input len=" << input->len() << ", value=" << current_value << " (count=" << debug_count2 << ")" << std::endl;
            debug_count2++;
        }
        
        if (isNaN(current_value)) {
            setOutput(0, NaN);
            return;
        }
        
        if (use_incremental_) {
            calculateIncremental(current_value);
        } else {
            calculateDirect();
        }
    }
    
    /**
     * @brief 批量计算（向量化版本）
     * @param start 起始位置
     * @param end 结束位置
     */
    void calculateBatch(size_t start, size_t end) override {
        if (!hasValidInput()) {
            return;
        }
        
        auto input = getInput(0);
        
        // 批量计算模式通常使用直接计算
        for (size_t i = start; i < end; ++i) {
            if (input->len() >= period_) {
                double sum = 0.0;
                bool has_nan = false;
                
                // 计算当前窗口的和
                for (size_t j = 0; j < period_; ++j) {
                    double value = input->get(-static_cast<int>(j));
                    if (isNaN(value)) {
                        has_nan = true;
                        break;
                    }
                    sum += value;
                }
                
                if (has_nan) {
                    setOutput(0, NaN);
                } else {
                    setOutput(0, sum / period_);
                }
            } else {
                setOutput(0, NaN);
            }
            
            // 移动到下一个位置
            input->forward();
        }
    }
    
    /**
     * @brief 获取当前窗口的数据
     * @return 窗口数据
     */
    std::vector<double> getCurrentWindow() const {
        std::vector<double> result;
        if (!hasValidInput()) {
            return result;
        }
        
        auto input = getInput(0);
        result.reserve(period_);
        
        for (size_t i = 0; i < period_ && i < input->len(); ++i) {
            result.push_back(input->get(-static_cast<int>(i)));
        }
        
        return result;
    }
    
    /**
     * @brief 获取当前和值（仅在增量模式下有效）
     * @return 当前和值
     */
    double getCurrentSum() const {
        return use_incremental_ ? sum_ : NaN;
    }
    
    /**
     * @brief 检查是否使用增量计算
     * @return true if using incremental calculation
     */
    bool isUsingIncremental() const {
        return use_incremental_;
    }
    
private:
    /**
     * @brief 增量计算方法
     * @param current_value 当前值
     */
    void calculateIncremental(double current_value) {
        window_.push_back(current_value);
        sum_ += current_value;
        
        // 如果窗口超出周期，移除最老的值
        if (window_.size() > period_) {
            double old_value = window_.front();
            window_.pop_front();
            sum_ -= old_value;
        }
        
        // 只有当窗口满了才输出有效值
        if (window_.size() == period_) {
            setOutput(0, sum_ / period_);
        } else {
            setOutput(0, NaN);
        }
    }
    
    /**
     * @brief 直接计算方法 - 使用高精度求和算法匹配Python的math.fsum()
     */
    void calculateDirect() {
        auto input = getInput(0);
        
        if (input->len() < period_) {
            setOutput(0, NaN);
            return;
        }
        
        // 收集数据
        std::vector<double> values;
        values.reserve(period_);
        bool has_nan = false;
        
        for (size_t i = 0; i < period_; ++i) {
            double value = input->get(-static_cast<int>(i));
            if (isNaN(value)) {
                has_nan = true;
                break;
            }
            values.push_back(value);
        }
        
        if (has_nan) {
            setOutput(0, NaN);
        } else {
            // 使用高精度求和算法（接近Python的math.fsum）
            double sum = highPrecisionSum(values);
            setOutput(0, sum / period_);
        }
    }
    
    /**
     * @brief 高精度求和算法，模拟Python的math.fsum()行为
     * 使用改进的Kahan求和和部分补偿
     */
    double highPrecisionSum(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        if (values.size() == 1) return values[0];
        
        // 使用双重补偿的Kahan求和算法
        double sum = 0.0;
        double c = 0.0;  // 补偿值
        
        for (double value : values) {
            // 标准Kahan求和
            double y = value - c;
            double t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }
        
        return sum;
    }
};

} // namespace backtrader