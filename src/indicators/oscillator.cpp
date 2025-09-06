#include "indicators/oscillator.h"
#include "indicators/sma.h"
#include "indicators/ema.h"
#include "dataseries.h"
#include "linebuffer.h"
#include <limits>
#include <cstdio>
#include <algorithm>
#include <iostream>

namespace backtrader {
namespace indicators {

// Oscillator implementation
Oscillator::Oscillator() : Indicator(), data_source_(nullptr), base_indicator_(nullptr), sma_indicator_(nullptr), current_index_(0), period_(30) {
    setup_lines();
    _minperiod(30);
}

Oscillator::Oscillator(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), base_indicator_(nullptr), sma_indicator_(nullptr), current_index_(0), period_(30) {
    setup_lines();
    _minperiod(30);
    
    // Create SMA indicator for single data source mode
    if (data_source) {
        sma_indicator_ = std::make_shared<SMA>(data_source, period_);
        this->datas.push_back(data_source);
    }
}

Oscillator::Oscillator(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), base_indicator_(nullptr), sma_indicator_(nullptr), current_index_(0), period_(period) {
    setup_lines();
    _minperiod(period);
    
    // Create SMA indicator for single data source mode
    if (data_source) {
        sma_indicator_ = std::make_shared<SMA>(data_source, period_);
        this->datas.push_back(data_source);
    }
}

Oscillator::Oscillator(std::shared_ptr<LineSeries> data_source, std::shared_ptr<Indicator> base_indicator) 
    : Indicator(), data_source_(data_source), base_indicator_(base_indicator), sma_indicator_(nullptr), current_index_(0), period_(1) {
    setup_lines();
    
    // Set minperiod from base indicator
    if (base_indicator) {
        _minperiod(base_indicator->getMinPeriod());
        period_ = base_indicator->getMinPeriod();
    } else {
        _minperiod(1);
    }
    
    // Add data sources
    if (data_source) {
        this->datas.push_back(data_source);
    }
    if (base_indicator) {
        // Also add base indicator as second data source
        auto base_lineseries = std::dynamic_pointer_cast<LineSeries>(base_indicator);
        if (base_lineseries) {
            this->datas.push_back(base_lineseries);
        }
    }
}

Oscillator::Oscillator(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(nullptr), base_indicator_(nullptr), sma_indicator_(nullptr), current_index_(0), period_(period) {
    setup_lines();
    _minperiod(period);
    
    // Cast DataSeries to LineSeries for compatibility
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        data_source_ = lineseries;
        this->data = lineseries;
        this->datas.push_back(lineseries);
        
        // Create SMA indicator for single data source mode
        sma_indicator_ = std::make_shared<SMA>(lineseries, period_);
    }
}


double Oscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(osc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Simply delegate to the line's operator[] which handles indexing correctly
    return (*line)[ago];
}

int Oscillator::getMinPeriod() const {
    return minperiod_;
}

size_t Oscillator::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto osc_line = lines->getline(osc);
    if (!osc_line) {
        return 0;
    }
    return osc_line->size();
}

void Oscillator::calculate() {
    // Handle base_indicator case first
    if (base_indicator_ && data_source_) {
        // Get data size from data source
        auto data_line = data_source_->lines->getline(0);
        if (data_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer && data_buffer->array().size() > 0) {
                once(0, data_buffer->array().size());
            }
        }
        return;
    }
    
