#include "indicators/RSI.h"
#include <cmath>

namespace backtrader {

void RSI::calculateWithWilders(double current_price) {
    // 计算价格变化
    double change = current_price - prev_price_;
    double gain = change > 0 ? change : 0.0;
    double loss = change < 0 ? -change : 0.0;
    
    count_++;
    
    // 初始period个数据，计算简单平均
    if (count_ <= period_) {
        avg_gain_ += gain;
        avg_loss_ += loss;
        
        if (count_ == period_) {
            // 计算初始平均值
            avg_gain_ /= period_;
            avg_loss_ /= period_;
            
            // 计算RSI
            if (avg_loss_ != 0.0) {
                double rs = avg_gain_ / avg_loss_;
                double rsi = 100.0 - (100.0 / (1.0 + rs));
                setOutput(0, rsi);
            } else {
                // 没有损失，RSI = 100
                setOutput(0, 100.0);
            }
        } else {
            setOutput(0, NaN);
        }
    } else {
        // Wilder's平滑：新平均 = ((n-1) * 旧平均 + 新值) / n
        double alpha = 1.0 / period_;
        avg_gain_ = (1.0 - alpha) * avg_gain_ + alpha * gain;
        avg_loss_ = (1.0 - alpha) * avg_loss_ + alpha * loss;
        
        // 计算RSI
        if (avg_loss_ != 0.0) {
            double rs = avg_gain_ / avg_loss_;
            double rsi = 100.0 - (100.0 / (1.0 + rs));
            setOutput(0, rsi);
        } else {
            // 没有损失，RSI = 100
            setOutput(0, 100.0);
        }
    }
}

void RSI::calculateWithEMA(double current_price) {
    // 计算价格变化
    double change = current_price - prev_price_;
    double gain = change > 0 ? change : 0.0;
    double loss = change < 0 ? -change : 0.0;
    
    // 将收益和损失添加到对应的数据线
    gain_line_->forward(gain);
    loss_line_->forward(loss);
    
    // 计算收益和损失的EMA
    gain_ema_->calculate();
    loss_ema_->calculate();
    
    count_++;
    
    // 需要足够的数据点才能计算有效的RSI
    if (count_ >= period_) {
        double avg_gain = gain_ema_->get(0);
        double avg_loss = loss_ema_->get(0);
        
        if (!isNaN(avg_gain) && !isNaN(avg_loss)) {
            if (avg_loss != 0.0) {
                double rs = avg_gain / avg_loss;
                double rsi = 100.0 - (100.0 / (1.0 + rs));
                setOutput(0, rsi);
            } else {
                // 没有损失，RSI = 100
                setOutput(0, 100.0);
            }
        } else {
            setOutput(0, NaN);
        }
    } else {
        setOutput(0, NaN);
    }
}

} // namespace backtrader