#include "indicators/dma.h"
#include "indicator_utils.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace backtrader {
namespace indicators {

// DicksonMovingAverage implementation
DicksonMovingAverage::DicksonMovingAverage() : Indicator(), data_source_(nullptr) {
    setup_lines();
    _minperiod(params.period);
}

DicksonMovingAverage::DicksonMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source) {
    params.period = period;
    // Don't override hperiod - keep default value of 7
    setup_lines();
    _minperiod(params.period);
    
    // Create internal indicators
    zlind_ = std::make_shared<ZeroLagIndicator>(data_source, period);
    zlind_->params.gainlimit = params.gainlimit;  // Set gainlimit after creation
    hma_ = std::make_shared<HullMovingAverage>(data_source, params.hperiod);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

DicksonMovingAverage::DicksonMovingAverage(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)) {
    setup_lines();
    _minperiod(params.period);
    
    // Create internal indicators
    zlind_ = std::make_shared<ZeroLagIndicator>(data_source, params.period);
    zlind_->params.gainlimit = params.gainlimit;  // Set gainlimit after creation
    hma_ = std::make_shared<HullMovingAverage>(data_source, params.hperiod);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

DicksonMovingAverage::DicksonMovingAverage(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)) {
    params.period = period;
    // Don't override hperiod - keep default value of 7
    setup_lines();
    _minperiod(params.period);
    
    // Create internal indicators
    zlind_ = std::make_shared<ZeroLagIndicator>(data_source, period);
    zlind_->params.gainlimit = params.gainlimit;  // Set gainlimit after creation
    hma_ = std::make_shared<HullMovingAverage>(data_source, params.hperiod);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

void DicksonMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

double DicksonMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto dma_line = lines->getline(dma);
    if (!dma_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Backtrader Python uses a special indexing scheme:
    // - indicator[0] is the most recent value (same as indicator[-1])
    // - indicator[1] is 1 bar ago
    // - indicator[-1] is also the most recent value
    // - indicator[-2] is 1 bar ago, etc.
    //
    // For negative indices, we need to add 1 to match Python behavior
    if (ago < 0) {
        // Convert Python negative index to positive ago
        // Python DMA[-225] means 224 bars ago (not 225!)
        int bars_ago = -ago - 1;
        return (*dma_line)[bars_ago];
    } else {
        // Non-negative indices: 0 is current, 1 is 1 bar ago
        return (*dma_line)[ago];
    }
}

int DicksonMovingAverage::getMinPeriod() const {
    // The minimum period is the maximum of the two indicators' minimum periods
    int zlind_min = params.period;  // ZeroLagIndicator minperiod = period
    int hma_min = params.hperiod + static_cast<int>(std::sqrt(params.hperiod)) - 1;  // HMA minperiod
    return std::max(zlind_min, hma_min);
}

size_t DicksonMovingAverage::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto dma_line = lines->getline(dma);
    return dma_line ? dma_line->size() : 0;
}

void DicksonMovingAverage::calculate() {
    // Get data size
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        data_size = utils::getDataSize(datas[0]);
    } else if (data_source_) {
        data_size = utils::getDataSize(data_source_);
    } else if (this->data) {
        data_size = utils::getDataSize(this->data);
    }
    
    if (data_size > 0) {
        // Use once() method for batch processing all data points
        once(0, data_size);
    }
}

void DicksonMovingAverage::prenext() {
    Indicator::prenext();
}

void DicksonMovingAverage::next() {
    // In the next() method, we don't call next() on sub-indicators
    // Instead, we just read their current values
    // The sub-indicators should have been calculated already
    
    if (!zlind_ || !hma_ || !lines) return;
    
    auto dma_line = lines->getline(dma);
    if (!dma_line) return;
    
    // Get values from both indicators
    double ec_value = zlind_->get(0);
    double hma_value = hma_->get(0);
    
    // Calculate DMA = (EC + HMA) / 2
    double dma_value = std::numeric_limits<double>::quiet_NaN();
    if (!std::isnan(ec_value) && !std::isnan(hma_value)) {
        dma_value = (ec_value + hma_value) / 2.0;
    }
    
    
    dma_line->set(0, dma_value);
}

void DicksonMovingAverage::once(int start, int end) {
    // Get data source
    std::shared_ptr<LineSingle> data_line;
    if (!datas.empty() && datas[0]) {
        data_line = utils::getDataLine(datas[0]);
    } else if (data_source_) {
        data_line = utils::getDataLine(data_source_);
    } else if (data) {
        data_line = utils::getDataLine(data);
    }
    
    if (!data_line) return;
    
    auto dma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dma));
    if (!dma_line) return;
    
    // Clear the line buffer and reset to initial state
    dma_line->reset();
    
    // Indicators should already be created in constructor
    if (!zlind_ || !hma_) {
        std::cerr << "DMA::once() - ERROR: Indicators not created in constructor!" << std::endl;
        return;
    }
    
    // Calculate all values for both indicators
    zlind_->calculate();
    hma_->calculate();
    
    
    // Get indicator lines
    auto zlind_line = zlind_->lines->getline(0);
    auto hma_line = hma_->lines->getline(0);
    if (!zlind_line || !hma_line) return;
    
    auto zlind_buffer = std::dynamic_pointer_cast<LineBuffer>(zlind_line);
    auto hma_buffer = std::dynamic_pointer_cast<LineBuffer>(hma_line);
    if (!zlind_buffer || !hma_buffer) return;
    
    const auto& zlind_array = zlind_buffer->array();
    const auto& hma_array = hma_buffer->array();
    
    // Pre-allocate space
    dma_line->reserve(end - start);
    
    // Calculate DMA values
    for (int i = start; i < end; ++i) {
        double dma_value = std::numeric_limits<double>::quiet_NaN();
        
        if (i < static_cast<int>(zlind_array.size()) && i < static_cast<int>(hma_array.size())) {
            double ec_value = zlind_array[i];
            double hma_value = hma_array[i];
            
            if (!std::isnan(ec_value) && !std::isnan(hma_value)) {
                dma_value = (ec_value + hma_value) / 2.0;
            }
            
        }
        
        dma_line->append(dma_value);
    }
    
    // Finalize the line buffer
    utils::finalizeLineBuffer(dma_line);
    
}

} // namespace indicators
} // namespace backtrader