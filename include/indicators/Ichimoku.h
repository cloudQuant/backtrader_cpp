#pragma once

#include "IndicatorBase.h"
#include <algorithm>
#include <deque>

namespace backtrader {

/**
 * @brief 一目均衡表（Ichimoku Kinko Hyo）
 * 
 * 一目均衡表包含5条线：
 * - 转换线 (Tenkan-sen): (9日高点+9日低点)/2
 * - 基准线 (Kijun-sen): (26日高点+26日低点)/2  
 * - 先行线A (Senkou Span A): (转换线+基准线)/2 (向前偏移26期)
 * - 先行线B (Senkou Span B): (52日高点+52日低点)/2 (向前偏移26期)
 * - 滞后线 (Chikou Span): 收盘价 (向后偏移26期)
 * 
 * 这是一个多线指标，包含5条输出线：
 * - Line 0: 转换线 (Tenkan-sen)
 * - Line 1: 基准线 (Kijun-sen)  
 * - Line 2: 先行线A (Senkou Span A)
 * - Line 3: 先行线B (Senkou Span B)
 * - Line 4: 滞后线 (Chikou Span)
 */
class Ichimoku : public MultiLineIndicator {
private:
    size_t tenkan_period_;   // 转换线周期 (默认9)
    size_t kijun_period_;    // 基准线周期 (默认26)
    size_t senkou_period_;   // 先行线B周期 (默认52)
    size_t displacement_;    // 位移周期 (默认26)
    
    std::shared_ptr<LineRoot> high_line_;   // 最高价数据线
    std::shared_ptr<LineRoot> low_line_;    // 最低价数据线
    std::shared_ptr<LineRoot> close_line_;  // 收盘价数据线
    
    // 缓存区用于存储高低价
    std::deque<double> tenkan_high_buffer_;
    std::deque<double> tenkan_low_buffer_;
    std::deque<double> kijun_high_buffer_;
    std::deque<double> kijun_low_buffer_;
    std::deque<double> senkou_high_buffer_;
    std::deque<double> senkou_low_buffer_;
    
    // 位移缓存 (用于先行线和滞后线)
    std::deque<double> senkou_a_buffer_;
    std::deque<double> senkou_b_buffer_;
    std::deque<double> chikou_buffer_;
    
public:
    /**
     * @brief 构造函数
     * @param close_input 收盘价数据线
     * @param high_input 最高价数据线
     * @param low_input 最低价数据线
     * @param tenkan_period 转换线周期，默认9
     * @param kijun_period 基准线周期，默认26
     * @param senkou_period 先行线B周期，默认52
     * @param displacement 位移周期，默认26
     */
    explicit Ichimoku(std::shared_ptr<LineRoot> close_input,
                     std::shared_ptr<LineRoot> high_input,
                     std::shared_ptr<LineRoot> low_input,
                     size_t tenkan_period = 9,
                     size_t kijun_period = 26,
                     size_t senkou_period = 52,
                     size_t displacement = 26)
        : MultiLineIndicator(close_input, {"Tenkan", "Kijun", "SenkouA", "SenkouB", "Chikou"}, "Ichimoku"),
          tenkan_period_(tenkan_period),
          kijun_period_(kijun_period),
          senkou_period_(senkou_period),
          displacement_(displacement),
          high_line_(high_input),
          low_line_(low_input),
          close_line_(close_input) {
        
        if (tenkan_period == 0 || kijun_period == 0 || senkou_period == 0) {
            throw std::invalid_argument("Ichimoku periods must be greater than 0");
        }
        
        if (!high_input || !low_input) {
            throw std::invalid_argument("High and low price lines are required for Ichimoku");
        }
        
        // 设置参数
        setParam("tenkan_period", static_cast<double>(tenkan_period));
        setParam("kijun_period", static_cast<double>(kijun_period));
        setParam("senkou_period", static_cast<double>(senkou_period));
        setParam("displacement", static_cast<double>(displacement));
        
        // 最小周期是最长周期 + 位移
        setMinPeriod(std::max({tenkan_period, kijun_period, senkou_period}) + displacement);
    }
    
    /**
     * @brief 获取转换线周期
     */
    size_t getTenkanPeriod() const { return tenkan_period_; }
    
    /**
     * @brief 获取基准线周期
     */
    size_t getKijunPeriod() const { return kijun_period_; }
    
    /**
     * @brief 获取先行线B周期
     */
    size_t getSenkouPeriod() const { return senkou_period_; }
    
    /**
     * @brief 获取位移周期
     */
    size_t getDisplacement() const { return displacement_; }
    
    /**
     * @brief 获取转换线值
     * @param ago 偏移量，默认0
     */
    double getTenkanSen(int ago = 0) const {
        return getOutputValue(0, ago);
    }
    
    /**
     * @brief 获取基准线值
     * @param ago 偏移量，默认0
     */
    double getKijunSen(int ago = 0) const {
        return getOutputValue(1, ago);
    }
    
