#include "indicators/prettygoodoscillator.h"
#include "dataseries.h"
#include <limits>
#include <iostream>

namespace backtrader {

// PrettyGoodOscillator implementation
PrettyGoodOscillator::PrettyGoodOscillator() : Indicator() {
    setup_lines();
    // ATR has minperiod of period+1
    _minperiod(params.period + 1);
}

PrettyGoodOscillator::PrettyGoodOscillator(std::shared_ptr<LineSeries> data_source) : PrettyGoodOscillator() {
    this->data = data_source;
    this->datas.push_back(data_source);
}

PrettyGoodOscillator::PrettyGoodOscillator(std::shared_ptr<LineSeries> data_source, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    this->data = data_source;
    this->datas.push_back(data_source);
}

PrettyGoodOscillator::PrettyGoodOscillator(std::shared_ptr<DataSeries> data_source) : PrettyGoodOscillator() {
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}

PrettyGoodOscillator::PrettyGoodOscillator(std::shared_ptr<DataSeries> data_source, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
}


double PrettyGoodOscillator::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(pgo);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int PrettyGoodOscillator::getMinPeriod() const {
    return params.period + 1;  // ATR needs period+1
}

void PrettyGoodOscillator::calculate() {
    if (datas.empty() || !datas[0]) {
            return;
    }
    
    // Get the actual data size from the buffer
    auto data_series = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    std::shared_ptr<LineBuffer> close_line;
    if (data_series) {
        close_line = std::dynamic_pointer_cast<LineBuffer>(datas[0]->lines->getline(DataSeries::Close));
    } else {
        close_line = std::dynamic_pointer_cast<LineBuffer>(datas[0]->lines->getline(0));
    }
    
    if (!close_line) {
            return;
    }
    
    int data_len = close_line->array().size();
    
    // Calculate all values from the beginning
    once(0, data_len);
}

void PrettyGoodOscillator::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void PrettyGoodOscillator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto pgo_line = lines->getline(pgo);
    if (!pgo_line) return;
    
    // Get the close price line
    // If this is a DataSeries, get the close line (index 3)
    // If this is a simple LineSeries with one line, use line 0
    std::shared_ptr<LineBuffer> close_line;
    auto data_series = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (data_series) {
        // DataSeries: use close price (index 3)
        close_line = std::dynamic_pointer_cast<LineBuffer>(datas[0]->lines->getline(4));
    } else {
        // LineSeries: use first line
        close_line = std::dynamic_pointer_cast<LineBuffer>(datas[0]->lines->getline(0));
    }
    
    if (!close_line) return;
    
    // Create SMA if not exists - pass the full data source
    if (!sma_) {
        if (data_series) {
            // For DataSeries, SMA should use close price
            sma_ = std::make_shared<indicators::SMA>(datas[0], params.period);
        } else {
            // For LineSeries, pass as is
            sma_ = std::make_shared<indicators::SMA>(datas[0], params.period);
        }
    }
    
    // Create ATR if not exists - ATR needs full OHLC data
    if (!atr_) {
        if (data_series) {
            // Pass DataSeries directly for ATR
            atr_ = std::make_shared<indicators::ATR>(data_series, params.period);
        } else {
            // For single line data, ATR can't work properly but we'll try
            atr_ = std::make_shared<indicators::ATR>(datas[0], params.period);
        }
    }
    
    // Calculate SMA and ATR
    sma_->calculate();
    atr_->calculate();
    
    // Pretty Good Oscillator formula: (Close - SMA) / ATR
    double current_price = (*close_line)[0];
    double sma_value = sma_->get(0);
    double atr_value = atr_->get(0);
    
    if (!std::isnan(sma_value) && !std::isnan(atr_value) && atr_value != 0.0) {
        double pgo_value = (current_price - sma_value) / atr_value;
        pgo_line->set(0, pgo_value);
    } else {
        pgo_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
}

void PrettyGoodOscillator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto pgo_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(pgo));
    if (!pgo_line) return;
    
    // Create sub-indicators if not exists
    auto data_series = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (!sma_) {
        sma_ = std::make_shared<indicators::SMA>(datas[0], params.period);
    }
    if (!atr_) {
        if (data_series) {
            atr_ = std::make_shared<indicators::ATR>(data_series, params.period);
        } else {
            atr_ = std::make_shared<indicators::ATR>(datas[0], params.period);
        }
    }
    
    // Calculate sub-indicators first
    sma_->calculate();
    atr_->calculate();
    
    // Get close line
    std::shared_ptr<LineBuffer> close_line;
    if (data_series) {
        close_line = std::dynamic_pointer_cast<LineBuffer>(datas[0]->lines->getline(DataSeries::Close));
    } else {
        close_line = std::dynamic_pointer_cast<LineBuffer>(datas[0]->lines->getline(0));
    }
    
    if (!close_line) return;
    
    
    // Reset the buffer if calculating from start
    if (start == 0) {
        pgo_line->clear();  // Clear without adding initial NaN
    }
    
    // Build a temporary array with calculated values
    std::vector<double> pgo_values;
    pgo_values.reserve(end - start);
    
    // Calculate PGO values
    for (int i = start; i < end; ++i) {
        if (i >= getMinPeriod() - 1) {
            // Get values from buffers at absolute position i
            double current_price = close_line->array()[i];
            
            // Get SMA and ATR values using their buffer arrays
            auto sma_line = std::dynamic_pointer_cast<LineBuffer>(sma_->lines->getline(0));
            auto atr_line = std::dynamic_pointer_cast<LineBuffer>(atr_->lines->getline(0));
            
            if (!sma_line || !atr_line) {
                pgo_line->append(std::numeric_limits<double>::quiet_NaN());
                continue;
            }
            
            // Access values from the arrays
            const auto& sma_array = sma_line->array();
            const auto& atr_array = atr_line->array();
            
            if (i < static_cast<int>(sma_array.size()) && i < static_cast<int>(atr_array.size())) {
                double sma_value = sma_array[i];
                double atr_value = atr_array[i];
                
                
                if (!std::isnan(sma_value) && !std::isnan(atr_value) && atr_value != 0.0) {
                    double pgo_value = (current_price - sma_value) / atr_value;
                    pgo_values.push_back(pgo_value);
                } else {
                    pgo_values.push_back(std::numeric_limits<double>::quiet_NaN());
                }
            } else {
                pgo_values.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        } else {
            pgo_values.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Now add all values to the buffer
    for (size_t i = 0; i < pgo_values.size(); ++i) {
        pgo_line->append(pgo_values[i]);
    }
    
    // Set LineBuffer index to last valid position for proper ago indexing
    if (end > start && pgo_line->array().size() > 0) {
        pgo_line->set_idx(pgo_line->array().size() - 1);
    }
}

size_t PrettyGoodOscillator::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto pgo_line = lines->getline(pgo);
    return pgo_line ? pgo_line->size() : 0;
}

} // namespace backtrader