#pragma once

#include "IndicatorBase.h"
#include "EMA.h"

namespace backtrader {

/**
 * @brief MACD指标（Moving Average Convergence Divergence）
 * 
 * MACD = EMA(12) - EMA(26)
 * Signal = EMA(MACD, 9)
 * Histogram = MACD - Signal
 * 
 * 这是一个多线指标，包含三条输出线：
 * - Line 0: MACD线
 * - Line 1: Signal线
 * - Line 2: Histogram线
 */
class MACD : public MultiLineIndicator {
private:
    size_t fast_period_;
    size_t slow_period_;
    size_t signal_period_;
    
    // EMA指标
    std::unique_ptr<EMA> fast_ema_;
    std::unique_ptr<EMA> slow_ema_;
    std::unique_ptr<EMA> signal_ema_;
    
    // MACD数据线（用于Signal EMA的输入）
    std::shared_ptr<LineRoot> macd_line_;
    
public:
    /**
     * @brief 构造函数
     * @param input 输入数据线
     * @param fast_period 快速EMA周期，默认12
     * @param slow_period 慢速EMA周期，默认26
     * @param signal_period 信号线EMA周期，默认9
     */
    explicit MACD(std::shared_ptr<LineRoot> input,
                  size_t fast_period = 12,
                  size_t slow_period = 26,
                  size_t signal_period = 9)
        : MultiLineIndicator(input, {"macd", "signal", "histogram"}, "MACD"),
          fast_period_(fast_period),
          slow_period_(slow_period),
          signal_period_(signal_period) {
        
        if (fast_period == 0 || slow_period == 0 || signal_period == 0) {
            throw std::invalid_argument("MACD periods must be greater than 0");
        }
        
        if (fast_period >= slow_period) {
            throw std::invalid_argument("Fast period must be less than slow period");
        }
        
        // 设置参数
        setParam("fast_period", static_cast<double>(fast_period));
        setParam("slow_period", static_cast<double>(slow_period));
        setParam("signal_period", static_cast<double>(signal_period));
        
        // 最小周期是慢速EMA + 信号EMA的周期
        setMinPeriod(slow_period + signal_period);
        
        // 初始化EMA指标
        fast_ema_ = std::make_unique<EMA>(input, fast_period);
        slow_ema_ = std::make_unique<EMA>(input, slow_period);
        
        // 创建MACD数据线用于Signal计算
        macd_line_ = std::make_shared<LineRoot>(1000, "macd_data");
        signal_ema_ = std::make_unique<EMA>(macd_line_, signal_period);
    }
    
    /**
     * @brief 获取快速周期
     * @return 快速EMA周期
     */
    size_t getFastPeriod() const { return fast_period_; }
    
    /**
     * @brief 获取慢速周期
     * @return 慢速EMA周期
     */
    size_t getSlowPeriod() const { return slow_period_; }
    
    /**
     * @brief 获取信号线周期
     * @return 信号线EMA周期
     */
    size_t getSignalPeriod() const { return signal_period_; }
    
    /**
     * @brief 获取MACD线值
     * @param ago 偏移量，默认0
     * @return MACD线值
     */
    double getMACDLine(int ago = 0) const {
        return getOutputValue(0, ago);
    }
    
    /**
     * @brief 获取Signal线值
     * @param ago 偏移量，默认0
     * @return Signal线值
     */
    double getSignalLine(int ago = 0) const {
        return getOutputValue(1, ago);
    }
    
    /**
     * @brief 获取Histogram值
     * @param ago 偏移量，默认0
     * @return Histogram值
     */
    double getHistogram(int ago = 0) const {
        return getOutputValue(2, ago);
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        MultiLineIndicator::reset();
        
        if (fast_ema_) fast_ema_->reset();
        if (slow_ema_) slow_ema_->reset();
        if (signal_ema_) signal_ema_->reset();
        if (macd_line_) macd_line_->home();
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput()) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            setOutput(2, NaN);
            return;
        }
        
        // 计算快速和慢速EMA
        fast_ema_->calculate();
        slow_ema_->calculate();
        
        double fast_ema_value = fast_ema_->get(0);
        double slow_ema_value = slow_ema_->get(0);
        
