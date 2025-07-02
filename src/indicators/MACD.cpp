#include "indicators/MACD.h"
#include "Common.h"

namespace backtrader {

void MACD::calculate() {
    if (!hasValidInput()) {
        setOutput(0, NaN);  // MACD线
        setOutput(1, NaN);  // Signal线
        setOutput(2, NaN);  // Histogram线
        return;
    }
    
    auto input = getInput(0);
    double current_value = input->get(0);
    
    if (isNaN(current_value)) {
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
    
    // 计算MACD线 = 快速EMA - 慢速EMA
    double macd_value = fast_ema_value - slow_ema_value;
    setOutput(0, macd_value);
    
    // 将MACD值添加到MACD数据线中，用于Signal计算
    macd_line_->forward(macd_value);
    
    // 计算Signal线（MACD的EMA）
    signal_ema_->calculate();
    double signal_value = signal_ema_->get(0);
    
    if (isNaN(signal_value)) {
        setOutput(1, NaN);
        setOutput(2, NaN);
    } else {
        setOutput(1, signal_value);
        
        // 计算Histogram = MACD - Signal
        double histogram_value = macd_value - signal_value;
        setOutput(2, histogram_value);
    }
}

} // namespace backtrader