    // For single data source with SMA, use batch calculation
    if (datas.size() == 1 && sma_indicator_ && datas[0]->lines) {
        // Get data size
        std::shared_ptr<LineSingle> data_line;
        if (datas[0]->lines->size() > 4) {
            data_line = datas[0]->lines->getline(4);  // Close for DataSeries
        } else {
            data_line = datas[0]->lines->getline(0);  // First line for LineSeries
        }
        
        if (data_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            if (data_buffer && data_buffer->array().size() > 0) {
                once(0, data_buffer->array().size());
            }
        }
    } else if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void Oscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void Oscillator::next() {
    if (datas.empty()) return;
    
    auto osc_line = lines->getline(osc);
    if (!osc_line) return;
    
    if (datas.size() > 1) {
        // Two data sources: osc = data0 - data1
        if (datas[0]->lines && datas[1]->lines) {
            auto data0_line = datas[0]->lines->getline(0);
            auto data1_line = datas[1]->lines->getline(0);
            
            if (data0_line && data1_line) {
                osc_line->set(0, (*data0_line)[0] - (*data1_line)[0]);
            }
        }
    }
}

void Oscillator::once(int start, int end) {
    if (datas.empty() && !base_indicator_) return;
    
    auto osc_line = lines->getline(osc);
    if (!osc_line) return;
    
    auto osc_buffer = std::dynamic_pointer_cast<LineBuffer>(osc_line);
    if (!osc_buffer) return;
    
    // Handle case with base_indicator first
    if (base_indicator_ && data_source_) {
        // Calculate oscillator as data - base_indicator
        base_indicator_->calculate();
        
        auto data_line = data_source_->lines->getline(0);
        auto base_line = base_indicator_->lines->getline(0);
        
        if (data_line && base_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            auto base_buffer = std::dynamic_pointer_cast<LineBuffer>(base_line);
            
            if (data_buffer && base_buffer) {
                const auto& data_array = data_buffer->array();
                const auto& base_array = base_buffer->array();
                
                // Clear the buffer first to ensure clean state
                osc_buffer->clear();
                
                // Process data - both arrays should be same size now
                // Both data and base arrays should have the same structure (including initial NaN if present)
                int loop_end = std::min({end, 
                                        static_cast<int>(data_array.size()), 
                                        static_cast<int>(base_array.size())});
                
                for (int i = start; i < loop_end; ++i) {
                    double data_val = data_array[i];
                    double base_val = base_array[i];
                    
                    double osc_val;
                    if (!std::isnan(data_val) && !std::isnan(base_val)) {
                        osc_val = data_val - base_val;
                    } else {
                        osc_val = std::numeric_limits<double>::quiet_NaN();
                    }
                    
                    osc_buffer->append(osc_val);
                }
                
                // Set buffer position to the end
                if (osc_buffer->array().size() > 0) {
                    osc_buffer->set_idx(osc_buffer->array().size() - 1);
                }
            }
        }
        return;
    }
    
    if (datas.size() > 1) {
        // Two data sources: osc = data0 - data1
        if (datas[0]->lines && datas[1]->lines) {
            auto data0_line = datas[0]->lines->getline(0);
            auto data1_line = datas[1]->lines->getline(0);
            
            if (data0_line && data1_line) {
                for (int i = start; i < end; ++i) {
                    osc_line->set(i, (*data0_line)[i] - (*data1_line)[i]);
                }
            }
        }
    } else if (datas.size() == 1 && sma_indicator_) {
        // Single data source with SMA: osc = data - SMA(data)
        
        // First calculate the SMA
        sma_indicator_->calculate();
        
        // Get the data line (close price for DataSeries)
        std::shared_ptr<LineSingle> data_line;
        if (datas[0]->lines->size() > 4) {
            data_line = datas[0]->lines->getline(4);  // Close for DataSeries
        } else {
            data_line = datas[0]->lines->getline(0);  // First line for LineSeries
        }
        
        // Get the SMA line
        auto sma_line = sma_indicator_->lines->getline(0);
        
        if (data_line && sma_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            auto sma_buffer = std::dynamic_pointer_cast<LineBuffer>(sma_line);
            
            if (data_buffer && sma_buffer) {
                const auto& data_array = data_buffer->array();
                const auto& sma_array = sma_buffer->array();
                
                // Clear and calculate oscillator values
                osc_buffer->clear();
                
                // Both data and SMA buffers should have the same size
                // Process them in parallel
                int loop_end = std::min({end, 
                                        static_cast<int>(data_array.size()), 
                                        static_cast<int>(sma_array.size())});
                
                // Debug: Check last values
                static int debug_count = 0;
                if (debug_count++ == 0 && loop_end > 30) {
                    std::cout << "Oscillator::once() debug:\n";
                    std::cout << "  Data array size: " << data_array.size() << "\n";
                    std::cout << "  SMA array size: " << sma_array.size() << "\n";
                    std::cout << "  Last data value: " << data_array[data_array.size()-1] << "\n";
                    std::cout << "  Last SMA value: " << sma_array[sma_array.size()-1] << "\n";
                    std::cout << "  Expected osc: " << (data_array[data_array.size()-1] - sma_array[sma_array.size()-1]) << "\n";
                    
                    // Check if there's initial NaN
                    if (data_array.size() > 0) {
                        std::cout << "  First data value: " << data_array[0] << "\n";
                    }
                    if (sma_array.size() > 0) {
                        std::cout << "  First SMA value: " << sma_array[0] << "\n";
                    }
                    
                    // Check what SMA computes for last 30 data points
                    if (data_array.size() >= 30) {
                        double manual_sma = 0.0;
                        for (int i = data_array.size() - 30; i < static_cast<int>(data_array.size()); ++i) {
                            manual_sma += data_array[i];
                        }
                        manual_sma /= 30.0;
                        std::cout << "  Manual SMA of last 30 data: " << manual_sma << "\n";
                        std::cout << "  Manual Osc (last_data - manual_sma): " << (data_array[data_array.size()-1] - manual_sma) << "\n";
                    }
                }
                
                // Both data and SMA should have the same size after calculation
                // Process them directly without offset
                for (int i = start; i < loop_end; ++i) {
                    if (i < static_cast<int>(data_array.size()) && 
                        i < static_cast<int>(sma_array.size())) {
                        double data_val = data_array[i];
                        double sma_val = sma_array[i];
                        
                        if (!std::isnan(data_val) && !std::isnan(sma_val)) {
                            osc_buffer->append(data_val - sma_val);
                        } else {
                            osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
                        }
                    } else {
                        osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
                    }
                }
                
                // Set buffer position to the end
                if (osc_buffer->array().size() > 0) {
                    osc_buffer->set_idx(osc_buffer->array().size() - 1);
                }
            }
        }
    }
}

// SMAOscillator implementation
SMAOscillator::SMAOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

SMAOscillator::SMAOscillator(std::shared_ptr<LineSeries> data_source, int period) : SMAOscillator() {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

SMAOscillator::SMAOscillator(std::shared_ptr<DataSeries> data_source, int period) : SMAOscillator() {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

void SMAOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double SMAOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(sma_osc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // In C++ LineBuffer: positive indices = past, 0 = current, negative = future
    // In Python backtrader: ago parameter where ago=1 means 1 bar ago
    // Therefore: ago maps directly to LineBuffer index
    return (*line)[ago];
}

int SMAOscillator::getMinPeriod() const {
    return params.period;
}

void SMAOscillator::calculate() {
    // Batch processing mode
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    if (!data_line || data_line->size() == 0) return;
    
    auto osc_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(sma_osc));
    if (!osc_buffer) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // Clear buffer and calculate all values
    osc_buffer->home();
    // Start from 0 to process all data
    once(0, data_buffer->size());
    
    // Position the buffer index at the end after calculation
    if (osc_buffer->size() > 0) {
        osc_buffer->set_idx(osc_buffer->size() - 1);
    }
}

size_t SMAOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto osc_line = lines->getline(sma_osc);
    return osc_line ? osc_line->size() : 0;
}

void SMAOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    auto osc_line = lines->getline(sma_osc);
    
