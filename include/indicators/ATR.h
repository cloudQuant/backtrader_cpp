#pragma once

#include "IndicatorBase.h"
#include "EMA.h"
#include <algorithm>

namespace backtrader {

/**
 * @brief 平均真实波幅（Average True Range）
 * 
 * True Range (TR) = max(H-L, |H-PC|, |L-PC|)
 * ATR = EMA(TR, period) 或 SMA(TR, period)
 * 
 * 其中：
 * H = 当期最高价
 * L = 当期最低价
 * PC = 前期收盘价
 * 
 * ATR是衡量价格波动性的重要指标
 */
class ATR : public IndicatorBase {
private:
    size_t period_;
    bool use_ema_;              // 是否使用EMA平滑
    
    std::shared_ptr<LineRoot> high_line_;   // 最高价数据线
    std::shared_ptr<LineRoot> low_line_;    // 最低价数据线
    std::shared_ptr<LineRoot> close_line_;  // 收盘价数据线
    
    // EMA方法
    std::unique_ptr<EMA> tr_ema_;
    std::shared_ptr<LineRoot> tr_line_;
    
    // SMA方法（手动实现）
    std::deque<double> tr_buffer_;
    double tr_sum_;
    
    // 状态变量
    double prev_close_;
    bool has_prev_close_;
    
public:
    /**
     * @brief 构造函数
     * @param high_input 最高价数据线
     * @param low_input 最低价数据线
     * @param close_input 收盘价数据线
     * @param period 计算周期，默认14
     * @param use_ema 是否使用EMA平滑，默认true（Wilder's方法）
     */
    explicit ATR(std::shared_ptr<LineRoot> high_input,
                std::shared_ptr<LineRoot> low_input,
                std::shared_ptr<LineRoot> close_input,
                size_t period = 14,
                bool use_ema = true)
        : IndicatorBase(close_input, "ATR"),
          period_(period),
          use_ema_(use_ema),
          high_line_(high_input),
          low_line_(low_input),
          close_line_(close_input),
          tr_sum_(0.0),
          prev_close_(0.0),
          has_prev_close_(false) {
        
        if (period == 0) {
            throw std::invalid_argument("ATR period must be greater than 0");
        }
        
        if (!high_input || !low_input || !close_input) {
            throw std::invalid_argument("High, low, and close price lines are required for ATR");
        }
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        setParam("use_ema", use_ema ? 1.0 : 0.0);
        
        // ATR需要至少period+1个数据点
        setMinPeriod(period + 1);
        
        // 如果使用EMA方法，初始化EMA指标
        if (use_ema_) {
            tr_line_ = std::make_shared<LineRoot>(1000, "true_range");
            tr_ema_ = std::make_unique<EMA>(tr_line_, period);
        }
    }
    
    /**
     * @brief 获取周期
     * @return 计算周期
     */
    size_t getPeriod() const { return period_; }
    
    /**
     * @brief 是否使用EMA平滑
     * @return true if using EMA
     */
    bool isUsingEMA() const { return use_ema_; }
    
    /**
     * @brief 获取当前真实波幅
     * @return 当前TR值
     */
    double getCurrentTR() const {
        if (!has_prev_close_ || !high_line_ || !low_line_ || !close_line_) {
            return NaN;
        }
        
        double current_high = high_line_->get(0);
        double current_low = low_line_->get(0);
        
        if (isNaN(current_high) || isNaN(current_low) || isNaN(prev_close_)) {
            return NaN;
        }
        
        // 计算真实波幅的三个候选值
        double hl = current_high - current_low;
        double hc = std::abs(current_high - prev_close_);
        double lc = std::abs(current_low - prev_close_);
        
        return std::max({hl, hc, lc});
    }
    
    /**
     * @brief 设置周期
     * @param period 新周期
     */
    void setPeriod(size_t period) {
        if (period == 0) {
            throw std::invalid_argument("ATR period must be greater than 0");
        }
        
        period_ = period;
        setParam("period", static_cast<double>(period));
        setMinPeriod(period + 1);
        
        // 重置状态
        reset();
    }
    
    /**
     * @brief 计算ATR的相对值
     * @param reference_price 参考价格（通常为收盘价）
     * @return ATR相对值（百分比）
     */
    double getRelativeATR(double reference_price = 0.0) const {
        double atr_value = get(0);
        if (isNaN(atr_value)) {
            return NaN;
        }
        
        if (reference_price == 0.0) {
            reference_price = close_line_->get(0);
        }
        
        if (isNaN(reference_price) || reference_price == 0.0) {
            return NaN;
        }
        
        return (atr_value / reference_price) * 100.0;
    }
    
