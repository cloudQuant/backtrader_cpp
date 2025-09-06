#include "indicators/rmi.h"
#include "indicator_utils.h"
#include <algorithm>
#include <limits>
#include <iostream>

namespace backtrader {

// RelativeMomentumIndex implementation
RelativeMomentumIndex::RelativeMomentumIndex() : Indicator() {
    setup_lines();
    _minperiod(params.period + params.lookback);
    
    // Create SMMA indicators for smoothing
    up_smma_ = std::make_shared<indicators::SMMA>();
    up_smma_->params.period = params.period;
    
    down_smma_ = std::make_shared<indicators::SMMA>();
    down_smma_->params.period = params.period;
}

RelativeMomentumIndex::RelativeMomentumIndex(std::shared_ptr<LineSeries> data_source) : Indicator() {
    setup_lines();
    _minperiod(params.period + params.lookback);
    
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Create SMMA indicators for smoothing
    up_smma_ = std::make_shared<indicators::SMMA>();
    up_smma_->params.period = params.period;
    
    down_smma_ = std::make_shared<indicators::SMMA>();
    down_smma_->params.period = params.period;
}

RelativeMomentumIndex::RelativeMomentumIndex(std::shared_ptr<LineSeries> data_source, int period, int lookback) : Indicator() {
    params.period = period;
    params.lookback = lookback;
    setup_lines();
    _minperiod(params.period + params.lookback);
    
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Create SMMA indicators for smoothing
    up_smma_ = std::make_shared<indicators::SMMA>();
    up_smma_->params.period = params.period;
    
    down_smma_ = std::make_shared<indicators::SMMA>();
    down_smma_->params.period = params.period;
}

RelativeMomentumIndex::RelativeMomentumIndex(std::shared_ptr<DataSeries> data_source) : Indicator() {
    setup_lines();
    _minperiod(params.period + params.lookback);
    
    // Add data source to datas for traditional indicator interface
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    
    // Create SMMA indicators for smoothing
    up_smma_ = std::make_shared<indicators::SMMA>();
    up_smma_->params.period = params.period;
    
    down_smma_ = std::make_shared<indicators::SMMA>();
    down_smma_->params.period = params.period;
}

RelativeMomentumIndex::RelativeMomentumIndex(std::shared_ptr<DataSeries> data_source, int period, int lookback) : Indicator() {
    params.period = period;
    params.lookback = lookback;
    setup_lines();
    _minperiod(params.period + params.lookback);
    
    // Add data source to datas for traditional indicator interface
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    
    // Create SMMA indicators for smoothing
    up_smma_ = std::make_shared<indicators::SMMA>();
    up_smma_->params.period = params.period;
    
    down_smma_ = std::make_shared<indicators::SMMA>();
    down_smma_->params.period = params.period;
}

void RelativeMomentumIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void RelativeMomentumIndex::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get data line using utility function
    auto data_line = utils::getDataLine(datas[0]);
    if (!data_line) return;
    
    // Check if we have enough indicator lines
    if (lines->size() == 0) return;
    
    auto rmi_line = lines->getline(rmi);
    
    if (!data_line || !rmi_line) return;
    
    // Calculate up/down moves based on lookback period
    double current = (*data_line)[0];
    double lookback_value = (*data_line)[-params.lookback];
    
    double diff = current - lookback_value;
    double up_move = (diff > 0) ? diff : 0.0;
    double down_move = (diff < 0) ? -diff : 0.0;
    
    // Store moves
    up_moves_.push_back(up_move);
    down_moves_.push_back(down_move);
    
    // Calculate SMMA of up and down moves
    if (up_moves_.size() >= params.period) {
        // Manual SMMA calculation using instance variables
        if (first_calc_) {
            // First SMMA is SMA of first 'period' values
            double up_sum = 0.0;
            double down_sum = 0.0;
            
            // Use first 'period' values (indices 0 to period-1)
            for (int i = 0; i < params.period; ++i) {
                up_sum += up_moves_[i];
                down_sum += down_moves_[i];
            }
            
            up_smma_value_ = up_sum / params.period;
            down_smma_value_ = down_sum / params.period;
            first_calc_ = false;
        } else {
            // SMMA calculation: update with new move
            up_smma_value_ = (up_smma_value_ * (params.period - 1) + up_move) / params.period;
            down_smma_value_ = (down_smma_value_ * (params.period - 1) + down_move) / params.period;
        }
        
        // Calculate RMI
        double rmi;
        if (down_smma_value_ == 0.0) {
            if (up_smma_value_ == 0.0) {
                // No movement at all - neutral
                rmi = 50.0;
            } else {
                // Only up movements - maximum
                rmi = 100.0;
            }
        } else {
            double rs = up_smma_value_ / down_smma_value_;
            rmi = 100.0 - (100.0 / (1.0 + rs));
        }
        
        rmi_line->set(0, rmi);
    } else {
        // Not enough data - set NaN
        rmi_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
    
    // Keep history manageable
    if (up_moves_.size() > params.period * 2) {
        up_moves_.erase(up_moves_.begin());
        down_moves_.erase(down_moves_.begin());
    }
}

void RelativeMomentumIndex::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get data line using utility function
    auto data_line = utils::getDataLine(datas[0]);
    if (!data_line) return;
    
