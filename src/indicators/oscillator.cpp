#include "indicators/oscillator.h"
#include "indicators/sma.h"
#include "indicators/ema.h"
#include <limits>

namespace backtrader {
namespace indicators {

// Oscillator implementation
Oscillator::Oscillator() : Indicator(), data_source_(nullptr), base_indicator_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(1);
}

Oscillator::Oscillator(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), base_indicator_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(1);
}

Oscillator::Oscillator(std::shared_ptr<LineSeries> data_source, std::shared_ptr<Indicator> base_indicator) 
    : Indicator(), data_source_(data_source), base_indicator_(base_indicator), current_index_(0) {
    setup_lines();
    _minperiod(1);
}

double Oscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::osc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Oscillator::getMinPeriod() const {
    return 1;
}

void Oscillator::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void Oscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Oscillator::next() {
    if (datas.empty()) return;
    
    auto osc_line = lines->getline(Lines::osc);
    if (!osc_line) return;
    
    if (datas.size() > 1) {
        // Two data sources: osc = data0 - data1
        if (datas[0]->lines && datas[1]->lines) {
            auto data0_line = datas[0]->lines->getline(0);
            auto data1_line = datas[1]->lines->getline(0);
            
            if (data0_line && data1_line) {
                osc_line->set(0, (*data0_line)[0] - (*data1_line)[0]);
            }
        }
    } else {
        // One data source: assume it's an indicator with its own data
        // For now, just use the first data line
        if (datas[0]->lines) {
            auto data_line = datas[0]->lines->getline(0);
            if (data_line) {
                osc_line->set(0, (*data_line)[0]);
            }
        }
    }
}

void Oscillator::once(int start, int end) {
    if (datas.empty()) return;
    
    auto osc_line = lines->getline(Lines::osc);
    if (!osc_line) return;
    
    if (datas.size() > 1) {
        // Two data sources: osc = data0 - data1
        if (datas[0]->lines && datas[1]->lines) {
            auto data0_line = datas[0]->lines->getline(0);
            auto data1_line = datas[1]->lines->getline(0);
            
            if (data0_line && data1_line) {
                for (int i = start; i < end; ++i) {
                    osc_line->set(i, (*data0_line)[i] - (*data1_line)[i]);
                }
            }
        }
    } else {
        // One data source
        if (datas[0]->lines) {
            auto data_line = datas[0]->lines->getline(0);
            if (data_line) {
                for (int i = start; i < end; ++i) {
                    osc_line->set(i, (*data_line)[i]);
                }
            }
        }
    }
}

// SMAOscillator implementation
SMAOscillator::SMAOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void SMAOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SMAOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto osc_line = lines->getline(Lines::sma_osc);
    
    if (!data_line || !osc_line) return;
    
    // Calculate SMA
    double sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        sum += (*data_line)[-i];
    }
    double sma = sum / params.period;
    
    // Oscillator = data - sma
    osc_line->set(0, (*data_line)[0] - sma);
}

void SMAOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto osc_line = lines->getline(Lines::sma_osc);
    
    if (!data_line || !osc_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate SMA
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += (*data_line)[i - j];
        }
        double sma = sum / params.period;
        
        // Oscillator = data - sma
        osc_line->set(i, (*data_line)[i] - sma);
    }
}

// EMAOscillator implementation
EMAOscillator::EMAOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void EMAOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void EMAOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto osc_line = lines->getline(Lines::ema_osc);
    
    if (!data_line || !osc_line) return;
    
    // Calculate EMA
    static double ema = 0.0;
    static bool first_calc = true;
    double alpha = 2.0 / (params.period + 1.0);
    
    if (first_calc && data_line->size() >= static_cast<size_t>(params.period)) {
        // First EMA is SMA
        double sum = 0.0;
        for (int i = 0; i < params.period; ++i) {
            sum += (*data_line)[-i];
        }
        ema = sum / params.period;
        first_calc = false;
    } else if (!first_calc) {
        // EMA calculation
        ema = alpha * (*data_line)[0] + (1.0 - alpha) * ema;
    }
    
    // Oscillator = data - ema
    osc_line->set(0, (*data_line)[0] - ema);
}

void EMAOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto osc_line = lines->getline(Lines::ema_osc);
    
    if (!data_line || !osc_line) return;
    
    double alpha = 2.0 / (params.period + 1.0);
    double ema = 0.0;
    
    for (int i = start; i < end; ++i) {
        if (i < params.period - 1) {
            // Calculate SMA for initial values
            double sum = 0.0;
            int count = i + 1;
            for (int j = 0; j < count; ++j) {
                sum += (*data_line)[i - j];
            }
            ema = sum / count;
        } else if (i == params.period - 1) {
            // First full SMA
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += (*data_line)[i - j];
            }
            ema = sum / params.period;
        } else {
            // EMA calculation
            ema = alpha * (*data_line)[i] + (1.0 - alpha) * ema;
        }
        
        // Oscillator = data - ema
        osc_line->set(i, (*data_line)[i] - ema);
    }
}

} // namespace indicators
} // namespace backtrader