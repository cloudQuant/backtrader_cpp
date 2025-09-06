#include "indicators/wmaosc.h"
#include "indicators/wma.h"
#include "linebuffer.h"
#include "dataseries.h"
#include "lineseries.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator() {
    params.period = 30;
    setup_lines();
}

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source) {
    params.period = 30;
    setup_lines();
    
    // Create WMA indicator
    wma_ = std::make_shared<WeightedMovingAverage>(data_source, params.period);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Set minimum period
    _minperiod(params.period);
}

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator(std::shared_ptr<LineSeries> data_source, 
                                                                 int period)
    : Indicator(), data_source_(data_source) {
    params.period = period;
    setup_lines();
    
    // Create WMA indicator
    wma_ = std::make_shared<WeightedMovingAverage>(data_source, params.period);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Set minimum period
    _minperiod(params.period);
}

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source)
    : Indicator() {
    params.period = 30;
    setup_lines();
    
    // Create WMA indicator
    wma_ = std::make_shared<WeightedMovingAverage>(data_source, params.period);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Set minimum period
    _minperiod(params.period);
}

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator(std::shared_ptr<DataSeries> data_source, 
                                                                 int period)
    : Indicator() {
    params.period = period;
    setup_lines();
    
    // Create WMA indicator
    wma_ = std::make_shared<WeightedMovingAverage>(data_source, params.period);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Set minimum period
    _minperiod(params.period);
}

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator(std::shared_ptr<LineBuffer> data)
    : Indicator() {
    params.period = 30;
    setup_lines();
    auto line_series = std::make_shared<LineSeries>();
    line_series->lines->add_line(data);
    line_series->lines->add_alias("data", 0);
    wma_ = std::make_shared<WeightedMovingAverage>(line_series, params.period);
}

WeightedMovingAverageOscillator::WeightedMovingAverageOscillator(std::shared_ptr<LineBuffer> data, 
                                                                 int period)
    : Indicator() {
    params.period = period;
    setup_lines();
    auto line_series = std::make_shared<LineSeries>();
    line_series->lines->add_line(data);
    line_series->lines->add_alias("data", 0);
    wma_ = std::make_shared<WeightedMovingAverage>(line_series, params.period);
}

void WeightedMovingAverageOscillator::setup_lines() {
    if (!lines) {
        lines = std::make_shared<backtrader::Lines>();
    }
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

double WeightedMovingAverageOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto wmaosc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(wmaosc));
    if (!wmaosc_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Get the array directly for proper indexing
    const auto& wmaosc_array = wmaosc_line->array();
    if (wmaosc_array.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Python backtrader semantics:
    // ago = 0 means the last value (current)
    // ago > 0 means counting backward from the end
    // ago < 0 means direct index access where -1 is index 0, -2 is index 1, etc.
    int index;
    if (ago >= 0) {
        // Positive ago means counting from the end
        index = wmaosc_array.size() - 1 - ago;
    } else {
        // Negative ago in Python means direct position from start
        // Python: wmaosc[-225] means position 29 (because -225 + 255 = 30, 0-based = 29)
        // So for negative ago, we need: index = size + ago - 1
        // But since array[0] is NaN, actual data starts at [1], so we need to add 1
        // Final formula: index = size + ago
        index = wmaosc_array.size() + ago;
    }
    
    if (index < 0 || index >= static_cast<int>(wmaosc_array.size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return wmaosc_array[index];
}

int WeightedMovingAverageOscillator::getMinPeriod() const {
    return params.period;
}

size_t WeightedMovingAverageOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto wmaosc_line = lines->getline(wmaosc);
    return wmaosc_line ? wmaosc_line->size() : 0;
}

void WeightedMovingAverageOscillator::calculate() {
    if (!data || !data->lines || data->lines->size() == 0) {
        return;
    }
    
    // Get data line - for DataSeries it's close (index 3), for single LineSeries it's index 0
    std::shared_ptr<LineBuffer> data_line;
    if (data->lines->size() > 3) {
        data_line = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(4));  // Close price for DataSeries
    } else {
        data_line = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(0));  // First line for single LineSeries
    }
    
    if (!data_line) {
        return;
    }
    
    // Call once() to perform batch calculation
    // Use the array size minus 1 (since array[0] is NaN)
    int data_size = static_cast<int>(data_line->array().size() - 1);
    once(0, data_size);
}

