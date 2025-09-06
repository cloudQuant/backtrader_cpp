#include "indicators/bollinger.h"
#include "indicators/sma.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// BollingerBands implementation
BollingerBands::BollingerBands() : Indicator(), data_source_(nullptr), current_index_(0), values_() {
    setup_lines();
    
    // Set minimum period
    _minperiod(params.period);
}

BollingerBands::BollingerBands(std::shared_ptr<LineSeries> data_source, int period, double devfactor) 
    : Indicator(), data_source_(data_source), current_index_(0), values_() {
    params.period = period;
    params.devfactor = devfactor;
    
    setup_lines();
    
    // Set minimum period
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

BollingerBands::BollingerBands(std::shared_ptr<DataSeries> data_source, int period, double devfactor) 
    : Indicator(), data_source_(data_source), current_index_(0), values_() {
    params.period = period;
    params.devfactor = devfactor;
    
    setup_lines();
    
    // Set minimum period
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

void BollingerBands::setup_lines() {
    // Create 3 lines: mid, top, bot
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // mid line
        lines->add_line(std::make_shared<LineBuffer>());  // top line
        lines->add_line(std::make_shared<LineBuffer>());  // bot line
        lines->add_alias("mid", 0);
        lines->add_alias("top", 1);
        lines->add_alias("bot", 2);
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector for size() method
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
    
}

double BollingerBands::calculate_sma(int period, int current_index) const {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    // Use close price (index 3) for OHLC data, or primary line (index 0) for simple data
    auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    if (!close_line) return 0.0;
    
    // Check if we need to use array() method for LineBuffer at index -1
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    std::vector<double> close_array;
    bool use_array = false;
    
    if (close_buffer && close_buffer->size() == 0) {
        close_array = close_buffer->array();
        use_array = true;
    }
    
    double sum = 0.0;
    if (use_array && !close_array.empty()) {
        int array_size = static_cast<int>(close_array.size());
        for (int i = 0; i < period && i < array_size; ++i) {
            int idx = array_size - 1 - i;
            sum += close_array[idx];
        }
    } else if (close_buffer && close_buffer->size() > 0) {
        for (int i = 0; i < period; ++i) {
            sum += (*close_buffer)[i];
        }
    } else {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return sum / period;
}

double BollingerBands::calculate_stddev(int period, int current_index, double mean) const {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    // Use close price (index 3) for OHLC data, or primary line (index 0) for simple data
    auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    if (!close_line) return 0.0;
    
    // Check if we need to use array() method for LineBuffer at index -1
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    std::vector<double> close_array;
    bool use_array = false;
    
    if (close_buffer && close_buffer->size() == 0) {
        close_array = close_buffer->array();
        use_array = true;
    }
    
    double sum_squared_diff = 0.0;
    if (use_array && !close_array.empty()) {
        int array_size = static_cast<int>(close_array.size());
        for (int i = 0; i < period && i < array_size; ++i) {
            int idx = array_size - 1 - i;
            double value = close_array[idx];
            double diff = value - mean;
            sum_squared_diff += diff * diff;
        }
    } else if (close_buffer && close_buffer->size() > 0) {
        for (int i = 0; i < period; ++i) {
            double value = (*close_buffer)[i];
            double diff = value - mean;
            sum_squared_diff += diff * diff;
        }
    } else {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double variance = sum_squared_diff / period;
    return std::sqrt(variance);
}

void BollingerBands::prenext() {
    Indicator::prenext();
}

void BollingerBands::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Calculate moving average (middle band)
    double mid_value = calculate_sma(params.period, 0);
    
    // Calculate standard deviation
    double stddev = calculate_stddev(params.period, 0, mid_value);
    
    // Calculate upper and lower bands
    double deviation = params.devfactor * stddev;
    double top_value = mid_value + deviation;
    double bot_value = mid_value - deviation;
    
    // Set line values
    auto mid_line = lines->getline(mid);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (mid_line) mid_line->set(0, mid_value);
    if (top_line) top_line->set(0, top_value);
    if (bot_line) bot_line->set(0, bot_value);
}

void BollingerBands::once(int start, int end) {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    // Use the close price line (index 4) for OHLC data, or primary line (index 0) for simple data
    auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    auto mid_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(mid));
    auto top_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(top));
    auto bot_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(bot));
    
    if (!data_buffer || !mid_line || !top_line || !bot_line) {
        return;
    }
    
    // Get the data as an array for easier processing
    std::vector<double> data_array;
    size_t buffer_size = data_buffer->data_size();  // Use data_size() instead of size()
    if (buffer_size == 0) {
        return;
    }
    
    // Get the actual data array from buffer
    data_array = data_buffer->array();
    
    
    // Clear the line buffers and reset to initial state
    mid_line->reset();
    top_line->reset();
    bot_line->reset();
    
    // Process data in forward order (matching Python's behavior)
    for (int i = start; i < end && i < static_cast<int>(data_array.size()); ++i) {
        double mid_value = std::numeric_limits<double>::quiet_NaN();
        double top_value = std::numeric_limits<double>::quiet_NaN();
        double bot_value = std::numeric_limits<double>::quiet_NaN();
        
        if (i >= params.period - 1) {
            // Calculate SMA for position i using the window [i-period+1, i]
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += data_array[i - j];
            }
            mid_value = sum / params.period;
            
            // Calculate standard deviation
            double sum_squared_diff = 0.0;
            for (int j = 0; j < params.period; ++j) {
                double value = data_array[i - j];
                double diff = value - mid_value;
                sum_squared_diff += diff * diff;
            }
            double stddev = std::sqrt(sum_squared_diff / params.period);
            
            // Calculate bands
            double deviation = params.devfactor * stddev;
            top_value = mid_value + deviation;
            bot_value = mid_value - deviation;
        }
        
        // Append values to buffers
        mid_line->append(mid_value);
        top_line->append(top_value);
        bot_line->append(bot_value);
    }
}

