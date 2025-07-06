#include "indicators/bollinger.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// BollingerBands implementation
BollingerBands::BollingerBands() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    
    // Set minimum period
    _minperiod(params.period);
}

BollingerBands::BollingerBands(std::shared_ptr<LineRoot> data) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period);
    // This constructor is for test framework compatibility
}

BollingerBands::BollingerBands(std::shared_ptr<LineRoot> data, int period, double devfactor) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    params.devfactor = devfactor;
    setup_lines();
    _minperiod(params.period);
    // This constructor is for test framework with parameters
}

BollingerBands::BollingerBands(std::shared_ptr<LineSeries> data_source, int period, double devfactor) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    params.devfactor = devfactor;
    
    setup_lines();
    
    // Set minimum period
    _minperiod(params.period);
}

void BollingerBands::setup_lines() {
    // Create 3 lines: mid, top, bot
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // mid line
        lines->add_line(std::make_shared<LineBuffer>());  // top line
        lines->add_line(std::make_shared<LineBuffer>());  // bot line
        lines->add_alias("mid", 0);
        lines->add_alias("top", 1);
        lines->add_alias("bot", 2);
    }
}

double BollingerBands::calculate_sma(int period, int current_index) const {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto close_line = datas[0]->lines->getline(0);
    if (!close_line) return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += (*close_line)[-i];
    }
    
    return sum / period;
}

double BollingerBands::calculate_stddev(int period, int current_index, double mean) const {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto close_line = datas[0]->lines->getline(0);
    if (!close_line) return 0.0;
    
    double sum_squared_diff = 0.0;
    for (int i = 0; i < period; ++i) {
        double value = (*close_line)[-i];
        double diff = value - mean;
        sum_squared_diff += diff * diff;
    }
    
    double variance = sum_squared_diff / period;
    return std::sqrt(variance);
}

void BollingerBands::prenext() {
    Indicator::prenext();
}

void BollingerBands::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Calculate moving average (middle band)
    double mid_value = calculate_sma(params.period, 0);
    
    // Calculate standard deviation
    double stddev = calculate_stddev(params.period, 0, mid_value);
    
    // Calculate upper and lower bands
    double deviation = params.devfactor * stddev;
    double top_value = mid_value + deviation;
    double bot_value = mid_value - deviation;
    
    // Set line values
    auto mid_line = lines->getline(mid);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (mid_line) mid_line->set(0, mid_value);
    if (top_line) top_line->set(0, top_value);
    if (bot_line) bot_line->set(0, bot_value);
}

void BollingerBands::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(0);
    if (!close_line) return;
    
    auto mid_line = lines->getline(mid);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (!mid_line || !top_line || !bot_line) return;
    
    for (int i = start; i < end; ++i) {
        // Calculate SMA
        double sum = 0.0;
        for (int j = 0; j < params.period; ++j) {
            sum += (*close_line)[i - j];
        }
        double mid_value = sum / params.period;
        
        // Calculate standard deviation
        double sum_squared_diff = 0.0;
        for (int j = 0; j < params.period; ++j) {
            double value = (*close_line)[i - j];
            double diff = value - mid_value;
            sum_squared_diff += diff * diff;
        }
        double stddev = std::sqrt(sum_squared_diff / params.period);
        
        // Calculate bands
        double deviation = params.devfactor * stddev;
        double top_value = mid_value + deviation;
        double bot_value = mid_value - deviation;
        
        // Set values
        mid_line->set(i, mid_value);
        top_line->set(i, top_value);
        bot_line->set(i, bot_value);
    }
}

// BollingerBandsPct implementation
BollingerBandsPct::BollingerBandsPct() : BollingerBands() {
    // Add pctb line (BollingerBands already has mid, top, bot lines)
    if (lines->size() < 4) {
        lines->add_line(std::make_shared<LineBuffer>());  // pctb line
        lines->add_alias("pctb", 3);
    }
}

void BollingerBandsPct::prenext() {
    BollingerBands::prenext();
}

