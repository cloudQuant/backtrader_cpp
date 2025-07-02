#pragma once

#include "IndicatorBase.h"
#include "SMA.h"
#include "Common.h"
#include <cmath>
#include <deque>

namespace backtrader {

/**
 * @brief 布林带指标（Bollinger Bands）
 * 
 * 中轨：SMA(n)
 * 上轨：SMA(n) + k * StdDev(n)
 * 下轨：SMA(n) - k * StdDev(n)
 * 
 * 这是一个多线指标，包含三条输出线：
 * - Line 0: 中轨 (Middle Band / SMA)
 * - Line 1: 上轨 (Upper Band)
 * - Line 2: 下轨 (Lower Band)
 */
class BollingerBands : public MultiLineIndicator {
private:
    size_t period_;
    double dev_factor_;
    
    // SMA指标用于中轨
    std::unique_ptr<SMA> sma_;
    
    // 标准差计算状态
    bool use_incremental_;
    std::deque<double> window_;  // 滑动窗口
    
public:
    /**
     * @brief 构造函数
     * @param input 输入数据线
     * @param period 计算周期，默认20
     * @param dev_factor 标准差倍数，默认2.0
     * @param use_incremental 是否使用增量计算，默认true
     */
    explicit BollingerBands(std::shared_ptr<LineRoot> input,
                           size_t period = 20,
                           double dev_factor = 2.0,
                           bool use_incremental = true)
        : MultiLineIndicator(input, {"Middle", "Upper", "Lower"}, "BollingerBands"),
          period_(period),
          dev_factor_(dev_factor),
          use_incremental_(use_incremental) {
        
        if (period == 0) {
            throw std::invalid_argument("BollingerBands period must be greater than 0");
        }
        
        if (dev_factor < 0.0) {
            throw std::invalid_argument("BollingerBands deviation factor must be non-negative");
        }
        
        // 设置参数
        setParam("period", static_cast<double>(period));
        setParam("dev_factor", dev_factor);
        setParam("use_incremental", use_incremental ? 1.0 : 0.0);
        
        // 最小周期
        setMinPeriod(period);
        
        // 初始化SMA指标
        sma_ = std::make_unique<SMA>(input, period, use_incremental);
        
        // 注意：std::deque没有reserve方法，不需要预分配
    }
    
    /**
     * @brief 获取周期
     * @return 计算周期
     */
    size_t getPeriod() const { return period_; }
    
    /**
     * @brief 获取标准差倍数
     * @return 标准差倍数
     */
    double getDevFactor() const { return dev_factor_; }
    
    /**
     * @brief 是否使用增量计算
     * @return true if using incremental calculation
     */
    bool isUsingIncremental() const { return use_incremental_; }
    
    /**
     * @brief 获取中轨值
     * @param ago 偏移量，默认0
     * @return 中轨值
     */
    double getMiddleBand(int ago = 0) const {
        return getOutputValue(0, ago);
    }
    
    /**
     * @brief 获取上轨值
     * @param ago 偏移量，默认0
     * @return 上轨值
     */
    double getUpperBand(int ago = 0) const {
        return getOutputValue(1, ago);
    }
    
    /**
     * @brief 获取下轨值
     * @param ago 偏移量，默认0
     * @return 下轨值
     */
    double getLowerBand(int ago = 0) const {
        return getOutputValue(2, ago);
    }
    
    /**
     * @brief 设置标准差倍数
     * @param dev_factor 新的标准差倍数
     */
    void setDevFactor(double dev_factor) {
        if (dev_factor < 0.0) {
            throw std::invalid_argument("BollingerBands deviation factor must be non-negative");
        }
        
        dev_factor_ = dev_factor;
        setParam("dev_factor", dev_factor);
    }
    
