#include "indicators/kst.h"
#include <limits>
#include <iostream>

namespace backtrader {

// KnowSureThing implementation
KnowSureThing::KnowSureThing() : Indicator() {
    setup_lines();
    
    // Calculate minimum period needed
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    _minperiod(max_roc_period + max_ma_period + params.rsignal - 1);
    
}

KnowSureThing::KnowSureThing(std::shared_ptr<DataSeries> data_source) : Indicator() {
    setup_lines();
    
    // Calculate minimum period needed
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    _minperiod(max_roc_period + max_ma_period + params.rsignal - 1);
    
    // Connect data to this indicator
    auto lineseries = std::dynamic_pointer_cast<LineSeries>(data_source);
    if (lineseries) {
        this->data = lineseries;
        this->datas.push_back(lineseries);
    }
    
}

KnowSureThing::KnowSureThing(std::shared_ptr<LineSeries> data_source) : Indicator() {
    setup_lines();
    
    // Calculate minimum period needed
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    _minperiod(max_roc_period + max_ma_period + params.rsignal - 1);
    
    // Connect data to this indicator
    this->data = data_source;
    this->datas.push_back(data_source);
    
}

double KnowSureThing::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(kst);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int KnowSureThing::getMinPeriod() const {
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    return max_roc_period + max_ma_period + params.rsignal - 1;
}

size_t KnowSureThing::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto kst_line = lines->getline(Lines::kst);
    if (!kst_line) {
        return 0;
    }
    return kst_line->size();
}

void KnowSureThing::calculate() {
    if (datas.empty() || !datas[0]) return;
    
    // Get the data size - for DataSeries, use close price line
    auto data = datas[0];
    if (!data->lines || data->lines->size() == 0) return;
    
    // Get close line - index 4 for DataSeries, index 0 for LineSeries
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(data);
    int line_index = dataseries ? DataSeries::Close : 0;  // Close price for DataSeries
    auto close_line = data->lines->getline(line_index);
    if (!close_line) return;
    
    // Get the buffer to access array size
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (!close_buffer) return;
    
    // Get actual data size from the buffer
    size_t data_size = close_buffer->data_size();
    
    // Process all data points using the once() method
    if (data_size > 0) {
        once(0, data_size);
    }
}

std::shared_ptr<LineBuffer> KnowSureThing::getLine(int index) const {
    // Ensure lines are initialized
    if (!lines || lines->size() == 0) {
        // Force initialization if needed (shouldn't happen normally)
        const_cast<KnowSureThing*>(this)->setup_lines();
    }
    
    if (!lines || static_cast<size_t>(index) >= lines->size()) {
        return nullptr;
    }
    
    try {
        return std::dynamic_pointer_cast<LineBuffer>(lines->getline(index));
    } catch (...) {
        // If getline throws, return nullptr
        return nullptr;
    }
}

