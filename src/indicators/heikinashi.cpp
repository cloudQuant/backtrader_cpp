#include "indicators/heikinashi.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include "dataseries.h"

namespace backtrader {
namespace indicators {

// HeikinAshi implementation
HeikinAshi::HeikinAshi() : Indicator() {
    _minperiod(2);  // HeikinAshi minimum period is 2
    
    // Initialize lines directly like SMA does
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // ha_open
        lines->add_line(std::make_shared<LineBuffer>());  // ha_high
        lines->add_line(std::make_shared<LineBuffer>());  // ha_low
        lines->add_line(std::make_shared<LineBuffer>());  // ha_close
        lines->add_alias("ha_open", 0);
        lines->add_alias("ha_high", 1);
        lines->add_alias("ha_low", 2);
        lines->add_alias("ha_close", 3);
    }
}

HeikinAshi::HeikinAshi(std::shared_ptr<LineSeries> data_source) : Indicator() {
    _minperiod(2);  // HeikinAshi minimum period is 2
    
    // Initialize lines directly like SMA does
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // ha_open
        lines->add_line(std::make_shared<LineBuffer>());  // ha_high
        lines->add_line(std::make_shared<LineBuffer>());  // ha_low
        lines->add_line(std::make_shared<LineBuffer>());  // ha_close
        lines->add_alias("ha_open", 0);
        lines->add_alias("ha_high", 1);
        lines->add_alias("ha_low", 2);
        lines->add_alias("ha_close", 3);
    }
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

HeikinAshi::HeikinAshi(std::shared_ptr<DataSeries> data_source) : Indicator() {
    _minperiod(2);  // HeikinAshi minimum period is 2
    
    // Initialize lines directly like SMA does
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // ha_open
        lines->add_line(std::make_shared<LineBuffer>());  // ha_high
        lines->add_line(std::make_shared<LineBuffer>());  // ha_low
        lines->add_line(std::make_shared<LineBuffer>());  // ha_close
        lines->add_alias("ha_open", 0);
        lines->add_alias("ha_high", 1);
        lines->add_alias("ha_low", 2);
        lines->add_alias("ha_close", 3);
    }
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
        this->_clock = lineseries;  // Set clock like SMA does
    }
}

double HeikinAshi::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ha_close);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int HeikinAshi::getMinPeriod() const {
    return 2;  // HeikinAshi minimum period is 2
}

size_t HeikinAshi::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto ha_close_line = lines->getline(ha_close);
    return ha_close_line ? ha_close_line->size() : 0;
}

void HeikinAshi::calculate() {
    if (datas.empty() || !datas[0]->lines) {
        return;
    }
    
    auto data_lines = datas[0]->lines;
    auto open_line = data_lines->getline(DataSeries::Open);
    auto high_line = data_lines->getline(DataSeries::High);
    auto low_line = data_lines->getline(DataSeries::Low);
    auto close_line = data_lines->getline(DataSeries::Close);
    
    if (!open_line || !high_line || !low_line || !close_line) {
        return;
    }
    
    // Call once() to process all data in batch mode
    // Get the current index from the input data to ensure we match it
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(open_line);
    if (open_buffer) {
        int input_idx = open_buffer->get_idx();
        int input_size = open_line->size();
        // std::cerr << "HeikinAshi::calculate() - input_idx=" << input_idx 
        //           << ", input_size=" << input_size << std::endl;
        once(0, input_size);
    } else {
        once(0, open_line->size());
    }
}