        if (isNaN(fast_ema_value) || isNaN(slow_ema_value)) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            setOutput(2, NaN);
            return;
        }
        
        // 计算MACD线
        double macd_value = fast_ema_value - slow_ema_value;
        setOutput(0, macd_value);
        
        // 更新MACD数据线并计算Signal线
        macd_line_->forward(macd_value);
        signal_ema_->calculate();
        
        double signal_value = signal_ema_->get(0);
        setOutput(1, signal_value);
        
        // 计算Histogram
        if (!isNaN(signal_value)) {
            double histogram_value = macd_value - signal_value;
            setOutput(2, histogram_value);
        } else {
            setOutput(2, NaN);
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
        
        for (size_t i = start; i < end; ++i) {
            calculate();
            
            if (i < end - 1) {
                input->forward();
            }
        }
    }
    
    /**
     * @brief 检测MACD信号
     * @return 交易信号 (1.0=买入, -1.0=卖出, 0.0=无信号)
     */
    double getTradeSignal() const {
        try {
            double macd_current = getMACDLine(0);
            double macd_prev = getMACDLine(-1);
            double signal_current = getSignalLine(0);
            double signal_prev = getSignalLine(-1);
            
            if (isNaN(macd_current) || isNaN(macd_prev) || 
                isNaN(signal_current) || isNaN(signal_prev)) {
                return 0.0;
            }
            
            // 金叉：MACD线上穿Signal线
            if (macd_prev <= signal_prev && macd_current > signal_current) {
                return 1.0;  // 买入信号
            }
            
            // 死叉：MACD线下穿Signal线
            if (macd_prev >= signal_prev && macd_current < signal_current) {
                return -1.0; // 卖出信号
            }
            
            return 0.0; // 无信号
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测零轴交叉
     * @return 零轴交叉信号 (1.0=上穿零轴, -1.0=下穿零轴, 0.0=无交叉)
     */
    double getZeroCrossSignal() const {
        try {
            double macd_current = getMACDLine(0);
            double macd_prev = getMACDLine(-1);
            
            if (isNaN(macd_current) || isNaN(macd_prev)) {
                return 0.0;
            }
            
            // 上穿零轴
            if (macd_prev <= 0.0 && macd_current > 0.0) {
                return 1.0;
            }
            
            // 下穿零轴
            if (macd_prev >= 0.0 && macd_current < 0.0) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测Histogram趋势变化
     * @return 趋势信号 (1.0=转为上升, -1.0=转为下降, 0.0=无变化)
     */
    double getHistogramTrendSignal() const {
        try {
            double hist_current = getHistogram(0);
            double hist_prev = getHistogram(-1);
            double hist_prev2 = getHistogram(-2);
            
            if (isNaN(hist_current) || isNaN(hist_prev) || isNaN(hist_prev2)) {
                return 0.0;
            }
            
            // 检测趋势转折点
            bool was_declining = hist_prev2 > hist_prev;
            bool now_rising = hist_prev < hist_current;
            
            bool was_rising = hist_prev2 < hist_prev;
            bool now_declining = hist_prev > hist_current;
            
            if (was_declining && now_rising) {
                return 1.0;  // 转为上升
            }
            
            if (was_rising && now_declining) {
                return -1.0; // 转为下降
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 计算MACD发散
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
            // 寻找价格的高点和低点
            double price_high = price_line->get(0);
            double price_low = price_line->get(0);
            double macd_high = getMACDLine(0);
            double macd_low = getMACDLine(0);
            
            for (size_t i = 1; i < lookback; ++i) {
                double price = price_line->get(-static_cast<int>(i));
                double macd = getMACDLine(-static_cast<int>(i));
                
                if (!isNaN(price) && price > price_high) {
                    price_high = price;
                }
                if (!isNaN(price) && price < price_low) {
                    price_low = price;
                }
                if (!isNaN(macd) && macd > macd_high) {
                    macd_high = macd;
                }
                if (!isNaN(macd) && macd < macd_low) {
                    macd_low = macd;
                }
            }
            
            double price_range = price_high - price_low;
            double macd_range = macd_high - macd_low;
            
            if (price_range == 0.0 || macd_range == 0.0) {
                return 0.0;
            }
            
            // 简化的发散检测
            double price_trend = (price_line->get(0) - price_line->get(-static_cast<int>(lookback-1))) / price_range;
            double macd_trend = (getMACDLine(0) - getMACDLine(-static_cast<int>(lookback-1))) / macd_range;
            
            // 发散：价格和MACD趋势相反
            return macd_trend - price_trend;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 获取MACD强度
     * @return MACD强度指标 (0-100)
     */
    double getMACDStrength() const {
        try {
            double macd_value = getMACDLine(0);
            double signal_value = getSignalLine(0);
            
            if (isNaN(macd_value) || isNaN(signal_value)) {
                return 0.0;
            }
            
            double diff = std::abs(macd_value - signal_value);
            double avg = (std::abs(macd_value) + std::abs(signal_value)) / 2.0;
            
            if (avg == 0.0) {
                return 0.0;
            }
            
            return std::min(100.0, (diff / avg) * 100.0);
            
        } catch (...) {
            return 0.0;
        }
    }
};

} // namespace backtrader