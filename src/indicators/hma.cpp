#include "indicators/hma.h"
#include "indicator_utils.h"
#include "dataseries.h"
#include <cmath>
#include <limits>
#include <vector>
#include <numeric>
#include <algorithm>

namespace backtrader {
namespace indicators {

// Helper function to calculate WMA (Weighted Moving Average)
static double calculateWMA(const std::vector<double>& data, int start, int end) {
    if (start < 0 || end > data.size() || start >= end) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double weighted_sum = 0.0;
    double weight_sum = 0.0;
    int period = end - start;
    
    for (int i = 0; i < period; ++i) {
        double weight = i + 1;  // Weights: 1, 2, 3, ..., period
        weighted_sum += data[start + i] * weight;
        weight_sum += weight;
    }
    
    return weighted_sum / weight_sum;
}

// HullMovingAverage implementation
HullMovingAverage::HullMovingAverage() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // HMA minimum period calculation:
    // Following Python implementation: period + sqrt(period) - 1
    int sqrt_period = static_cast<int>(std::sqrt(params.period));
    _minperiod(params.period + sqrt_period - 1);
}

HullMovingAverage::HullMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    int sqrt_period = static_cast<int>(std::sqrt(params.period));
    _minperiod(params.period + sqrt_period - 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
    
    // Calculate immediately upon construction if we have data
    if (data_source && data_source->lines && data_source->lines->size() > 0) {
        calculate();
    }
}

// DataSeries constructors for disambiguation
HullMovingAverage::HullMovingAverage(std::shared_ptr<DataSeries> data_source) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    setup_lines();
    
    int sqrt_period = static_cast<int>(std::sqrt(params.period));
    _minperiod(params.period + sqrt_period - 1);
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
    
    // Calculate immediately upon construction if we have data
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        calculate();
    }
}

HullMovingAverage::HullMovingAverage(std::shared_ptr<DataSeries> data_source, int period) 
    : Indicator(), data_source_(std::static_pointer_cast<LineSeries>(data_source)), current_index_(0) {
    params.period = period;
    setup_lines();
    
    int sqrt_period = static_cast<int>(std::sqrt(params.period));
    _minperiod(params.period + sqrt_period - 1);
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
    
    // Calculate immediately upon construction if we have data
    if (data_source_ && data_source_->lines && data_source_->lines->size() > 0) {
        calculate();
    }
}

double HullMovingAverage::get(int ago) const {
    // Use the base class implementation which uses lines_
    return IndicatorBase::get(ago);
}

int HullMovingAverage::getMinPeriod() const {
    // HMA minimum period = period + sqrt(period) - 1
    int sqrt_period = static_cast<int>(std::sqrt(params.period));
    return params.period + sqrt_period - 1;
}

size_t HullMovingAverage::size() const {
    // Use the base class implementation which uses lines_
    return IndicatorBase::size();
}

void HullMovingAverage::calculate() {
    // Get data size
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        data_size = utils::getDataSize(datas[0]);
    } else if (data_source_) {
        data_size = utils::getDataSize(data_source_);
    } else if (this->data) {
        data_size = utils::getDataSize(this->data);
    }
    
    auto hma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(0));
    if (!hma_line) {
        return;
    }
    
    // Only calculate if we haven't already
    if (hma_line->size() <= 1 && data_size > 0) {
        // Use once() method for batch processing all data points
        once(0, data_size);
        
        // Sync lines to lines_ after calculation
        lines_.clear();
        for (size_t i = 0; i < lines->size(); ++i) {
            lines_.push_back(lines->getline(i));
        }
    }
}

void HullMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Connect LineCollection to IndicatorBase lines_ vector
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

void HullMovingAverage::prenext() {
    Indicator::prenext();
}

void HullMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = utils::getDataLine(datas[0]);
    if (!data_line) return;
    
    auto hma_line = lines->getline(hma);
    if (!hma_line) return;
    
    // Get current price
    double price = (*data_line)[0];
    
    if (std::isnan(price)) {
        hma_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Store price for calculation
    prices_.push_back(price);
    
    // Calculate HMA
    int sqrt_period = static_cast<int>(std::sqrt(params.period));
    int min_required = params.period + sqrt_period - 1;
    
    if (prices_.size() >= min_required) {
        // Calculate WMA(period)
        int start_idx = prices_.size() - params.period;
        double wma_period = calculateWMA(prices_, start_idx, prices_.size());
        
        // Calculate WMA(period/2)
        int half_period = params.period / 2;
        start_idx = prices_.size() - half_period;
        double wma_half = calculateWMA(prices_, start_idx, prices_.size());
        
        // Calculate 2*WMA(period/2) - WMA(period)
        std::vector<double> diff_values;
        diff_values.reserve(sqrt_period);
        
        // We need sqrt_period values for the final WMA
        // Build the difference series
        for (int i = sqrt_period - 1; i >= 0; --i) {
            int idx = prices_.size() - 1 - i;
            if (idx >= params.period - 1) {
                // Calculate WMA values at this point
                int end_idx = idx + 1;
                
                // WMA(period) at this point
                int start_period = end_idx - params.period;
                double wma_p = calculateWMA(prices_, start_period, end_idx);
                
                // WMA(period/2) at this point
                int start_half = end_idx - half_period;
                double wma_h = calculateWMA(prices_, start_half, end_idx);
                
                diff_values.push_back(2.0 * wma_h - wma_p);
            }
        }
        
        // Apply final WMA with sqrt(period)
        if (diff_values.size() == sqrt_period) {
            double hma_value = calculateWMA(diff_values, 0, diff_values.size());
            hma_line->set(0, hma_value);
        } else {
            hma_line->set(0, std::numeric_limits<double>::quiet_NaN());
        }
    } else {
        hma_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
    
    // Keep buffer manageable
    if (prices_.size() > min_required + 10) {
        prices_.erase(prices_.begin());
    }
}

void HullMovingAverage::once(int start, int end) {
    // Get data source using utility function
    std::shared_ptr<LineSingle> data_line;
    if (!datas.empty() && datas[0]) {
        data_line = utils::getDataLine(datas[0]);
    } else if (data_source_) {
        data_line = utils::getDataLine(data_source_);
    } else if (data) {
        data_line = utils::getDataLine(data);
    }
    
    if (!data_line) return;
    
    // Get the actual data size
    size_t data_size = 0;
    if (!datas.empty() && datas[0]) {
        data_size = utils::getDataSize(datas[0]);
    } else if (data_source_) {
        data_size = utils::getDataSize(data_source_);
    } else if (data) {
        data_size = utils::getDataSize(data);
    }
    
    if (data_size == 0 || end == 0) return;
    
    auto hma_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(hma));
    if (!hma_line) return;
    
    // Only reset if the buffer is empty or nearly empty (avoid resetting already calculated data)
    if (hma_line->size() <= 1) {
        hma_line->reset();
    } else if (hma_line->size() >= data_size) {
        // Already calculated, no need to recalculate
        return;
    }
    
    // Get LineBuffer for direct array access
    auto line_buffer = std::dynamic_pointer_cast<LineBuffer>(data_line);
    if (!line_buffer) return;
    
    const auto& data_array = line_buffer->array();
    if (data_array.empty()) return;
    
    // Calculate HMA parameters
    int period = params.period;
    int half_period = period / 2;
    int sqrt_period = static_cast<int>(std::sqrt(period));
    int min_period = period + sqrt_period - 1;
    
    // Pre-allocate space
    hma_line->reserve(end - start);
    
    // Calculate HMA for each point
    for (int i = start; i < end && i < static_cast<int>(data_size); ++i) {
        if (i < min_period - 1) {
            // Not enough data yet
            hma_line->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Build data vector for this point
        std::vector<double> point_data;
        point_data.reserve(period);
        
        for (int j = i - period + 1; j <= i; ++j) {
            if (j >= 0 && j < data_array.size()) {
                point_data.push_back(data_array[j]);
            }
        }
        
        if (point_data.size() != period) {
            hma_line->append(std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        // Calculate difference series: 2*WMA(period/2) - WMA(period)
        std::vector<double> diff_series;
        diff_series.reserve(sqrt_period);
        
        for (int k = i - sqrt_period + 1; k <= i; ++k) {
            if (k >= period - 1) {
                // Get data for WMA calculations
                std::vector<double> wma_data;
                wma_data.reserve(period);
                
                for (int m = k - period + 1; m <= k; ++m) {
                    if (m >= 0 && m < data_array.size()) {
                        wma_data.push_back(data_array[m]);
                    }
                }
                
                if (wma_data.size() == period) {
                    // Calculate WMA(period)
                    double wma_period = calculateWMA(wma_data, 0, period);
                    
                    // Calculate WMA(period/2) using last half_period values
                    double wma_half = calculateWMA(wma_data, period - half_period, period);
                    
                    // Add to difference series
                    diff_series.push_back(2.0 * wma_half - wma_period);
                }
            }
        }
        
        // Calculate final HMA value
        if (diff_series.size() == sqrt_period) {
            double hma_value = calculateWMA(diff_series, 0, sqrt_period);
            hma_line->append(hma_value);
        } else {
            hma_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Finalize the line buffer
    utils::finalizeLineBuffer(hma_line);
}

} // namespace indicators
} // namespace backtrader