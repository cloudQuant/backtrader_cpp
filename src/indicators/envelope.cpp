#include "indicators/envelope.h"
#include <limits>

namespace backtrader {
namespace indicators {

// Envelope implementation
Envelope::Envelope() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(1);
}

Envelope::Envelope(std::shared_ptr<LineRoot> data, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.perc = perc;
    setup_lines();
    _minperiod(1);
}

Envelope::Envelope(std::shared_ptr<LineRoot> data, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.perc = perc;
    setup_lines();
    _minperiod(period);
}

double Envelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(src);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int Envelope::getMinPeriod() const {
    return 1;
}

double Envelope::getMidLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(src);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

double Envelope::getUpperLine(int ago) const {
    if (!lines || lines->size() <= top) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(top);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

double Envelope::getLowerLine(int ago) const {
    if (!lines || lines->size() <= bot) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(bot);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
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
    auto src_line = lines->getline(src);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
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
    auto src_line = lines->getline(src);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
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
SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    sma_ = std::make_shared<SMA>(params.period);
    
    _minperiod(params.period);
}

SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    sma_ = std::make_shared<SMA>(data_source, params.period);
    
    _minperiod(params.period);
}

SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope(std::shared_ptr<LineRoot> data_source, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    sma_ = std::make_shared<SMA>(data_source, period);
    
    _minperiod(params.period);
}

double SimpleMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(sma);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int SimpleMovingAverageEnvelope::getMinPeriod() const {
    return params.period;
}

void SimpleMovingAverageEnvelope::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        sma_->calculate();
        current_index_++;
        
        // Calculate envelope bands
        auto sma_line = lines->getline(sma);
        auto top_line = lines->getline(top);
        auto bot_line = lines->getline(bot);
        auto sma_indicator_line = sma_->lines->getline(0);
        
        if (sma_line && top_line && bot_line && sma_indicator_line) {
            double sma_value = (*sma_indicator_line)[0];
            double perc = params.perc / 100.0;
            
            sma_line->set(0, sma_value);
            top_line->set(0, sma_value * (1.0 + perc));
            bot_line->set(0, sma_value * (1.0 - perc));
        }
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

std::shared_ptr<LineBuffer> SimpleMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
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
    
    // Update SMA using calculate method
    sma_->calculate();
    
    auto sma_line = lines->getline(sma);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
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
    
    // Calculate SMA for the range using calculate method
    for (int i = start; i < end; ++i) {
        sma_->calculate();
    }
    
    auto sma_line = lines->getline(sma);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto sma_indicator_line = sma_->lines->getline(0);
    
    if (!sma_line || !top_line || !bot_line || !sma_indicator_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        if (i < static_cast<int>(sma_indicator_line->size())) {
            double sma_value = (*sma_indicator_line)[i];
            
            sma_line->set(i, sma_value);
            top_line->set(i, sma_value * (1.0 + perc));
            bot_line->set(i, sma_value * (1.0 - perc));
        }
    }
}

// ExponentialMovingAverageEnvelope implementation
ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    ema_ = std::make_shared<EMA>(params.period);
    
    _minperiod(params.period);
}

ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    ema_ = std::make_shared<EMA>(data_source, params.period);
    
    _minperiod(params.period);
}

ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope(std::shared_ptr<LineRoot> data_source, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    ema_ = std::make_shared<EMA>(data_source);
    
    _minperiod(params.period);
}

double ExponentialMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ema);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

void ExponentialMovingAverageEnvelope::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        ema_->calculate();
        current_index_++;
        
        // Calculate envelope bands
        auto ema_line = lines->getline(ema);
        auto top_line = lines->getline(top);
        auto bot_line = lines->getline(bot);
        auto ema_indicator_line = ema_->lines->getline(0);
        
        if (ema_line && top_line && bot_line && ema_indicator_line) {
            double ema_value = (*ema_indicator_line)[0];
            double perc = params.perc / 100.0;
            
            ema_line->set(0, ema_value);
            top_line->set(0, ema_value * (1.0 + perc));
            bot_line->set(0, ema_value * (1.0 - perc));
        }
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

int ExponentialMovingAverageEnvelope::getMinPeriod() const {
    return params.period;
}