    if (!data_line || !osc_line) return;
    
    // Calculate SMA
    double sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        sum += (*data_line)[-i];
    }
    double sma = sum / params.period;
    
    // Oscillator = data - sma
    osc_line->set(0, (*data_line)[0] - sma);
}

void SMAOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    auto osc_line = lines->getline(sma_osc);
    
    if (!data_line || !osc_line) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    auto osc_buffer = std::dynamic_pointer_cast<LineBuffer>(osc_line);
    if (!data_buffer || !osc_buffer) return;
    
    auto data_array = data_buffer->array();
    
    for (int i = start; i < end && i < static_cast<int>(data_array.size()); ++i) {
        if (i < params.period - 1) {
            // Not enough data for full period
            osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Calculate SMA
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += data_array[i - j];
        }
        double sma = sum / params.period;
        
        // Oscillator = data - sma
        osc_buffer->append(data_array[i] - sma);
    }
}

// EMAOscillator implementation
EMAOscillator::EMAOscillator() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

EMAOscillator::EMAOscillator(std::shared_ptr<LineSeries> data_source, int period) : EMAOscillator() {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    if (data_source) {
        this->data = data_source;
        this->datas.push_back(data_source);
    }
}

EMAOscillator::EMAOscillator(std::shared_ptr<DataSeries> data_source, int period) : EMAOscillator() {
    params.period = period;
    setup_lines();
    _minperiod(period + 1);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

void EMAOscillator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

double EMAOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ema_osc);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Debug output for troubleshooting
    static int debug_count = 0;
    if (debug_count < 10) {
        auto buffer = std::dynamic_pointer_cast<const LineBuffer>(line);
        if (buffer) {
            int idx = buffer->get_idx();
            int access_idx = idx + ago;
            std::cout << "EMAOsc::get(" << ago << ") - buffer idx=" << idx 
                      << ", access_idx=" << access_idx 
                      << ", array_size=" << buffer->array().size() << std::endl;
            debug_count++;
        }
    }
    
    // In C++ LineBuffer: positive indices = past, 0 = current, negative = future
    // In Python backtrader: ago parameter where ago=1 means 1 bar ago
    // Therefore: ago maps directly to LineBuffer index
    return (*line)[ago];
}

