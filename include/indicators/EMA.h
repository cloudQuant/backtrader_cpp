#pragma once

#include "IndicatorBase.h"
#include <cmath>

namespace backtrader {

/**
 * @brief 指数移动平均线（Exponential Moving Average）
 * 
 * 使用指数平滑方法，给予近期数据更高权重
 * EMA(t) = α * Price(t) + (1-α) * EMA(t-1)
 * 其中 α = 2 / (period + 1)
 */
class EMA : public IndicatorBase {
private:
    size_t period_;
    double alpha_;           // 平滑因子
    double previous_ema_;    // 前一个EMA值
    bool has_previous_;      // 是否有前一个值
    
public:
    /**
     * @brief 构造函数
     * @param input 输入数据线
     * @param period 计算周期
     */
    explicit EMA(std::shared_ptr<LineRoot> input, size_t period = 30)
        : IndicatorBase(input, "EMA"), 
          period_(period), 
          previous_ema_(0.0), 
          has_previous_(false) {
        
        if (period == 0) {
            throw std::invalid_argument("EMA period must be greater than 0");
        }
        
        // 计算平滑因子
        alpha_ = 2.0 / (period + 1);
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        setParam("alpha", alpha_);
        
        // EMA的最小周期通常设为1，但为了稳定性可以设为period
        setMinPeriod(1);  // 可以根据需要调整
    }
    
    /**
     * @brief 获取周期
     * @return 计算周期
     */
    size_t getPeriod() const { return period_; }
    
    /**
     * @brief 获取平滑因子
     * @return 平滑因子α
     */
    double getAlpha() const { return alpha_; }
    
    /**
     * @brief 设置周期
     * @param period 新周期
     */
    void setPeriod(size_t period) {
        if (period == 0) {
            throw std::invalid_argument("EMA period must be greater than 0");
        }
        
        period_ = period;
        alpha_ = 2.0 / (period + 1);
        
        setParam("period", static_cast<double>(period));
        setParam("alpha", alpha_);
        
        // 重置状态
        reset();
    }
    
    /**
     * @brief 设置自定义平滑因子
     * @param alpha 平滑因子 (0 < alpha <= 1)
     */
    void setAlpha(double alpha) {
        if (alpha <= 0.0 || alpha > 1.0) {
            throw std::invalid_argument("EMA alpha must be in range (0, 1]");
        }
        
        alpha_ = alpha;
        setParam("alpha", alpha_);
        
        // 反推周期（仅用于显示）
        period_ = static_cast<size_t>((2.0 / alpha_) - 1);
        setParam("period", static_cast<double>(period_));
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        IndicatorBase::reset();
        previous_ema_ = 0.0;
        has_previous_ = false;
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput()) {
            setOutput(0, NaN);
            return;
        }
        
        auto input = getInput(0);
        double current_value = input->get(0);
        
        if (isNaN(current_value)) {
            setOutput(0, NaN);
            return;
        }
        
        double ema_value;
        
        if (!has_previous_) {
            // 第一个值：使用输入值作为初始EMA
            ema_value = current_value;
            has_previous_ = true;
        } else {
            // 标准EMA计算
            ema_value = alpha_ * current_value + (1.0 - alpha_) * previous_ema_;
        }
        
        previous_ema_ = ema_value;
        setOutput(0, ema_value);
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
        
        for (size_t i = start; i < end; ++i) {
            double current_value = input->get(0);
            
            if (isNaN(current_value)) {
                setOutput(0, NaN);
            } else {
                double ema_value;
                
                if (!has_previous_) {
                    ema_value = current_value;
                    has_previous_ = true;
                } else {
                    ema_value = alpha_ * current_value + (1.0 - alpha_) * previous_ema_;
                }
                
                previous_ema_ = ema_value;
                setOutput(0, ema_value);
            }
            
            // 移动到下一个位置
            input->forward();
        }
    }
    
    /**
     * @brief 获取前一个EMA值
     * @return 前一个EMA值
     */
    double getPreviousEMA() const {
        return has_previous_ ? previous_ema_ : NaN;
    }
    
    /**
     * @brief 检查是否有前一个值
     * @return true if has previous value
     */
    bool hasPreviousValue() const {
        return has_previous_;
    }
    
    /**
     * @brief 手动设置初始EMA值
     * @param initial_value 初始值
     * 
     * 这在某些情况下很有用，比如从特定的SMA值开始计算EMA
     */
    void setInitialValue(double initial_value) {
        if (isNaN(initial_value)) {
            throw std::invalid_argument("Initial EMA value cannot be NaN");
        }
        
        previous_ema_ = initial_value;
        has_previous_ = true;
    }
    
    /**
     * @brief 计算理论稳定性
     * @return 到达稳定状态所需的大概周期数
     * 
     * 通常认为经过3-4倍周期后EMA会相对稳定
     */
    size_t getStabilizationPeriod() const {
        return period_ * 3;  // 经验值
    }
    
    /**
     * @brief 计算当前权重分布
     * @param lookback 回溯期数
     * @return 权重向量
     * 
     * 计算当前值之前lookback个值的权重分布
     */
    std::vector<double> getWeights(size_t lookback) const {
        std::vector<double> weights(lookback);
        
        if (lookback == 0) {
            return weights;
        }
        
        // EMA中第i个历史值的权重为: α * (1-α)^i
        weights[0] = alpha_;  // 当前值权重
        
        for (size_t i = 1; i < lookback; ++i) {
            weights[i] = alpha_ * std::pow(1.0 - alpha_, i);
        }
        
        return weights;
    }
    
    /**
     * @brief 计算有效权重总和
     * @param lookback 回溯期数
     * @return 权重总和（应该接近1.0）
     */
    double getTotalWeight(size_t lookback) const {
        if (lookback == 0) {
            return 0.0;
        }
        
        // 几何级数求和：α * (1 - (1-α)^n) / (1 - (1-α)) = 1 - (1-α)^n
        return 1.0 - std::pow(1.0 - alpha_, lookback);
    }
};

} // namespace backtrader