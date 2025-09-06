#include "indicators/envelope.h"
#include "dataseries.h"
#include <limits>
#include <iostream>
#include <iomanip>

namespace backtrader {
namespace indicators {

// Envelope implementation
Envelope::Envelope() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(30);  // Default to 30 like Python SMA envelope
}

Envelope::Envelope(std::shared_ptr<LineSeries> data) 
    : Indicator(), data_source_(data), current_index_(0) {
    // Use default params (period=30, perc=2.5)
    setup_lines();
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
}

Envelope::Envelope(std::shared_ptr<LineSeries> data, double perc) 
    : Indicator(), data_source_(data), current_index_(0) {
    params.perc = perc;
    setup_lines();
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
}

Envelope::Envelope(std::shared_ptr<LineSeries> data, int period, double perc) 
    : Indicator(), data_source_(data), current_index_(0) {
    params.perc = perc;
    params.period = period;  // Store the period
    setup_lines();
    _minperiod(period);
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
}

Envelope::Envelope(std::shared_ptr<DataSeries> data) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    // Use default params (period=30, perc=2.5)
    setup_lines();
    _minperiod(params.period);
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
}

Envelope::Envelope(std::shared_ptr<DataSeries> data, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.perc = perc;
    params.period = period;  // Store the period
    setup_lines();
    _minperiod(period);
    
    // Set data for test framework compatibility
    this->data = data;
    this->datas.push_back(data);
}

double Envelope::get(int ago) const {
    return getMidLine(ago);  // Default get() returns the mid line
}

int Envelope::getMinPeriod() const {
    return params.period;  // Return the configured period
}

double Envelope::getMidLine(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(src);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        if (buffer->size() == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // LineBuffer already correctly implements Python-style indexing
        // Just pass the ago value directly
        return buffer->get(ago);
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}

double Envelope::getUpperLine(int ago) const {
    if (!lines || lines->size() <= top) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(top);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        if (buffer->size() == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // LineBuffer already correctly implements Python-style indexing
        // Just pass the ago value directly
        return buffer->get(ago);
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}

double Envelope::getLowerLine(int ago) const {
    if (!lines || lines->size() <= bot) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(bot);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        if (buffer->size() == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        
        // LineBuffer already correctly implements Python-style indexing
        // Just pass the ago value directly
        return buffer->get(ago);
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}

void Envelope::calculate() {
    // std::cerr << "Envelope::calculate() called" << std::endl;
    // For indicators, calculate() is usually called once to process all data
    // Get data from appropriate source
    std::shared_ptr<LineSingle> data_line;
    size_t data_size = 0;
    
    if (!datas.empty() && datas[0]) {
        if (datas[0]->lines && datas[0]->lines->size() > 0) {
            // For DataSeries (OHLCV), use close line (index 4)
            // For LineSeries with single line, use index 0
            int line_index = 0;
            auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
            if (dataseries) {
                line_index = 4;  // Close price for DataSeries
            }
            data_line = datas[0]->lines->getline(line_index);
        }
    } else if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        data_line = data_source_->lines->getline(0);
    }
    
    if (data_line) {
        auto buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
        if (buffer) {
            data_size = buffer->array().size();
        }
    }
    
    // Only calculate if we haven't processed this data yet
    auto src_line = lines->getline(src);
    auto src_buffer = std::dynamic_pointer_cast<LineBuffer>(src_line);
    
    // Check actual array size, not just size() which might be 0 due to indexing
    size_t current_size = 0;
    if (src_buffer) {
        current_size = src_buffer->array().size();
    }
    
    if (current_size < data_size) {
        // Calculate from where we left off
        int start = current_size;
        once(start, data_size);
    }
}

void Envelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void Envelope::prenext() {
    Indicator::prenext();
}

void Envelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto src_line = lines->getline(src);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (data_line && src_line && top_line && bot_line) {
        double data_value = (*data_line)[0];
        double perc = params.perc / 100.0;
        
        src_line->set(0, data_value);
        top_line->set(0, data_value * (1.0 + perc));
        bot_line->set(0, data_value * (1.0 - perc));
    }
}

void Envelope::once(int start, int end) {
    // Get data from appropriate source
    std::shared_ptr<LineSingle> data_line;
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        // For DataSeries (OHLCV), use close line (index 4)
        // For LineSeries with single line, use index 0
        int line_index = 0;
        auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (dataseries) {
            line_index = 4;  // Close price for DataSeries
        }
        data_line = datas[0]->lines->getline(line_index);
    } else if (data_source_ && data_source_->lines) {
        // For LineSeries constructor
        data_line = data_source_->lines->getline(0);
    }
    
    auto src_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(src));
    auto top_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(top));
    auto bot_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(bot));
    
    if (!data_line || !src_line || !top_line || !bot_line) return;
    
    // Get LineBuffer for direct array access
    auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!data_buffer) return;
    
    // Clear line buffers only if starting from beginning and they're empty
    if (start == 0 && src_line->array().empty()) {
        src_line->reset();
        top_line->reset();
        bot_line->reset();
    }
    
    const auto& data_array = data_buffer->array();
    size_t data_size = data_array.size();
    double perc = params.perc / 100.0;
    // Get period from params
    int period = params.period;
    
    // Debug: check data array
    std::cerr << "Envelope once(): data_size=" << data_size << ", period=" << period 
              << ", start=" << start << ", end=" << end << std::endl;
    if (data_size > 0) {
        std::cerr << "First data value: " << data_array[0] << std::endl;
        if (data_size > 29) {
            std::cerr << "Data[29]: " << data_array[29] << std::endl;
        }
    }
    
    // Calculate SMA for mid line and envelope bands
    // Note: If there's an initial NaN in data, we need to account for off-by-one error
    int data_offset = 0;
    if (data_size > 0 && std::isnan(data_array[0])) {
        data_offset = 1;
    }
    
    for (int i = start; i < end && i < static_cast<int>(data_size); ++i) {
        // Adjust the minimum period check based on data offset
        if (i < period - 1 + data_offset) {
            // Not enough data for SMA
            src_line->append(std::numeric_limits<double>::quiet_NaN());
            top_line->append(std::numeric_limits<double>::quiet_NaN());
            bot_line->append(std::numeric_limits<double>::quiet_NaN());
        } else {
            // Calculate SMA
            double sum = 0.0;
            int count = 0;
            for (int j = 0; j < period; ++j) {
                int idx = i - period + 1 + j;
                if (idx >= 0 && idx < static_cast<int>(data_size)) {
                    double value = data_array[idx];
                    if (!std::isnan(value)) {
                        sum += value;
                        count++;
                    }
                }
            }
            
            if (count == period) {
                double sma_value = sum / period;
                src_line->append(sma_value);
                top_line->append(sma_value * (1.0 + perc));
                bot_line->append(sma_value * (1.0 - perc));
                
                // Debug: output key values
                if (i == 254 || i == 255 || i == 29 || i == 30 || i == 142 || i == 143) {
                    std::cerr << "SMA[" << i << "] = " << std::fixed << std::setprecision(6) << sma_value 
                              << ", upper = " << (sma_value * (1.0 + perc))
                              << ", lower = " << (sma_value * (1.0 - perc)) << std::endl;
                }
                
                // Debug output for first few SMA values
                // if (src_line->array().size() <= 3) {
                //     std::cerr << "SMA[" << i << "] = " << std::fixed << std::setprecision(6) << sma_value << std::endl;
                // }
            } else {
                src_line->append(std::numeric_limits<double>::quiet_NaN());
                top_line->append(std::numeric_limits<double>::quiet_NaN());
                bot_line->append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    
    // Set LineBuffer indices to last valid position for proper ago indexing
    if (end > start && src_line->array().size() > 0) {
        src_line->set_idx(src_line->array().size() - 1);
        top_line->set_idx(top_line->array().size() - 1);
        bot_line->set_idx(bot_line->array().size() - 1);
        
        // Debug output
        // std::cerr << "Envelope once() completed: buffer size = " << src_line->array().size() 
        //           << ", _idx = " << src_line->get_idx() << std::endl;
        
        // // Check values at key points
        // if (src_line->array().size() >= 255) {
        //     std::cerr << "SMA[254] (ago=0) = " << src_line->get(0) << std::endl;
        //     std::cerr << "SMA[30] (ago=-224) = " << src_line->get(-224) << std::endl;
        //     std::cerr << "SMA[142] (ago=-112) = " << src_line->get(-112) << std::endl;
        // }
    }
}

// SimpleMovingAverageEnvelope implementation
SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    sma_ = std::make_shared<SMA>(params.period);
    
    _minperiod(params.period);
}

SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    sma_ = std::make_shared<SMA>(data_source, params.period);
    
    _minperiod(params.period);
}

SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    std::cout << "*** CONSTRUCTOR: SimpleMovingAverageEnvelope period=" << period << " perc=" << perc << " ***" << std::endl;
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    // Set up datas array for Indicator base class
    this->data = data_source;
    this->datas.push_back(data_source);
    
    sma_ = std::make_shared<SMA>(data_source, period);
    
    _minperiod(params.period);
}

SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope(std::shared_ptr<LineBuffer> data_source, int period, double perc) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    // Create a LineSeries wrapper for the LineBuffer
    auto line_series = std::make_shared<LineSeries>();
    line_series->lines->add_line(data_source);
    data_source_ = line_series;
    
    // Create SMA with the wrapper
    sma_ = std::make_shared<SMA>(line_series, period);
    
    _minperiod(params.period);
}

SimpleMovingAverageEnvelope::SimpleMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    // Set up datas array for Indicator base class
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Create SMA with DataSeries
    sma_ = std::make_shared<SMA>(data_source, period);
    
    _minperiod(params.period);
}

double SimpleMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(sma);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert backtrader ago semantics to LineBuffer index
    int linebuffer_index = -ago;
    
    if (linebuffer_index < 0 || linebuffer_index >= static_cast<int>(line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[linebuffer_index];
}

int SimpleMovingAverageEnvelope::getMinPeriod() const {
    return params.period;
}

void SimpleMovingAverageEnvelope::calculate() {
    std::cout << "*** SIMPLEENVELOPE::CALCULATE() CALLED ***" << std::endl;
    std::cout << "*** data_source_=" << (data_source_ ? "set" : "null") 
              << " datas.size()=" << datas.size() << " ***" << std::endl;
    
    if (data_source_) {
        auto data_line = data_source_->lines->getline(0);
        if (data_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            size_t data_size = data_buffer ? data_buffer->array().size() : data_line->size();
            
            // Check if we've already calculated for this data size to avoid redundant calculations
            auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
            if (sma_line && sma_line->array().size() >= data_size) {
                std::cout << "*** SMAEnvelope already calculated for size " << data_size << ", skipping ***" << std::endl;
                return;
            }
            
            // Always use once() for data_source_ mode as it handles both scenarios correctly
            std::cout << "*** SMAEnvelope calling once(0, " << data_size << ") ***" << std::endl;
            once(0, data_size);
        }
    } else {
        std::cout << "*** SMAEnvelope calling next() ***" << std::endl;
        // Use existing next() method for traditional calculation
        next();
    }
}

std::shared_ptr<LineBuffer> SimpleMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    
    auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
    return line_buffer;
}

void SimpleMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SimpleMovingAverageEnvelope::prenext() {
    // prenext() is protected, so we can't call it directly
    Indicator::prenext();
}

void SimpleMovingAverageEnvelope::next() {
    std::cerr << "SimpleMovingAverageEnvelope::next() called" << std::endl;
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to SMA if not already done
    if (sma_->datas.empty() && !datas.empty()) {
        sma_->datas = datas;
        sma_->data = datas[0];
    }
    
    // Update SMA using next method
    sma_->next();
    
    auto sma_line = lines->getline(sma);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto sma_indicator_line = sma_->lines->getline(0);
    
    if (sma_line && top_line && bot_line && sma_indicator_line) {
        double sma_value = (*sma_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        sma_line->set(0, sma_value);
        top_line->set(0, sma_value * (1.0 + perc));
        bot_line->set(0, sma_value * (1.0 - perc));
    }
}

void SimpleMovingAverageEnvelope::once(int start, int end) {
    std::cerr << "SimpleMovingAverageEnvelope::once(" << start << ", " << end << ") called" << std::endl;
    if (datas.empty() || !datas[0]->lines) {
        return;
    }
    
    // Connect data to SMA if not already done
    if (sma_->datas.empty() && !datas.empty()) {
        sma_->datas = datas;
        sma_->data = datas[0];
    }
    
    // Call SMA's once method to calculate all values
    sma_->once(start, end);
    
    auto sma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(sma));
    auto top_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(top));
    auto bot_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(bot));
    auto sma_indicator_line = sma_->lines->getline(0);
    
    if (!sma_line || !top_line || !bot_line || !sma_indicator_line) return;
    
    // Get SMA LineBuffer for direct array access
    auto sma_buffer = std::dynamic_pointer_cast<LineBuffer>(sma_indicator_line);
    if (!sma_buffer) {
        return;
    }
    
    const auto& sma_array = sma_buffer->array();
    double perc = params.perc / 100.0;
    
    // Debug: Check SMA values
    if (sma_array.size() > 0) {
        std::cerr << "*** NEW DEBUG: SMA first value=" << sma_array[0] << std::endl;
        std::cerr << "*** NEW DEBUG: SMA last value=" << sma_array[sma_array.size()-1] << std::endl;
        if (sma_array.size() > 225) {
            int pos30 = 30;
            int pos225 = sma_array.size()-225;  // This calculates to 256-225=31, which is wrong
            int correct_pos = 255 - 225;  // This should be 255-225=30
            std::cerr << "*** NEW DEBUG: SMA[30]=" << sma_array[pos30] << std::endl;
            std::cerr << "*** NEW DEBUG: SMA[" << pos225 << "]=" << sma_array[pos225] << std::endl;
            std::cerr << "*** NEW DEBUG: SMA buffer size=" << sma_array.size() << std::endl;
            std::cerr << "*** NEW DEBUG: ago=-225 should access position " << correct_pos << " (255-225)" << std::endl;
            std::cerr << "*** NEW DEBUG: but size-225 gives position " << pos225 << std::endl;
        }
    }
    
    // Calculate envelope bands based on SMA values
    // Clear buffers and rebuild with same structure as SMA buffer
    sma_line->reset();  // This creates [NaN] with _idx=0
    top_line->reset();
    bot_line->reset();
    
    // Copy SMA values exactly - use set(0) for first value to overwrite initial NaN,
    // then append() for remaining values
    for (size_t i = 0; i < sma_array.size(); ++i) {
        double sma_value = sma_array[i];
        double upper_value, lower_value;
        
        if (!std::isnan(sma_value)) {
            upper_value = sma_value * (1.0 + perc);
            lower_value = sma_value * (1.0 - perc);
        } else {
            upper_value = std::numeric_limits<double>::quiet_NaN();
            lower_value = std::numeric_limits<double>::quiet_NaN();
        }
        
        if (i == 0) {
            // First value: overwrite the initial NaN from reset()
            sma_line->set(0, sma_value);
            top_line->set(0, upper_value);
            bot_line->set(0, lower_value);
        } else {
            // Subsequent values: append to grow the buffer
            sma_line->append(sma_value);
            top_line->append(upper_value);
            bot_line->append(lower_value);
        }
    }
    
    // Set LineBuffer indices to last valid position
    if (sma_array.size() > 0) {
        int target_idx = sma_array.size() - 1;
        sma_line->set_idx(target_idx);
        top_line->set_idx(target_idx);
        bot_line->set_idx(target_idx);
        
        std::cerr << "*** DEBUG: Set envelope _idx to " << target_idx << std::endl;
        std::cerr << "*** DEBUG: SMA buffer _idx is " << sma_buffer->get_idx() << std::endl;
        std::cerr << "*** DEBUG: Envelope buffer _idx is " << sma_line->get_idx() << std::endl;
        
        // Check if envelope buffer copied correctly at key positions
        auto env_array = sma_line->array();
        if (env_array.size() > 30) {
            std::cerr << "*** DEBUG: Envelope[30] = " << env_array[30] << std::endl;
            std::cerr << "*** DEBUG: ago=-225 test: " << sma_line->get(-225) << std::endl;
        }
    }
}

