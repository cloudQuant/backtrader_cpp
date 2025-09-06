#include "indicators/dm.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

DirectionalMovement::DirectionalMovement() : Indicator() {
    setup_lines();
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<DataSeries> data_source)
    : Indicator() {
    setup_lines();
    _minperiod(params.period * 3);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize internal indicators and buffers
    atr_ = std::make_shared<ATR>(data_source, params.period);
    
    // Create temporary series for DM calculations
    plusDM_series_ = std::make_shared<LineSeries>();
    plusDM_series_->lines->add_line(std::make_shared<LineBuffer>());
    minusDM_series_ = std::make_shared<LineSeries>();
    minusDM_series_->lines->add_line(std::make_shared<LineBuffer>());
    dx_series_ = std::make_shared<LineSeries>();
    dx_series_->lines->add_line(std::make_shared<LineBuffer>());
    
    // Create moving averages for DM values
    plusDMav_ = std::make_shared<SMMA>(plusDM_series_, params.period);
    minusDMav_ = std::make_shared<SMMA>(minusDM_series_, params.period);
    adx_smma_ = std::make_shared<SMMA>(dx_series_, params.period);
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<DataSeries> data_source, int period)
    : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period * 3);
    
    // Connect to data source
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Initialize internal indicators and buffers
    atr_ = std::make_shared<ATR>(data_source, params.period);
    
    // Create temporary series for DM calculations
    plusDM_series_ = std::make_shared<LineSeries>();
    plusDM_series_->lines->add_line(std::make_shared<LineBuffer>());
    minusDM_series_ = std::make_shared<LineSeries>();
    minusDM_series_->lines->add_line(std::make_shared<LineBuffer>());
    dx_series_ = std::make_shared<LineSeries>();
    dx_series_->lines->add_line(std::make_shared<LineBuffer>());
    
    // Create moving averages for DM values
    plusDMav_ = std::make_shared<SMMA>(plusDM_series_, params.period);
    minusDMav_ = std::make_shared<SMMA>(minusDM_series_, params.period);
    adx_smma_ = std::make_shared<SMMA>(dx_series_, params.period);
}

void DirectionalMovement::setup_lines() {
    // Always ensure we have the right number of lines
    if (!lines || lines->size() != 4) {
        if (!lines) {
            lines = std::make_shared<Lines>();
        }
        
        // If lines already exist but wrong count, recreate
        if (lines->size() != 4) {
            lines = std::make_shared<Lines>();
            // Create 4 lines: Plus DI, Minus DI, DX, ADX
            for (int i = 0; i < 4; ++i) {
                lines->add_line(std::make_shared<LineBuffer>());
            }
            // Add aliases
            lines->add_alias("plusDI", plusDI);
            lines->add_alias("minusDI", minusDI);
            lines->add_alias("dx", dx);
            lines->add_alias("adx", adx);
        }
    }
}

void DirectionalMovement::calculate() {
    calculate_dm_values();
}