void KnowSureThing::setup_lines() {
    if (!lines) {
        std::cerr << "KST::setup_lines - lines is null! This should not happen!" << std::endl;
        return;
    }
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void KnowSureThing::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto kst_line = lines->getline(kst);
    auto signal_line = lines->getline(signal);
    
    if (!kst_line || !signal_line) return;
    
    // Get close line - index 4 for DataSeries, index 0 for LineSeries
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    int line_index = dataseries ? DataSeries::Close : 0;  // Close price for DataSeries
    auto close_line = datas[0]->lines->getline(line_index);
    if (!close_line) return;
    
    double close_price = (*close_line)[0];
    if (std::isnan(close_price)) {
        kst_line->set(0, std::numeric_limits<double>::quiet_NaN());
        signal_line->set(0, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    
    // Calculate Rate of Change for each period
    double roc1_val = std::numeric_limits<double>::quiet_NaN();
    double roc2_val = std::numeric_limits<double>::quiet_NaN();
    double roc3_val = std::numeric_limits<double>::quiet_NaN();
    double roc4_val = std::numeric_limits<double>::quiet_NaN();
    
    if (close_line->size() > params.rp1) {
        double close_ago = (*close_line)[params.rp1];
        if (!std::isnan(close_ago) && close_ago != 0.0) {
            roc1_val = 100.0 * (close_price - close_ago) / close_ago;
        }
    }
    
    if (close_line->size() > params.rp2) {
        double close_ago = (*close_line)[params.rp2];
        if (!std::isnan(close_ago) && close_ago != 0.0) {
            roc2_val = 100.0 * (close_price - close_ago) / close_ago;
        }
    }
    
    if (close_line->size() > params.rp3) {
        double close_ago = (*close_line)[params.rp3];
        if (!std::isnan(close_ago) && close_ago != 0.0) {
            roc3_val = 100.0 * (close_price - close_ago) / close_ago;
        }
    }
    
    if (close_line->size() > params.rp4) {
        double close_ago = (*close_line)[params.rp4];
        if (!std::isnan(close_ago) && close_ago != 0.0) {
            roc4_val = 100.0 * (close_price - close_ago) / close_ago;
        }
    }
    
    // Store ROC values
    roc1_values_.push_back(roc1_val);
    roc2_values_.push_back(roc2_val);
    roc3_values_.push_back(roc3_val);
    roc4_values_.push_back(roc4_val);
    
    // Keep only necessary history
    if (roc1_values_.size() > params.rma1) roc1_values_.pop_front();
    if (roc2_values_.size() > params.rma2) roc2_values_.pop_front();
    if (roc3_values_.size() > params.rma3) roc3_values_.pop_front();
    if (roc4_values_.size() > params.rma4) roc4_values_.pop_front();
    
    // Calculate SMAs of ROCs
    double rcma1 = std::numeric_limits<double>::quiet_NaN();
    double rcma2 = std::numeric_limits<double>::quiet_NaN();
    double rcma3 = std::numeric_limits<double>::quiet_NaN();
    double rcma4 = std::numeric_limits<double>::quiet_NaN();
    
    if (roc1_values_.size() >= params.rma1) {
        double sum = 0.0;
        int count = 0;
        for (const auto& val : roc1_values_) {
            if (!std::isnan(val)) {
                sum += val;
                count++;
            }
        }
        if (count > 0) rcma1 = sum / count;
    }
    
    if (roc2_values_.size() >= params.rma2) {
        double sum = 0.0;
        int count = 0;
        for (const auto& val : roc2_values_) {
            if (!std::isnan(val)) {
                sum += val;
                count++;
            }
        }
        if (count > 0) rcma2 = sum / count;
    }
    
    if (roc3_values_.size() >= params.rma3) {
        double sum = 0.0;
        int count = 0;
        for (const auto& val : roc3_values_) {
            if (!std::isnan(val)) {
                sum += val;
                count++;
            }
        }
        if (count > 0) rcma3 = sum / count;
    }
    
    if (roc4_values_.size() >= params.rma4) {
        double sum = 0.0;
        int count = 0;
        for (const auto& val : roc4_values_) {
            if (!std::isnan(val)) {
                sum += val;
                count++;
            }
        }
        if (count > 0) rcma4 = sum / count;
    }
    
    // Calculate KST if all SMAs are valid
    if (!std::isnan(rcma1) && !std::isnan(rcma2) && !std::isnan(rcma3) && !std::isnan(rcma4)) {
        double kst_val = params.rfactors[0] * rcma1 + 
                        params.rfactors[1] * rcma2 + 
                        params.rfactors[2] * rcma3 + 
                        params.rfactors[3] * rcma4;
        
        kst_line->set(0, kst_val);
        
        // Store KST value for signal calculation
        kst_values_.push_back(kst_val);
        if (kst_values_.size() > params.rsignal) {
            kst_values_.pop_front();
        }
        
        // Calculate signal line (SMA of KST)
        if (kst_values_.size() >= params.rsignal) {
            double sum = 0.0;
            for (const auto& val : kst_values_) {
                sum += val;
            }
            double signal_val = sum / kst_values_.size();
            signal_line->set(0, signal_val);
        } else {
            signal_line->set(0, kst_val);  // Use KST value until we have enough data
        }
    } else {
        kst_line->set(0, std::numeric_limits<double>::quiet_NaN());
        signal_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
    
    // Forward the buffers
    kst_line->forward();
    signal_line->forward();
}

void KnowSureThing::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto kst_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(kst));
    auto signal_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(signal));
    
    if (!kst_line || !signal_line) return;
    
    // Get close line - index 4 for DataSeries, index 0 for LineSeries
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    int line_index = dataseries ? DataSeries::Close : 0;  // Close price for DataSeries
    auto close_line = datas[0]->lines->getline(line_index);
    if (!close_line) return;
    
    // Get the raw array data from the buffer
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (!close_buffer) return;
    
    // Get the data directly from buffer's internal array
    const auto& prices = close_buffer->data_ptr();
    size_t data_size = close_buffer->data_size();
    if (data_size == 0) return;
    
    // Adjust end to actual data size
    end = std::min(end, static_cast<int>(data_size));
    
    // Clear buffers before batch processing
    kst_line->reset();
    signal_line->reset();
    
    // Clear and prepare deques for batch processing
    roc1_values_.clear();
    roc2_values_.clear();
    roc3_values_.clear();
    roc4_values_.clear();
    kst_values_.clear();
    
    for (int i = start; i < end; ++i) {
        double close_price = prices[i];
        
        // Calculate Rate of Change for each period
        double roc1_val = std::numeric_limits<double>::quiet_NaN();
        double roc2_val = std::numeric_limits<double>::quiet_NaN();
        double roc3_val = std::numeric_limits<double>::quiet_NaN();
        double roc4_val = std::numeric_limits<double>::quiet_NaN();
        
        if (i >= params.rp1 && (i - params.rp1) >= 0) {
            double close_ago = prices[i - params.rp1];
            if (!std::isnan(close_ago) && close_ago != 0.0) {
                roc1_val = 100.0 * (close_price - close_ago) / close_ago;
            }
        }
        
        if (i >= params.rp2 && (i - params.rp2) >= 0) {
            double close_ago = prices[i - params.rp2];
            if (!std::isnan(close_ago) && close_ago != 0.0) {
                roc2_val = 100.0 * (close_price - close_ago) / close_ago;
            }
        }
        
        if (i >= params.rp3 && (i - params.rp3) >= 0) {
            double close_ago = prices[i - params.rp3];
            if (!std::isnan(close_ago) && close_ago != 0.0) {
                roc3_val = 100.0 * (close_price - close_ago) / close_ago;
            }
        }
        
        if (i >= params.rp4 && (i - params.rp4) >= 0) {
            double close_ago = prices[i - params.rp4];
            if (!std::isnan(close_ago) && close_ago != 0.0) {
                roc4_val = 100.0 * (close_price - close_ago) / close_ago;
            }
        }
        
        // Store ROC values
        roc1_values_.push_back(roc1_val);
        roc2_values_.push_back(roc2_val);
        roc3_values_.push_back(roc3_val);
        roc4_values_.push_back(roc4_val);
        
        // Keep only necessary history
        if (roc1_values_.size() > params.rma1) roc1_values_.pop_front();
        if (roc2_values_.size() > params.rma2) roc2_values_.pop_front();
        if (roc3_values_.size() > params.rma3) roc3_values_.pop_front();
        if (roc4_values_.size() > params.rma4) roc4_values_.pop_front();
        
        // Calculate SMAs of ROCs
        double rcma1 = std::numeric_limits<double>::quiet_NaN();
        double rcma2 = std::numeric_limits<double>::quiet_NaN();
        double rcma3 = std::numeric_limits<double>::quiet_NaN();
        double rcma4 = std::numeric_limits<double>::quiet_NaN();
        
        if (roc1_values_.size() >= params.rma1) {
            double sum = 0.0;
            int count = 0;
            for (const auto& val : roc1_values_) {
                if (!std::isnan(val)) {
                    sum += val;
                    count++;
                }
            }
            if (count > 0) rcma1 = sum / count;
        }
        
        if (roc2_values_.size() >= params.rma2) {
            double sum = 0.0;
            int count = 0;
            for (const auto& val : roc2_values_) {
                if (!std::isnan(val)) {
                    sum += val;
                    count++;
                }
            }
            if (count > 0) rcma2 = sum / count;
        }
        
        if (roc3_values_.size() >= params.rma3) {
            double sum = 0.0;
            int count = 0;
            for (const auto& val : roc3_values_) {
                if (!std::isnan(val)) {
                    sum += val;
                    count++;
                }
            }
            if (count > 0) rcma3 = sum / count;
        }
        
        if (roc4_values_.size() >= params.rma4) {
            double sum = 0.0;
            int count = 0;
            for (const auto& val : roc4_values_) {
                if (!std::isnan(val)) {
                    sum += val;
                    count++;
                }
            }
            if (count > 0) rcma4 = sum / count;
        }
        
        // Calculate KST if all SMAs are valid
        if (!std::isnan(rcma1) && !std::isnan(rcma2) && !std::isnan(rcma3) && !std::isnan(rcma4)) {
            double kst_val = params.rfactors[0] * rcma1 + 
                            params.rfactors[1] * rcma2 + 
                            params.rfactors[2] * rcma3 + 
                            params.rfactors[3] * rcma4;
            
            kst_line->append(kst_val);
            
            // Store KST value for signal calculation
            kst_values_.push_back(kst_val);
            if (kst_values_.size() > params.rsignal) {
                kst_values_.pop_front();
            }
            
            // Calculate signal line (SMA of KST)
            if (kst_values_.size() >= params.rsignal) {
                double sum = 0.0;
                for (const auto& val : kst_values_) {
                    sum += val;
                }
                double signal_val = sum / kst_values_.size();
                signal_line->append(signal_val);
            } else {
                signal_line->append(kst_val);  // Use KST value until we have enough data
            }
        } else {
            kst_line->append(std::numeric_limits<double>::quiet_NaN());
            signal_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // Set buffer indices to the end for proper ago indexing
    if (kst_line->size() > 0) {
        kst_line->set_idx(kst_line->size() - 1);
    }
    if (signal_line->size() > 0) {
        signal_line->set_idx(signal_line->size() - 1);
    }
}

} // namespace backtrader