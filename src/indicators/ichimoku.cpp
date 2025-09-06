#include "indicators/ichimoku.h"
#include "dataseries.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

// Ichimoku implementation
Ichimoku::Ichimoku() : Indicator() {
    setup_lines();
    
    // Maximum period needed is max(senkou, kijun) + kijun
    _minperiod(std::max(params.senkou, params.kijun) + params.kijun);
    
    // Initialize storage vectors
    senkou_span_a_values_.resize(params.senkou_lead, 0.0);
    senkou_span_b_values_.resize(params.senkou_lead, 0.0);
    chikou_span_values_.resize(params.chikou, 0.0);
}

Ichimoku::Ichimoku(std::shared_ptr<LineSeries> data_source) : Indicator() {
    (void)data_source; // Mark parameter as used
    setup_lines();
    
    // Maximum period needed is max(senkou, kijun) + kijun
    _minperiod(std::max(params.senkou, params.kijun) + params.kijun);
    
    // Initialize storage vectors
    senkou_span_a_values_.resize(params.senkou_lead, 0.0);
    senkou_span_b_values_.resize(params.senkou_lead, 0.0);
    chikou_span_values_.resize(params.chikou, 0.0);
}

Ichimoku::Ichimoku(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low, std::shared_ptr<LineSeries> close,
                   int tenkan, int kijun, int senkou) : Indicator() {
    params.tenkan = tenkan;
    params.kijun = kijun;
    params.senkou = senkou;
    
    setup_lines();
    
    // Maximum period needed is max(senkou, kijun) + kijun
    _minperiod(std::max(params.senkou, params.kijun) + params.kijun);
    
    // Initialize storage vectors
    senkou_span_a_values_.resize(params.senkou_lead, 0.0);
    senkou_span_b_values_.resize(params.senkou_lead, 0.0);
    chikou_span_values_.resize(params.chikou, 0.0);
    
    // Store line references - create a custom LineSeries with the right lines
    if (high && low && close) {
        // Create a mock DataSeries-like structure with lines in the correct order
        auto combined = std::make_shared<LineSeries>();
        // Add dummy lines for DateTime and Open
        combined->lines->add_line(std::make_shared<LineBuffer>());  // DateTime (index 0)
        combined->lines->add_line(std::make_shared<LineBuffer>());  // Open (index 1)
        // Add the actual lines we need
        combined->lines->add_line(high->lines->getline(0));         // High (index 2)
        combined->lines->add_line(low->lines->getline(0));          // Low (index 3)
        combined->lines->add_line(close->lines->getline(0));        // Close (index 4)
        
        this->datas.push_back(combined);
    }
}

