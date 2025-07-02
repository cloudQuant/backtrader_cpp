#pragma once

#include "IndicatorBase.h"
#include "SMA.h"
#include <deque>
#include <cmath>

namespace backtrader {

/**
 * @brief 商品通道指数（Commodity Channel Index）
 * 
 * CCI = (Typical Price - SMA(Typical Price, period)) / (0.015 * Mean Deviation)
 * 
 * 其中：
 * Typical Price = (High + Low + Close) / 3
 * Mean Deviation = SMA(|Typical Price - SMA(Typical Price)|, period)
 * 
 * CCI是一个摆荡指标，用于识别超买超卖状态和趋势变化
 * 通常在+100到-100之间震荡，超出此范围表示强烈的超买或超卖
 */
class CCI : public IndicatorBase {
private:
    size_t period_;
    double constant_;  // 通常为0.015
    
    std::shared_ptr<LineRoot> high_line_;   // 最高价数据线
    std::shared_ptr<LineRoot> low_line_;    // 最低价数据线
    std::shared_ptr<LineRoot> close_line_;  // 收盘价数据线
    
    // 典型价格相关
    std::shared_ptr<SMA> tp_sma_;           // 典型价格的SMA
    std::shared_ptr<LineRoot> tp_line_;     // 典型价格数据线
    
    // 平均偏差计算
    std::deque<double> tp_buffer_;          // 典型价格缓存
    std::deque<double> deviation_buffer_;   // 偏差缓存
    double deviation_sum_;
    
public:
    /**
     * @brief 构造函数
     * @param high_input 最高价数据线
     * @param low_input 最低价数据线
     * @param close_input 收盘价数据线
     * @param period 计算周期，默认20
     * @param constant 常数因子，默认0.015
     */
    explicit CCI(std::shared_ptr<LineRoot> high_input,
                std::shared_ptr<LineRoot> low_input,
                std::shared_ptr<LineRoot> close_input,
                size_t period = 20,
                double constant = 0.015)
        : IndicatorBase(close_input, "CCI"),
          period_(period),
          constant_(constant),
          high_line_(high_input),
          low_line_(low_input),
          close_line_(close_input),
          deviation_sum_(0.0) {
        
        if (period == 0) {
            throw std::invalid_argument("CCI period must be greater than 0");
        }
        
        if (!high_input || !low_input) {
            throw std::invalid_argument("High and low price lines are required for CCI");
        }
        
        if (constant <= 0.0) {
            throw std::invalid_argument("CCI constant must be positive");
        }
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        setParam("constant", constant);
        
        // 设置最小周期
        setMinPeriod(period);
        
        // 初始化典型价格数据线和SMA
        tp_line_ = std::make_shared<LineRoot>(1000, "typical_price");
        tp_sma_ = std::make_unique<SMA>(tp_line_, period);
    }
    
    /**
     * @brief 获取周期
     * @return 计算周期
     */
    size_t getPeriod() const { return period_; }
    
    /**
     * @brief 获取常数因子
     * @return 常数因子
     */
    double getConstant() const { return constant_; }
    
    /**
     * @brief 设置周期
     * @param period 新周期
     */
    void setPeriod(size_t period) {
        if (period == 0) {
            throw std::invalid_argument("CCI period must be greater than 0");
        }
        
        period_ = period;
        setParam("period", static_cast<double>(period));
        setMinPeriod(period);
        
        // 重置状态
        reset();
    }
    
    /**
     * @brief 设置常数因子
     * @param constant 新常数因子
     */
    void setConstant(double constant) {
        if (constant <= 0.0) {
            throw std::invalid_argument("CCI constant must be positive");
        }
        
        constant_ = constant;
        setParam("constant", constant);
    }
    
    /**
     * @brief 获取当前典型价格
     * @return 典型价格
     */
    double getCurrentTypicalPrice() const {
        if (!high_line_ || !low_line_ || !close_line_) {
            return NaN;
        }
        
        double high = high_line_->get(0);
        double low = low_line_->get(0);
        double close = close_line_->get(0);
        
        if (isNaN(high) || isNaN(low) || isNaN(close)) {
            return NaN;
        }
        
        return (high + low + close) / 3.0;
    }
    
    /**
     * @brief 检测超买超卖状态
     * @param overbought_level 超买阈值，默认100
     * @param oversold_level 超卖阈值，默认-100
     * @return 1.0=超买, -1.0=超卖, 0.0=中性
     */
    double getOverboughtOversoldStatus(double overbought_level = 100.0,
                                     double oversold_level = -100.0) const {
        double cci_value = get(0);
        if (isNaN(cci_value)) {
            return 0.0;
        }
        
        if (cci_value >= overbought_level) {
            return 1.0;  // 超买
        } else if (cci_value <= oversold_level) {
            return -1.0; // 超卖
        } else {
            return 0.0;  // 中性
        }
    }
    