    /**
     * @brief 计算ATR通道
     * @param multiplier ATR倍数，默认2.0
     * @param price_base 基准价格，默认使用收盘价
     * @return {上轨, 下轨}
     */
    std::pair<double, double> getATRChannel(double multiplier = 2.0,
                                           double price_base = 0.0) const {
        double atr_value = get(0);
        if (isNaN(atr_value)) {
            return {NaN, NaN};
        }
        
        if (price_base == 0.0) {
            price_base = close_line_->get(0);
        }
        
        if (isNaN(price_base)) {
            return {NaN, NaN};
        }
        
        double atr_distance = atr_value * multiplier;
        return {price_base + atr_distance, price_base - atr_distance};
    }
    
    /**
     * @brief 计算ATR趋势强度
     * @param lookback 回看期数
     * @return 趋势强度 (正值=上升, 负值=下降)
     */
    double getTrendStrength(size_t lookback = 5) const {
        try {
            if (lookback == 0) lookback = 1;
            
            double current_atr = get(0);
            double past_atr = get(-static_cast<int>(lookback));
            
            if (isNaN(current_atr) || isNaN(past_atr) || past_atr == 0.0) {
                return 0.0;
            }
            
            // 返回ATR变化的百分比
            return (current_atr - past_atr) / past_atr;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        IndicatorBase::reset();
        
        tr_sum_ = 0.0;
        prev_close_ = 0.0;
        has_prev_close_ = false;
        tr_buffer_.clear();
        
        if (use_ema_) {
            if (tr_line_) tr_line_->home();
            if (tr_ema_) tr_ema_->reset();
        }
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput() || !high_line_ || !low_line_) {
            setOutput(0, NaN);
            return;
        }
        
        double current_high = high_line_->get(0);
        double current_low = low_line_->get(0);
        double current_close = close_line_->get(0);
        
        if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close)) {
            setOutput(0, NaN);
            return;
        }
        
        // 第一个数据点，无法计算ATR
        if (!has_prev_close_) {
            prev_close_ = current_close;
            has_prev_close_ = true;
            setOutput(0, NaN);
            return;
        }
        
        // 计算真实波幅
        double true_range = getCurrentTR();
        if (isNaN(true_range)) {
            setOutput(0, NaN);
            return;
        }
        
        if (use_ema_) {
            calculateWithEMA(true_range);
        } else {
            calculateWithSMA(true_range);
        }
        
        prev_close_ = current_close;
    }
    
    /**
     * @brief 批量计算（向量化版本）
     * @param start 起始位置
     * @param end 结束位置
     */
    void calculateBatch(size_t start, size_t end) override {
        if (!hasValidInput() || !high_line_ || !low_line_) {
            return;
        }
        
        for (size_t i = start; i < end; ++i) {
            calculate();
            
            if (i < end - 1) {
                // 前进所有输入数据线
                high_line_->forward();
                low_line_->forward();
                close_line_->forward();
            }
        }
    }
    
    /**
     * @brief 获取波动性等级
     * @return 波动性等级 (1=低, 2=中, 3=高)
     */
    int getVolatilityLevel() const {
        double relative_atr = getRelativeATR();
        if (isNaN(relative_atr)) {
            return 0;
        }
        
        // 根据相对ATR值分级
        if (relative_atr < 1.0) {
            return 1;  // 低波动
        } else if (relative_atr < 3.0) {
            return 2;  // 中等波动
        } else {
            return 3;  // 高波动
        }
    }
    
private:
    /**
     * @brief 使用EMA方法计算ATR
     * @param true_range 真实波幅
     */
    void calculateWithEMA(double true_range) {
        // 更新TR数据线并计算EMA
        tr_line_->forward(true_range);
        tr_ema_->calculate();
        
        double atr_value = tr_ema_->get(0);
        setOutput(0, atr_value);
    }
    
    /**
     * @brief 使用SMA方法计算ATR
     * @param true_range 真实波幅
     */
    void calculateWithSMA(double true_range) {
        tr_buffer_.push_back(true_range);
        tr_sum_ += true_range;
        
        if (tr_buffer_.size() > period_) {
            tr_sum_ -= tr_buffer_.front();
            tr_buffer_.pop_front();
        }
        
        if (tr_buffer_.size() < period_) {
            setOutput(0, NaN);
            return;
        }
        
        double atr_value = tr_sum_ / period_;
        setOutput(0, atr_value);
    }
};

} // namespace backtrader