// DataSeries constructors for disambiguation
Ichimoku::Ichimoku(std::shared_ptr<DataSeries> data_source) : Indicator() {
    setup_lines();
    
    // Maximum period needed is max(senkou, kijun) + kijun
    _minperiod(std::max(params.senkou, params.kijun) + params.kijun);
    
    // Initialize storage vectors
    senkou_span_a_values_.resize(params.senkou_lead, 0.0);
    senkou_span_b_values_.resize(params.senkou_lead, 0.0);
    chikou_span_values_.resize(params.chikou, 0.0);
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

Ichimoku::Ichimoku(std::shared_ptr<DataSeries> data_source, int tenkan, int kijun, int senkou) : Indicator() {
    params.tenkan = tenkan;
    params.kijun = kijun;
    params.senkou = senkou;
    
    setup_lines();
    
    // Maximum period needed is max(senkou, kijun) + kijun
    _minperiod(std::max(params.senkou, params.kijun) + params.kijun);
    
    // Initialize storage vectors
    senkou_span_a_values_.resize(params.senkou_lead, 0.0);
    senkou_span_b_values_.resize(params.senkou_lead, 0.0);
    chikou_span_values_.resize(params.chikou, 0.0);
    
    this->data = std::static_pointer_cast<LineSeries>(data_source);
    this->datas.push_back(std::static_pointer_cast<LineSeries>(data_source));
}

void Ichimoku::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void Ichimoku::prenext() {
    Indicator::prenext();
}

void Ichimoku::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto tenkan_line = lines->getline(tenkan_sen);
    auto kijun_line = lines->getline(kijun_sen);
    auto senkou_a_line = lines->getline(senkou_span_a);
    auto senkou_b_line = lines->getline(senkou_span_b);
    auto chikou_line = lines->getline(chikou_span);
    
    if (!tenkan_line || !kijun_line || !senkou_a_line || !senkou_b_line || !chikou_line) return;
    
    // Calculate Tenkan-sen (Conversion Line)
    tenkan_line->set(0, calculate_tenkan_sen());
    
    // Calculate Kijun-sen (Base Line)
    kijun_line->set(0, calculate_kijun_sen());
    
    // Calculate Senkou Span A (Leading Span A) - shifted forward
    double senkou_a_value = calculate_senkou_span_a();
    senkou_span_a_values_.push_back(senkou_a_value);
    if (senkou_span_a_values_.size() > static_cast<size_t>(params.senkou_lead)) {
        senkou_a_line->set(0, senkou_span_a_values_.front());
        senkou_span_a_values_.erase(senkou_span_a_values_.begin());
    }
    
    // Calculate Senkou Span B (Leading Span B) - shifted forward
    double senkou_b_value = calculate_senkou_span_b();
    senkou_span_b_values_.push_back(senkou_b_value);
    if (senkou_span_b_values_.size() > static_cast<size_t>(params.senkou_lead)) {
        senkou_b_line->set(0, senkou_span_b_values_.front());
        senkou_span_b_values_.erase(senkou_span_b_values_.begin());
    }
    
    // Calculate Chikou Span (Lagging Span) - shifted backward
    auto close_line = datas[0]->lines->getline(DataSeries::Close); // Close line
    if (close_line) {
        double close_value = (*close_line)[0];
        chikou_span_values_.push_back(close_value);
        if (chikou_span_values_.size() > static_cast<size_t>(params.chikou)) {
            chikou_line->set(0, chikou_span_values_.front());
            chikou_span_values_.erase(chikou_span_values_.begin());
        }
    }
}