std::shared_ptr<LineBuffer> ExponentialMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
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
    
    // Update EMA using calculate method
    ema_->calculate();
    
    auto ema_line = lines->getline(ema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
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
    
    // Calculate EMA for the range using calculate method
    for (int i = start; i < end; ++i) {
        ema_->calculate();
    }
    
    auto ema_line = lines->getline(ema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto ema_indicator_line = ema_->lines->getline(0);
    
    if (!ema_line || !top_line || !bot_line || !ema_indicator_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        if (i < static_cast<int>(ema_indicator_line->size())) {
            double ema_value = (*ema_indicator_line)[i];
            
            ema_line->set(i, ema_value);
            top_line->set(i, ema_value * (1.0 + perc));
            bot_line->set(i, ema_value * (1.0 - perc));
        }
    }
}

// DoubleExponentialMovingAverageEnvelope implementation
DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>();
    
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>(data_source, params.period);
    
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope(std::shared_ptr<LineRoot> data_source, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>(data_source, period);
    
    _minperiod(2 * params.period - 1);
}

double DoubleExponentialMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(dema);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int DoubleExponentialMovingAverageEnvelope::getMinPeriod() const {
    return 2 * params.period - 1;
}

void DoubleExponentialMovingAverageEnvelope::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        dema_->calculate();
        current_index_++;
        
        // Calculate envelope bands
        auto dema_line = lines->getline(dema);
        auto top_line = lines->getline(top);
        auto bot_line = lines->getline(bot);
        auto dema_indicator_line = dema_->lines->getline(0);
        
        if (dema_line && top_line && bot_line && dema_indicator_line) {
            double dema_value = (*dema_indicator_line)[0];
            double perc = params.perc / 100.0;
            
            dema_line->set(0, dema_value);
            top_line->set(0, dema_value * (1.0 + perc));
            bot_line->set(0, dema_value * (1.0 - perc));
        }
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

std::shared_ptr<LineBuffer> DoubleExponentialMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
}

void DoubleExponentialMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void DoubleExponentialMovingAverageEnvelope::prenext() {
    Indicator::prenext();
}

void DoubleExponentialMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to DEMA if not already done
    if (dema_->datas.empty() && !datas.empty()) {
        dema_->datas = datas;
    }
    
    // Update DEMA using public calculate method
    dema_->calculate();
    
    auto dema_line = lines->getline(dema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto dema_indicator_line = dema_->lines->getline(0);
    
    if (dema_line && top_line && bot_line && dema_indicator_line) {
        double dema_value = (*dema_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        dema_line->set(0, dema_value);
        top_line->set(0, dema_value * (1.0 + perc));
        bot_line->set(0, dema_value * (1.0 - perc));
    }
}

void DoubleExponentialMovingAverageEnvelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to DEMA if not already done
    if (dema_->datas.empty() && !datas.empty()) {
        dema_->datas = datas;
    }
    
    // Calculate DEMA using public calculate method for the range
    for (int i = start; i < end; ++i) {
        dema_->calculate();
    }
    
    auto dema_line = lines->getline(dema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto dema_indicator_line = dema_->lines->getline(0);
    
    if (!dema_line || !top_line || !bot_line || !dema_indicator_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        if (i < static_cast<int>(dema_indicator_line->size())) {
            double dema_value = (*dema_indicator_line)[i];
            
            dema_line->set(i, dema_value);
            top_line->set(i, dema_value * (1.0 + perc));
            bot_line->set(i, dema_value * (1.0 - perc));
        }
    }
}

// TripleExponentialMovingAverageEnvelope implementation
TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>();
    
    _minperiod(3 * params.period - 2);
}

TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    
    _minperiod(3 * params.period - 2);
}

TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope(std::shared_ptr<LineRoot> data_source, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, period);
    
    _minperiod(3 * params.period - 2);
}

double TripleExponentialMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(tema);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int TripleExponentialMovingAverageEnvelope::getMinPeriod() const {
    return 3 * params.period - 2;
}

void TripleExponentialMovingAverageEnvelope::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

std::shared_ptr<LineSingle> TripleExponentialMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index >= lines->size()) {
        return nullptr;
    }
    return lines->getline(index);
}

void TripleExponentialMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // TEMA line
        lines->add_line(std::make_shared<LineBuffer>());  // Top envelope
        lines->add_line(std::make_shared<LineBuffer>());  // Bottom envelope
    }
}

void TripleExponentialMovingAverageEnvelope::prenext() {
    if (tema_) {
        tema_->calculate();
    }
}

