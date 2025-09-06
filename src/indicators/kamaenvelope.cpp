#include "indicators/kamaenvelope.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

KAMAEnvelope::KAMAEnvelope() : Indicator() {
    setup_lines();
    _minperiod(params.period + 1);  // KAMA needs period + 1
}

KAMAEnvelope::KAMAEnvelope(std::shared_ptr<DataSeries> data_source, 
                          int period, int fast, int slow, double perc) : KAMAEnvelope() {
    params.period = period;
    params.fast = fast;
    params.slow = slow;
    params.perc = perc;
    
    setup_lines();
    _minperiod(period + 1);  // KAMA needs period + 1
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        
        // Initialize KAMA with the same data
        kama_ = std::make_shared<KAMA>(lineseries, period, fast, slow);
    }
}

KAMAEnvelope::KAMAEnvelope(std::shared_ptr<LineSeries> price_series, 
                          int period, int fast, int slow, double perc) : KAMAEnvelope() {
    params.period = period;
    params.fast = fast;
    params.slow = slow;
    params.perc = perc;
    
    setup_lines();
    _minperiod(period + 1);  // KAMA needs period + 1
    
    if (price_series) {
        this->data = price_series;
        this->datas.push_back(price_series);
        
        // Initialize KAMA with the same data
        kama_ = std::make_shared<KAMA>(price_series, period, fast, slow);
    }
}

void KAMAEnvelope::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // mid
        lines->add_line(std::make_shared<LineBuffer>());  // top
        lines->add_line(std::make_shared<LineBuffer>());  // bot
    }
}

double KAMAEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(Lines::mid);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*mid_line)[ago];
}

int KAMAEnvelope::getMinPeriod() const {
    // KAMA needs period + 1 for direction calculation
    return params.period + 1;
}

void KAMAEnvelope::calculate() {
    std::cout << "KAMAEnvelope::calculate() called" << std::endl;
    
    if (!kama_) {
        std::cout << "  ERROR: kama_ is null!" << std::endl;
        return;
    }
    
    // Make sure KAMA has the same data
    if (kama_->datas.empty() && !datas.empty()) {
        std::cout << "  Setting KAMA data" << std::endl;
        kama_->datas = datas;
        kama_->data = data;
    }
    
    std::cout << "  Calling kama_->calculate()" << std::endl;
    kama_->calculate();
    
    std::cout << "  KAMA size after calculate: " << kama_->size() << std::endl;
    
    // Now calculate envelope values
    if (!lines || lines->size() < 3) {
        std::cout << "  ERROR: lines not initialized properly" << std::endl;
        return;
    }
    
    auto mid_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Lines::mid));
    auto top_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Lines::top));
    auto bot_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Lines::bot));
    
    if (!mid_buffer || !top_buffer || !bot_buffer) {
        std::cout << "  ERROR: Could not get line buffers" << std::endl;
        return;
    }
    
    // Reset buffers
    mid_buffer->reset();
    top_buffer->reset();
    bot_buffer->reset();
    
    // Calculate envelope values for all KAMA values
    size_t kama_size = kama_->size();
    std::cout << "  Processing " << kama_size << " KAMA values" << std::endl;
    
    for (size_t i = 0; i < kama_size; ++i) {
        double kama_value = kama_->get(static_cast<int>(kama_size - i - 1));
        
        if (!std::isnan(kama_value)) {
            double perc_factor = params.perc / 100.0;
            double mid_value = kama_value;
            double upper_value = kama_value * (1.0 + perc_factor);
            double lower_value = kama_value * (1.0 - perc_factor);
            
            mid_buffer->append(mid_value);
            top_buffer->append(upper_value);
            bot_buffer->append(lower_value);
        } else {
            mid_buffer->append(std::numeric_limits<double>::quiet_NaN());
            top_buffer->append(std::numeric_limits<double>::quiet_NaN());
            bot_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Set buffer indices to end for proper ago indexing
    if (mid_buffer->size() > 0) {
        mid_buffer->set_idx(mid_buffer->size() - 1);
        top_buffer->set_idx(top_buffer->size() - 1);
        bot_buffer->set_idx(bot_buffer->size() - 1);
    }
    
    std::cout << "  KAMAEnvelope calculate complete: " << mid_buffer->size() << " values" << std::endl;
}

double KAMAEnvelope::getMidLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(Lines::mid);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*mid_line)[ago];
}

double KAMAEnvelope::getUpperLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto top_line = lines->getline(Lines::top);
    if (!top_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*top_line)[ago];
}

double KAMAEnvelope::getLowerLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto bot_line = lines->getline(Lines::bot);
    if (!bot_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*bot_line)[ago];
}

void KAMAEnvelope::next() {
    if (!kama_) return;
    
    // Get KAMA value
    double kama_value = kama_->get(0);
    
    if (!std::isnan(kama_value)) {
        double perc_factor = params.perc / 100.0;
        
        // Calculate envelope values
        double mid_value = kama_value;
        double upper_value = kama_value * (1.0 + perc_factor);
        double lower_value = kama_value * (1.0 - perc_factor);
        
        // Set line values using the standard pattern
        if (lines && lines->size() > Lines::mid) {
            auto mid_line = lines->getline(Lines::mid);
            if (mid_line) mid_line->set(0, mid_value);
        }
        
        if (lines && lines->size() > Lines::top) {
            auto top_line = lines->getline(Lines::top);
            if (top_line) top_line->set(0, upper_value);
        }
        
        if (lines && lines->size() > Lines::bot) {
            auto bot_line = lines->getline(Lines::bot);
            if (bot_line) bot_line->set(0, lower_value);
        }
    }
}

void KAMAEnvelope::once(int start, int end) {
    for (int i = start; i <= end; ++i) {
        next();
    }
}

size_t KAMAEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto mid_line = lines->getline(mid);
    return mid_line ? mid_line->size() : 0;
}

} // namespace indicators
} // namespace backtrader