void Ichimoku::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto tenkan_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(tenkan_sen));
    auto kijun_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(kijun_sen));
    auto senkou_a_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(senkou_span_a));
    auto senkou_b_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(senkou_span_b));
    auto chikou_buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(chikou_span));
    
    if (!tenkan_buffer || !kijun_buffer || !senkou_a_buffer || !senkou_b_buffer || !chikou_buffer) return;
    
    auto high_line = datas[0]->lines->getline(DataSeries::High);   // High (index 2)
    auto low_line = datas[0]->lines->getline(DataSeries::Low);     // Low (index 3)
    auto close_line = datas[0]->lines->getline(DataSeries::Close); // Close (index 4)
    
    if (!high_line || !low_line || !close_line) return;
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    
    if (!high_buffer || !low_buffer || !close_buffer) return;
    
    auto high_array = high_buffer->array();
    auto low_array = low_buffer->array();
    auto close_array = close_buffer->array();
    
    // Pre-allocate all buffers to the data size
    size_t data_size = high_array.size();
    
    // Temporary storage for values that will be shifted
    std::vector<double> tenkan_values(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> kijun_values(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> senkou_a_unshifted(data_size, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> senkou_b_unshifted(data_size, std::numeric_limits<double>::quiet_NaN());
    
    // Calculate Tenkan-sen and Kijun-sen
    for (size_t i = 0; i < data_size; ++i) {
        // Calculate Tenkan-sen (9-period)
        if (i >= static_cast<size_t>(params.tenkan - 1)) {
            double highest = high_array[i];
            double lowest = low_array[i];
            for (int j = 1; j < params.tenkan; ++j) {
                if (i >= static_cast<size_t>(j)) {
                    highest = std::max(highest, high_array[i - j]);
                    lowest = std::min(lowest, low_array[i - j]);
                }
            }
            tenkan_values[i] = (highest + lowest) / 2.0;
        }
        
        // Calculate Kijun-sen (26-period)
        if (i >= static_cast<size_t>(params.kijun - 1)) {
            double highest = high_array[i];
            double lowest = low_array[i];
            for (int j = 1; j < params.kijun; ++j) {
                if (i >= static_cast<size_t>(j)) {
                    highest = std::max(highest, high_array[i - j]);
                    lowest = std::min(lowest, low_array[i - j]);
                }
            }
            kijun_values[i] = (highest + lowest) / 2.0;
        }
    }
    
    // Calculate unshifted Senkou values
    for (size_t i = 0; i < data_size; ++i) {
        // Senkou Span A = (Tenkan + Kijun) / 2
        if (i >= static_cast<size_t>(params.kijun - 1)) {
            senkou_a_unshifted[i] = (tenkan_values[i] + kijun_values[i]) / 2.0;
        }
        
        // Senkou Span B = (52-period high + low) / 2
        if (i >= static_cast<size_t>(params.senkou - 1)) {
            double highest = high_array[i];
            double lowest = low_array[i];
            for (int j = 1; j < params.senkou; ++j) {
                if (i >= static_cast<size_t>(j)) {
                    highest = std::max(highest, high_array[i - j]);
                    lowest = std::min(lowest, low_array[i - j]);
                }
            }
            senkou_b_unshifted[i] = (highest + lowest) / 2.0;
        }
    }
    
    // Create shifted buffers - we need to allocate extra space for forward shifts
    std::vector<double> senkou_a_shifted(data_size + params.senkou_lead, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> senkou_b_shifted(data_size + params.senkou_lead, std::numeric_limits<double>::quiet_NaN());
    
    // Fill the shifted senkou buffers
    // Python: senkou_span_a(-26) means values are pushed 26 periods into the future
    for (size_t i = 0; i < data_size; ++i) {
        if (!std::isnan(senkou_a_unshifted[i])) {
            // Place this value 26 periods in the future
            senkou_a_shifted[i + params.senkou_lead] = senkou_a_unshifted[i];
        }
        if (!std::isnan(senkou_b_unshifted[i])) {
            senkou_b_shifted[i + params.senkou_lead] = senkou_b_unshifted[i];
        }
    }
    
    // Fill output buffers
    for (size_t i = 0; i < data_size; ++i) {
        // Tenkan and Kijun - no shift
        tenkan_buffer->append(tenkan_values[i]);
        kijun_buffer->append(kijun_values[i]);
        
        // Senkou values - take from shifted arrays
        senkou_a_buffer->append(senkou_a_shifted[i]);
        senkou_b_buffer->append(senkou_b_shifted[i]);
        
        // Chikou Span - shifted backward by chikou (26) periods
        // Python: close(26) means show the close from 26 periods in the past
        if (i + params.chikou < data_size) {
            chikou_buffer->append(close_array[i + params.chikou]);
        } else {
            chikou_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

double Ichimoku::get_highest(int period, int offset) {
    if (datas.empty() || !datas[0]->lines) return std::numeric_limits<double>::quiet_NaN();
    
    auto high_line = datas[0]->lines->getline(DataSeries::High); // High line
    if (!high_line) return std::numeric_limits<double>::quiet_NaN();
    
    // Handle LineBuffer data access
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    if (high_buffer && high_buffer->size() == 0) {
        auto high_array = high_buffer->array();
        if (high_array.empty()) return std::numeric_limits<double>::quiet_NaN();
        
        int array_size = static_cast<int>(high_array.size());
        int idx = array_size - 1 - offset;
        if (idx < 0 || idx >= array_size) return std::numeric_limits<double>::quiet_NaN();
        
        double highest = high_array[idx];
        for (int i = 1; i < period && (idx - i) >= 0; ++i) {
            double value = high_array[idx - i];
            if (value > highest) {
                highest = value;
            }
        }
        return highest;
    }
    
    // Regular LineSeries access
    if (high_line->size() == 0) return std::numeric_limits<double>::quiet_NaN();
    
    double highest = (*high_line)[-offset];
    for (int i = 1; i < period; ++i) {
        double value = (*high_line)[-(i + offset)];
        if (value > highest) {
            highest = value;
        }
    }
    
    return highest;
}

double Ichimoku::get_lowest(int period, int offset) {
    if (datas.empty() || !datas[0]->lines) return std::numeric_limits<double>::quiet_NaN();
    
    auto low_line = datas[0]->lines->getline(DataSeries::Low); // Low line
    if (!low_line) return std::numeric_limits<double>::quiet_NaN();
    
    // Handle LineBuffer data access
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line);
    if (low_buffer && low_buffer->size() == 0) {
        auto low_array = low_buffer->array();
        if (low_array.empty()) return std::numeric_limits<double>::quiet_NaN();
        
        int array_size = static_cast<int>(low_array.size());
        int idx = array_size - 1 - offset;
        if (idx < 0 || idx >= array_size) return std::numeric_limits<double>::quiet_NaN();
        
        double lowest = low_array[idx];
        for (int i = 1; i < period && (idx - i) >= 0; ++i) {
            double value = low_array[idx - i];
            if (value < lowest) {
                lowest = value;
            }
        }
        return lowest;
    }
    
    // Regular LineSeries access
    if (low_line->size() == 0) return std::numeric_limits<double>::quiet_NaN();
    
    double lowest = (*low_line)[-offset];
    for (int i = 1; i < period; ++i) {
        double value = (*low_line)[-(i + offset)];
        if (value < lowest) {
            lowest = value;
        }
    }
    
    return lowest;
}

double Ichimoku::calculate_tenkan_sen(int offset) {
    double highest = get_highest(params.tenkan, offset);
    double lowest = get_lowest(params.tenkan, offset);
    
    if (std::isnan(highest) || std::isnan(lowest)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (highest + lowest) / 2.0;
}

double Ichimoku::calculate_kijun_sen(int offset) {
    double highest = get_highest(params.kijun, offset);
    double lowest = get_lowest(params.kijun, offset);
    
    if (std::isnan(highest) || std::isnan(lowest)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (highest + lowest) / 2.0;
}

double Ichimoku::calculate_senkou_span_a(int offset) {
    double tenkan = calculate_tenkan_sen(offset);
    double kijun = calculate_kijun_sen(offset);
    return (tenkan + kijun) / 2.0;
}

double Ichimoku::calculate_senkou_span_b(int offset) {
    double highest = get_highest(params.senkou, offset);
    double lowest = get_lowest(params.senkou, offset);
    return (highest + lowest) / 2.0;
}

// Utility methods for test framework compatibility
double Ichimoku::get(int ago) const {
    // Return Tenkan-sen as the main line for test framework
    auto tenkan_line = lines->getline(tenkan_sen);
    if (!tenkan_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*tenkan_line)[ago];
}

int Ichimoku::getMinPeriod() const {
    return std::max(params.senkou, params.kijun) + params.kijun;
}

size_t Ichimoku::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    // Return the size of the first line (tenkan_sen)
    auto tenkan_line = lines->getline(tenkan_sen);
    if (!tenkan_line) {
        return 0;
    }
    
    return tenkan_line->size();
}

void Ichimoku::calculate() {
    // For batch processing, use once()
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(DataSeries::High);
    if (!high_line) return;
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
    if (!high_buffer) return;
    
    // Use array data size for LineBuffer
    size_t data_size = high_buffer->array().size();
    if (data_size == 0) {
        data_size = high_buffer->size();
    }
    
    if (data_size > 0) {
        // Clear existing buffers
        for (int i = 0; i < 5; ++i) {
            auto buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(i));
            if (buffer) {
                buffer->reset();  // Use reset instead of home
            }
        }
        
        // Calculate all values
        once(0, data_size);
        
        // Set buffer indices to the last position for proper ago indexing
        for (int i = 0; i < 5; ++i) {
            auto buffer = std::dynamic_pointer_cast<LineBuffer>(lines->getline(i));
            if (buffer && buffer->size() > 0) {
                buffer->set_idx(buffer->size() - 1);
            }
        }
    }
}

// Individual line accessors
double Ichimoku::getTenkanSen(int ago) const {
    auto line = lines->getline(tenkan_sen);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

double Ichimoku::getKijunSen(int ago) const {
    auto line = lines->getline(kijun_sen);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

double Ichimoku::getSenkouSpanA(int ago) const {
    auto line = lines->getline(senkou_span_a);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

double Ichimoku::getSenkouSpanB(int ago) const {
    auto line = lines->getline(senkou_span_b);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

double Ichimoku::getChikouSpan(int ago) const {
    auto line = this->lines->getline(Ichimoku::chikou_span);
    if (!line) return std::numeric_limits<double>::quiet_NaN();
    return (*line)[ago];
}

} // namespace indicators
} // namespace backtrader