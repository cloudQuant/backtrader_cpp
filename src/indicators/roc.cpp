#include "indicators/roc.h"
#include "linebuffer.h"
#include <cmath>

namespace backtrader {
namespace indicators {

RateOfChange::RateOfChange() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1);
}

RateOfChange::RateOfChange(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

RateOfChange::RateOfChange(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

RateOfChange::RateOfChange(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
}

RateOfChange::RateOfChange(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
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
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto roc_line = lines->getline(roc);
    if (!roc_line || roc_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Simply delegate to the LineBuffer's get() method
    return roc_line->get(ago);
}

int RateOfChange::getMinPeriod() const {
    return params.period + 1;
}

void RateOfChange::calculate() {
    // Get data source from either data_source_ or datas vector
    std::shared_ptr<LineSeries> source = data_source_;
    if (!source && !datas.empty()) {
        source = datas[0];
    }
    
    if (source && source->lines && source->lines->size() > 0) {
        // Get actual data size
        int data_size = 0;
        
        // Determine which line to use based on the data type
        std::shared_ptr<LineSingle> data_line;
        if (source->lines->size() > 4) {
            // DataSeries with OHLCV data: use close price (index 4)
            data_line = source->lines->getline(4);
        } else {
            // Simple LineSeries: use first line (index 0)
            data_line = source->lines->getline(0);
        }
        
        if (!data_line) return;
        
        // Get actual data size from buffer array
        auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (data_buffer) {
            const auto& array_data = data_buffer->array();
            data_size = static_cast<int>(array_data.size());
        } else {
            data_size = data_line->size();
        }
        
        // Calculate for the entire dataset using once() method
        if (data_size > 0) {
            once(0, data_size);
        }
    } else {
        // For normal constructor, use the existing next() logic
        next();
    }
}

void RateOfChange::next() {
    if (!data || data->lines->size() == 0) {
        // Try with datas vector if data is not set
        if (datas.empty() || !datas[0] || !datas[0]->lines || datas[0]->lines->size() == 0) {
            return;
        }
    }
    
    // Use the close price line (index 3) for OHLC data, or primary line (index 0) for simple data
    auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
    if (!data_line) return;
    
    auto roc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!roc_line) return;
    
    double current_price = (*data_line)[0];
    
    if (std::isnan(current_price)) {
        // Still need to advance the line buffer even with NaN
        roc_line->append(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Store current price in buffer
    price_buffer_.push_back(current_price);
    
    // Keep only the required number of prices (period + 1)
    if (static_cast<int>(price_buffer_.size()) > params.period + 1) {
        price_buffer_.pop_front();
    }
    
    double roc_value = std::numeric_limits<double>::quiet_NaN();
    
    // Calculate ROC when we have enough data
    if (static_cast<int>(price_buffer_.size()) == params.period + 1) {
        double old_price = price_buffer_.front();  // Price 'period' steps back
        double current_price = price_buffer_.back(); // Current price
        
        if (!std::isnan(old_price) && old_price != 0.0) {
            roc_value = (current_price - old_price) / old_price;
        }
    }
    
    // Always append a value to maintain line buffer sync
    roc_line->append(roc_value);
}

void RateOfChange::once(int start, int end) {
    // Handle both data and datas patterns
    std::shared_ptr<LineSeries> data_source = data;
    if (!data_source && !datas.empty()) {
        data_source = datas[0];
    }
    
    if (!data_source || !data_source->lines || data_source->lines->size() == 0) {
        return;
    }
    
    // Use adaptive line selection like other indicators
    std::shared_ptr<LineSingle> data_line;
    if (data_source->lines->size() > 4) {
        // For DataSeries: use close line (index 4)
        data_line = data_source->lines->getline(4);
    } else {
        // For LineSeries: use line 0
        data_line = data_source->lines->getline(0);
    }
    
    auto roc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!roc_line || !data_line) return;
    
    // Reset the ROC line buffer (keeps initial NaN for alignment)
    roc_line->reset();
    
    // Get data buffer
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    const auto& data_array = data_buffer->array();
    int data_size = static_cast<int>(data_array.size());
    
    
    // Process data in forward order - data is already in chronological order
    // Start from index 1 to skip the initial NaN that reset() adds
    for (int i = 1; i < data_size; ++i) {
        double roc_value = std::numeric_limits<double>::quiet_NaN();
        
        // Calculate ROC only when we have enough data (period + 1 points minimum)
        // Since we have initial NaN at index 0, we need i > period
        if (i > params.period) {
            double current_price = data_array[i];
            double past_price = data_array[i - params.period];
            
            if (!std::isnan(current_price) && !std::isnan(past_price) && past_price != 0.0) {
                // ROC formula: (current - past) / past
                roc_value = (current_price - past_price) / past_price;
            }
        }
        
        // Append value to line buffer
        roc_line->append(roc_value);
    }
    
    // Set buffer position to match the data buffer index
    // If data buffer is at -1 (initial state), set ROC buffer to data_size - 1
    int data_idx = data_buffer->get_idx();
    if (data_idx == -1) {
        // Data buffer is at initial state, set ROC buffer to last valid index
        roc_line->set_idx(data_size - 1, true);
    } else {
        roc_line->set_idx(data_idx, true);
    }
}

size_t RateOfChange::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto roc_line = lines->getline(roc);
    if (!roc_line) {
        return 0;
    }
    return roc_line->size();
}

} // namespace indicators
} // namespace backtrader