    /**
     * @brief 设置周期
     * @param period 新周期
     */
    void setPeriod(size_t period) {
        if (period == 0) {
            throw std::invalid_argument("BollingerBands period must be greater than 0");
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
        MultiLineIndicator::reset();
        
        if (sma_) {
            sma_->setPeriod(period_);  // 这会重置SMA
        }
        
        window_.clear();
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override;
    
    /**
     * @brief 批量计算（向量化版本）
     * @param start 起始位置
     * @param end 结束位置
     */
    void calculateBatch(size_t start, size_t end) override;
    
    /**
     * @brief 计算%B指标
     * @return %B值 (价格在布林带中的相对位置)
     */
    double getPercentB() const {
        try {
            auto input = getInput(0);
            double price = input->get(0);
            double upper = getUpperBand(0);
            double lower = getLowerBand(0);
            
            if (isNaN(price) || isNaN(upper) || isNaN(lower) || upper == lower) {
                return NaN;
            }
            
            return (price - lower) / (upper - lower);
            
        } catch (...) {
            return NaN;
        }
    }
    
    /**
     * @brief 计算带宽（Band Width）
     * @return 带宽值 (上轨和下轨的相对距离)
     */
    double getBandWidth() const {
        try {
            double upper = getUpperBand(0);
            double lower = getLowerBand(0);
            double middle = getMiddleBand(0);
            
            if (isNaN(upper) || isNaN(lower) || isNaN(middle) || middle == 0.0) {
                return NaN;
            }
            
            return (upper - lower) / middle;
            
        } catch (...) {
            return NaN;
        }
    }
    
    /**
     * @brief 检测布林带挤压（Squeeze）
     * @param threshold 挤压阈值，默认0.1
     * @return true if squeeze detected
     */
    bool detectSqueeze(double threshold = 0.1) const {
        double band_width = getBandWidth();
        return !isNaN(band_width) && band_width < threshold;
    }
    
    /**
     * @brief 检测布林带突破
     * @return 突破信号 (1.0=上突破, -1.0=下突破, 0.0=无突破)
     */
    double getBreakoutSignal() const {
        try {
            auto input = getInput(0);
            double price_current = input->get(0);
            double price_prev = input->get(-1);
            double upper = getUpperBand(0);
            double lower = getLowerBand(0);
            
            if (isNaN(price_current) || isNaN(price_prev) || 
                isNaN(upper) || isNaN(lower)) {
                return 0.0;
            }
            
            // 上突破：价格从内部突破上轨
            if (price_prev <= upper && price_current > upper) {
                return 1.0;
            }
            
            // 下突破：价格从内部突破下轨
            if (price_prev >= lower && price_current < lower) {
                return -1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 检测均值回归信号
     * @return 回归信号 (1.0=向上回归, -1.0=向下回归, 0.0=无信号)
     */
    double getMeanReversionSignal() const {
        try {
            auto input = getInput(0);
            double price = input->get(0);
            double upper = getUpperBand(0);
            double lower = getLowerBand(0);
            double middle = getMiddleBand(0);
            
            if (isNaN(price) || isNaN(upper) || isNaN(lower) || isNaN(middle)) {
                return 0.0;
            }
            
            double percent_b = getPercentB();
            
            if (isNaN(percent_b)) {
                return 0.0;
            }
            
            // 超买区域（%B > 0.8），预期向下回归
            if (percent_b > 0.8) {
                return -1.0;
            }
            
            // 超卖区域（%B < 0.2），预期向上回归
            if (percent_b < 0.2) {
                return 1.0;
            }
            
            return 0.0;
            
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief 获取当前标准差
     * @return 当前周期的标准差
     */
    double getCurrentStandardDeviation() const {
        double middle = getMiddleBand(0);
        if (isNaN(middle)) {
            return NaN;
        }
        
        return const_cast<BollingerBands*>(this)->calculateStandardDeviation(middle);
    }
    
private:
    /**
     * @brief 计算标准差
     * @param mean 均值
     * @return 标准差
     */
    double calculateStandardDeviation(double mean) const;
    
    /**
     * @brief 增量计算标准差
     * @param mean 均值
     * @return 标准差
     */
    double calculateStandardDeviationIncremental(double mean);
    
    /**
     * @brief 直接计算标准差
     * @param mean 均值
     * @return 标准差
     */
    double calculateStandardDeviationDirect(double mean);
    
    /**
     * @brief 获取趋势强度
     * @return 趋势强度值 (0-1)
     */
    double getTrendStrength() const;
    
    /**
     * @brief 设置所有输出为NaN
     */
    void setAllOutputsNaN();
};

} // namespace backtrader