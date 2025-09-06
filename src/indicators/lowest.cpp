#include "indicators/lowest.h"
#include "linebuffer.h"
#include "lineroot.h"
#include "dataseries.h"
#include <algorithm>
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

Lowest::Lowest() 
    : OperationN(), data_source_(nullptr), current_index_(0) {
    // Ensure lines are set up
    if (lines->size() == 0) {
        setup_lines();
    }
    if (lines->size() > 0) {
        lines->add_alias("lowest", 0);
    }
}

Lowest::Lowest(std::shared_ptr<LineSeries> data_source, int period)
    : OperationN(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    if (lines->size() > 0) {
        lines->add_alias("lowest", 0);
    }
    
    // Set data for OperationN
    this->data = data_source;
    this->datas.push_back(data_source);
}

// DataSeries constructors for disambiguation
Lowest::Lowest(std::shared_ptr<DataSeries> data_source) 
    : OperationN(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    if (lines->size() > 0) {
        lines->add_alias("lowest", 0);
    }
    
    // Set data for OperationN
    this->data = data_source;
    this->datas.push_back(data_source);
}

Lowest::Lowest(std::shared_ptr<DataSeries> data_source, int period) 
    : OperationN(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    if (lines->size() > 0) {
        lines->add_alias("lowest", 0);
    }
    
    // Set data for OperationN
    this->data = data_source;
    this->datas.push_back(data_source);
}

double Lowest::get(int ago) const {
    std::cerr << "Lowest::get(" << ago << ") called" << std::endl;
    if (!lines || lines->size() == 0) {
        std::cerr << "  No lines!" << std::endl;
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto lowest_line = lines->getline(lowest);
    if (!lowest_line || lowest_line->size() == 0) {
        std::cerr << "  No lowest line or empty!" << std::endl;
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Debug: check line content
    auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(lowest_line);
    if (line_buffer) {
        std::cerr << "  Line buffer size: " << line_buffer->array().size() 
                  << ", idx: " << line_buffer->get_idx() << std::endl;
    }
    
    // Backtrader Python uses a special indexing scheme:
    // - indicator[0] is the most recent value
    // - indicator[-1] is also the most recent value
    // - indicator[-2] is 1 bar ago, etc.
    if (ago < 0) {
        // Convert Python negative index to positive ago
        // Python Lowest[-225] means 224 bars ago (not 225!)
        int bars_ago = -ago - 1;
        double result = (*lowest_line)[bars_ago];
        std::cerr << "  Negative ago, returning value: " << result << std::endl;
        return result;
    } else {
        // Non-negative indices: 0 is current, 1 is 1 bar ago
        // LineBuffer uses: negative indices for past, positive for future
        // So we need to negate the ago value
        double result = (*lowest_line)[-ago];
        std::cerr << "  Positive ago, accessing index " << -ago << ", returning value: " << result << std::endl;
        return result;
    }
}

int Lowest::getMinPeriod() const {
    return params.period;
}

double Lowest::calculate_func(const std::vector<double>& data) {
    if (data.empty()) return std::numeric_limits<double>::quiet_NaN();
    return *std::min_element(data.begin(), data.end());
}

size_t Lowest::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto lowest_line = lines->getline(0);
    if (!lowest_line) {
        return 0;
    }
    return lowest_line->size();
}

void Lowest::calculate() {
    std::cerr << "Lowest::calculate() called" << std::endl;
    if (data_source_) {
        std::cerr << "  Using data_source_ path" << std::endl;
        // For LineSeries constructor, process entire dataset
        if (data_source_->lines && data_source_->lines->size() > 0) {
            auto data_line = data_source_->lines->getline(0);
            std::cerr << "  Got data line, checking buffer" << std::endl;
            
            // Set up datas for OperationN
            if (datas.empty()) {
                datas.push_back(data_source_);
            }
            
            // Calculate Lowest for the entire dataset using once() method
            // Use array size instead of size() because data line idx might be -1
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer) {
                size_t array_size = data_buffer->array().size();
                std::cerr << "  Calling once(0, " << array_size << ")" << std::endl;
                once(0, array_size);
            } else {
                std::cerr << "  No buffer, using size()" << std::endl;
                once(0, data_line->size());
            }
        }
    } else if (data && data->lines && data->lines->size() > 0) {
        std::cerr << "  Using data path (DataSeries)" << std::endl;
        // For DataSeries constructor, calculate for entire dataset
        // Use the correct line based on get_dataseries_line_index()
        std::shared_ptr<LineSingle> data_line;
        if (data->lines->size() > 4) {
            // For DataSeries: use low line (index 3)
            data_line = data->lines->getline(get_dataseries_line_index());
        } else {
            // For LineSeries: use line 0
            data_line = data->lines->getline(0);
        }
        
        if (data_line) {
            // Debug: Check data line content
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer) {
                const auto& array = data_buffer->array();
                std::cerr << "Lowest::calculate() - data line has " << array.size() 
                          << " values, idx=" << data_buffer->get_idx() << std::endl;
                if (array.size() > 130) {
                    std::cerr << "  Value at position 130: " << array[130] << std::endl;
                }
            }
            
            // Set up datas for OperationN if not already done
            if (datas.empty()) {
                datas.push_back(data);
            }
            
            // Calculate Lowest for the entire dataset using once() method
            // Use array size instead of size() because data line idx might be -1
            auto data_buffer_2 = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer_2) {
                once(0, data_buffer_2->array().size());
            } else {
                once(0, data_line->size());
            }
        }
    } else {
        // Use base class implementation for standard case
        OperationN::next();
    }
}

void Lowest::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

} // namespace indicators
} // namespace backtrader