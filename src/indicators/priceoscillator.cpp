#include "indicators/priceoscillator.h"

namespace backtrader {

// PriceOscBase implementation
PriceOscBase::PriceOscBase() : Indicator() {
    // Create EMAs
    ma1_ = std::make_shared<EMA>();
    ma1_->params.period = params.period1;
    
    ma2_ = std::make_shared<EMA>();
    ma2_->params.period = params.period2;
    
    _minperiod(std::max(params.period1, params.period2));
}

void PriceOscBase::prenext() {
    if (ma1_) ma1_->prenext();
    if (ma2_) ma2_->prenext();
    Indicator::prenext();
}

void PriceOscBase::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to EMAs if not already done
    if (ma1_->datas.empty() && !datas.empty()) {
        ma1_->datas = datas;
    }
    if (ma2_->datas.empty() && !datas.empty()) {
        ma2_->datas = datas;
    }
    
    // Update EMAs
    ma1_->next();
    ma2_->next();
    
    // Calculate oscillator
    calculate_oscillator();
}

void PriceOscBase::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to EMAs if not already done
    if (ma1_->datas.empty() && !datas.empty()) {
        ma1_->datas = datas;
    }
    if (ma2_->datas.empty() && !datas.empty()) {
        ma2_->datas = datas;
    }
    
    // Calculate EMAs
    ma1_->once(start, end);
    ma2_->once(start, end);
    
    // Calculate oscillator for all values
    for (int i = start; i < end; ++i) {
        calculate_oscillator();
    }
}

// PriceOscillator implementation
PriceOscillator::PriceOscillator() : PriceOscBase() {
    setup_lines();
}

void PriceOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PriceOscillator::calculate_oscillator() {
    auto po_line = lines->getline(po);
    auto ma1_line = ma1_->lines->getline(EMA::ema);
    auto ma2_line = ma2_->lines->getline(EMA::ema);
    
    if (po_line && ma1_line && ma2_line) {
        double ma1_value = (*ma1_line)[0];
        double ma2_value = (*ma2_line)[0];
        po_line->set(0, ma1_value - ma2_value);
    }
}

// PercentagePriceOscillator implementation
PercentagePriceOscillator::PercentagePriceOscillator(bool use_long_denominator) 
    : PriceOscBase(), use_long_denominator_(use_long_denominator) {
    setup_lines();
    
    // Create signal line EMA
    signal_ema_ = std::make_shared<EMA>();
    signal_ema_->params.period = params.period_signal;
    
    _minperiod(std::max(params.period1, params.period2) + params.period_signal - 1);
}

void PercentagePriceOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PercentagePriceOscillator::prenext() {
    PriceOscBase::prenext();
    if (signal_ema_) signal_ema_->prenext();
}

void PercentagePriceOscillator::next() {
    PriceOscBase::next();
    
    // Connect PPO line to signal EMA if not already done
    if (signal_ema_->datas.empty() && lines) {
        signal_ema_->add_data(lines);
    }
    
    // Update signal EMA
    signal_ema_->next();
    
    // Calculate signal and histogram
    auto ppo_line = lines->getline(ppo);
    auto signal_line = lines->getline(signal);
    auto histo_line = lines->getline(histo);
    auto signal_ema_line = signal_ema_->lines->getline(EMA::ema);
    
    if (signal_line && signal_ema_line) {
        signal_line->set(0, (*signal_ema_line)[0]);
    }
    
    if (histo_line && ppo_line && signal_line) {
        histo_line->set(0, (*ppo_line)[0] - (*signal_line)[0]);
    }
}

void PercentagePriceOscillator::once(int start, int end) {
    PriceOscBase::once(start, end);
    
    // Connect PPO line to signal EMA if not already done
    if (signal_ema_->datas.empty() && lines) {
        signal_ema_->add_data(lines);
    }
    
    // Calculate signal EMA
    signal_ema_->once(start + std::max(params.period1, params.period2) - 1, end);
    
    // Calculate signal and histogram values
    auto ppo_line = lines->getline(ppo);
    auto signal_line = lines->getline(signal);
    auto histo_line = lines->getline(histo);
    auto signal_ema_line = signal_ema_->lines->getline(EMA::ema);
    
    if (signal_line && signal_ema_line && ppo_line && histo_line) {
        for (int i = start; i < end; ++i) {
            int signal_idx = i - start;
            if (signal_idx >= 0) {
                signal_line->set(i, (*signal_ema_line)[signal_idx]);
                histo_line->set(i, (*ppo_line)[i] - (*signal_line)[i]);
            }
        }
    }
}

void PercentagePriceOscillator::calculate_oscillator() {
    auto ppo_line = lines->getline(ppo);
    auto ma1_line = ma1_->lines->getline(EMA::ema);
    auto ma2_line = ma2_->lines->getline(EMA::ema);
    
    if (ppo_line && ma1_line && ma2_line) {
        double ma1_value = (*ma1_line)[0];
        double ma2_value = (*ma2_line)[0];
        double difference = ma1_value - ma2_value;
        
        // Choose denominator based on configuration
        double denominator = use_long_denominator_ ? ma2_value : ma1_value;
        
        if (denominator != 0.0) {
            ppo_line->set(0, 100.0 * difference / denominator);
        } else {
            ppo_line->set(0, 0.0);
        }
    }
}

// PercentagePriceOscillatorShort implementation
PercentagePriceOscillatorShort::PercentagePriceOscillatorShort() 
    : PercentagePriceOscillator(false) {  // Use short MA as denominator
}

} // namespace backtrader