int EMAOscillator::getMinPeriod() const {
    return params.period;
}

void EMAOscillator::calculate() {
    // Batch processing mode
    if (datas.empty() || !datas[0]->lines) {
        std::cout << "EMAOsc::calculate() - No data or lines" << std::endl;
        return;
    }
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    if (!data_line) {
        std::cout << "EMAOsc::calculate() - No data line at index " << line_index << std::endl;
        return;
    }
    
    auto osc_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ema_osc));
    if (!osc_buffer) {
        std::cout << "EMAOsc::calculate() - No osc buffer" << std::endl;
        return;
    }
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) {
        std::cout << "EMAOsc::calculate() - Cannot cast data line to buffer" << std::endl;
        return;
    }
    
    // Get actual data size from array
    size_t data_size = data_buffer->array().size();
    std::cout << "EMAOsc::calculate() - Data size: " << data_size 
              << ", osc buffer size: " << osc_buffer->array().size() << std::endl;
    
    if (data_size == 0) {
        std::cout << "EMAOsc::calculate() - Data size is 0" << std::endl;
        return;
    }
    
    // Only reset buffer if it's empty or has different size
    if (osc_buffer->array().size() != data_size) {
        std::cout << "EMAOsc::calculate() - Processing data" << std::endl;
        osc_buffer->reset();
        
        // Process all data
        once(0, data_size);
        
        // Position the buffer index at the end after calculation
        // Use array().size() - 1, not size() - 1, to position correctly
        if (osc_buffer->array().size() > 0) {
            osc_buffer->set_idx(osc_buffer->array().size() - 1);
            std::cout << "EMAOsc::calculate() - Set buffer idx to " << (osc_buffer->array().size() - 1) << std::endl;
        }
    } else {
        std::cout << "EMAOsc::calculate() - Buffer already has data, skipping" << std::endl;
    }
}

size_t EMAOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto osc_line = lines->getline(ema_osc);
    if (!osc_line) return 0;
    
    auto osc_buffer = std::dynamic_pointer_cast<const LineBuffer>(osc_line);
    if (!osc_buffer) {
        return osc_line->size();
    }
    
    // Return the actual number of values in the array
    return osc_buffer->array().size();
}

void EMAOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    auto osc_line = lines->getline(ema_osc);
    
    if (!data_line || !osc_line) return;
    
    // Calculate EMA
    static double ema = 0.0;
    static bool first_calc = true;
    double alpha = 2.0 / (params.period + 1.0);
    
    if (first_calc && data_line->size() >= static_cast<size_t>(params.period)) {
        // First EMA is SMA
        double sum = 0.0;
        for (int i = 0; i < params.period; ++i) {
            sum += (*data_line)[-i];
        }
        ema = sum / params.period;
        first_calc = false;
    } else {
        ema = alpha * (*data_line)[0] + (1.0 - alpha) * ema;
    }
    
    // Oscillator = data - ema
    osc_line->set(0, (*data_line)[0] - ema);
}

void EMAOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Get correct line index - for DataSeries, use close (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = 4;  // Close price for DataSeries
    }
    
    auto data_line = datas[0]->lines->getline(line_index);
    auto osc_line = lines->getline(ema_osc);
    
    if (!data_line || !osc_line) return;
    
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    auto osc_buffer = std::dynamic_pointer_cast<LineBuffer>(osc_line);
    if (!data_buffer || !osc_buffer) return;
    
    double alpha = 2.0 / (params.period + 1.0);
    double alpha1 = 1.0 - alpha;
    double prev_ema = std::numeric_limits<double>::quiet_NaN();
    
    const auto& data_array = data_buffer->array();
    
    // Debug: Check first few values
    static bool debug_once = true;
    if (debug_once && data_array.size() > 5) {
        std::cout << "EMAOsc::once() - First 5 data values: ";
        for (int i = 0; i < 5; ++i) {
            std::cout << data_array[i] << " ";
        }
        std::cout << "\nData array size: " << data_array.size() 
                  << ", start: " << start << ", end: " << end << std::endl;
        std::cout << "Period: " << params.period << ", alpha: " << alpha << std::endl;
        debug_once = false;
    }
    
    // Skip the initial NaN value that LineBuffer starts with
    int actual_start = start;
    if (actual_start == 0 && data_array.size() > 0 && std::isnan(data_array[0])) {
        actual_start = 1;
        // Add initial NaN to match data array
        osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Process data starting from actual_start
    for (int i = actual_start; i < end && i < static_cast<int>(data_array.size()); ++i) {
        double price = data_array[i];
        
        // Skip NaN values in the input data
        if (std::isnan(price)) {
            osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Check if we have enough data for EMA calculation
        // To match Python backtrader behavior, first EMA is placed at index = period - 1
        // when counting from the first valid data point
        if (i < actual_start + params.period - 1) {
            // Not enough data for EMA
            osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
        } else if (i == actual_start + params.period - 1) {
            // Calculate initial SMA as seed for EMA using Kahan summation for precision
            double sum = 0.0;
            double compensation = 0.0;
            int count = 0;
            // Calculate SMA of the first 'period' values starting from actual_start
            if (params.period == 30) {  // Debug for the failing test
                std::cout << "  Calculating SMA at i=" << i << " from indices " << actual_start << " to " << (actual_start + params.period - 1) << std::endl;
            }
            for (int j = 0; j < params.period; ++j) {
                int idx = actual_start + j;
                if (idx >= 0 && idx < data_array.size()) {
                    double val = data_array[idx];
                    if (!std::isnan(val)) {
                        if (params.period == 5 && j < 5) {
                            std::cout << "    idx=" << idx << ", val=" << val << std::endl;
                        }
                        // Kahan summation algorithm
                        double y = val - compensation;
                        double t = sum + y;
                        compensation = (t - sum) - y;
                        sum = t;
                        count++;
                    } else if (params.period == 30 && j < 5) {
                        std::cout << "    idx=" << idx << ", val=nan (skipped)" << std::endl;
                    }
                } else if (params.period == 30 && j < 5) {
                    std::cout << "    idx=" << idx << " out of bounds (array size=" << data_array.size() << ")" << std::endl;
                }
            }
            
            if (count == params.period) {
                prev_ema = sum / params.period;
                osc_buffer->append(price - prev_ema);
                if (params.period == 5) {
                    std::cout << "EMAOsc: Initial SMA calculation at i=" << i << std::endl;
                    std::cout << "  Sum=" << sum << ", count=" << count << ", SMA=" << prev_ema << std::endl;
                    std::cout << "  Current price=" << price << ", osc=" << (price - prev_ema) << std::endl;
                }
            } else {
                osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
                if (params.period == 30) {  // Debug for the failing test
                    std::cout << "EMAOsc: Not enough valid data at i=" << i 
                              << ", count=" << count << std::endl;
                }
            }
        } else {
            // Regular EMA calculation
            if (!std::isnan(prev_ema)) {
                prev_ema = prev_ema * alpha1 + price * alpha;
                osc_buffer->append(price - prev_ema);
                if (i < params.period + 5) {  // Debug first few EMA values
                    std::cout << "EMAOsc: i=" << i << ", price=" << price 
                              << ", ema=" << prev_ema << ", osc=" << (price - prev_ema) << std::endl;
                }
            } else {
                osc_buffer->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
}

} // namespace indicators
} // namespace backtrader