void DirectionalMovement::calculate_dm_values() {
    // Get data lines
    if (!data || !data->lines || data->lines->size() < 5) {
        throw std::runtime_error("Invalid data source for DM calculation - need at least 5 lines");
    }
    
    // DataSeries actual line order from _get_line_names(): 
    // datetime(0), open(1), high(2), low(3), close(4), volume(5), openinterest(6)
    auto high_line = data->lines->getline(2);   // high
    auto low_line = data->lines->getline(4);    // low
    auto close_line = data->lines->getline(4);  // close
    
    if (!high_line || !low_line || !close_line) {
        throw std::runtime_error("Required data lines not available");
    }
    
    int data_size = static_cast<int>(high_line->size());
    if (data_size < 2) return;
    
    // Get output line buffers
    auto plusDI_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(plusDI));
    auto minusDI_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(minusDI));
    auto dx_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dx));
    auto adx_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(adx));
    
    if (!plusDI_line || !minusDI_line || !dx_line || !adx_line) {
        throw std::runtime_error("Output line buffers not available");
    }
    
    // Get internal buffers
    auto plusDM_buffer = std::dynamic_pointer_cast<LineBuffer>(plusDM_series_->lines->getline(0));
    auto minusDM_buffer = std::dynamic_pointer_cast<LineBuffer>(minusDM_series_->lines->getline(0));
    auto dx_buffer = std::dynamic_pointer_cast<LineBuffer>(dx_series_->lines->getline(0));
    
    if (!plusDM_buffer || !minusDM_buffer || !dx_buffer) {
        throw std::runtime_error("Internal buffers not available");
    }
    
    // Reset all buffers
    plusDM_buffer->reset();
    minusDM_buffer->reset();
    dx_buffer->reset();
    plusDI_line->reset();
    minusDI_line->reset();
    dx_line->reset();
    adx_line->reset();
    
    // First value is NaN
    plusDM_buffer->append(std::numeric_limits<double>::quiet_NaN());
    minusDM_buffer->append(std::numeric_limits<double>::quiet_NaN());
    
    // Calculate DM values for each bar
    for (int i = 1; i < data_size; ++i) {
        // Get current and previous values (reverse chronological order)
        int curr_idx = data_size - 1 - i;
        int prev_idx = data_size - 1 - (i - 1);
        
        double high = (*high_line)[curr_idx];
        double low = (*low_line)[curr_idx];
        double prev_high = (*high_line)[prev_idx];
        double prev_low = (*low_line)[prev_idx];
        
        // Calculate directional movements
        double up_move = high - prev_high;
        double down_move = prev_low - low;
        
        // Plus DM: up_move if up_move > down_move and up_move > 0, else 0
        double plusDM = 0.0;
        if (up_move > down_move && up_move > 0.0) {
            plusDM = up_move;
        }
        
        // Minus DM: down_move if down_move > up_move and down_move > 0, else 0
        double minusDM = 0.0;
        if (down_move > up_move && down_move > 0.0) {
            minusDM = down_move;
        }
        
        plusDM_buffer->append(plusDM);
        minusDM_buffer->append(minusDM);
    }
    
    // Calculate ATR
    atr_->calculate();
    
    // Calculate smoothed DM averages
    plusDMav_->calculate();
    minusDMav_->calculate();
    
    // Calculate DI values
    auto atr_line = atr_->lines->getline(0);
    auto plusDMav_line = plusDMav_->lines->getline(0);
    auto minusDMav_line = minusDMav_->lines->getline(0);
    
    if (!atr_line || !plusDMav_line || !minusDMav_line) {
        throw std::runtime_error("Failed to get indicator lines");
    }
    
    for (int i = 0; i < data_size; ++i) {
        double atr_val = (*atr_line)[i];
        double plusDMav_val = (*plusDMav_line)[i];
        double minusDMav_val = (*minusDMav_line)[i];
        
        if (std::isnan(atr_val) || atr_val == 0.0) {
            plusDI_line->append(std::numeric_limits<double>::quiet_NaN());
            minusDI_line->append(std::numeric_limits<double>::quiet_NaN());
            dx_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // DI+ = 100 * smoothed(+DM) / ATR
            double plus_di = 100.0 * plusDMav_val / atr_val;
            plusDI_line->append(plus_di);
            
            // DI- = 100 * smoothed(-DM) / ATR
            double minus_di = 100.0 * minusDMav_val / atr_val;
            minusDI_line->append(minus_di);
            
            // DX = 100 * |DI+ - DI-| / (DI+ + DI-)
            double di_sum = plus_di + minus_di;
            if (di_sum == 0.0) {
                dx_buffer->append(0.0);
            } else {
                double dx_val = 100.0 * std::abs(plus_di - minus_di) / di_sum;
                dx_buffer->append(dx_val);
            }
        }
    }
    
    // Copy DX values to output
    for (int i = 0; i < data_size; ++i) {
        dx_line->append((*dx_buffer)[i]);
    }
    
    // Calculate ADX (smoothed DX)
    adx_smma_->calculate();
    auto adx_smma_line = adx_smma_->lines->getline(0);
    
    if (!adx_smma_line) {
        throw std::runtime_error("Failed to get ADX SMMA line");
    }
    
    // Copy ADX values to output
    for (int i = 0; i < data_size; ++i) {
        adx_line->append((*adx_smma_line)[i]);
    }
}

void DirectionalMovement::next() {
    calculate();
}

void DirectionalMovement::once(int start, int end) {
    calculate();
}

double DirectionalMovement::getDIPlus(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(plusDI);
    if (!line) return 0.0;
    return (*line)[ago];
}

double DirectionalMovement::getDIMinus(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(minusDI);
    if (!line) return 0.0;
    return (*line)[ago];
}

double DirectionalMovement::getDX(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(dx);
    if (!line) return 0.0;
    return (*line)[ago];
}

double DirectionalMovement::getADX(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(adx);
    if (!line) return 0.0;
    return (*line)[ago];
}

size_t DirectionalMovement::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto line = lines->getline(plusDI);
    if (!line) {
        return 0;
    }
    return line->size();
}

} // namespace indicators
} // namespace backtrader