// BollingerBandsPct implementation
BollingerBandsPct::BollingerBandsPct() : BollingerBands() {
    // Add pctb line (BollingerBands already has mid, top, bot lines)
    if (lines->size() < 4) {
        lines->add_line(std::make_shared<LineBuffer>());  // pctb line
        lines->add_alias("pctb", 3);
    }
}

void BollingerBandsPct::prenext() {
    BollingerBands::prenext();
}

void BollingerBandsPct::next() {
    // Calculate Bollinger Bands first
    BollingerBands::next();
    
    // Calculate percentage B
    if (datas.empty() || !datas[0]->lines) return;
    
    // Use close price (index 3) for OHLC data, or primary line (index 0) for simple data
    auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    auto top_line = lines->getline(BollingerBands::top);
    auto bot_line = lines->getline(BollingerBands::bot);
    auto pctb_line = lines->getline(pctb);
    
    if (close_line && top_line && bot_line && pctb_line) {
        double current_price = (*close_line)[0];
        double top_value = (*top_line)[0];
        double bot_value = (*bot_line)[0];
        
        double band_width = top_value - bot_value;
        if (band_width != 0.0) {
            double pctb_value = (current_price - bot_value) / band_width;
            pctb_line->set(0, pctb_value);
        } else {
            pctb_line->set(0, 0.5); // Middle of the range when band width is zero
        }
    }
}