// ExponentialMovingAverageEnvelope implementation
ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    ema_ = std::make_shared<EMA>(params.period);
    
    _minperiod(params.period);
}

ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    ema_ = std::make_shared<EMA>(data_source, params.period);
    
    _minperiod(params.period);
}

ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    ema_ = std::make_shared<EMA>(data_source, period);
    
    _minperiod(params.period);
}

ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    
    ema_ = std::make_shared<EMA>(data_source, params.period);
    
    _minperiod(params.period);
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

ExponentialMovingAverageEnvelope::ExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    ema_ = std::make_shared<EMA>(data_source, period);
    
    _minperiod(params.period);
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

double ExponentialMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(ema);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just pass the ago value directly
    return (*line)[ago];
}

void ExponentialMovingAverageEnvelope::calculate() {
    // Check if datas is set up
    if (datas.empty() && data_source_) {
        // Set up data connection if not already done
        this->data = data_source_;
        this->datas.push_back(data_source_);
    }
    
    // Use once() method for batch processing all data points
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        // For LineSeries with single line, use index 0
        // For DataSeries with OHLCV, use close line at index 3
        int line_index = 0;
        if (datas[0]->lines->size() > 3) {
            line_index = 3;  // Close line in DataSeries
        }
        
        auto data_line = datas[0]->lines->getline(line_index);
        if (data_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            data_size = data_buffer ? data_buffer->array().size() : data_line->size();
        }
    }
    
    if (data_size > 0) {
        once(0, data_size);
    } else {
        // Fallback to next() if no batch data available
        next();
    }
}

int ExponentialMovingAverageEnvelope::getMinPeriod() const {
    return params.period;
}

std::shared_ptr<LineBuffer> ExponentialMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    
    auto original_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
    if (!original_buffer) {
        return nullptr;
    }
    
    // LineBuffer already correctly implements Python-style indexing
    // Just return the buffer directly
    return original_buffer;
}

void ExponentialMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void ExponentialMovingAverageEnvelope::prenext() {
    // prenext() is protected, so we can't call it directly
    Indicator::prenext();
}

void ExponentialMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to EMA if not already done
    if (ema_->datas.empty() && !datas.empty()) {
        ema_->datas = datas;
    }
    
    // Update EMA using calculate method
    ema_->calculate();
    
    auto ema_line = lines->getline(ema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto ema_indicator_line = ema_->lines->getline(0);
    
    if (ema_line && top_line && bot_line && ema_indicator_line) {
        double ema_value = (*ema_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        ema_line->set(0, ema_value);
        top_line->set(0, ema_value * (1.0 + perc));
        bot_line->set(0, ema_value * (1.0 - perc));
    }
}

void ExponentialMovingAverageEnvelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to EMA if not already done
    if (ema_->datas.empty() && !datas.empty()) {
        ema_->datas = datas;
        ema_->data = datas[0];
    }
    
    // Call EMA's calculate method which will handle batch processing
    ema_->calculate();
    
    auto ema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(ema));
    auto top_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(top));
    auto bot_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(bot));
    auto ema_indicator_line = ema_->lines->getline(0);
    
    if (!ema_line || !top_line || !bot_line || !ema_indicator_line) return;
    
    // Get EMA LineBuffer for direct array access
    auto ema_buffer = std::dynamic_pointer_cast<LineBuffer>(ema_indicator_line);
    if (!ema_buffer) return;
    
    const auto& ema_array = ema_buffer->array();
    double perc = params.perc / 100.0;
    
    // Clear line buffers only if starting from beginning
    if (start == 0) {
        ema_line->reset();
        top_line->reset();
        bot_line->reset();
    }
    
    // Calculate envelope bands based on EMA values
    for (size_t i = 0; i < ema_array.size(); ++i) {
        double ema_value = ema_array[i];
        if (!std::isnan(ema_value)) {
            ema_line->append(ema_value);
            top_line->append(ema_value * (1.0 + perc));
            bot_line->append(ema_value * (1.0 - perc));
        } else {
            ema_line->append(std::numeric_limits<double>::quiet_NaN());
            top_line->append(std::numeric_limits<double>::quiet_NaN());
            bot_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Set LineBuffer indices to last valid position for proper ago indexing
    // For proper ago indexing, _idx should be set to the last element index
    // so that ago=0 accesses the last element, ago=1 accesses second-to-last, etc.
    if (ema_array.size() > 0) {
        ema_line->set_idx(ema_array.size() - 1);
        top_line->set_idx(ema_array.size() - 1);
        bot_line->set_idx(ema_array.size() - 1);
    }
}

// DoubleExponentialMovingAverageEnvelope implementation
DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>();
    
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>(data_source, params.period);
    
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc)
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>(data_source, period);
    
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>(data_source, params.period);
    
    _minperiod(2 * params.period - 1);
    this->data = data_source;
    this->datas.push_back(data_source);
}

DoubleExponentialMovingAverageEnvelope::DoubleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc)
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    dema_ = std::make_shared<DoubleExponentialMovingAverage>(data_source, period);
    
    _minperiod(2 * params.period - 1);
    this->data = data_source;
    this->datas.push_back(data_source);
}

double DoubleExponentialMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(dema);
    if (!line || line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert backtrader ago semantics to LineBuffer index
    // ago=0 means current (most recent) value -> index=0
    // ago=-1 means previous value -> index=1
    int linebuffer_index = -ago;
    
    if (linebuffer_index < 0 || linebuffer_index >= static_cast<int>(line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[linebuffer_index];
}

int DoubleExponentialMovingAverageEnvelope::getMinPeriod() const {
    return 2 * params.period - 1;
}

void DoubleExponentialMovingAverageEnvelope::calculate() {
    // Check if datas is set up
    if (datas.empty() && data_source_) {
        // Set up data connection if not already done
        this->data = data_source_;
        this->datas.push_back(data_source_);
    }
    
    // Use once() method for batch processing all data points
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        // For LineSeries with single line, use index 0
        // For DataSeries with OHLCV, use close line at index 3
        int line_index = 0;
        if (datas[0]->lines->size() > 3) {
            line_index = 3;  // Close line in DataSeries
        }
        
        auto data_line = datas[0]->lines->getline(line_index);
        if (data_line) {
            auto data_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
            data_size = data_buffer ? data_buffer->array().size() : data_line->size();
        }
    }
    
    if (data_size > 0) {
        once(0, data_size);
    } else {
        // Fallback to next() if no batch data available
        next();
    }
}

std::shared_ptr<LineBuffer> DoubleExponentialMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    
    auto original_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
    if (!original_buffer) {
        return nullptr;
    }
    
    // Create a wrapper that handles Python-style negative indexing
    class PythonStyleLineBuffer : public LineBuffer {
    private:
        std::shared_ptr<LineBuffer> wrapped_buffer_;
        
    public:
        PythonStyleLineBuffer(std::shared_ptr<LineBuffer> buffer) : wrapped_buffer_(buffer) {}
        
        double get(int ago = 0) const override {
            if (ago >= 0) {
                // Standard positive ago indexing
                return wrapped_buffer_->get(ago);
            } else {
                // Python-style negative indexing
                const auto& arr = wrapped_buffer_->array();
                int array_index = static_cast<int>(arr.size()) + ago;
                
                if (array_index < 0 || array_index >= static_cast<int>(arr.size())) {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                
                return arr[array_index];
            }
        }
        
        // Forward all other methods to the wrapped buffer
        void set(int index, double value) override { wrapped_buffer_->set(index, value); }
        double operator[](int index) const override { return get(index); }
        size_t size() const override { return wrapped_buffer_->size(); }
        const std::vector<double>& array() const { return wrapped_buffer_->array(); }
        void append(double value) { wrapped_buffer_->append(value); }
        void reset() { wrapped_buffer_->reset(); }
        int get_idx() const { return wrapped_buffer_->get_idx(); }
        void set_idx(int idx, bool force = false) { wrapped_buffer_->set_idx(idx, force); }
    };
    
    return std::make_shared<PythonStyleLineBuffer>(original_buffer);
}

void DoubleExponentialMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void DoubleExponentialMovingAverageEnvelope::prenext() {
    Indicator::prenext();
}

void DoubleExponentialMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to DEMA if not already done
    if (dema_->datas.empty() && !datas.empty()) {
        dema_->datas = datas;
    }
    
    // Update DEMA using public calculate method
    dema_->calculate();
    
    auto dema_line = lines->getline(dema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto dema_indicator_line = dema_->lines->getline(0);
    
    if (dema_line && top_line && bot_line && dema_indicator_line) {
        double dema_value = (*dema_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        dema_line->set(0, dema_value);
        top_line->set(0, dema_value * (1.0 + perc));
        bot_line->set(0, dema_value * (1.0 - perc));
    }
}

void DoubleExponentialMovingAverageEnvelope::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to DEMA if not already done
    if (dema_->datas.empty() && !datas.empty()) {
        dema_->datas = datas;
        dema_->data = datas[0];
    }
    
    // Calculate DEMA for all data at once
    dema_->calculate();
    
    auto dema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dema));
    auto top_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(top));
    auto bot_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(bot));
    auto dema_indicator_line = dema_->lines->getline(0);
    
    if (!dema_line || !top_line || !bot_line || !dema_indicator_line) return;
    
    // Get DEMA LineBuffer for direct array access
    auto dema_buffer = std::dynamic_pointer_cast<LineBuffer>(dema_indicator_line);
    if (!dema_buffer) return;
    
    const auto& dema_array = dema_buffer->array();
    double perc = params.perc / 100.0;
    
    // Clear and rebuild buffers
    dema_line->reset();
    top_line->reset();
    bot_line->reset();
    
    // Copy all values from DEMA
    for (size_t i = 0; i < dema_array.size(); ++i) {
        double dema_value = dema_array[i];
        dema_line->append(dema_value);
        
        if (!std::isnan(dema_value)) {
            top_line->append(dema_value * (1.0 + perc));
            bot_line->append(dema_value * (1.0 - perc));
        } else {
            top_line->append(std::numeric_limits<double>::quiet_NaN());
            bot_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Set the index to the last element
    if (dema_line->size() > 0) {
        int last_idx = static_cast<int>(dema_line->size() - 1);
        dema_line->set_idx(last_idx);
        top_line->set_idx(last_idx);
        bot_line->set_idx(last_idx);
    }
}

// TripleExponentialMovingAverageEnvelope implementation
TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>();
    
    _minperiod(3 * params.period - 2);
}

TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    
    _minperiod(3 * params.period - 2);
    
    // Add data source to datas array for calculate() to work
    this->data = data_source;
    this->datas.push_back(data_source);
}

TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, period);
    
    _minperiod(3 * params.period - 2);
    
    // Add data source to datas array for calculate() to work
    this->data = data_source;
    this->datas.push_back(data_source);
}

TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, params.period);
    
    _minperiod(3 * params.period - 2);
    
    // Add data source to datas vector
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