void WeightedMovingAverageOscillator::next() {
    if (!lines || lines->size() == 0) {
        setup_lines();
    }
    
    auto wmaosc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(wmaosc));
    if (!wmaosc_line) {
        return;
    }
    
    // Ensure we have data and WMA
    if (!data || !data->lines || data->lines->size() == 0 || !wma_) {
        wmaosc_line->set(0, std::numeric_limits<double>::quiet_NaN());
        wmaosc_line->forward();
        return;
    }
    
    // Get data line - for DataSeries it's close (index 3), for single LineSeries it's index 0
    std::shared_ptr<LineBuffer> data_line;
    if (data->lines->size() > 3) {
        data_line = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(4));  // Close price for DataSeries
    } else {
        data_line = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(0));  // First line for single LineSeries
    }
    
    if (!data_line) {
        wmaosc_line->set(0, std::numeric_limits<double>::quiet_NaN());
        wmaosc_line->forward();
        return;
    }
    
    // Calculate WMA
    wma_->calculate();
    
    // Get current values
    double data_value = (*data_line)[0];
    double wma_value = wma_->get(0);
    
    // Calculate oscillator value: data - WMA(data)
    if (!std::isnan(data_value) && !std::isnan(wma_value)) {
        double osc_value = data_value - wma_value;
        wmaosc_line->set(0, osc_value);
    } else {
        wmaosc_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
    
    wmaosc_line->forward();
}

void WeightedMovingAverageOscillator::once(int start, int end) {
    if (!lines || lines->size() == 0) {
        setup_lines();
    }
    
    auto wmaosc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(wmaosc));
    if (!wmaosc_line || !wma_ || !data || !data->lines || data->lines->size() == 0) {
        return;
    }
    
    // The wmaosc_line should be empty initially
    
    // Get data line - for DataSeries it's close (index 3), for single LineSeries it's index 0
    std::shared_ptr<LineBuffer> data_line;
    if (data->lines->size() > 3) {
        data_line = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(4));  // Close price for DataSeries
    } else {
        data_line = std::dynamic_pointer_cast<LineBuffer>(data->lines->getline(0));  // First line for single LineSeries
    }
    
    if (!data_line) {
        return;
    }
    
    // Calculate WMA in batch mode
    wma_->calculate();
    
    // Get the data array and WMA line
    const auto& data_array = data_line->array();
    auto wma_lines = wma_->lines;
    std::shared_ptr<LineBuffer> wma_line;
    if (wma_lines && wma_lines->size() > 0) {
        wma_line = std::dynamic_pointer_cast<LineBuffer>(wma_lines->getline(0));
    }
    
    // Calculate oscillator for each point
    for (int i = start; i < end; ++i) {
        if (i < params.period - 1 || !wma_line) {
            // Not enough data for WMA calculation
            wmaosc_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Get values from arrays directly
            // Note: array[0] is NaN, actual data starts at array[1]
            double data_value = (i + 1 < static_cast<int>(data_array.size())) ? data_array[i + 1] : std::numeric_limits<double>::quiet_NaN();
            
            const auto& wma_array = wma_line->array();
            // WMA array starts with NaN at index 0, so we need to offset by 1
            double wma_value = (i + 1 < static_cast<int>(wma_array.size())) ? wma_array[i + 1] : std::numeric_limits<double>::quiet_NaN();
            
            if (!std::isnan(data_value) && !std::isnan(wma_value)) {
                wmaosc_line->append(data_value - wma_value);
            } else {
                wmaosc_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
}
} // namespace indicators
} // namespace backtrader