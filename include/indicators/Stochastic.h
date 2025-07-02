#pragma once

#include "IndicatorBase.h"
#include "SMA.h"
#include <algorithm>

namespace backtrader {

/**
 * @brief 随机指标（Stochastic Oscillator）
 * 
 * %K = 100 * (C - L) / (H - L)
 * %D = SMA(%K, period)
 * 
 * 其中：
 * C = 当前收盘价
 * L = 指定周期内最低价
 * H = 指定周期内最高价
 * 
 * 这是一个多线指标，包含两条输出线：
 * - Line 0: %K线（快线）
 * - Line 1: %D线（慢线，%K的移动平均）
 */
class Stochastic : public MultiLineIndicator {
private:
    size_t k_period_;       // %K计算周期
    size_t d_period_;       // %D计算周期
    
    std::shared_ptr<LineRoot> high_line_;   // 最高价数据线
    std::shared_ptr<LineRoot> low_line_;    // 最低价数据线
    std::shared_ptr<LineRoot> close_line_;  // 收盘价数据线（输入）
    
    // %D计算用的SMA
    std::shared_ptr<SMA> d_sma_;
    std::shared_ptr<LineRoot> k_line_;  // %K数据线，用于计算%D
    
    // 缓存区用于存储高低价
    std::deque<double> high_buffer_;
    std::deque<double> low_buffer_;
    
public:
    /**
     * @brief 构造函数
     * @param close_input 收盘价数据线
     * @param high_input 最高价数据线
     * @param low_input 最低价数据线
     * @param k_period %K计算周期，默认14
     * @param d_period %D计算周期，默认3
     */
    explicit Stochastic(std::shared_ptr<LineRoot> close_input,
                       std::shared_ptr<LineRoot> high_input,
                       std::shared_ptr<LineRoot> low_input,
                       size_t k_period = 14,
                       size_t d_period = 3)
        : MultiLineIndicator(close_input, {"%K", "%D"}, "Stochastic"),
          k_period_(k_period),
          d_period_(d_period),
          high_line_(high_input),
          low_line_(low_input),
          close_line_(close_input) {
        
        if (k_period == 0 || d_period == 0) {
            throw std::invalid_argument("Stochastic periods must be greater than 0");
        }
        
        if (!high_input || !low_input) {
            throw std::invalid_argument("High and low price lines are required for Stochastic");
        }
        
        // 设置参数
        setParam("k_period", static_cast<double>(k_period));
        setParam("d_period", static_cast<double>(d_period));
        
        // 最小周期为%K周期 + %D周期
        setMinPeriod(k_period + d_period);
        
        // 创建%K数据线用于%D计算
        k_line_ = std::make_shared<LineRoot>(1000, "stoch_k");
        d_sma_ = std::make_shared<SMA>(k_line_, d_period);
    }
    
    /**
     * @brief 获取%K周期
     * @return %K计算周期
     */
    size_t getKPeriod() const { return k_period_; }
    
    /**
     * @brief 获取%D周期
     * @return %D计算周期
     */
    size_t getDPeriod() const { return d_period_; }
    
    /**
     * @brief 获取%K值
     * @param ago 偏移量，默认0
     * @return %K值
     */
    double getPercentK(int ago = 0) const {
        return getOutputValue(0, ago);
    }
    
    /**
     * @brief 获取%D值
     * @param ago 偏移量，默认0
     * @return %D值
     */
    double getPercentD(int ago = 0) const {
        return getOutputValue(1, ago);
    }
    
    /**
     * @brief 检测超买超卖状态
     * @param overbought_level 超买阈值，默认80
     * @param oversold_level 超卖阈值，默认20
     * @return 1.0=超买, -1.0=超卖, 0.0=中性
     */
    double getOverboughtOversoldStatus(double overbought_level = 80.0,
                                     double oversold_level = 20.0) const {
        double k_value = getPercentK(0);
        double d_value = getPercentD(0);
        
        if (isNaN(k_value) || isNaN(d_value)) {
            return 0.0;
        }
        
        // 同时考虑%K和%D的状态
        if (k_value >= overbought_level && d_value >= overbought_level) {
            return 1.0;  // 超买
        } else if (k_value <= oversold_level && d_value <= oversold_level) {
            return -1.0; // 超卖
        } else {
            return 0.0;  // 中性
        }
    }
    