void HeikinAshi::prenext() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Get OHLC data
    double open = (*data_lines->getline(DataSeries::Open))[0];
    double high = (*data_lines->getline(DataSeries::High))[0];
    double low = (*data_lines->getline(DataSeries::Low))[0];
    double close = (*data_lines->getline(DataSeries::Close))[0];
    
    // Calculate initial seed values
    double ha_close_val = (open + high + low + close) / 4.0;
    double ha_open_val = (open + close) / 2.0;  // Seed value for first bar
    double ha_high_val = std::max({high, ha_open_val, ha_close_val});
    double ha_low_val = std::min({low, ha_open_val, ha_close_val});
    
    // Set the values in the lines
    auto ha_open_line = lines->getline(ha_open);
    auto ha_high_line = lines->getline(ha_high);
    auto ha_low_line = lines->getline(ha_low);
    auto ha_close_line = lines->getline(ha_close);
    
    // Append values
    auto ha_open_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_open_line);
    if (ha_open_buffer) ha_open_buffer->append(ha_open_val);
    
    auto ha_high_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_high_line);
    if (ha_high_buffer) ha_high_buffer->append(ha_high_val);
    
    auto ha_low_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_low_line);
    if (ha_low_buffer) ha_low_buffer->append(ha_low_val);
    
    auto ha_close_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_close_line);
    if (ha_close_buffer) ha_close_buffer->append(ha_close_val);
}

void HeikinAshi::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Get OHLC data
    double open = (*data_lines->getline(DataSeries::Open))[0];
    double high = (*data_lines->getline(DataSeries::High))[0];
    double low = (*data_lines->getline(DataSeries::Low))[0];
    double close = (*data_lines->getline(DataSeries::Close))[0];
    
    // Calculate Heikin Ashi values
    double ha_close_val = (open + high + low + close) / 4.0;
    double ha_open_val;
    
    // For first bar, use seed value, otherwise use recursive formula
    auto ha_open_line = lines->getline(ha_open);
    auto ha_close_line = lines->getline(ha_close);
    
    if (ha_open_line && ha_close_line && ha_open_line->size() == 1) {
        ha_open_val = (open + close) / 2.0;
    } else if (ha_open_line && ha_close_line) {
        double prev_ha_open = (*ha_open_line)[-1];
        double prev_ha_close = (*ha_close_line)[-1];
        ha_open_val = (prev_ha_open + prev_ha_close) / 2.0;
    } else {
        ha_open_val = (open + close) / 2.0;
    }
    
    double ha_high_val = std::max({high, ha_open_val, ha_close_val});
    double ha_low_val = std::min({low, ha_open_val, ha_close_val});
    
    // Append line values for streaming mode
    auto ha_open_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_open_line);
    if (ha_open_buffer) ha_open_buffer->append(ha_open_val);
    
    auto ha_high_line = lines->getline(ha_high);
    auto ha_high_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_high_line);
    if (ha_high_buffer) ha_high_buffer->append(ha_high_val);
    
    auto ha_low_line = lines->getline(ha_low);
    auto ha_low_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_low_line);
    if (ha_low_buffer) ha_low_buffer->append(ha_low_val);
    
    auto ha_close_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_close_line);
    if (ha_close_buffer) ha_close_buffer->append(ha_close_val);
}