void BollingerBandsPct::next() {
    // Calculate Bollinger Bands first
    BollingerBands::next();
    
    // Calculate percentage B
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(0);
    auto top_line = lines->getline(BollingerBands::top);
    auto bot_line = lines->getline(BollingerBands::bot);
    auto pctb_line = lines->getline(pctb);
    
    if (close_line && top_line && bot_line && pctb_line) {
        double current_price = (*close_line)[0];
        double top_value = (*top_line)[0];
        double bot_value = (*bot_line)[0];
        
        double band_width = top_value - bot_value;
        if (band_width != 0.0) {
            double pctb_value = (current_price - bot_value) / band_width;
            pctb_line->set(0, pctb_value);
        } else {
            pctb_line->set(0, 0.5); // Middle of the range when band width is zero
        }
    }
}

void BollingerBandsPct::once(int start, int end) {
    // Calculate Bollinger Bands first
    BollingerBands::once(start, end);
    
    // Calculate percentage B for the entire range
    if (datas.empty() || !datas[0]->lines) return;
    
    auto close_line = datas[0]->lines->getline(0);
    auto top_line = lines->getline(BollingerBands::top);
    auto bot_line = lines->getline(BollingerBands::bot);
    auto pctb_line = lines->getline(pctb);
    
    if (close_line && top_line && bot_line && pctb_line) {
        for (int i = start; i < end; ++i) {
            double current_price = (*close_line)[i];
            double top_value = (*top_line)[i];
            double bot_value = (*bot_line)[i];
            
            double band_width = top_value - bot_value;
            if (band_width != 0.0) {
                double pctb_value = (current_price - bot_value) / band_width;
                pctb_line->set(i, pctb_value);
            } else {
                pctb_line->set(i, 0.5);
            }
        }
    }
}

double BollingerBands::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(0);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*mid_line)[ago];
}

double BollingerBands::getBandwidth(int ago) const {
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    auto mid_line = lines->getline(mid);
    
    if (!top_line || !bot_line || !mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double top_value = (*top_line)[ago];
    double bot_value = (*bot_line)[ago];
    double mid_value = (*mid_line)[ago];
    
    if (mid_value == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Bandwidth = (Upper Band - Lower Band) / Middle Band
    return (top_value - bot_value) / mid_value;
}

double BollingerBands::getPercentB(int ago) const {
    if (datas.empty() || !datas[0]->lines) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto close_line = datas[0]->lines->getline(0);
    auto top_line = lines->getline(top);
    auto bot_line = lines->getline(bot);
    
    if (!close_line || !top_line || !bot_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double close_value = (*close_line)[ago];
    double top_value = (*top_line)[ago];
    double bot_value = (*bot_line)[ago];
    
    double band_width = top_value - bot_value;
    if (band_width == 0.0) {
        return 0.5; // Middle of the range when band width is zero
    }
    
    // %B = (Close - Lower Band) / (Upper Band - Lower Band)
    return (close_value - bot_value) / band_width;
}

double BollingerBands::getMiddleBand(int ago) const {
    if (!lines || lines->size() < 1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto mid_line = lines->getline(mid);
    if (!mid_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*mid_line)[ago];
}

double BollingerBands::getUpperBand(int ago) const {
    if (!lines || lines->size() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto top_line = lines->getline(top);
    if (!top_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*top_line)[ago];
}

double BollingerBands::getLowerBand(int ago) const {
    if (!lines || lines->size() < 3) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto bot_line = lines->getline(bot);
    if (!bot_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*bot_line)[ago];
}

void BollingerBands::calculate() {
    if (!data_source_ || !data_source_->lines || data_source_->lines->size() == 0) {
        return;
    }
    
    auto data_line = data_source_->lines->getline(0);
    
    if (!data_line) {
        return;
    }
    
    // Calculate Bollinger Bands for the entire dataset using once() method
    once(0, data_line->size());
}

} // namespace indicators
} // namespace backtrader