    /**
     * @brief 检测金叉死叉信号
     * @return 1.0=金叉(%K上穿%D), -1.0=死叉(%K下穿%D), 0.0=无信号
     */
    double getCrossoverSignal() const {
        try {
            double k_current = getPercentK(0);
            double k_prev = getPercentK(-1);
            double d_current = getPercentD(0);
            double d_prev = getPercentD(-1);
            
            if (isNaN(k_current) || isNaN(k_prev) || 
                isNaN(d_current) || isNaN(d_prev)) {
                return 0.0;
            }
            
            // 金叉：%K从下方上穿%D
            if (k_prev <= d_prev && k_current > d_current) {
                return 1.0;
            }
            
            // 死叉：%K从上方下穿%D
            if (k_prev >= d_prev && k_current < d_current) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测背离信号
     * @param price_line 价格线（用于比较）
     * @param lookback 回看期数
     * @return 发散强度 (正值=看涨发散, 负值=看跌发散)
     */
    double calculateDivergence(std::shared_ptr<LineRoot> price_line,
                              size_t lookback = 10) const {
        if (!price_line || lookback < 4) {
            return 0.0;
        }
        
        try {
            // 计算价格和随机指标的趋势
            double price_start = price_line->get(-static_cast<int>(lookback-1));
            double price_end = price_line->get(0);
            double stoch_start = getPercentK(-static_cast<int>(lookback-1));
            double stoch_end = getPercentK(0);
            
            if (isNaN(price_start) || isNaN(price_end) || 
                isNaN(stoch_start) || isNaN(stoch_end)) {
                return 0.0;
            }
            
            double price_direction = price_end - price_start;
            double stoch_direction = stoch_end - stoch_start;
            
            // 标准化方向
            if (price_start != 0) price_direction /= std::abs(price_start);
            if (stoch_start != 0) stoch_direction /= std::abs(stoch_start);
            
            // 背离：价格和指标方向相反
            if (price_direction > 0 && stoch_direction < 0) {
                return -1.0; // 看跌背离
            } else if (price_direction < 0 && stoch_direction > 0) {
                return 1.0;  // 看涨背离
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        MultiLineIndicator::reset();
        
        high_buffer_.clear();
        low_buffer_.clear();
        
        if (k_line_) k_line_->home();
        if (d_sma_) d_sma_->reset();
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput() || !high_line_ || !low_line_) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            return;
        }
        
        double current_close = close_line_->get(0);
        double current_high = high_line_->get(0);
        double current_low = low_line_->get(0);
        
        if (isNaN(current_close) || isNaN(current_high) || isNaN(current_low)) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            return;
        }
        
        // 更新高低价缓存
        high_buffer_.push_back(current_high);
        low_buffer_.push_back(current_low);
        
        if (high_buffer_.size() > k_period_) {
            high_buffer_.pop_front();
            low_buffer_.pop_front();
        }
        
        if (high_buffer_.size() < k_period_) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            return;
        }
        
        // 计算周期内的最高价和最低价
        double highest = *std::max_element(high_buffer_.begin(), high_buffer_.end());
        double lowest = *std::min_element(low_buffer_.begin(), low_buffer_.end());
        
        // 计算%K
        double k_value;
        if (highest == lowest) {
            k_value = 50.0;  // 避免除零错误，设为中位值
        } else {
            k_value = 100.0 * (current_close - lowest) / (highest - lowest);
        }
        
        // 限制%K在0-100范围内
        k_value = std::max(0.0, std::min(100.0, k_value));
        
        setOutput(0, k_value);
        
        // 更新%K数据线并计算%D
        k_line_->forward(k_value);
        d_sma_->calculate();
        
        double d_value = d_sma_->get(0);
        setOutput(1, d_value);
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
                close_line_->forward();
                high_line_->forward();
                low_line_->forward();
            }
        }
    }
    
    /**
     * @brief 获取当前随机指标强度
     * @return 强度值 (0-100)
     */
    double getStochasticStrength() const {
        try {
            double k_value = getPercentK(0);
            double d_value = getPercentD(0);
            
            if (isNaN(k_value) || isNaN(d_value)) {
                return 0.0;
            }
            
            // 计算%K和%D的一致性强度
            double diff = std::abs(k_value - d_value);
            double avg = (k_value + d_value) / 2.0;
            
            // 强度与一致性和极值程度相关
            double consistency = 100.0 - diff;  // 一致性越高强度越大
            double extremeness = std::max(avg, 100.0 - avg);  // 越接近极值强度越大
            
            return (consistency + extremeness) / 2.0;
            
        } catch (...) {
            return 0.0;
        }
    }
};

} // namespace backtrader