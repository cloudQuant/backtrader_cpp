#pragma once

#include "IndicatorBase.h"
#include "EMA.h"
#include <deque>

namespace backtrader {

/**
 * @brief 相对强弱指数（Relative Strength Index）
 * 
 * RSI = 100 - (100 / (1 + RS))
 * RS = Average Gain / Average Loss
 * 
 * 支持两种计算方法：
 * 1. Wilder's平滑方法（默认）
 * 2. EMA平滑方法
 */
class RSI : public IndicatorBase {
private:
    size_t period_;
    bool use_ema_;              // 是否使用EMA平滑
    
    // Wilder's方法状态
    double avg_gain_;
    double avg_loss_;
    double prev_price_;
    bool has_prev_price_;
    size_t count_;              // 已处理的数据点数
    
    // EMA方法状态
    std::unique_ptr<EMA> gain_ema_;
    std::unique_ptr<EMA> loss_ema_;
    std::shared_ptr<LineRoot> gain_line_;
    std::shared_ptr<LineRoot> loss_line_;
    
public:
    /**
     * @brief 构造函数
     * @param input 输入数据线
     * @param period 计算周期，默认14
     * @param use_ema 是否使用EMA平滑，默认false（使用Wilder's方法）
     */
    explicit RSI(std::shared_ptr<LineRoot> input, 
                 size_t period = 14,
                 bool use_ema = false)
        : IndicatorBase(input, "RSI"), 
          period_(period), 
          use_ema_(use_ema),
          avg_gain_(0.0), 
          avg_loss_(0.0),
          prev_price_(0.0),
          has_prev_price_(false),
          count_(0) {
        
        if (period == 0) {
            throw std::invalid_argument("RSI period must be greater than 0");
        }
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        setParam("use_ema", use_ema ? 1.0 : 0.0);
        
        // RSI需要至少period+1个数据点才能产生第一个值
        setMinPeriod(period + 1);
        
        // 如果使用EMA方法，初始化EMA指标
        if (use_ema_) {
            gain_line_ = std::make_shared<LineRoot>(1000, "gain");
            loss_line_ = std::make_shared<LineRoot>(1000, "loss");
            gain_ema_ = std::make_unique<EMA>(gain_line_, period);
            loss_ema_ = std::make_unique<EMA>(loss_line_, period);
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
     * @brief 获取当前平均收益（仅Wilder's方法）
     * @return 平均收益
     */
    double getAverageGain() const {
        return use_ema_ ? NaN : avg_gain_;
    }
    
    /**
     * @brief 获取当前平均损失（仅Wilder's方法）
     * @return 平均损失
     */
    double getAverageLoss() const {
        return use_ema_ ? NaN : avg_loss_;
    }
    
    /**
     * @brief 获取当前RS值
     * @return RS值 (Average Gain / Average Loss)
     */
    double getRS() const {
        if (use_ema_) {
            if (!gain_ema_ || !loss_ema_) return NaN;
            double avg_gain = gain_ema_->get(0);
            double avg_loss = loss_ema_->get(0);
            return (avg_loss != 0.0) ? avg_gain / avg_loss : NaN;
        } else {
            return (avg_loss_ != 0.0) ? avg_gain_ / avg_loss_ : NaN;
        }
    }
    
    /**
     * @brief 设置周期
     * @param period 新周期
     */
    void setPeriod(size_t period) {
        if (period == 0) {
            throw std::invalid_argument("RSI period must be greater than 0");
        }
        
        period_ = period;
        setParam("period", static_cast<double>(period));
        setMinPeriod(period + 1);
        
        // 重置状态
        reset();
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        IndicatorBase::reset();
        avg_gain_ = 0.0;
        avg_loss_ = 0.0;
        prev_price_ = 0.0;
        has_prev_price_ = false;
        count_ = 0;
        
        if (use_ema_) {
            gain_line_->home();
            loss_line_->home();
            gain_ema_->reset();
            loss_ema_->reset();
        }
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
        double current_price = input->get(0);
        
        if (isNaN(current_price)) {
            setOutput(0, NaN);
            return;
        }
        
        // 第一个价格，无法计算变化
        if (!has_prev_price_) {
            prev_price_ = current_price;
            has_prev_price_ = true;
            setOutput(0, NaN);
            return;
        }
        
        if (use_ema_) {
            calculateWithEMA(current_price);
        } else {
            calculateWithWilders(current_price);
        }
        
        prev_price_ = current_price;
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
     * @brief 获取RSI的超买超卖状态
     * @param overbought_level 超买阈值，默认70
     * @param oversold_level 超卖阈值，默认30
     * @return 1.0=超买, -1.0=超卖, 0.0=中性
     */
    double getOverboughtOversoldStatus(double overbought_level = 70.0, 
                                     double oversold_level = 30.0) const {
        double rsi_value = get(0);
        if (isNaN(rsi_value)) {
            return 0.0;
        }
        
        if (rsi_value >= overbought_level) {
            return 1.0;  // 超买
        } else if (rsi_value <= oversold_level) {
            return -1.0; // 超卖
        } else {
            return 0.0;  // 中性
        }
    }
    
    /**
     * @brief 计算RSI发散
     * @param price_line 价格线
     * @param lookback 回看期数
     * @return 发散强度 (正值=看涨发散, 负值=看跌发散)
     */
    double calculateDivergence(std::shared_ptr<LineRoot> price_line, 
                              size_t lookback = 5) const {
        if (!price_line || lookback < 2) {
            return 0.0;
        }
        
        try {
            // 检查价格和RSI的趋势方向
            double price_start = price_line->get(-static_cast<int>(lookback-1));
            double price_end = price_line->get(0);
            double rsi_start = get(-static_cast<int>(lookback-1));
            double rsi_end = get(0);
            
            if (isNaN(price_start) || isNaN(price_end) || 
                isNaN(rsi_start) || isNaN(rsi_end)) {
                return 0.0;
            }
            
            double price_direction = price_end - price_start;
            double rsi_direction = rsi_end - rsi_start;
            
            // 发散：价格和RSI方向相反
            if (price_direction > 0 && rsi_direction < 0) {
                return -1.0; // 看跌发散
            } else if (price_direction < 0 && rsi_direction > 0) {
                return 1.0;  // 看涨发散
            }
            
            return 0.0; // 无发散
            
        } catch (...) {
            return 0.0;
        }
    }
    
private:
    /**
     * @brief 使用Wilder's平滑方法计算
     * @param current_price 当前价格
     */
    void calculateWithWilders(double current_price) {
        double change = current_price - prev_price_;
        double gain = (change > 0) ? change : 0.0;
        double loss = (change < 0) ? -change : 0.0;
        
        count_++;
        
        if (count_ <= period_) {
            // 初始期间：累积计算平均值
            avg_gain_ += gain;
            avg_loss_ += loss;
            
            if (count_ == period_) {
                // 第一个周期结束，计算初始平均值
                avg_gain_ /= period_;
                avg_loss_ /= period_;
                
                // 计算第一个RSI值
                double rs = (avg_loss_ != 0.0) ? avg_gain_ / avg_loss_ : 0.0;
                double rsi = (avg_loss_ != 0.0) ? 100.0 - (100.0 / (1.0 + rs)) : 100.0;
                setOutput(0, rsi);
            } else {
                setOutput(0, NaN);
            }
        } else {
            // Wilder's平滑公式
            avg_gain_ = (avg_gain_ * (period_ - 1) + gain) / period_;
            avg_loss_ = (avg_loss_ * (period_ - 1) + loss) / period_;
            
            // 计算RSI
            double rs = (avg_loss_ != 0.0) ? avg_gain_ / avg_loss_ : 0.0;
            double rsi = (avg_loss_ != 0.0) ? 100.0 - (100.0 / (1.0 + rs)) : 100.0;
            setOutput(0, rsi);
        }
    }
    
    /**
     * @brief 使用EMA平滑方法计算
     * @param current_price 当前价格
     */
    void calculateWithEMA(double current_price) {
        double change = current_price - prev_price_;
        double gain = (change > 0) ? change : 0.0;
        double loss = (change < 0) ? -change : 0.0;
        
        // 更新gain和loss数据线
        gain_line_->forward(gain);
        loss_line_->forward(loss);
        
        // 计算EMA
        gain_ema_->calculate();
        loss_ema_->calculate();
        
        // 获取平滑后的平均收益和损失
        double avg_gain = gain_ema_->get(0);
        double avg_loss = loss_ema_->get(0);
        
        if (isNaN(avg_gain) || isNaN(avg_loss)) {
            setOutput(0, NaN);
            return;
        }
        
        // 计算RSI
        double rs = (avg_loss != 0.0) ? avg_gain / avg_loss : 0.0;
        double rsi = (avg_loss != 0.0) ? 100.0 - (100.0 / (1.0 + rs)) : 100.0;
        setOutput(0, rsi);
    }
};

} // namespace backtrader