void HeikinAshi::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) {
        return;
    }
    
    auto data_lines = datas[0]->lines;
    auto open_line = data_lines->getline(DataSeries::Open);
    auto high_line = data_lines->getline(DataSeries::High);
    auto low_line = data_lines->getline(DataSeries::Low);
    auto close_line = data_lines->getline(DataSeries::Close);
    
    if (!open_line || !high_line || !low_line || !close_line) {
        return;
    }
    
    auto ha_open_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ha_open));
    auto ha_high_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ha_high));
    auto ha_low_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ha_low));
    auto ha_close_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ha_close));
    
    if (!ha_open_buffer || !ha_high_buffer || !ha_low_buffer || !ha_close_buffer) {
        return;
    }
    
    // Get LineBuffers for direct array access
    auto open_buf = std::dynamic_pointer_cast<LineBuffer>(open_line);
    auto high_buf = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buf = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buf = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!open_buf || !high_buf || !low_buf || !close_buf) {
        return;
    }
    
    const auto& open_array = open_buf->array();
    const auto& high_array = high_buf->array();
    const auto& low_array = low_buf->array();
    const auto& close_array = close_buf->array();
    
    // Reset buffers - this clears them but LineBuffer constructor adds initial NaN
    ha_open_buffer->reset();
    ha_high_buffer->reset(); 
    ha_low_buffer->reset();
    ha_close_buffer->reset();
    
    // After reset, buffers have one initial NaN
    // We need to start processing from index 1 of input (skipping LineBuffer's initial NaN)
    
    double prev_ha_open = std::numeric_limits<double>::quiet_NaN();
    double prev_ha_close = std::numeric_limits<double>::quiet_NaN();
    bool first_valid_bar = true;
    
    // Process data from start to end
    // Note: The test data has 256 elements (0-255), where element 0 is NaN
    // We should process 255 elements total (from index 0 to 254)
    int actual_end = std::min(end, static_cast<int>(open_array.size()));
    
    // Debug: log the sizes
    // std::cerr << "HeikinAshi::once() - open_array.size()=" << open_array.size() 
    //           << ", start=" << start << ", end=" << end << ", actual_end=" << actual_end << std::endl;
    
    // Track number of valid data points seen so far
    int valid_data_seen = 0;
    
    for (int i = start; i < actual_end; ++i) {
        double open_val = open_array[i];
        double high_val = high_array[i];
        double low_val = low_array[i];
        double close_val = close_array[i];
        
        // Handle NaN data
        if (std::isnan(open_val) || std::isnan(high_val) || 
            std::isnan(low_val) || std::isnan(close_val)) {
            ha_open_buffer->append(std::numeric_limits<double>::quiet_NaN());
            ha_high_buffer->append(std::numeric_limits<double>::quiet_NaN());
            ha_low_buffer->append(std::numeric_limits<double>::quiet_NaN());
            ha_close_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Count this valid data point
        valid_data_seen++;
        
        // For batch processing, we need to handle minperiod differently
        // Python's framework calls prenext() for the first valid bar, then next() from minperiod onwards
        // We simulate this by calculating from the first valid bar
        
        // Calculate HeikinAshi values
        double ha_close_val = (open_val + high_val + low_val + close_val) / 4.0;
        double ha_open_val;
        
        // HeikinAshi minimum period is 2
        if (valid_data_seen == 1) {
            // First valid bar: calculate seed values but they're not yet valid output
            // Set up the seed for next calculation but output NaN (minperiod not met)
            prev_ha_open = (open_val + close_val) / 2.0;
            prev_ha_close = (open_val + high_val + low_val + close_val) / 4.0;
            first_valid_bar = false;
            
            // Output NaN since minperiod (2) is not met
            ha_open_buffer->append(std::numeric_limits<double>::quiet_NaN());
            ha_high_buffer->append(std::numeric_limits<double>::quiet_NaN());
            ha_low_buffer->append(std::numeric_limits<double>::quiet_NaN());
            ha_close_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        } else if (valid_data_seen >= 2) {
            // From second valid bar onwards: use recursive calculation (like Python's next())
            ha_open_val = (prev_ha_open + prev_ha_close) / 2.0;
        } else {
            // Should never reach here
            ha_open_val = std::numeric_limits<double>::quiet_NaN();
        }
        
        double ha_high_val = std::max({high_val, ha_open_val, ha_close_val});
        double ha_low_val = std::min({low_val, ha_open_val, ha_close_val});
        
        // Store values in buffers (valid output from 2nd data point onwards)
        ha_open_buffer->append(ha_open_val);
        ha_high_buffer->append(ha_high_val);
        ha_low_buffer->append(ha_low_val);
        ha_close_buffer->append(ha_close_val);
        
        // Update previous values for next iteration
        prev_ha_open = ha_open_val;
        prev_ha_close = ha_close_val;
    }
    
    // Set the buffer indices to the last valid position
    // The buffers now have the same number of elements as processed (minus skipped NaN)
    int final_size = ha_open_buffer->array().size();
    
    // Set the buffer indices to the last element in the buffer
    // After processing, buffers have (number_of_valid_elements + 1) elements due to initial NaN
    int final_buffer_size = ha_open_buffer->array().size();
    if (final_buffer_size > 1) {
        // Set index to the last valid element (buffer_size - 1)
        int final_idx = final_buffer_size - 1;
        ha_open_buffer->set_idx(final_idx);
        ha_high_buffer->set_idx(final_idx);
        ha_low_buffer->set_idx(final_idx);
        ha_close_buffer->set_idx(final_idx);
        
        // std::cerr << "HeikinAshi::once() - set idx to " << final_idx
        //           << ", buffer size=" << final_buffer_size << std::endl;
    }
}

