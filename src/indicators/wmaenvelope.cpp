#include "indicators/wmaenvelope.h"
#include "linebuffer.h"
#include "dataseries.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

WMAEnvelope::WMAEnvelope() : Indicator(), current_index_(0) {
    // Setup lines
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    // Add 3 lines: mid, upper, lower
    lines->add_line(std::make_shared<LineBuffer>()); // mid (WMA)
    lines->add_line(std::make_shared<LineBuffer>()); // upper
    lines->add_line(std::make_shared<LineBuffer>()); // lower
    
    _minperiod(params.period);
    
    // Initialize wma_ as nullptr - will be created when data is set
    wma_ = nullptr;
}

WMAEnvelope::WMAEnvelope(std::shared_ptr<LineIterator> data_line, 
                         int period, double perc) 
    : Indicator(), current_index_(0) {
    params.period = period;
    params.perc = perc;
    
    // Setup lines
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    // Add 3 lines: mid, upper, lower
    lines->add_line(std::make_shared<LineBuffer>()); // mid (WMA)
    lines->add_line(std::make_shared<LineBuffer>()); // upper
    lines->add_line(std::make_shared<LineBuffer>()); // lower
    
    _minperiod(params.period);
    
    // Create WMA indicator - cast to LineSeries for constructor compatibility
    auto data_series = std::dynamic_pointer_cast<LineSeries>(data_line);
    if (data_series) {
        wma_ = std::make_shared<WMA>(data_series, period);
        this->data = data_series;
        this->datas.push_back(data_series);
    } else {
        // Try DataSeries cast
        auto data_dataseries = std::dynamic_pointer_cast<DataSeries>(data_line);
        if (data_dataseries) {
            wma_ = std::make_shared<WMA>(data_dataseries, period);
            auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_dataseries);
            if (lineseries) {
                this->data = lineseries;
                this->datas.push_back(lineseries);
            }
        }
    }
}

