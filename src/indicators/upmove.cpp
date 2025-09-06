#include "indicators/upmove.h"
#include "dataseries.h"
#include "lineseries.h"
#include <limits>
#include <iostream>

namespace backtrader {
namespace indicators {

// UpMove implementation
UpMove::UpMove() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(2);
}

UpMove::UpMove(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    _minperiod(2);
    
    // Store data source for compatibility
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
    
    // Calculate immediately upon construction if we have data
    if (data_source && data_source->lines && data_source->lines->size() > 0) {
        calculate();
    }
}

UpMove::UpMove(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::dynamic_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    _minperiod(2);
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

UpMove::UpMove(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::dynamic_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    _minperiod(2);
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

double UpMove::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(upmove);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        // LineBuffer's operator[] expects ago convention:
        // ago = 0 => current position (_idx)
        // ago < 0 => past values (e.g., -1 = previous)
        // ago > 0 => future values (rare)
        return buffer->get(ago);
    }
    
    return (*line)[ago];
}

int UpMove::getMinPeriod() const {
    return 2;
}

void UpMove::calculate() {
    // Handle direct data access through data or datas
    std::shared_ptr<LineSingle> data_line = nullptr;
    
    if (data && data->lines && data->lines->size() > 0) {
        // Use the close price line (index 4) for OHLC data, or primary line (index 0) for single line data
        data_line = data->lines->getline(data->lines->size() > 4 ? DataSeries::Close : 0);
    } else if (!datas.empty() && datas[0] && datas[0]->lines && datas[0]->lines->size() > 0) {
        // Use the first data series from datas vector
        data = datas[0];
        data_line = data->lines->getline(data->lines->size() > 4 ? DataSeries::Close : 0);
    } else if (data_source_) {
        // Use the data_source_ directly if it's a LineSeries
        if (data_source_->lines && data_source_->lines->size() > 0) {
            data_line = data_source_->lines->getline(0);
        }
    }
    
    if (!data_line || data_line->size() == 0) {
        return;
    }
    
    auto upmove_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!upmove_line) return;
    
    // Get LineBuffer for direct array access
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // Clear and recalculate all values
    upmove_line->reset();
    
    const auto& data_array = data_buffer->array();
    int data_size = static_cast<int>(data_array.size());
    
    // First value is NaN (no previous value for comparison)
    upmove_line->append(std::numeric_limits<double>::quiet_NaN());
    
    // Process remaining data points in chronological order
    for (int i = 1; i < data_size; ++i) {
        // UpMove = current - previous (can be negative in Python implementation!)
        double current_val = data_array[i];
        double prev_val = data_array[i-1];
        
        if (std::isnan(current_val) || std::isnan(prev_val)) {
            upmove_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // UpMove = current - previous (can be negative!)
            double upmove = current_val - prev_val;
            upmove_line->append(upmove);
        }
    }
    
    // Set the LineBuffer's _idx to the last element
    upmove_line->set_idx(upmove_line->size() - 1);
}

size_t UpMove::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto upmove_line = lines->getline(upmove);
    if (!upmove_line) {
        return 0;
    }
    return upmove_line->size();
}

void UpMove::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void UpMove::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? DataSeries::Close : 0);  // Close line
    auto upmove_line = lines->getline(upmove);
    
    if (!data_line || !upmove_line) return;
    
    if (data_line->size() < 2) {
        upmove_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // UpMove = close - close[-1] (can be negative!)
    double current_close = (*data_line)[0];
    double prev_close = (*data_line)[-1];
    
    if (std::isnan(current_close) || std::isnan(prev_close)) {
        upmove_line->set(0, std::numeric_limits<double>::quiet_NaN());
    } else {
        double upmove = current_close - prev_close;
        upmove_line->set(0, upmove);
    }
}

void UpMove::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? DataSeries::Close : 0);  // Close line
    auto upmove_line = lines->getline(upmove);
    
    if (!data_line || !upmove_line) return;
    
    // Get LineBuffer for direct array access
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    const auto& data_array = data_buffer->array();
    
    for (int i = start; i < end && i < static_cast<int>(data_array.size()); ++i) {
        if (i == 0 || std::isnan(data_array[i]) || std::isnan(data_array[i-1])) {
            upmove_line->set(i, std::numeric_limits<double>::quiet_NaN());
        } else {
            double current_close = data_array[i];
            double prev_close = data_array[i-1];
            double upmove = current_close - prev_close;  // can be negative!
            upmove_line->set(i, upmove);
        }
    }
}

// DownMove implementation

DownMove::DownMove() : Indicator(), data_source_(nullptr), current_index_(0) {
    std::cout << "DownMove default constructor called" << std::endl;
    setup_lines();
    _minperiod(2);
}

DownMove::DownMove(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    std::cout << "DownMove(LineSeries) constructor called" << std::endl;
    setup_lines();
    _minperiod(2);
    // Set up data member for calculate() to use
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Calculate immediately upon construction if we have data
    if (data_source && data_source->lines && data_source->lines->size() > 0) {
        std::cout << "Calling calculate() from constructor" << std::endl;
        calculate();
    }
}

