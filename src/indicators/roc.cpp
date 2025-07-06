#include "indicators/roc.h"
#include "linebuffer.h"
#include <cmath>

namespace backtrader {
namespace indicators {

RateOfChange::RateOfChange() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

RateOfChange::RateOfChange(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
}

RateOfChange::RateOfChange(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
}

RateOfChange::RateOfChange(std::shared_ptr<LineRoot> data)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
}

RateOfChange::RateOfChange(std::shared_ptr<LineRoot> data, int period)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
}

void RateOfChange::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double RateOfChange::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto roc_line = lines->getline(roc);
    if (!roc_line) {
        return 0.0;
    }
    
    return (*roc_line)[ago];
}

int RateOfChange::getMinPeriod() const {
    return params.period + 1;
}

void RateOfChange::calculate() {
    double price;
    
    // Get price data
    if (data_source_) {
        if (current_index_ >= data_source_->size()) return;
        price = (*data_source_)[current_index_];
    } else if (!datas.empty() && datas[0]->lines) {
        auto data_line = datas[0]->lines->getline(0);
        if (!data_line) return;
        price = (*data_line)[0];
    } else {
        return;
    }
    
    // Add to price buffer
    price_buffer_.push_back(price);
    if (price_buffer_.size() > static_cast<size_t>(params.period + 1)) {
        price_buffer_.pop_front();
    }
    
    // Calculate ROC when we have enough data
    double roc_value = 0.0;
    if (price_buffer_.size() == static_cast<size_t>(params.period + 1)) {
        double old_price = price_buffer_.front();
        if (old_price != 0.0) {
            // ROC = (current_price - old_price) / old_price * 100
            roc_value = ((price - old_price) / old_price) * 100.0;
        }
    }
    
    // Set the calculated value
    if (lines) {
        auto roc_line = lines->getline(roc);
        if (roc_line) {
            roc_line->set(0, roc_value);
        }
    }
    
    if (data_source_) {
        current_index_++;
    }
}

void RateOfChange::next() {
    calculate();
}

void RateOfChange::once(int start, int end) {
    // Reset state
    price_buffer_.clear();
    
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

} // namespace indicators
} // namespace backtrader