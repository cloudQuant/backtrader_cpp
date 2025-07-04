#include "indicators/prettygoodoscillator.h"

namespace backtrader {

// PrettyGoodOscillator implementation
PrettyGoodOscillator::PrettyGoodOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period);
    
    // Create component indicators
    sma_ = std::make_shared<SMA>();
    sma_->params.period = params.period;
    
    atr_ = std::make_shared<ATR>();
    atr_->params.period = params.period;
}

void PrettyGoodOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PrettyGoodOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    auto pgo_line = lines->getline(Lines::pgo);
    
    if (!pgo_line) return;
    
    // Get close price
    double close = (*data_lines->getline(3))[0];  // Close
    
    // Set data for component indicators
    sma_->datas = datas;
    atr_->datas = datas;
    
    // Calculate SMA and ATR
    sma_->next();
    atr_->next();
    
    // Get values
    double sma_value = 0.0;
    double atr_value = 0.0;
    
    if (sma_->lines && sma_->lines->getline(0)) {
        sma_value = (*sma_->lines->getline(0))[0];
    }
    
    if (atr_->lines && atr_->lines->getline(0)) {
        atr_value = (*atr_->lines->getline(0))[0];
    }
    
    // Calculate PGO: (close - sma) / atr
    if (atr_value > 0.0) {
        pgo_line->set(0, (close - sma_value) / atr_value);
    } else {
        pgo_line->set(0, 0.0);
    }
}

void PrettyGoodOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    auto pgo_line = lines->getline(Lines::pgo);
    
    if (!pgo_line) return;
    
    // Set data for component indicators
    sma_->datas = datas;
    atr_->datas = datas;
    
    // Calculate SMA and ATR for all bars
    sma_->once(0, end);
    atr_->once(0, end);
    
    if (!sma_->lines || !sma_->lines->getline(0) || 
        !atr_->lines || !atr_->lines->getline(0)) {
        return;
    }
    
    auto sma_line = sma_->lines->getline(0);
    auto atr_line = atr_->lines->getline(0);
    
    // Calculate PGO for each bar
    for (int i = start; i < end; ++i) {
        double close = (*data_lines->getline(3))[i];  // Close
        double sma_value = (*sma_line)[i];
        double atr_value = (*atr_line)[i];
        
        // Calculate PGO: (close - sma) / atr
        if (atr_value > 0.0) {
            pgo_line->set(i, (close - sma_value) / atr_value);
        } else {
            pgo_line->set(i, 0.0);
        }
    }
}

} // namespace backtrader