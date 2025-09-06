#include "indicators/kamaosc.h"
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

KAMAOscillator::KAMAOscillator() : Indicator() {
    setup_lines();
    // KAMAOsc uses default KAMA parameters (period=30)
    params.period1 = 30;  // Default KAMA period
    params.period2 = 30;  // Not used but set for consistency
    params.fast = 2;      // KAMA default fast
    params.slow = 30;     // KAMA default slow
    _minperiod(params.period1 + 1);  // KAMA minperiod is period + 1
}

KAMAOscillator::KAMAOscillator(std::shared_ptr<LineSeries> data, int period1, int period2, int fast, int slow) 
    : Indicator() {
    // In Python, KAMAOsc is data - KAMA(data) with KAMA using period1
    // period2, fast, slow are used for KAMA parameters, not dual KAMA
    
    setup_lines();
    params.period1 = period1;  // Use provided period for KAMA
    params.fast = fast;
    params.slow = slow;
    _minperiod(params.period1 + 1);  // KAMA minperiod is period + 1
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
    
    // Create KAMA indicator with specified parameters
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(data);
    if (dataseries) {
        kama1_ = std::make_shared<KAMA>(dataseries, params.period1, params.fast, params.slow);
    } else {
        // Handle LineSeries case
        kama1_ = std::make_shared<KAMA>(data, params.period1, params.fast, params.slow);
    }
}

KAMAOscillator::KAMAOscillator(std::shared_ptr<DataSeries> data, int period1, int period2, int fast, int slow) 
    : Indicator() {
    // In Python, KAMAOsc is data - KAMA(data) with KAMA using period1
    // period2, fast, slow are used for KAMA parameters, not dual KAMA
    
    setup_lines();
    params.period1 = period1;  // Use provided period for KAMA
    params.fast = fast;
    params.slow = slow;
    _minperiod(params.period1 + 1);  // KAMA minperiod is period + 1
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
    
    // Create KAMA indicator with specified parameters
    kama1_ = std::make_shared<KAMA>(data, params.period1, params.fast, params.slow);
}

void KAMAOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // oscillator line
    }
}

double KAMAOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(0);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        // For LineBuffer, use the standard approach like other indicators
        return buffer->get(ago);
    }
    
    return (*line)[ago];
}

int KAMAOscillator::getMinPeriod() const {
    return params.period1 + 1;  // KAMA minperiod is period + 1
}

size_t KAMAOscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto line = lines->getline(0);
    if (!line) {
        return 0;
    }
    
    return line->size();
}

void KAMAOscillator::calculate() {
    // Create KAMA if not already created
    if (!kama1_ && !datas.empty() && datas[0]) {
        auto ds = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (ds) {
            kama1_ = std::make_shared<KAMA>(ds, params.period1, params.fast, params.slow);
        } else {
            auto ls = std::dynamic_pointer_cast<LineSeries>(datas[0]);
            if (ls) {
                kama1_ = std::make_shared<KAMA>(ls, params.period1, params.fast, params.slow);
            }
        }
    }
    
    if (!kama1_) {
        return;
    }
    
    // Calculate KAMA indicator first
    kama1_->calculate();
    
    // Get data size
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        auto ds = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (ds) {
            auto close_line = ds->lines->getline(DataSeries::Close);
            if (close_line) {
                data_size = close_line->size();
                // If size is 0 after reset(), check array size
                if (data_size == 0) {
                    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
                    if (close_buffer) {
                        data_size = close_buffer->array().size();
                    }
                }
            }
        } else {
            auto ls = std::dynamic_pointer_cast<LineSeries>(datas[0]);
            if (ls && ls->lines->size() > 0) {
                auto line = ls->lines->getline(0);
                if (line) {
                    data_size = line->size();
                    // If size is 0 after reset(), check array size
                    if (data_size == 0) {
                        auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(line);
                        if (line_buffer) {
                            data_size = line_buffer->array().size();
                        }
                    }
                }
            }
        }
    }
    
    // Process all data points
    if (data_size > 0) {
        once(0, data_size);
    }
}

