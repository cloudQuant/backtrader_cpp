#include "indicators/accdecoscillator.h"

namespace backtrader {

// AccelerationDecelerationOscillator implementation
AccelerationDecelerationOscillator::AccelerationDecelerationOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period + 5);  // AO needs 5 periods, plus SMA period
    
    // Create component indicators
    awesome_oscillator_ = std::make_shared<AwesomeOscillator>();
    
    sma_ = std::make_shared<SMA>();
    sma_->params.period = params.period;
}

void AccelerationDecelerationOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AccelerationDecelerationOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto accde_line = lines->getline(Lines::accde);
    if (!accde_line) return;
    
    // Set data for component indicators
    if (awesome_oscillator_ && sma_) {
        awesome_oscillator_->datas = datas;
        
        // Calculate Awesome Oscillator
        awesome_oscillator_->next();
        
        if (awesome_oscillator_->lines && awesome_oscillator_->lines->getline(0)) {
            double ao_value = (*awesome_oscillator_->lines->getline(0))[0];
            
            // Store AO values for SMA calculation
            static std::vector<double> ao_values;
            ao_values.push_back(ao_value);
            
            // Calculate SMA of AO if we have enough values
            if (ao_values.size() >= params.period) {
                double sum = 0.0;
                int start_idx = std::max(0, static_cast<int>(ao_values.size()) - params.period);
                for (int i = start_idx; i < ao_values.size(); ++i) {
                    sum += ao_values[i];
                }
                double ao_sma = sum / params.period;
                
                // AccDec = AO - SMA(AO)
                accde_line->set(0, ao_value - ao_sma);
            } else {
                accde_line->set(0, 0.0);
            }
            
            // Keep only necessary history
            if (ao_values.size() > params.period * 2) {
                ao_values.erase(ao_values.begin());
            }
        } else {
            accde_line->set(0, 0.0);
        }
    } else {
        accde_line->set(0, 0.0);
    }
}

void AccelerationDecelerationOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto accde_line = lines->getline(Lines::accde);
    if (!accde_line) return;
    
    // Set data for component indicators
    if (awesome_oscillator_ && sma_) {
        awesome_oscillator_->datas = datas;
        
        // Calculate Awesome Oscillator for all bars
        awesome_oscillator_->once(0, end);
        
        if (awesome_oscillator_->lines && awesome_oscillator_->lines->getline(0)) {
            auto ao_line = awesome_oscillator_->lines->getline(0);
            
            // Calculate AccDec for each bar
            for (int i = start; i < end; ++i) {
                double ao_value = (*ao_line)[i];
                
                // Calculate SMA of AO if we have enough data
                if (i >= params.period - 1) {
                    double sum = 0.0;
                    for (int j = 0; j < params.period; ++j) {
                        sum += (*ao_line)[i - j];
                    }
                    double ao_sma = sum / params.period;
                    
                    // AccDec = AO - SMA(AO)
                    accde_line->set(i, ao_value - ao_sma);
                } else {
                    accde_line->set(i, 0.0);
                }
            }
        } else {
            // Fill with zeros if AO calculation failed
            for (int i = start; i < end; ++i) {
                accde_line->set(i, 0.0);
            }
        }
    } else {
        // Fill with zeros if component indicators are not available
        for (int i = start; i < end; ++i) {
            accde_line->set(i, 0.0);
        }
    }
}

} // namespace backtrader