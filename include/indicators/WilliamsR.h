#pragma once

#include "IndicatorBase.h"
#include <algorithm>

namespace backtrader {

/**
 * @brief 威廉指标（Williams %R）
 * 
 * %R = -100 * (H - C) / (H - L)
 * 
 * 其中：
 * H = 指定周期内最高价
 * L = 指定周期内最低价
 * C = 当前收盘价
 * 
 * 威廉指标的值域为-100到0，与随机指标相似但取值相反
 * - 接近0表示超买
 * - 接近-100表示超卖
 */
class WilliamsR : public IndicatorBase {
private:
    size_t period_;
    
    std::shared_ptr<LineRoot> high_line_;   // 最高价数据线
    std::shared_ptr<LineRoot> low_line_;    // 最低价数据线
    std::shared_ptr<LineRoot> close_line_;  // 收盘价数据线（输入）
    
    // 缓存区用于存储高低价
    std::deque<double> high_buffer_;
    std::deque<double> low_buffer_;
    
public:
    /**
     * @brief 构造函数
     * @param close_input 收盘价数据线
     * @param high_input 最高价数据线
     * @param low_input 最低价数据线
     * @param period 计算周期，默认14
     */
    explicit WilliamsR(std::shared_ptr<LineRoot> close_input,
                      std::shared_ptr<LineRoot> high_input,
                      std::shared_ptr<LineRoot> low_input,
                      size_t period = 14)
        : IndicatorBase(close_input, "WilliamsR"),
          period_(period),
          high_line_(high_input),
          low_line_(low_input),
          close_line_(close_input) {
        
        if (period == 0) {
            throw std::invalid_argument("WilliamsR period must be greater than 0");
        }
        
        if (!high_input || !low_input) {
            throw std::invalid_argument("High and low price lines are required for WilliamsR");
        }
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        
        // 设置最小周期
        setMinPeriod(period);
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
            throw std::invalid_argument("WilliamsR period must be greater than 0");
        }
        
        period_ = period;
        setParam("period", static_cast<double>(period));
        setMinPeriod(period);
        
        // 重置状态
        reset();
    }
    
    /**
     * @brief 检测超买超卖状态
     * @param overbought_level 超买阈值，默认-20
     * @param oversold_level 超卖阈值，默认-80
     * @return 1.0=超买, -1.0=超卖, 0.0=中性
     */
    double getOverboughtOversoldStatus(double overbought_level = -20.0,
                                     double oversold_level = -80.0) const {
        double wr_value = get(0);
        if (isNaN(wr_value)) {
            return 0.0;
        }
        
        if (wr_value >= overbought_level) {
            return 1.0;  // 超买
        } else if (wr_value <= oversold_level) {
            return -1.0; // 超卖
        } else {
            return 0.0;  // 中性
        }
    }
    
    /**
     * @brief 检测反转信号
     * @return 1.0=看涨反转, -1.0=看跌反转, 0.0=无信号
     */
    double getReversalSignal() const {
        try {
            double wr_current = get(0);
            double wr_prev = get(-1);
            double wr_prev2 = get(-2);
            
            if (isNaN(wr_current) || isNaN(wr_prev) || isNaN(wr_prev2)) {
                return 0.0;
            }
            
            // 看涨反转：从超卖区域(-80以下)开始上升
            if (wr_prev2 <= -80.0 && wr_prev <= -80.0 && wr_current > wr_prev && wr_prev > wr_prev2) {
                return 1.0;
            }
            
            // 看跌反转：从超买区域(-20以上)开始下降
            if (wr_prev2 >= -20.0 && wr_prev >= -20.0 && wr_current < wr_prev && wr_prev < wr_prev2) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 计算威廉指标的动量
     * @param lookback 回看期数
     * @return 动量值
     */
    double getMomentum(size_t lookback = 5) const {
        try {
            if (lookback == 0) lookback = 1;
            
            double current = get(0);
            double past = get(-static_cast<int>(lookback));
            
            if (isNaN(current) || isNaN(past)) {
                return 0.0;
            }
            
            return current - past;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 计算威廉指标背离
     * @param price_line 价格线
     * @param lookback 回看期数
     * @return 发散强度 (正值=看涨发散, 负值=看跌发散)
     */
    double calculateDivergence(std::shared_ptr<LineRoot> price_line,
                              size_t lookback = 10) const {
        if (!price_line || lookback < 4) {
            return 0.0;
        }
        
        try {
            double price_start = price_line->get(-static_cast<int>(lookback-1));
            double price_end = price_line->get(0);
            double wr_start = get(-static_cast<int>(lookback-1));
            double wr_end = get(0);
            
            if (isNaN(price_start) || isNaN(price_end) || 
                isNaN(wr_start) || isNaN(wr_end)) {
                return 0.0;
            }
            
            double price_direction = price_end - price_start;
            double wr_direction = wr_end - wr_start;
            
            // 标准化方向
            if (price_start != 0) price_direction /= std::abs(price_start);
            if (wr_start != 0) wr_direction /= std::abs(wr_start);
            
            // 背离：价格和指标方向相反
            if (price_direction > 0 && wr_direction < 0) {
                return -1.0; // 看跌背离
            } else if (price_direction < 0 && wr_direction > 0) {
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
        IndicatorBase::reset();
        high_buffer_.clear();
        low_buffer_.clear();
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput() || !high_line_ || !low_line_) {
            setOutput(0, NaN);
            return;
        }
        
        double current_close = close_line_->get(0);
        double current_high = high_line_->get(0);
        double current_low = low_line_->get(0);
        
        if (isNaN(current_close) || isNaN(current_high) || isNaN(current_low)) {
            setOutput(0, NaN);
            return;
        }
        
        // 更新高低价缓存
        high_buffer_.push_back(current_high);
        low_buffer_.push_back(current_low);
        
        if (high_buffer_.size() > period_) {
            high_buffer_.pop_front();
            low_buffer_.pop_front();
        }
        
        if (high_buffer_.size() < period_) {
            setOutput(0, NaN);
            return;
        }
        
        // 计算周期内的最高价和最低价
        double highest = *std::max_element(high_buffer_.begin(), high_buffer_.end());
        double lowest = *std::min_element(low_buffer_.begin(), low_buffer_.end());
        
        // 计算威廉指标
        double wr_value;
        if (highest == lowest) {
            wr_value = -50.0;  // 避免除零错误，设为中位值
        } else {
            wr_value = -100.0 * (highest - current_close) / (highest - lowest);
        }
        
        // 限制在-100到0范围内
        wr_value = std::max(-100.0, std::min(0.0, wr_value));
        
        setOutput(0, wr_value);
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
     * @brief 转换为随机指标K值
     * @return 对应的随机指标%K值 (0-100)
     */
    double toStochasticK() const {
        double wr_value = get(0);
        if (isNaN(wr_value)) {
            return NaN;
        }
        // %K = %R + 100
        return wr_value + 100.0;
    }
    
    /**
     * @brief 获取威廉指标强度
     * @return 强度值 (0-100)
     */
    double getWilliamsRStrength() const {
        try {
            double wr_value = get(0);
            if (isNaN(wr_value)) {
                return 0.0;
            }
            
            // 计算距离中位值(-50)的程度作为强度
            double distance_from_center = std::abs(wr_value + 50.0);
            return (distance_from_center / 50.0) * 100.0;
            
        } catch (...) {
            return 0.0;
        }
    }
};

} // namespace backtrader