    /**
     * @brief 检测CCI背离
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
            double cci_start = get(-static_cast<int>(lookback-1));
            double cci_end = get(0);
            
            if (isNaN(price_start) || isNaN(price_end) || 
                isNaN(cci_start) || isNaN(cci_end)) {
                return 0.0;
            }
            
            double price_direction = price_end - price_start;
            double cci_direction = cci_end - cci_start;
            
            // 标准化方向
            if (price_start != 0) price_direction /= std::abs(price_start);
            if (cci_start != 0) cci_direction /= std::abs(cci_start);
            
            // 背离：价格和CCI方向相反
            if (price_direction > 0 && cci_direction < 0) {
                return -1.0; // 看跌背离
            } else if (price_direction < 0 && cci_direction > 0) {
                return 1.0;  // 看涨背离
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测零轴交叉信号
     * @return 1.0=上穿零轴, -1.0=下穿零轴, 0.0=无交叉
     */
    double getZeroCrossSignal() const {
        try {
            double cci_current = get(0);
            double cci_prev = get(-1);
            
            if (isNaN(cci_current) || isNaN(cci_prev)) {
                return 0.0;
            }
            
            // 上穿零轴
            if (cci_prev <= 0.0 && cci_current > 0.0) {
                return 1.0;
            }
            
            // 下穿零轴
            if (cci_prev >= 0.0 && cci_current < 0.0) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测趋势反转信号
     * @return 1.0=看涨反转, -1.0=看跌反转, 0.0=无反转
     */
    double getTrendReversalSignal() const {
        try {
            double cci_current = get(0);
            double cci_prev = get(-1);
            double cci_prev2 = get(-2);
            
            if (isNaN(cci_current) || isNaN(cci_prev) || isNaN(cci_prev2)) {
                return 0.0;
            }
            
            // 看涨反转：从超卖区域(-100以下)开始上升
            if (cci_prev2 <= -100.0 && cci_prev <= -100.0 && 
                cci_current > cci_prev && cci_prev > cci_prev2) {
                return 1.0;
            }
            
            // 看跌反转：从超买区域(100以上)开始下降
            if (cci_prev2 >= 100.0 && cci_prev >= 100.0 && 
                cci_current < cci_prev && cci_prev < cci_prev2) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 获取CCI强度
     * @return 强度值 (0-100)
     */
    double getCCIStrength() const {
        try {
            double cci_value = get(0);
            if (isNaN(cci_value)) {
                return 0.0;
            }
            
            // 将CCI值转换为0-100的强度值
            double abs_cci = std::abs(cci_value);
            double strength = std::min(100.0, abs_cci);
            
            return strength;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        IndicatorBase::reset();
        
        tp_buffer_.clear();
        deviation_buffer_.clear();
        deviation_sum_ = 0.0;
        
        if (tp_line_) tp_line_->home();
        if (tp_sma_) tp_sma_->reset();
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
        
        // 计算典型价格
        double typical_price = (current_high + current_low + current_close) / 3.0;
        
        // 更新典型价格数据线并计算SMA
        tp_line_->forward(typical_price);
        tp_sma_->calculate();
        
        double tp_sma_value = tp_sma_->get(0);
        if (isNaN(tp_sma_value)) {
            setOutput(0, NaN);
            return;
        }
        
        // 更新典型价格缓存
        tp_buffer_.push_back(typical_price);
        if (tp_buffer_.size() > period_) {
            tp_buffer_.pop_front();
        }
        
        if (tp_buffer_.size() < period_) {
            setOutput(0, NaN);
            return;
        }
        
        // 计算平均偏差
        double mean_deviation = calculateMeanDeviation(tp_sma_value);
        if (isNaN(mean_deviation) || mean_deviation == 0.0) {
            setOutput(0, NaN);
            return;
        }
        
        // 计算CCI
        double cci = (typical_price - tp_sma_value) / (constant_ * mean_deviation);
        setOutput(0, cci);
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
    
private:
    /**
     * @brief 计算平均偏差
     * @param sma_value SMA值
     * @return 平均偏差
     */
    double calculateMeanDeviation(double sma_value) {
        if (tp_buffer_.size() < period_) {
            return NaN;
        }
        
        double deviation_sum = 0.0;
        for (double tp : tp_buffer_) {
            deviation_sum += std::abs(tp - sma_value);
        }
        
        return deviation_sum / period_;
    }
};

} // namespace backtrader