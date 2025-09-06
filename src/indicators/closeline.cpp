#include "indicators/closeline.h"
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

CloseLine::CloseLine() : Indicator() {
    setup_lines();
    _minperiod(1);  // No warmup period needed, just pass through the data
}

CloseLine::CloseLine(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(data_source) {
    setup_lines();
    _minperiod(1);
    
    // Set up the data connections
    this->data = data_source;
    this->datas.push_back(data_source);
    // Don't set _clock - let LineIterator use our own size() method
    // this->_clock = data_source;
}

void CloseLine::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_alias("close", 0);
    }
}

void CloseLine::prenext() {
    std::cerr << "CloseLine::prenext() called, this=" << this << std::endl;
    // During warm-up, just pass through the close value
    if (!data_source_) {
        std::cerr << "CloseLine::prenext() - NO data_source_!" << std::endl;
        return;
    }
    
    auto close_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(close));
    if (close_line) {
        double close_val = data_source_->close(0);
        close_line->append(close_val);
        
        static int count = 0;
        count++;
        if (count <= 10) {
            std::cerr << "CloseLine::prenext() #" << count << " - appended close_val=" << close_val 
                      << ", buffer size=" << close_line->size() << std::endl;
        }
    } else {
        std::cerr << "CloseLine::prenext() - NO close_line buffer!" << std::endl;
    }
}

void CloseLine::nextstart() {
    next();
}

void CloseLine::next() {
    std::cerr << "CloseLine::next() called, this=" << this << std::endl;
    if (!data_source_) {
        std::cerr << "CloseLine::next() - NO data_source_!" << std::endl;
        return;
    }
    
    auto close_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(close));
    if (close_line) {
        double close_val = data_source_->close(0);
        close_line->append(close_val);
        
        static int count = 0;
        count++;
        if (count <= 10) {
            std::cerr << "CloseLine::next() #" << count << " - appended close_val=" << close_val 
                      << ", buffer size=" << close_line->size() << std::endl;
        }
    } else {
        std::cerr << "CloseLine::next() - NO close_line buffer!" << std::endl;
    }
}

void CloseLine::once(int start, int end) {
    if (!data_source_) return;
    
    auto close_line = lines->getline(close);
    if (!close_line) return;
    
    // Ensure the line has the correct size
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (close_buffer) {
        close_buffer->array().clear();
        close_buffer->array().resize(end, 0.0);
    }
    
    // Copy all close values from the data source
    for (int i = start; i < end; ++i) {
        // In once mode, we need to access the data at absolute index
        double close_val = 0.0;
        if (data_source_->lines && data_source_->lines->size() > DataSeries::Close) {
            auto data_close_line = data_source_->lines->getline(DataSeries::Close);
            if (auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_close_line)) {
                const auto& array = data_buffer->array();
                if (i >= 0 && i < static_cast<int>(array.size())) {
                    close_val = array[i];
                }
            }
        }
        close_line->set(i, close_val);
    }
}

void CloseLine::_once() {
    // Get data's buflen
    size_t data_buflen = 0;
    if (data_source_) {
        data_buflen = data_source_->buflen();
    }
    
    // Calculate all values from the beginning
    if (data_buflen > 0) {
        once(0, data_buflen);
    }
    
    // Position at the last valid index
    auto close_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(close));
    if (close_line && close_line->array().size() > 0) {
        close_line->set_idx(close_line->array().size() - 1);
    }
    
    // Execute binding synchronization if any
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

double CloseLine::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto close_line = lines->getline(close);
    if (!close_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*close_line)[ago];
}

size_t CloseLine::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto close_line = lines->getline(close);
    return close_line ? close_line->size() : 0;
}

} // namespace indicators
} // namespace backtrader