    // Check if we have enough indicator lines
    if (lines->size() == 0) return;
    
    auto rmi_line = lines->getline(rmi);
    
    if (!data_line || !rmi_line) return;
    
    // Handle LineBuffer data access
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    std::vector<double> data_array;
    bool use_array = false;
    
    if (data_buffer) {
        data_array = data_buffer->array();
        use_array = true;
    }
    
    if (use_array && data_array.empty()) return;
    if (!use_array && data_line->size() == 0) return;
    
    // Clear and prepare RMI line buffer
    auto rmi_buffer = std::dynamic_pointer_cast<LineBuffer>(rmi_line);
    if (rmi_buffer) {
        rmi_buffer->reset();
        // Don't pre-fill with NaN, we'll append values as we calculate them
    }
    
    int actual_end = use_array ? std::min(end, static_cast<int>(data_array.size())) : std::min(end, static_cast<int>(data_line->size()));
    int min_required_data = params.period + params.lookback;
    
    // Calculate all up/down moves first
    std::vector<double> all_up_moves;
    std::vector<double> all_down_moves;
    
    for (int i = params.lookback; i < actual_end; ++i) {
        double current, lookback_value;
        
        if (use_array) {
            current = data_array[i];
            lookback_value = data_array[i - params.lookback];
        } else {
            current = (*data_line)[i];
            lookback_value = (*data_line)[i - params.lookback];
        }
        
        double diff = current - lookback_value;
        double up_move = (diff > 0) ? diff : 0.0;
        double down_move = (diff < 0) ? -diff : 0.0;
        
        all_up_moves.push_back(up_move);
        all_down_moves.push_back(down_move);
    }
    
    // Initialize SMMA values  
    double up_smma = 0.0, down_smma = 0.0;
    bool smma_initialized = false;
    
    // Process each time period
    for (int idx = start; idx < actual_end; ++idx) {
        int move_idx = idx - params.lookback;  // Index into moves arrays
        
        if (move_idx < 0 || idx < min_required_data - 1) {
            // Not enough data - append NaN
            rmi_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        if (move_idx < params.period - 1) {
            // Not enough moves for SMMA calculation
            rmi_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        if (!smma_initialized) {
            // Initialize SMMA with SMA of first 'period' moves 
            // Python backtrader initializes with the first period values starting from index 0
            for (int j = 0; j < params.period; ++j) {
                up_smma += all_up_moves[j];
                down_smma += all_down_moves[j];
            }
            up_smma /= params.period;
            down_smma /= params.period;
            smma_initialized = true;
        } else if (move_idx > params.period - 1) {
            // Update SMMA with new move
            up_smma = (up_smma * (params.period - 1) + all_up_moves[move_idx]) / params.period;
            down_smma = (down_smma * (params.period - 1) + all_down_moves[move_idx]) / params.period;
        }
        
        // Calculate RMI
        double rmi;
        if (down_smma == 0.0) {
            if (up_smma == 0.0) {
                // No movement at all - neutral
                rmi = 50.0;
            } else {
                // Only up movements - maximum
                rmi = 100.0;
            }
        } else {
            double rs = up_smma / down_smma;
            rmi = 100.0 - (100.0 / (1.0 + rs));
        }
        
        rmi_buffer->append(rmi);
    }
    
    // Set the correct buffer position for size() method
    if (rmi_buffer && actual_end > 0) {
        rmi_buffer->set_idx(actual_end - 1);
        
    }
}

double RelativeMomentumIndex::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Check if the rmi line index is valid
    if (static_cast<size_t>(rmi) >= lines->size()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(rmi);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*line)[ago];
}

size_t RelativeMomentumIndex::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    // Check if the rmi line index is valid
    if (static_cast<size_t>(rmi) >= lines->size()) {
        return 0;
    }
    
    auto line = lines->getline(rmi);
    if (!line) {
        return 0;
    }
    return line->size();
}

int RelativeMomentumIndex::getMinPeriod() const {
    return params.period + params.lookback;
}

void RelativeMomentumIndex::calculate() {
    // Get data size from parent LineSeries/DataSeries
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        // Get data line using utility function
        auto data_line = utils::getDataLine(datas[0]);
        if (data_line) {
            // Try to get LineBuffer to access array size
            auto buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (buffer) {
                // Use array().size() to get actual data size
                data_size = buffer->array().size();
            } else {
                // Fallback to regular size() method
                data_size = data_line->size();
            }
        } else {
        }
    }
    
    if (data_size > 0) {
        // Use once() method for batch processing
        once(0, data_size);
    } else {
    }
}

} // namespace indicators