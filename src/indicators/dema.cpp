#include "indicators/dema.h"
#include <numeric>
#include <limits>

namespace backtrader {
namespace indicators {

// DoubleExponentialMovingAverage implementation
DoubleExponentialMovingAverage::DoubleExponentialMovingAverage() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Create two EMA indicators
    ema1_ = std::make_shared<EMA>(params.period);
    ema2_ = std::make_shared<EMA>(params.period);
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Create two EMA indicators
    ema1_ = std::make_shared<EMA>(data_source, period);
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<LineRoot> data) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Create two EMA indicators
    ema1_ = std::make_shared<EMA>();
    ema2_ = std::make_shared<EMA>();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
    // This constructor is for test framework compatibility
}

DoubleExponentialMovingAverage::DoubleExponentialMovingAverage(std::shared_ptr<LineRoot> data, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    
    // Create two EMA indicators
    ema1_ = std::make_shared<EMA>();
    ema2_ = std::make_shared<EMA>();
    
    // DEMA needs 2 * period - 1 for full calculation
    _minperiod(2 * params.period - 1);
    // This constructor is for manual test compatibility
}

double DoubleExponentialMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) return std::numeric_limits<double>::quiet_NaN();
    auto dema_line = lines->getline(dema);
    if (!dema_line) return std::numeric_limits<double>::quiet_NaN();
    
    try {
        return (*dema_line)[ago];
    } catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

int DoubleExponentialMovingAverage::getMinPeriod() const {
    return 2 * params.period - 1;
}

void DoubleExponentialMovingAverage::calculate() {
    if (!data_source_) return;
    
    // Calculate first EMA
    ema1_->calculate();
    
    // Create second EMA using the first EMA's output as input
    if (!ema2_) {
        auto ema1_lineseries = std::make_shared<LineSeries>();
        ema1_lineseries->lines = ema1_->lines;
        ema2_ = std::make_shared<EMA>(ema1_lineseries, params.period);
    }
    
    // Calculate second EMA
    ema2_->calculate();
    
    // Calculate DEMA values
    auto dema_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(dema));
    auto ema1_line = ema1_->lines->getline(0);
    auto ema2_line = ema2_->lines->getline(0);
    
    if (!dema_line || !ema1_line || !ema2_line) return;
    
    // Calculate for all valid data points
    size_t min_size = std::min({ema1_line->size(), ema2_line->size()});
    for (size_t i = 0; i < min_size; ++i) {
        double ema1_value = (*ema1_line)[i];
        double ema2_value = (*ema2_line)[i];
        
        if (!std::isnan(ema1_value) && !std::isnan(ema2_value)) {
            double dema_value = 2.0 * ema1_value - ema2_value;
            if (i == 0) {
                dema_line->set(0, dema_value);
            } else {
                dema_line->append(dema_value);
            }
        }
    }
}

void DoubleExponentialMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void DoubleExponentialMovingAverage::prenext() {
    Indicator::prenext();
}

void DoubleExponentialMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to first EMA if not already done
    if (ema1_->datas.empty() && !datas.empty()) {
        ema1_->datas = datas;
    }
    
    // Update first EMA
    ema1_->next();
    
    // Connect EMA1 output to EMA2 input
    if (ema2_->datas.empty() && ema1_->lines) {
        auto ema1_lineseries = std::make_shared<LineSeries>();
        ema1_lineseries->lines = ema1_->lines;
        ema2_->datas.push_back(ema1_lineseries);
    }
    
    // Update second EMA
    ema2_->next();
    
    auto dema_line = lines->getline(dema);
    auto ema1_line = ema1_->lines->getline(0);
    auto ema2_line = ema2_->lines->getline(0);
    
    if (dema_line && ema1_line && ema2_line) {
        // DEMA = 2 * EMA1 - EMA2
        double ema1_value = (*ema1_line)[0];
        double ema2_value = (*ema2_line)[0];
        dema_line->set(0, 2.0 * ema1_value - ema2_value);
    }
}

void DoubleExponentialMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to first EMA if not already done
    if (ema1_->datas.empty() && !datas.empty()) {
        ema1_->datas = datas;
    }
    
    // Calculate first EMA
    ema1_->once(start, end);
    
    // Connect EMA1 output to EMA2 input
    if (ema2_->datas.empty() && ema1_->lines) {
        auto ema1_lineseries = std::make_shared<LineSeries>();
        ema1_lineseries->lines = ema1_->lines;
        ema2_->datas.push_back(ema1_lineseries);
    }
    
    // Calculate second EMA
    ema2_->once(start, end);
    
    auto dema_line = lines->getline(dema);
    auto ema1_line = ema1_->lines->getline(0);
    auto ema2_line = ema2_->lines->getline(0);
    
    if (!dema_line || !ema1_line || !ema2_line) return;
    
    for (int i = start; i < end; ++i) {
        double ema1_value = (*ema1_line)[i];
        double ema2_value = (*ema2_line)[i];
        dema_line->set(i, 2.0 * ema1_value - ema2_value);
    }
}