    /**
     * @brief 获取先行线A值
     * @param ago 偏移量，默认0
     */
    double getSenkouSpanA(int ago = 0) const {
        return getOutputValue(2, ago);
    }
    
    /**
     * @brief 获取先行线B值
     * @param ago 偏移量，默认0
     */
    double getSenkouSpanB(int ago = 0) const {
        return getOutputValue(3, ago);
    }
    
    /**
     * @brief 获取滞后线值
     * @param ago 偏移量，默认0
     */
    double getChikouSpan(int ago = 0) const {
        return getOutputValue(4, ago);
    }
    
    /**
     * @brief 获取云图厚度
     * @param ago 偏移量，默认0
     * @return 云图厚度 (先行线A和B的差值)
     */
    double getCloudThickness(int ago = 0) const {
        double senkou_a = getSenkouSpanA(ago);
        double senkou_b = getSenkouSpanB(ago);
        
        if (isNaN(senkou_a) || isNaN(senkou_b)) {
            return NaN;
        }
        
        return std::abs(senkou_a - senkou_b);
    }
    
    /**
     * @brief 获取云图方向
     * @param ago 偏移量，默认0
     * @return 1.0=上升云(牛市), -1.0=下降云(熊市), 0.0=平衡
     */
    double getCloudDirection(int ago = 0) const {
        double senkou_a = getSenkouSpanA(ago);
        double senkou_b = getSenkouSpanB(ago);
        
        if (isNaN(senkou_a) || isNaN(senkou_b)) {
            return 0.0;
        }
        
        if (senkou_a > senkou_b) {
            return 1.0;  // 上升云 (牛市)
        } else if (senkou_a < senkou_b) {
            return -1.0; // 下降云 (熊市)
        } else {
            return 0.0;  // 平衡
        }
    }
    
    /**
     * @brief 检测价格相对云图位置
     * @param price 价格，默认使用当前收盘价
     * @param ago 偏移量，默认0
     * @return 1.0=云图上方, -1.0=云图下方, 0.0=云图内部
     */
    double getPriceCloudPosition(double price = NaN, int ago = 0) const {
        if (isNaN(price)) {
            price = close_line_->get(ago);
        }
        
        double senkou_a = getSenkouSpanA(ago);
        double senkou_b = getSenkouSpanB(ago);
        
        if (isNaN(price) || isNaN(senkou_a) || isNaN(senkou_b)) {
            return 0.0;
        }
        
        double cloud_top = std::max(senkou_a, senkou_b);
        double cloud_bottom = std::min(senkou_a, senkou_b);
        
        if (price > cloud_top) {
            return 1.0;  // 云图上方
        } else if (price < cloud_bottom) {
            return -1.0; // 云图下方
        } else {
            return 0.0;  // 云图内部
        }
    }
    