DownMove::DownMove(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::dynamic_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    _minperiod(2);
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

DownMove::DownMove(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::dynamic_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    _minperiod(2);
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

double DownMove::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(downmove);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Cast to LineBuffer to handle positive ago indexing
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    
    // For positive ago values, we need to convert to array index
    // The buffer has 257 elements (0-256), with _idx at 256
    // ago=0 -> index 256 (most recent)
    // ago=253 -> index 3
    // ago=127 -> index 129
    if (buffer && ago >= 0) {
        int index = buffer->size() - 1 - ago;
        if (index >= 0 && index < buffer->size()) {
            return buffer->array()[index];
        }
    }
    
    // For negative ago values, use the line's operator[]
    return (*line)[ago];
}

int DownMove::getMinPeriod() const {
    return 2;
}

void DownMove::calculate() {
    std::cout << "*** DownMove::calculate() called ***" << std::endl;
    
    if (!data || !data->lines || data->lines->size() == 0) {
        // Try with datas vector if data is not set
        if (datas.empty() || !datas[0] || !datas[0]->lines || datas[0]->lines->size() == 0) {
            std::cout << "DownMove::calculate() - No data available" << std::endl;
            return;
        }
        data = datas[0];
    }
    
    // For test framework, the data passed in is either the primary line or close price line
    // So we use getline(0) to get the primary line
    auto data_line = data->lines->getline(0);
    if (!data_line) {
        std::cout << "DownMove::calculate() - No data line at index 0" << std::endl;
        return;
    }
    
    auto downmove_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!downmove_line) {
        std::cout << "DownMove::calculate() - No downmove line" << std::endl;
        return;
    }
    
    // Cast to LineBuffer to access raw data
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        std::cout << "DownMove::calculate() - Cannot cast data line to LineBuffer" << std::endl;
        return;
    }
    
    // Clear and recalculate all values
    downmove_line->reset();
    
    // Get the actual data size
    int data_size = static_cast<int>(data_buffer->size());
    
    std::cout << "DownMove::calculate() - data_size: " << data_size << std::endl;
    std::cout << "First 5 data values: ";
    for (int i = 0; i < std::min(5, data_size); ++i) {
        std::cout << data_buffer->array()[i] << " ";
    }
    std::cout << std::endl;
    
    // LineBuffer stores data in chronological order (oldest first)
    // First value should be NaN as there's no previous value
    downmove_line->append(std::numeric_limits<double>::quiet_NaN());
    
    // Process remaining data points
    for (int i = 1; i < data_size; ++i) {
        // DownMove formula: prev - current
        double current_val = data_buffer->array()[i];
        double prev_val = data_buffer->array()[i-1];
        double downmove = prev_val - current_val;  // DownMove can be negative
        
        if (i <= 5) {
            std::cout << "DownMove[" << i << "]: " << prev_val << " - " << current_val 
                      << " = " << downmove << std::endl;
        }
        
        downmove_line->append(downmove);
    }
    
    // Set the index to the last element (most recent)
    downmove_line->set_idx(downmove_line->size() - 1);
    
    std::cout << "DownMove::calculate() finished - size: " << downmove_line->size() 
              << ", _idx: " << downmove_line->get_idx() << std::endl;
    
    // Debug: Show a few values from the output
    std::cout << "Output buffer first 10: ";
    for (int i = 0; i < std::min(10, (int)downmove_line->size()); ++i) {
        std::cout << downmove_line->array()[i] << " ";
    }
    std::cout << std::endl;
}

size_t DownMove::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto downmove_line = lines->getline(downmove);
    if (!downmove_line) {
        return 0;
    }
    return downmove_line->size();
}

void DownMove::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void DownMove::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);  // First line (could be low or single line)
    auto downmove_line = lines->getline(downmove);
    
    if (!data_line || !downmove_line) return;
    
    // DownMove = prev - current - can be negative
    double current_low = (*data_line)[0];
    double prev_low = (*data_line)[1];
    double downmove = prev_low - current_low;  // DownMove can be negative
    
    downmove_line->set(0, downmove);
}

void DownMove::once(int start, int end) {
    // Use data member which is set in calculate()
    if (!data || !data->lines || data->lines->size() == 0) return;
    
    auto data_line = data->lines->getline(0);
    auto downmove_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(downmove));
    
    if (!data_line || !downmove_line) return;
    
    // Cast to LineBuffer to access raw data
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // LineBuffer stores data in reverse chronological order
    // The most recent data is at index 0, oldest at the end
    int data_size = static_cast<int>(data_buffer->size());
    
    // Process all data points in chronological order
    for (int i = start; i < end && i < data_size; ++i) {
        if (i == 0) {
            // First value, no previous value for comparison
            downmove_line->append(0.0);
        } else {
            // DownMove formula: prev - current - can be negative
            // Access LineBuffer using reverse indexing: newest first
            int current_idx = data_size - 1 - i;
            int prev_idx = data_size - 1 - (i - 1);
            double current_val = (*data_buffer)[current_idx];
            double prev_val = (*data_buffer)[prev_idx];
            double downmove = prev_val - current_val;  // DownMove can be negative
            
            downmove_line->append(downmove);
        }
    }
}

} // namespace indicators
} // namespace backtrader