TripleExponentialMovingAverageEnvelope::TripleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    tema_ = std::make_shared<TripleExponentialMovingAverage>(data_source, period);
    
    _minperiod(3 * params.period - 2);
    
    // Add data source to datas vector
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

double TripleExponentialMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(tema);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert backtrader ago semantics to LineBuffer index
    int linebuffer_index = -ago;
    
    if (linebuffer_index < 0 || linebuffer_index >= static_cast<int>(line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[linebuffer_index];
}

int TripleExponentialMovingAverageEnvelope::getMinPeriod() const {
    return 3 * params.period - 2;
}

void TripleExponentialMovingAverageEnvelope::calculate() {
    // Debug output
    std::cout << "TEMAEnvelope::calculate() called" << std::endl;
    std::cout << "  datas.empty()=" << datas.empty() 
              << ", data_source_=" << (data_source_ ? "set" : "null")
              << ", tema_=" << (tema_ ? "set" : "null") << std::endl;
    
    // For DataSeries-based calculation (batch mode)
    if (!datas.empty() && datas[0] && datas[0]->lines) {
        // Get correct line index - for DataSeries, use close (index 4)
        int line_index = 0;
        auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        if (dataseries) {
            line_index = 4;  // Close price for DataSeries
        }
        
        auto data_line = datas[0]->lines->getline(line_index);
        if (data_line && data_line->size() > 0) {
            std::cout << "  Data line size: " << data_line->size() << std::endl;
            // First calculate TEMA
            if (tema_) {
                std::cout << "  Calculating TEMA..." << std::endl;
                tema_->calculate();
                std::cout << "  TEMA size after calculate: " << tema_->size() << std::endl;
            }
            
            // Then calculate envelope bands
            std::cout << "  Calling once() to calculate envelopes..." << std::endl;
            once(0, data_line->size());
        }
    } else if (data_source_ && data_source_->lines) {
        // Implementation for LineSeries-based calculation
        auto data_line = data_source_->lines->getline(0);
        if (data_line && data_line->size() > 0) {
            std::cout << "  LineSeries mode - Data line size: " << data_line->size() << std::endl;
            // First calculate TEMA
            if (tema_) {
                std::cout << "  Calculating TEMA..." << std::endl;
                tema_->calculate();
                std::cout << "  TEMA size after calculate: " << tema_->size() << std::endl;
            }
            
            // Then calculate envelope bands
            std::cout << "  Calling once() to calculate envelopes..." << std::endl;
            once(0, data_line->size());
        }
    } else {
        // Use existing next() method for traditional calculation
        std::cout << "  Falling back to next()" << std::endl;
        next();
    }
}

std::shared_ptr<LineSingle> TripleExponentialMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index >= lines->size()) {
        return nullptr;
    }
    return lines->getline(index);
}

void TripleExponentialMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // TEMA line
        lines->add_line(std::make_shared<LineBuffer>());  // Top envelope
        lines->add_line(std::make_shared<LineBuffer>());  // Bottom envelope
    }
}

void TripleExponentialMovingAverageEnvelope::prenext() {
    if (tema_) {
        tema_->calculate();
    }
}

void TripleExponentialMovingAverageEnvelope::next() {
    if (!tema_ || !lines) return;
    
    // Get the TEMA value
    double tema_value = tema_->get();
    
    // Calculate envelope percentage
    double perc = params.perc / 100.0;
    
    // Set the lines
    auto tema_line = lines->getline(tema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (tema_line && top_line && bot_line) {
        tema_line->set(0, tema_value);
        top_line->set(0, tema_value * (1.0 + perc));
        bot_line->set(0, tema_value * (1.0 - perc));
    }
}

void TripleExponentialMovingAverageEnvelope::once(int start, int end) {
    std::cout << "TEMAEnvelope::once() called with start=" << start << ", end=" << end << std::endl;
    
    if (!tema_) {
        std::cout << "  ERROR: tema_ is null!" << std::endl;
        return;
    }
    if (!lines) {
        std::cout << "  ERROR: lines is null!" << std::endl;
        return;
    }
    
    // Get the TEMA indicator line
    auto tema_indicator_line = tema_->getLine(0);
    if (!tema_indicator_line) {
        std::cout << "  ERROR: tema_->getLine(0) returned null!" << std::endl;
        return;
    }
    
    std::cout << "  TEMA indicator line size: " << tema_indicator_line->size() << std::endl;
    
    auto tema_line = lines->getline(tema);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (tema_line && top_line && bot_line) {
        double perc = params.perc / 100.0;
        
        // Get buffers for direct access
        auto tema_buffer = std::dynamic_pointer_cast<LineBuffer>(tema_line);
        auto top_buffer = std::dynamic_pointer_cast<LineBuffer>(top_line);
        auto bot_buffer = std::dynamic_pointer_cast<LineBuffer>(bot_line);
        
        // Get TEMA buffer for direct array access
        auto tema_indicator_buffer = std::dynamic_pointer_cast<LineBuffer>(tema_indicator_line);
        
        if (tema_buffer && top_buffer && bot_buffer && tema_indicator_buffer) {
            const auto& tema_array = tema_indicator_buffer->array();
            
            // Clear buffers for fresh calculation
            tema_buffer->reset();
            top_buffer->reset();
            bot_buffer->reset();
            
            for (int i = start; i < end && i < static_cast<int>(tema_array.size()); ++i) {
                double tema_value = tema_array[i];
                
                tema_buffer->append(tema_value);
                top_buffer->append(tema_value * (1.0 + perc));
                bot_buffer->append(tema_value * (1.0 - perc));
            }
            
            // Set buffer positions to the end
            if (tema_buffer->array().size() > 0) {
                int last_idx = tema_buffer->array().size() - 1;
                tema_buffer->set_idx(last_idx);
                top_buffer->set_idx(last_idx);
                bot_buffer->set_idx(last_idx);
            }
        } else {
            // Fallback to index-based access
            for (int i = start; i < end; ++i) {
                double tema_value = (*tema_indicator_line)[i];
                
                tema_line->set(i, tema_value);
                top_line->set(i, tema_value * (1.0 + perc));
                bot_line->set(i, tema_value * (1.0 - perc));
            }
        }
    }
}

// SmoothedMovingAverageEnvelope implementation
SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope() 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Create SMMA with default constructor, data will be connected later
    smma_ = std::make_shared<SMMA>();
    smma_->params.period = params.period;
    
    _minperiod(params.period);
}

SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    
    smma_ = std::make_shared<SMMA>(data_source, params.period);
    
    _minperiod(params.period);
    
    // Connect data source for envelope calculation
    this->data = data_source;
    this->datas.push_back(data_source);
}

SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    smma_ = std::make_shared<SMMA>(data_source, period);
    
    _minperiod(params.period);
    
    // Connect data source for envelope calculation
    this->data = data_source;
    this->datas.push_back(data_source);
}

SmoothedMovingAverageEnvelope::SmoothedMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    params.perc = perc;
    setup_lines();
    
    smma_ = std::make_shared<SMMA>(data_source, period);
    
    _minperiod(params.period);
    
    // Connect data source for envelope calculation
    this->data = data_source;
    this->datas.push_back(data_source);
}

double SmoothedMovingAverageEnvelope::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(smma);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Use the buffer directly for all access
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(line);
    if (buffer) {
        if (ago == 0) {
            // Get the current (last) value
            if (buffer->size() > 0) {
                return buffer->array()[buffer->size() - 1];
            }
        } else if (ago < 0) {
            // Negative ago means access historical values
            // ago=0 is current (last), ago=-1 is one before last, etc.
            int idx = static_cast<int>(buffer->size()) - 1 + ago;
            if (idx >= 0 && idx < static_cast<int>(buffer->size())) {
                return buffer->array()[idx];
            }
        } else {
            // Positive ago - delegate to the line's get method
            return line->get(ago);
        }
    }
    
    return std::numeric_limits<double>::quiet_NaN();
}

int SmoothedMovingAverageEnvelope::getMinPeriod() const {
    return params.period;
}

void SmoothedMovingAverageEnvelope::calculate() {
    std::cout << "SMMAEnvelope::calculate() called" << std::endl;
    
    if (!smma_) {
        std::cout << "SMMAEnvelope::calculate() - smma_ is null!" << std::endl;
        return;
    }
    
    // CRITICAL FIX: Don't modify SMMA's data connection after construction
    // The SMMA was already created with the correct data source in constructor
    // Changing it here causes calculation differences
    
    // REMOVED: This code was causing the SMMA to use different data than intended
    // if (!datas.empty() && datas[0]) {
    //     smma_->data = datas[0];
    //     smma_->datas.clear();
    //     smma_->datas.push_back(datas[0]);
    // }
    
    // Just call SMMA's calculate method with its original data source
    std::cout << "SMMAEnvelope::calculate() - calling SMMA calculate" << std::endl;
    smma_->calculate();
    
    // Get data size for our calculation
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        // Get the appropriate data line
        std::shared_ptr<LineSingle> data_line;
        if (datas[0]->lines->size() > 4) {
            // DataSeries with OHLCV - use close line (index 4)
            data_line = datas[0]->lines->getline(4);
        } else {
            // Single line data - use line 0
            data_line = datas[0]->lines->getline(0);
        }
        
        if (data_line) {
            // Check if it's a LineBuffer and get actual data size
            if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(data_line)) {
                // Use data_size() directly like SMMA does
                // This gives us the total data loaded in the buffer
                data_size = linebuf->data_size();
            } else {
                // Fallback to size()
                data_size = data_line->size();
            }
        }
    }
    
    std::cout << "SMMAEnvelope::calculate() - data_size = " << data_size << std::endl;
    
    if (data_size > 0) {
        once(0, data_size);
    }
}

std::shared_ptr<LineBuffer> SmoothedMovingAverageEnvelope::getLine(int index) const {
    if (!lines || index < 0 || index >= static_cast<int>(lines->size())) {
        return nullptr;
    }
    return std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
}

void SmoothedMovingAverageEnvelope::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void SmoothedMovingAverageEnvelope::prenext() {
    // prenext() is protected, so we can't call it directly
    Indicator::prenext();
}

void SmoothedMovingAverageEnvelope::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Initialize SMMA if not already done
    if (!smma_ && !datas.empty()) {
        smma_ = std::make_shared<SMMA>(datas[0], params.period);
    }
    
    // Connect data to SMMA if not already done
    if (smma_ && smma_->datas.empty() && !datas.empty()) {
        smma_->datas = datas;
    }
    
    // Update SMMA using calculate method
    if (smma_) {
        smma_->calculate();
    }
    
    auto smma_line = lines->getline(smma);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto smma_indicator_line = smma_->lines->getline(0);
    
    if (smma_line && top_line && bot_line && smma_indicator_line) {
        double smma_value = (*smma_indicator_line)[0];
        double perc = params.perc / 100.0;
        
        smma_line->set(0, smma_value);
        top_line->set(0, smma_value * (1.0 + perc));
        bot_line->set(0, smma_value * (1.0 - perc));
    }
}