// TripleExponentialMovingAverage implementation
TripleExponentialMovingAverage::TripleExponentialMovingAverage() : Indicator() {
    setup_lines();
    
    // Create three EMA indicators
    ema1_ = std::make_shared<EMA>(params.period);
    ema2_ = std::make_shared<EMA>(params.period);
    ema3_ = std::make_shared<EMA>(params.period);
    
    // TEMA needs 3 * period - 2 for full calculation
    _minperiod(3 * params.period - 2);
}

TripleExponentialMovingAverage::TripleExponentialMovingAverage(std::shared_ptr<LineRoot> data) : Indicator() {
    setup_lines();
    
    // Create three EMA indicators
    ema1_ = std::make_shared<EMA>(params.period);
    ema2_ = std::make_shared<EMA>(params.period);
    ema3_ = std::make_shared<EMA>(params.period);
    
    // TEMA needs 3 * period - 2 for full calculation
    _minperiod(3 * params.period - 2);
}

TripleExponentialMovingAverage::TripleExponentialMovingAverage(std::shared_ptr<LineRoot> data, int period) : Indicator() {
    params.period = period;
    setup_lines();
    
    // Create three EMA indicators
    ema1_ = std::make_shared<EMA>(params.period);
    ema2_ = std::make_shared<EMA>(params.period);
    ema3_ = std::make_shared<EMA>(params.period);
    
    // TEMA needs 3 * period - 2 for full calculation
    _minperiod(3 * params.period - 2);
}

double TripleExponentialMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto tema_line = lines->getline(tema);
    if (!tema_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*tema_line)[ago];
}

int TripleExponentialMovingAverage::getMinPeriod() const {
    return 3 * params.period - 2;
}

void TripleExponentialMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void TripleExponentialMovingAverage::prenext() {
    Indicator::prenext();
}

void TripleExponentialMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto tema_line = lines->getline(tema);
    if (!tema_line) return;
    
    // Set data for sub-indicators
    ema1_->datas = datas;
    
    // Calculate EMAs in sequence
    ema1_->next();
    
    // Connect EMA1 output to EMA2 input
    if (ema2_->datas.empty() && ema1_->lines) {
        auto ema1_lineseries = std::make_shared<LineSeries>();
        ema1_lineseries->lines = ema1_->lines;
        ema2_->datas.push_back(ema1_lineseries);
    }
    ema2_->next();
    
    // Connect EMA2 output to EMA3 input
    if (ema3_->datas.empty() && ema2_->lines) {
        auto ema2_lineseries = std::make_shared<LineSeries>();
        ema2_lineseries->lines = ema2_->lines;
        ema3_->datas.push_back(ema2_lineseries);
    }
    ema3_->next();
    
    // Calculate TEMA: 3*EMA1 - 3*EMA2 + EMA3
    auto ema1_line = ema1_->lines->getline(0);
    auto ema2_line = ema2_->lines->getline(0);
    auto ema3_line = ema3_->lines->getline(0);
    
    if (ema1_line && ema2_line && ema3_line) {
        double ema1_value = (*ema1_line)[0];
        double ema2_value = (*ema2_line)[0];
        double ema3_value = (*ema3_line)[0];
        
        double tema_value = 3.0 * ema1_value - 3.0 * ema2_value + ema3_value;
        tema_line->set(0, tema_value);
    }
}

void TripleExponentialMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Set data for sub-indicators
    ema1_->datas = datas;
    
    // Calculate first EMA
    ema1_->once(start, end);
    
    // Connect EMA1 output to EMA2 input
    if (ema2_->datas.empty() && ema1_->lines) {
        auto ema1_lineseries = std::make_shared<LineSeries>();
        ema1_lineseries->lines = ema1_->lines;
        ema2_->datas.push_back(ema1_lineseries);
    }
    
    // Calculate second EMA
    ema2_->once(start, end);
    
    // Connect EMA2 output to EMA3 input
    if (ema3_->datas.empty() && ema2_->lines) {
        auto ema2_lineseries = std::make_shared<LineSeries>();
        ema2_lineseries->lines = ema2_->lines;
        ema3_->datas.push_back(ema2_lineseries);
    }
    
    // Calculate third EMA
    ema3_->once(start, end);
    
    auto tema_line = lines->getline(tema);
    auto ema1_line = ema1_->lines->getline(0);
    auto ema2_line = ema2_->lines->getline(0);
    auto ema3_line = ema3_->lines->getline(0);
    
    if (!tema_line || !ema1_line || !ema2_line || !ema3_line) return;
    
    for (int i = start; i < end; ++i) {
        double ema1_value = (*ema1_line)[i];
        double ema2_value = (*ema2_line)[i];
        double ema3_value = (*ema3_line)[i];
        
        // TEMA = 3*EMA1 - 3*EMA2 + EMA3
        double tema_value = 3.0 * ema1_value - 3.0 * ema2_value + ema3_value;
        tema_line->set(i, tema_value);
    }
}

} // namespace indicators
} // namespace backtrader