WMAEnvelope::WMAEnvelope(std::shared_ptr<LineSeries> data_source, 
                         int period, double perc) 
    : Indicator(), current_index_(0) {
    params.period = period;
    params.perc = perc;
    
    // Setup lines
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    // Add 3 lines: mid, upper, lower
    lines->add_line(std::make_shared<LineBuffer>()); // mid (WMA)
    lines->add_line(std::make_shared<LineBuffer>()); // upper
    lines->add_line(std::make_shared<LineBuffer>()); // lower
    
    _minperiod(params.period);
    
    // Create WMA directly with LineSeries - WMA should handle single line properly
    if (data_source) {
        wma_ = std::make_shared<WMA>(data_source, period);
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

WMAEnvelope::WMAEnvelope(std::shared_ptr<DataSeries> data_source, 
                         int period, double perc) 
    : Indicator(), current_index_(0) {
    params.period = period;
    params.perc = perc;
    
    // Setup lines
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    // Add 3 lines: mid, upper, lower
    lines->add_line(std::make_shared<LineBuffer>()); // mid (WMA)
    lines->add_line(std::make_shared<LineBuffer>()); // upper
    lines->add_line(std::make_shared<LineBuffer>()); // lower
    
    _minperiod(params.period);
    
    // Create WMA indicator with DataSeries (cast to LineSeries since DataSeries derives from LineSeries)
    if (data_source) {
        auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (lineseries) {
            wma_ = std::make_shared<WMA>(lineseries, period);
            this->data = lineseries;
            this->datas.push_back(lineseries);
        }
    }
}

double WMAEnvelope::get(int ago) const {
    return getMid(ago);
}

double WMAEnvelope::getMid(int ago) const {
    if (!lines || lines->size() < 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto mid_line = lines->getline(mid);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*mid_line)[ago];
}

double WMAEnvelope::getUpper(int ago) const {
    if (!lines || lines->size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto upper_line = lines->getline(upper);
    if (!upper_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*upper_line)[ago];
}

double WMAEnvelope::getLower(int ago) const {
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto lower_line = lines->getline(lower);
    if (!lower_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*lower_line)[ago];
}

std::shared_ptr<LineSingle> WMAEnvelope::getLine(size_t idx) const {
    if (!lines || idx >= lines->size()) {
        return nullptr;
    }
    return lines->getline(idx);
}

void WMAEnvelope::calculate() {
    // Create WMA if not already created (for default constructor case)
    if (!wma_ && !datas.empty() && datas[0]) {
        wma_ = std::make_shared<WMA>(datas[0], params.period);
    }
    
    if (!wma_) {
        return;
    }
    if (datas.empty()) {
        return;
    }
    if (!datas[0]->lines) {
        return;
    }
    
    // Get the appropriate line based on the data type
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() >= 5) {
        // This is a DataSeries with OHLC data, use close
        data_line = datas[0]->lines->getline(4); // Close price
    } else if (datas[0]->lines->size() > 0) {
        // This is a simple LineSeries, use the first line
        data_line = datas[0]->lines->getline(0);
    }
    
    if (!data_line || data_line->size() == 0) {
        return;
    }
    
    // Use once() to calculate all values at once
    once(0, data_line->size());
}

size_t WMAEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto mid_line = lines->getline(mid);
    if (!mid_line) return 0;
    
    // Return the actual size of the mid line buffer
    auto mid_buffer = std::dynamic_pointer_cast<LineBuffer>(mid_line);
    if (!mid_buffer) return 0;
    
    // Return the size minus 1 (to exclude the initial NaN that LineBuffer adds)
    size_t buffer_size = mid_buffer->size();
    if (buffer_size > 0) {
        return buffer_size - 1;
    }
    return 0;
}

void WMAEnvelope::next() {
    calculate();
}

void WMAEnvelope::once(int start, int end) {
    // Create WMA if not already created (for default constructor case)
    if (!wma_ && !datas.empty() && datas[0]) {
        wma_ = std::make_shared<WMA>(datas[0], params.period);
    }
    
    if (!wma_ || datas.empty() || !datas[0]->lines) return;
    
    // Get the appropriate line based on the data type
    std::shared_ptr<LineSingle> data_line;
    if (datas[0]->lines->size() >= 5) {
        // This is a DataSeries with OHLC data, use close
        data_line = datas[0]->lines->getline(4); // Close price
    } else if (datas[0]->lines->size() > 0) {
        // This is a simple LineSeries, use the first line
        data_line = datas[0]->lines->getline(0);
    }
    
    if (!data_line) return;
    
    auto mid_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(mid));
    auto upper_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(upper));
    auto lower_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(lower));
    
    if (!mid_buffer || !upper_buffer || !lower_buffer) return;
    
    // Calculate WMA first
    wma_->calculate();
    
    // Get all WMA values
    size_t wma_size = wma_->size();
    if (wma_size == 0) return;
    
    // Get the WMA line directly
    auto wma_line = wma_->lines->getline(0);
    auto wma_buffer = std::dynamic_pointer_cast<LineBuffer>(wma_line);
    if (!wma_buffer) return;
    
    // Copy WMA values and calculate envelopes
    // The WMA buffer already has the correct values in the correct order
    const auto& wma_array = wma_buffer->array();
    for (size_t i = 0; i < wma_array.size(); ++i) {
        double wma_val = wma_array[i];
        
        if (std::isnan(wma_val)) {
            mid_buffer->append(std::numeric_limits<double>::quiet_NaN());
            upper_buffer->append(std::numeric_limits<double>::quiet_NaN());
            lower_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            double envelope_offset = wma_val * (params.perc / 100.0);
            mid_buffer->append(wma_val);
            upper_buffer->append(wma_val + envelope_offset);
            lower_buffer->append(wma_val - envelope_offset);
        }
    }
    
    // Set the buffer index to the last appended element
    // After appending all values, the index should be at the last valid position
    if (mid_buffer->size() > 0) {
        int last_idx = mid_buffer->size() - 1;
        mid_buffer->set_idx(last_idx, true);
        upper_buffer->set_idx(last_idx, true);
        lower_buffer->set_idx(last_idx, true);
    }
}

} // namespace indicators
} // namespace backtrader