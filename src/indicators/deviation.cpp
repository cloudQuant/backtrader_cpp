#include "indicators/deviation.h"
#include <cmath>
#include <algorithm>

namespace backtrader {

// StandardDeviation implementation
StandardDeviation::StandardDeviation() : Indicator() {
    setup_lines();
    _minperiod(params.period);
    
    // Create SMA instances for calculations
    mean_sma_ = std::make_shared<SMA>();
    mean_sma_->params.period = params.period;
    
    meansq_sma_ = std::make_shared<SMA>();
    meansq_sma_->params.period = params.period;
}

void StandardDeviation::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void StandardDeviation::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto stddev_line = lines->getline(stddev);
    
    if (!data_line || !stddev_line) return;
    
    // Calculate mean of data over period
    double sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        sum += (*data_line)[-i];
    }
    double mean = sum / params.period;
    
    // Calculate mean of squared data over period
    double sumsq = 0.0;
    for (int i = 0; i < params.period; ++i) {
        double value = (*data_line)[-i];
        sumsq += value * value;
    }
    double meansq = sumsq / params.period;
    
    // Calculate standard deviation
    double variance = meansq - (mean * mean);
    
    if (params.safepow) {
        stddev_line->set(0, std::sqrt(std::abs(variance)));
    } else {
        stddev_line->set(0, std::sqrt(variance));
    }
}

void StandardDeviation::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto stddev_line = lines->getline(stddev);
    
    if (!data_line || !stddev_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate mean of data over period
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += (*data_line)[i - j];
        }
        double mean = sum / params.period;
        
        // Calculate mean of squared data over period
        double sumsq = 0.0;
        for (int j = 0; j < params.period; ++j) {
            double value = (*data_line)[i - j];
            sumsq += value * value;
        }
        double meansq = sumsq / params.period;
        
        // Calculate standard deviation
        double variance = meansq - (mean * mean);
        
        if (params.safepow) {
            stddev_line->set(i, std::sqrt(std::abs(variance)));
        } else {
            stddev_line->set(i, std::sqrt(variance));
        }

// MeanDeviation implementation
MeanDeviation::MeanDeviation() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void MeanDeviation::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void MeanDeviation::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto meandev_line = lines->getline(meandev);
    
    if (!data_line || !meandev_line) return;
    
    // Calculate mean of data over period
    double sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        sum += (*data_line)[-i];
    }
    double mean = sum / params.period;
    
    // Calculate mean of absolute deviations
    double absdev_sum = 0.0;
    for (int i = 0; i < params.period; ++i) {
        absdev_sum += std::abs((*data_line)[-i] - mean);
    }
    
    meandev_line->set(0, absdev_sum / params.period);
}

void MeanDeviation::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto meandev_line = lines->getline(meandev);
    
    if (!data_line || !meandev_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate mean of data over period
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += (*data_line)[i - j];
        }
        double mean = sum / params.period;
        
        // Calculate mean of absolute deviations
        double absdev_sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            absdev_sum += std::abs((*data_line)[i - j] - mean);
        }
        
        meandev_line->set(i, absdev_sum / params.period);
    }
}

size_t StandardDeviation::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto stddev_line = lines->getline(stddev);
    return stddev_line ? stddev_line->size() : 0;
}

size_t MeanDeviation::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto meandev_line = lines->getline(meandev);
    return meandev_line ? meandev_line->size() : 0;
}

}
}
} // namespace backtrader