void SmoothedMovingAverageEnvelope::once(int start, int end) {
    std::cout << "SMMAEnvelope::once() called with start=" << start << ", end=" << end << std::endl;
    
    // Check for data source - either datas or data_source_
    if ((datas.empty() || !datas[0]) && !data_source_) {
        std::cout << "SMMAEnvelope::once() - no data source!" << std::endl;
        return;
    }
    
    // Initialize SMMA if not already done
    if (!smma_) {
        std::cout << "SMMAEnvelope::once() - creating SMMA" << std::endl;
        if (!datas.empty() && datas[0]) {
            smma_ = std::make_shared<SMMA>(datas[0], params.period);
        } else if (data_source_) {
            smma_ = std::make_shared<SMMA>(data_source_, params.period);
        }
    }
    
    // CRITICAL FIX: Don't modify SMMA's data connection after construction
    // The SMMA was already created with the correct data source in constructor
    // Changing it here causes calculation differences with standalone SMMA
    
    // REMOVED: This code was causing the SMMA to use different data than intended
    // if (smma_ && !datas.empty() && datas[0]) {
    //     smma_->data = datas[0];
    //     smma_->datas.clear();
    //     smma_->datas.push_back(datas[0]);
    // }
    
    // Calculate SMMA once for all data
    if (smma_) {
        std::cout << "SMMAEnvelope::once() - calling SMMA calculate" << std::endl;
        smma_->calculate();
    }
    
    auto smma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(smma));
    auto top_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(top));
    auto bot_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(bot));
    
    if (!smma_line || !top_line || !bot_line || !smma_ || !smma_->lines) {
        std::cout << "SMMAEnvelope::once() - missing lines!" << std::endl;
        return;
    }
    
    auto smma_indicator_line = std::dynamic_pointer_cast<LineBuffer>(smma_->lines->getline(0));
    if (!smma_indicator_line) {
        std::cout << "SMMAEnvelope::once() - no SMMA indicator line!" << std::endl;
        return;
    }
    
    // Clear buffers and copy data from SMMA
    smma_line->reset();
    top_line->reset();
    bot_line->reset();
    
    double perc = params.perc / 100.0;
    
    // Get SMMA buffer data
    const auto& smma_array = smma_indicator_line->array();
    std::cout << "SMMAEnvelope::once() - SMMA array size=" << smma_array.size() << std::endl;
    
    // Copy SMMA values and calculate envelope bands
    // The SMMA buffer might have one extra element (initial NaN) compared to input data
    // We need to align properly
    size_t copy_size = smma_array.size();
    
    // If SMMA has one more element than expected (due to initial NaN), skip the first element
    size_t smma_start_idx = 0;
    if (copy_size == static_cast<size_t>(end) + 1 && std::isnan(smma_array[0])) {
        smma_start_idx = 1;
        copy_size = end;
    } else if (end > 0 && copy_size > static_cast<size_t>(end)) {
        copy_size = static_cast<size_t>(end);
    }
    
    std::cout << "SMMAEnvelope::once() - copying " << copy_size << " values from idx " << smma_start_idx << std::endl;
    
    // Debug: Look at the SMMA values being copied
    if (smma_array.size() > 254) {
        std::cerr << "Debug: SMMA[254] (should be ~4021.57) = " << std::fixed << std::setprecision(6) << smma_array[254] << std::endl;
        
        // Calculate what envelope mid should be
        double mid_expected = 4021.569725;
        double mid_actual = smma_array[254];
        std::cerr << "Expected envelope mid: " << mid_expected << ", SMMA actual: " << mid_actual << std::endl;
    }
    if (smma_array.size() > 30) {
        std::cerr << "Debug: SMMA[30] (should be ~3644.44) = " << std::fixed << std::setprecision(6) << smma_array[30] << std::endl;
    }
    if (smma_array.size() > 142) {
        std::cerr << "Debug: SMMA[142] (should be ~3616.43) = " << std::fixed << std::setprecision(6) << smma_array[142] << std::endl;
    }
    
    for (size_t i = 0; i < copy_size; ++i) {
        double smma_value = smma_array[i + smma_start_idx];
        
        smma_line->append(smma_value);
        top_line->append(smma_value * (1.0 + perc));
        bot_line->append(smma_value * (1.0 - perc));
    }
    
    // Set buffer indices to end
    if (smma_line->size() > 0) {
        smma_line->set_idx(smma_line->size() - 1);
        top_line->set_idx(top_line->size() - 1);
        bot_line->set_idx(bot_line->size() - 1);
        std::cout << "SMMAEnvelope::once() - final envelope size=" << smma_line->size() 
                  << ", idx=" << smma_line->get_idx() << std::endl;
    }
}

size_t Envelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto src_line = lines->getline(src);
    return src_line ? src_line->size() : 0;
}

size_t SimpleMovingAverageEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto sma_line = lines->getline(sma);
    return sma_line ? sma_line->size() : 0;
}

size_t ExponentialMovingAverageEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto ema_line = lines->getline(ema);
    return ema_line ? ema_line->size() : 0;
}

size_t DoubleExponentialMovingAverageEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto dema_line = lines->getline(dema);
    return dema_line ? dema_line->size() : 0;
}

size_t TripleExponentialMovingAverageEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto tema_line = lines->getline(tema);
    return tema_line ? tema_line->size() : 0;
}

size_t SmoothedMovingAverageEnvelope::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto smma_line = lines->getline(smma);
    return smma_line ? smma_line->size() : 0;
}

} // namespace indicators
} // namespace backtrader