void KAMAOscillator::next() {
    if (!kama1_ || !kama2_) return;
    
    // Get KAMA values
    double fast_kama = kama1_->get(0);
    double slow_kama = kama2_->get(0);
    
    if (!std::isnan(fast_kama) && !std::isnan(slow_kama)) {
        // Calculate oscillator as difference between fast and slow KAMA
        double osc_value = fast_kama - slow_kama;
        
        if (lines && lines->size() > 0) {
            auto osc_line = lines->getline(0);
            if (osc_line) osc_line->set(0, osc_value);
        }
    }
}

void KAMAOscillator::once(int start, int end) {
    // KAMAOsc in Python is created as: data - KAMA(data)
    // It's not the difference between two KAMAs!
    
    if (!datas.empty() && datas[0]) {
        auto osc_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
        if (!osc_line) {
            return;
        }
        
        // Clear buffer completely (don't add initial NaN)
        osc_line->clear();
        
        // We only need one KAMA indicator with default parameters
        if (!kama1_) {
            // Create KAMA if not already created
            auto ds = std::dynamic_pointer_cast<DataSeries>(datas[0]);
            if (ds) {
                kama1_ = std::make_shared<KAMA>(ds, params.period1, params.fast, params.slow);
                kama1_->calculate();
            } else {
                // Handle LineSeries case
                auto ls = std::dynamic_pointer_cast<LineSeries>(datas[0]);
                if (ls) {
                    kama1_ = std::make_shared<KAMA>(ls, params.period1, params.fast, params.slow);
                    kama1_->calculate();
                }
            }
        } else {
            // KAMA already exists, but ensure it has the data source
            if (kama1_->datas.empty() && !datas.empty()) {
                kama1_->datas = datas;
            }
            // Recalculate KAMA with current data
            kama1_->calculate();
        }
        
        if (!kama1_) {
            return;
        }
        
        // Get data line
        std::shared_ptr<LineSingle> data_line;
        auto ds = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (ds) {
            data_line = ds->lines->getline(DataSeries::Close);
        } else {
            auto ls = std::dynamic_pointer_cast<LineSeries>(datas[0]);
            if (ls && ls->lines->size() > 0) {
                // For KAMAOsc with LineSeries, use line 0 which contains the price data
                data_line = ls->lines->getline(0);
            }
        }
        
        if (!data_line) return;
        
        // Get data buffer like EMAOsc does
        auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (!data_buffer) return;
        
        // Process each data point: osc = data - KAMA(data)
        // Follow EMAOsc pattern: iterate through data array directly
        const auto& data_array = data_buffer->array();
        int effective_size = static_cast<int>(data_array.size());
        int min_period = params.period1;
        
        // Get KAMA line directly for easier access
        auto kama_line = kama1_->lines->getline(0);
        auto kama_buffer = std::dynamic_pointer_cast<LineBuffer>(kama_line);
        if (!kama_buffer) {
            return;
        }
        
        const auto& kama_array = kama_buffer->array();
        
        // Process all data points to match the input data size exactly
        for (int i = 0; i < effective_size; ++i) {
            // Get data value directly from array (like EMAOsc does)
            double data_val = data_array[i];
            
            // Get KAMA value directly from array at same index
            // Both data and KAMA arrays should be aligned
            double kama_val = std::numeric_limits<double>::quiet_NaN();
            if (i < static_cast<int>(kama_array.size())) {
                kama_val = kama_array[i];
            }
            
            // If KAMA is NaN (not enough data yet), KAMAOsc is also NaN
            if (std::isnan(kama_val)) {
                osc_line->append(std::numeric_limits<double>::quiet_NaN());
                continue;
            }
            
            if (!std::isnan(data_val) && !std::isnan(kama_val)) {
                // Calculate oscillator as data - KAMA
                double osc_value = data_val - kama_val;
                osc_line->append(osc_value);
            } else {
                osc_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
        
        // Set buffer index to last position for proper ago indexing (like EMAOsc)
        if (osc_line->size() > 0) {
            osc_line->set_idx(osc_line->size() - 1);
        }
    }
}

} // namespace indicators
} // namespace backtrader