void BollingerBandsPct::once(int start, int end) {
    // Calculate Bollinger Bands first
    BollingerBands::once(start, end);
    
    // Calculate percentage B for the entire range
    if (datas.empty() || !datas[0]->lines) return;
    
    // Use close price (index 3) for OHLC data, or primary line (index 0) for simple data
    auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    auto top_line = lines->getline(BollingerBands::top);
    auto bot_line = lines->getline(BollingerBands::bot);
    auto pctb_line = lines->getline(pctb);
    
    if (close_line && top_line && bot_line && pctb_line) {
        for (int i = start; i < end; ++i) {
            // Convert array index to LineBuffer index
            int close_linebuffer_index = static_cast<int>(close_line->size()) - 1 - i;
            int top_linebuffer_index = static_cast<int>(top_line->size()) - 1 - i;
            int bot_linebuffer_index = static_cast<int>(bot_line->size()) - 1 - i;
            double current_price = (*close_line)[close_linebuffer_index];
            double top_value = (*top_line)[top_linebuffer_index];
            double bot_value = (*bot_line)[bot_linebuffer_index];
            
            double band_width = top_value - bot_value;
            if (band_width != 0.0) {
                double pctb_value = (current_price - bot_value) / band_width;
                pctb_line->set(i, pctb_value);
            } else {
                pctb_line->set(i, 0.5);
            }
        }
    }
}

size_t BollingerBandsPct::size() const {
    // Use the parent's size method
    return BollingerBands::size();
}

double BollingerBands::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(0);
    if (!mid_line || mid_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already implements Python indexing convention correctly
    return (*mid_line)[ago];
}

double BollingerBands::getBandwidth(int ago) const {
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto mid_line = lines->getline(mid);
    
    if (!top_line || !bot_line || !mid_line || 
        top_line->size() == 0 || bot_line->size() == 0 || mid_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert ago to array index
    int index;
    if (ago <= 0) {
        index = static_cast<int>(mid_line->size()) - 1 + ago;
    } else {
        index = static_cast<int>(mid_line->size()) - 1 - ago;
    }
    
    if (index < 0 || index >= static_cast<int>(mid_line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double top_value = (*top_line)[index];
    double bot_value = (*bot_line)[index];
    double mid_value = (*mid_line)[index];
    
    if (mid_value == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Bandwidth = (Upper Band - Lower Band) / Middle Band
    return (top_value - bot_value) / mid_value;
}

double BollingerBands::getPercentB(int ago) const {
    if (datas.empty() || !datas[0]->lines) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use close price (index 3) for OHLC data, or primary line (index 0) for simple data
    auto close_line = datas[0]->lines->getline(datas[0]->lines->size() > 4 ? 4 : 0);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (!close_line || !top_line || !bot_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double close_value = (*close_line)[ago];
    double top_value = (*top_line)[ago];
    double bot_value = (*bot_line)[ago];
    
    double band_width = top_value - bot_value;
    if (band_width == 0.0) {
        return 0.5; // Middle of the range when band width is zero
    }
    
    // %B = (Close - Lower Band) / (Upper Band - Lower Band)
    return (close_value - bot_value) / band_width;
}

double BollingerBands::getMiddleBand(int ago) const {
    return get(ago);  // Middle band is the primary line
}

double BollingerBands::getUpperBand(int ago) const {
    if (!lines || lines->size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto top_line = lines->getline(top);
    if (!top_line || top_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already implements Python indexing convention correctly
    return (*top_line)[ago];
}

double BollingerBands::getLowerBand(int ago) const {
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto bot_line = lines->getline(bot);
    if (!bot_line || bot_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already implements Python indexing convention correctly
    return (*bot_line)[ago];
}

void BollingerBands::calculate() {
    if (!data || data->lines->size() == 0) {
        return;
    }
    
    // Use the close price line (index 4) for OHLC data, or primary line (index 0) for simple data
    auto data_line = data->lines->getline(data->lines->size() > 4 ? 4 : 0);
    if (!data_line) {
        return;
    }
    
    // Get the LineBuffer to access the full data array
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        return;
    }
    
    // Get actual data size from the buffer's internal array
    // LineBuffer::size() returns _idx + 1, which can be 0 when _idx is -1
    // So we need to use data_size() or array() to get the actual data
    int data_size = static_cast<int>(data_buffer->data_size());
    
    
    if (data_size > 0) {
        // Use once() method for batch processing all data points
        once(0, data_size);
    }
}

size_t BollingerBands::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto mid_line = lines->getline(mid);
    if (!mid_line) {
        return 0;
    }
    
    return mid_line->size();
}

} // namespace indicators
} // namespace backtrader