void TripleExponentialMovingAverageEnvelope::next() {
    if (!tema_ || !lines) return;
    
    // Get the TEMA value
    double tema_value = tema_->get();
    
    // Calculate envelope percentage
    double perc = params.perc / 100.0;
    
    // Set the lines
    auto tema_line = lines->getline(tema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (tema_line && top_line && bot_line) {
        tema_line->set(0, tema_value);
        top_line->set(0, tema_value * (1.0 + perc));
        bot_line->set(0, tema_value * (1.0 - perc));
    }
}

void TripleExponentialMovingAverageEnvelope::once(int start, int end) {
    if (!tema_ || !lines) return;
    
    // Get the TEMA indicator line
    auto tema_indicator_line = tema_->getLine(0);
    if (!tema_indicator_line) return;
    
    auto tema_line = lines->getline(tema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (tema_line && top_line && bot_line) {
        double perc = params.perc / 100.0;
        
        for (int i = start; i < end; ++i) {
            double tema_value = (*tema_indicator_line)[i];
            
            tema_line->set(i, tema_value);
            top_line->set(i, tema_value * (1.0 + perc));
            bot_line->set(i, tema_value * (1.0 - perc));
        }
    }
}

// SmoothedMovingAverageEnvelope implementation
SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // SMMA will be initialized in next() when data is available
    smma_ = nullptr;
    
    _minperiod(params.period);
}

SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    smma_ = std::make_shared<SMMA>(data_source, params.period);
    
    _minperiod(params.period);
}

SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope(std::shared_ptr<LineRoot> data_source, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    smma_ = std::make_shared<SMMA>(data_source, period);
    
    _minperiod(params.period);
}

double SmoothedMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(smma);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int SmoothedMovingAverageEnvelope::getMinPeriod() const {
    return params.period;
}

void SmoothedMovingAverageEnvelope::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        smma_->calculate();
        current_index_++;
        
        // Calculate envelope bands
        auto smma_line = lines->getline(smma);
        auto top_line = lines->getline(top);
        auto bot_line = lines->getline(bot);
        auto smma_indicator_line = smma_->lines->getline(0);
        
        if (smma_line && top_line && bot_line && smma_indicator_line) {
            double smma_value = (*smma_indicator_line)[0];
            double perc = params.perc / 100.0;
            
            smma_line->set(0, smma_value);
            top_line->set(0, smma_value * (1.0 + perc));
            bot_line->set(0, smma_value * (1.0 - perc));
        }
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

std::shared_ptr<LineBuffer> SmoothedMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
}

void SmoothedMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SmoothedMovingAverageEnvelope::prenext() {
    // prenext() is protected, so we can't call it directly
    Indicator::prenext();
}

void SmoothedMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Initialize SMMA if not already done
    if (!smma_ && !datas.empty()) {
        smma_ = std::make_shared<SMMA>(datas[0], params.period);
    }
    
    // Connect data to SMMA if not already done
    if (smma_ && smma_->datas.empty() && !datas.empty()) {
        smma_->datas = datas;
    }
    
    // Update SMMA using calculate method
    if (smma_) {
        smma_->calculate();
    }
    
    auto smma_line = lines->getline(smma);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto smma_indicator_line = smma_->lines->getline(0);
    
    if (smma_line && top_line && bot_line && smma_indicator_line) {
        double smma_value = (*smma_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        smma_line->set(0, smma_value);
        top_line->set(0, smma_value * (1.0 + perc));
        bot_line->set(0, smma_value * (1.0 - perc));
    }
}

void SmoothedMovingAverageEnvelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Initialize SMMA if not already done
    if (!smma_ && !datas.empty()) {
        smma_ = std::make_shared<SMMA>(datas[0], params.period);
    }
    
    // Connect data to SMMA if not already done
    if (smma_ && smma_->datas.empty() && !datas.empty()) {
        smma_->datas = datas;
    }
    
    // Calculate SMMA for the range using calculate method
    if (smma_) {
        for (int i = start; i < end; ++i) {
            smma_->calculate();
        }
    }
    
    auto smma_line = lines->getline(smma);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto smma_indicator_line = smma_->lines->getline(0);
    
    if (!smma_line || !top_line || !bot_line || !smma_indicator_line) return;
    
    double perc = params.perc / 100.0;
    
    for (int i = start; i < end; ++i) {
        if (i < static_cast<int>(smma_indicator_line->size())) {
            double smma_value = (*smma_indicator_line)[i];
            
            smma_line->set(i, smma_value);
            top_line->set(i, smma_value * (1.0 + perc));
            bot_line->set(i, smma_value * (1.0 - perc));
        }
    }
}

} // namespace indicators
} // namespace backtrader