    /**
     * @brief 检测TK交叉信号
     * @return 1.0=转换线上穿基准线, -1.0=转换线下穿基准线, 0.0=无信号
     */
    double getTKCrossSignal() const {
        try {
            double tenkan_current = getTenkanSen(0);
            double tenkan_prev = getTenkanSen(-1);
            double kijun_current = getKijunSen(0);
            double kijun_prev = getKijunSen(-1);
            
            if (isNaN(tenkan_current) || isNaN(tenkan_prev) || 
                isNaN(kijun_current) || isNaN(kijun_prev)) {
                return 0.0;
            }
            
            // 金叉：转换线上穿基准线
            if (tenkan_prev <= kijun_prev && tenkan_current > kijun_current) {
                return 1.0;
            }
            
            // 死叉：转换线下穿基准线
            if (tenkan_prev >= kijun_prev && tenkan_current < kijun_current) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测价格突破云图信号
     * @return 1.0=向上突破, -1.0=向下突破, 0.0=无突破
     */
    double getCloudBreakoutSignal() const {
        try {
            double prev_position = getPriceCloudPosition(NaN, -1);
            double current_position = getPriceCloudPosition(NaN, 0);
            
            if (prev_position == 0.0 || current_position == 0.0) {
                return 0.0; // 在云图内部，无明确突破
            }
            
            // 向上突破：从云图下方到上方
            if (prev_position < 0.0 && current_position > 0.0) {
                return 1.0;
            }
            
            // 向下突破：从云图上方到下方
            if (prev_position > 0.0 && current_position < 0.0) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测滞后线确认信号
     * @return 1.0=看涨确认, -1.0=看跌确认, 0.0=无确认
     */
    double getChikouConfirmation() const {
        try {
            // 滞后线当前值
            double chikou_current = getChikouSpan(0);
            
            // 26期前的价格（与滞后线对应）
            double price_26_ago = close_line_->get(-static_cast<int>(displacement_));
            
            if (isNaN(chikou_current) || isNaN(price_26_ago)) {
                return 0.0;
            }
            
            if (chikou_current > price_26_ago) {
                return 1.0;  // 看涨确认
            } else if (chikou_current < price_26_ago) {
                return -1.0; // 看跌确认
            } else {
                return 0.0;  // 无确认
            }
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 获取综合一目均衡表信号
     * @return 强度值 (-3到3，绝对值越大信号越强)
     */
    double getIchimokuSignal() const {
        double signal_strength = 0.0;
        
        // TK交叉信号
        signal_strength += getTKCrossSignal();
        
        // 云图突破信号
        signal_strength += getCloudBreakoutSignal();
        
        // 滞后线确认信号
        signal_strength += getChikouConfirmation();
        
        return signal_strength;
    }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        MultiLineIndicator::reset();
        
        tenkan_high_buffer_.clear();
        tenkan_low_buffer_.clear();
        kijun_high_buffer_.clear();
        kijun_low_buffer_.clear();
        senkou_high_buffer_.clear();
        senkou_low_buffer_.clear();
        
        senkou_a_buffer_.clear();
        senkou_b_buffer_.clear();
        chikou_buffer_.clear();
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput() || !high_line_ || !low_line_) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            setOutput(2, NaN);
            setOutput(3, NaN);
            setOutput(4, NaN);
            return;
        }
        
        double current_high = high_line_->get(0);
        double current_low = low_line_->get(0);
        double current_close = close_line_->get(0);
        
        if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close)) {
            setOutput(0, NaN);
            setOutput(1, NaN);
            setOutput(2, NaN);
            setOutput(3, NaN);
            setOutput(4, NaN);
            return;
        }
        
        // 更新各个周期的高低价缓存
        updateBuffers(current_high, current_low, current_close);
        
        // 计算转换线 (Tenkan-sen)
        double tenkan = calculateMidpoint(tenkan_high_buffer_, tenkan_low_buffer_, tenkan_period_);
        setOutput(0, tenkan);
        
        // 计算基准线 (Kijun-sen)
        double kijun = calculateMidpoint(kijun_high_buffer_, kijun_low_buffer_, kijun_period_);
        setOutput(1, kijun);
        
        // 计算先行线A (Senkou Span A) - 需要位移
        double senkou_a = NaN;
        if (!isNaN(tenkan) && !isNaN(kijun)) {
            senkou_a = (tenkan + kijun) / 2.0;
        }
        
        senkou_a_buffer_.push_back(senkou_a);
        if (senkou_a_buffer_.size() > displacement_) {
            setOutput(2, senkou_a_buffer_.front());
            senkou_a_buffer_.pop_front();
        } else {
            setOutput(2, NaN);
        }
        
        // 计算先行线B (Senkou Span B) - 需要位移
        double senkou_b = calculateMidpoint(senkou_high_buffer_, senkou_low_buffer_, senkou_period_);
        
        senkou_b_buffer_.push_back(senkou_b);
        if (senkou_b_buffer_.size() > displacement_) {
            setOutput(3, senkou_b_buffer_.front());
            senkou_b_buffer_.pop_front();
        } else {
            setOutput(3, NaN);
        }
        
        // 计算滞后线 (Chikou Span) - 当前收盘价，但输出位置需要调整
        chikou_buffer_.push_back(current_close);
        if (chikou_buffer_.size() > displacement_) {
            chikou_buffer_.pop_front();
        }
        
        // 滞后线实际上是当前收盘价
        setOutput(4, current_close);
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
    
private:
    /**
     * @brief 更新缓存区
     */
    void updateBuffers(double high, double low, double close) {
        // 更新转换线缓存
        tenkan_high_buffer_.push_back(high);
        tenkan_low_buffer_.push_back(low);
        if (tenkan_high_buffer_.size() > tenkan_period_) {
            tenkan_high_buffer_.pop_front();
            tenkan_low_buffer_.pop_front();
        }
        
        // 更新基准线缓存
        kijun_high_buffer_.push_back(high);
        kijun_low_buffer_.push_back(low);
        if (kijun_high_buffer_.size() > kijun_period_) {
            kijun_high_buffer_.pop_front();
            kijun_low_buffer_.pop_front();
        }
        
        // 更新先行线B缓存
        senkou_high_buffer_.push_back(high);
        senkou_low_buffer_.push_back(low);
        if (senkou_high_buffer_.size() > senkou_period_) {
            senkou_high_buffer_.pop_front();
            senkou_low_buffer_.pop_front();
        }
    }
    
    /**
     * @brief 计算中点值
     */
    double calculateMidpoint(const std::deque<double>& high_buffer,
                           const std::deque<double>& low_buffer,
                           size_t period) const {
        if (high_buffer.size() < period || low_buffer.size() < period) {
            return NaN;
        }
        
        double highest = *std::max_element(high_buffer.begin(), high_buffer.end());
        double lowest = *std::min_element(low_buffer.begin(), low_buffer.end());
        
        return (highest + lowest) / 2.0;
    }
};

} // namespace backtrader