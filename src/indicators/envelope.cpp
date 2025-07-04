#include "indicators/envelope.h"
#include <limits>

namespace backtrader {
namespace indicators {

// Envelope implementation
Envelope::Envelope() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(1);
}

Envelope::Envelope(std::shared_ptr<LineSeries> data_source, double perc) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.perc = perc;
    setup_lines();
    _minperiod(1);
}

double Envelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::src);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Envelope::getMinPeriod() const {
    return 1;
}

void Envelope::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void Envelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Envelope::prenext() {
    Indicator::prenext();
}

void Envelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto src_line = lines->getline(Lines::src);
    auto top_line = lines->getline(Lines::top);
    auto bot_line = lines->getline(Lines::bot);
    
    if (data_line && src_line && top_line && bot_line) {
        double data_value = (*data_line)[0];
        double perc = params.perc / 100.0;
        
        src_line->set(0, data_value);
        top_line->set(0, data_value * (1.0 + perc));
        bot_line->set(0, data_value * (1.0 - perc));
    }
}

void Envelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto src_line = lines->getline(Lines::src);
    auto top_line = lines->getline(Lines::top);
    auto bot_line = lines->getline(Lines::bot);
    
    if (!data_line || !src_line || !top_line || !bot_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        double data_value = (*data_line)[i];
        
        src_line->set(i, data_value);
        top_line->set(i, data_value * (1.0 + perc));
        bot_line->set(i, data_value * (1.0 - perc));
    }
}

// SimpleMovingAverageEnvelope implementation
SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope() : Indicator() {
    setup_lines();
    
    sma_ = std::make_shared<SMA>(params.period);
    
    _minperiod(params.period);
}

void SimpleMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SimpleMovingAverageEnvelope::prenext() {
    // prenext() is protected, so we can't call it directly
    Indicator::prenext();
}

void SimpleMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to SMA if not already done
    if (sma_->datas.empty() && !datas.empty()) {
        sma_->datas = datas;
    }
    
    // Update SMA
    sma_->next();
    
    auto sma_line = lines->getline(Lines::sma);
    auto top_line = lines->getline(Lines::top);
    auto bot_line = lines->getline(Lines::bot);
    auto sma_indicator_line = sma_->lines->getline(0);
    
    if (sma_line && top_line && bot_line && sma_indicator_line) {
        double sma_value = (*sma_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        sma_line->set(0, sma_value);
        top_line->set(0, sma_value * (1.0 + perc));
        bot_line->set(0, sma_value * (1.0 - perc));
    }
}

void SimpleMovingAverageEnvelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to SMA if not already done
    if (sma_->datas.empty() && !datas.empty()) {
        sma_->datas = datas;
    }
    
    // Calculate SMA
    sma_->once(start, end);
    
    auto sma_line = lines->getline(Lines::sma);
    auto top_line = lines->getline(Lines::top);
    auto bot_line = lines->getline(Lines::bot);
    auto sma_indicator_line = sma_->lines->getline(0);
    
    if (!sma_line || !top_line || !bot_line || !sma_indicator_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        int sma_idx = i - start;
        if (sma_idx >= 0) {
            double sma_value = (*sma_indicator_line)[sma_idx];
            
            sma_line->set(i, sma_value);
            top_line->set(i, sma_value * (1.0 + perc));
            bot_line->set(i, sma_value * (1.0 - perc));
        }
    }
}

// ExponentialMovingAverageEnvelope implementation
ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope() : Indicator() {
    setup_lines();
    
    ema_ = std::make_shared<EMA>(params.period);
    
    _minperiod(params.period);
}

void ExponentialMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void ExponentialMovingAverageEnvelope::prenext() {
    // prenext() is protected, so we can't call it directly
    Indicator::prenext();
}

void ExponentialMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to EMA if not already done
    if (ema_->datas.empty() && !datas.empty()) {
        ema_->datas = datas;
    }
    
    // Update EMA
    ema_->next();
    
    auto ema_line = lines->getline(Lines::ema);
    auto top_line = lines->getline(Lines::top);
    auto bot_line = lines->getline(Lines::bot);
    auto ema_indicator_line = ema_->lines->getline(0);
    
    if (ema_line && top_line && bot_line && ema_indicator_line) {
        double ema_value = (*ema_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        ema_line->set(0, ema_value);
        top_line->set(0, ema_value * (1.0 + perc));
        bot_line->set(0, ema_value * (1.0 - perc));
    }
}

void ExponentialMovingAverageEnvelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to EMA if not already done
    if (ema_->datas.empty() && !datas.empty()) {
        ema_->datas = datas;
    }
    
    // Calculate EMA
    ema_->once(start, end);
    
    auto ema_line = lines->getline(Lines::ema);
    auto top_line = lines->getline(Lines::top);
    auto bot_line = lines->getline(Lines::bot);
    auto ema_indicator_line = ema_->lines->getline(0);
    
    if (!ema_line || !top_line || !bot_line || !ema_indicator_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        int ema_idx = i - start;
        if (ema_idx >= 0) {
            double ema_value = (*ema_indicator_line)[ema_idx];
            
            ema_line->set(i, ema_value);
            top_line->set(i, ema_value * (1.0 + perc));
            bot_line->set(i, ema_value * (1.0 - perc));
        }
    }
}

} // namespace indicators
} // namespace backtrader