// HaDelta implementation
HaDelta::HaDelta() : Indicator() {
    setup_lines();
    _minperiod(params.period);
    
    // Create Heikin Ashi indicator if autoheikin is enabled
    if (params.autoheikin) {
        heikin_ashi_ = std::make_shared<HeikinAshi>();
    }
    
    // Create SMA for smoothing  
    sma_ = std::make_shared<indicators::SMA>();
    // Note: SMA params setup would be done differently based on actual SMA implementation
}

void HaDelta::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void HaDelta::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto hadelta_line = lines->getline(haDelta);
    auto smoothed_line = lines->getline(smoothed);
    
    if (!hadelta_line || !smoothed_line) return;
    
    double ha_open, ha_close;
    
    if (params.autoheikin && heikin_ashi_) {
        // Use Heikin Ashi transformed data
        heikin_ashi_->datas = datas;
        heikin_ashi_->calculate();
        
        if (heikin_ashi_->lines && 
            heikin_ashi_->lines->getline(HeikinAshi::ha_open) &&
            heikin_ashi_->lines->getline(HeikinAshi::ha_close)) {
            
            ha_open = (*heikin_ashi_->lines->getline(HeikinAshi::ha_open))[0];
            ha_close = (*heikin_ashi_->lines->getline(HeikinAshi::ha_close))[0];
        } else {
            ha_open = 0.0;
            ha_close = 0.0;
        }
    } else {
        // Use regular OHLC data
        auto data_lines = datas[0]->lines;
        ha_open = (*data_lines->getline(DataSeries::Open))[0];  // Open
        ha_close = (*data_lines->getline(DataSeries::Close))[0]; // Close
    }
    
    // Calculate haDelta
    double ha_delta = ha_close - ha_open;
    hadelta_line->set(0, ha_delta);
    
    // Calculate smoothed haDelta using SMA
    // For simplicity, calculate SMA manually here
    if (hadelta_line && hadelta_line->size() >= static_cast<size_t>(params.period)) {
        double sum = 0.0;
        for (int i = 0; i < params.period; ++i) {
            sum += (*hadelta_line)[-i];
        }
        
        // Set smoothed value
        smoothed_line->set(0, sum / params.period);
    }
}

void HaDelta::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto hadelta_line = lines->getline(haDelta);
    auto smoothed_line = lines->getline(smoothed);
    
    if (!hadelta_line || !smoothed_line) return;
    
    // Calculate Heikin Ashi if needed
    if (params.autoheikin && heikin_ashi_) {
        heikin_ashi_->datas = datas;
        // Use calculate method instead of once as it's protected
        for (int i = start; i < end; ++i) {
            heikin_ashi_->calculate();
        }
    }
    
    for (int i = start; i < end; ++i) {
        double ha_open, ha_close;
        
        if (params.autoheikin && heikin_ashi_ && heikin_ashi_->lines) {
            // Use Heikin Ashi transformed data
            if (heikin_ashi_->lines->getline(HeikinAshi::ha_open) &&
                heikin_ashi_->lines->getline(HeikinAshi::ha_close)) {
                
                ha_open = (*heikin_ashi_->lines->getline(HeikinAshi::ha_open))[i];
                ha_close = (*heikin_ashi_->lines->getline(HeikinAshi::ha_close))[i];
            } else {
                ha_open = 0.0;
                ha_close = 0.0;
            }
        } else {
            // Use regular OHLC data
            auto data_lines = datas[0]->lines;
            ha_open = (*data_lines->getline(DataSeries::Open))[i];  // Open
            ha_close = (*data_lines->getline(DataSeries::Close))[i]; // Close
        }
        
        // Calculate haDelta
        double ha_delta = ha_close - ha_open;
        hadelta_line->set(i, ha_delta);
        
        // Calculate smoothed haDelta using SMA
        if (i >= start + params.period - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.period; ++j) {
                sum += (*hadelta_line)[i - j];
            }
            smoothed_line->set(i, sum / params.period);
        }
    }
}

} // namespace indicators
} // namespace backtrader