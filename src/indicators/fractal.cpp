#include "indicators/fractal.h"
#include <cmath>
#include <limits>

namespace backtrader {
namespace indicators {

Fractal::Fractal() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

Fractal::Fractal(std::shared_ptr<LineRoot> data) : Indicator() {
    setup_lines();
    _minperiod(params.period);
    
    // This constructor is for test framework compatibility
    // It assumes data is a data series where high/low can be extracted
}

Fractal::Fractal(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, int period) : Indicator() {
    params.period = period;
    setup_lines();
    _minperiod(params.period);
    
    // Store the high/low data - in a full implementation this would be handled differently
    // For now, we'll assume the data will be available through the normal data feeds
}

void Fractal::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());  // up fractal line
        lines->add_line(std::make_shared<LineBuffer>());  // down fractal line
        lines->add_alias("up", 0);
        lines->add_alias("down", 1);
    }
}

double Fractal::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto up_line = lines->getline(up);
    if (!up_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*up_line)[ago];
}

int Fractal::getMinPeriod() const {
    return params.period;
}

void Fractal::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);  // high is usually line 1
    auto low_line = datas[0]->lines->getline(2);   // low is usually line 2
    auto up_line = lines->getline(up);
    auto down_line = lines->getline(down);
    
    if (!high_line || !low_line || !up_line || !down_line) return;
    
    // Initialize with NaN
    up_line->set(0, std::numeric_limits<double>::quiet_NaN());
    down_line->set(0, std::numeric_limits<double>::quiet_NaN());
    
    // Need at least 5 bars for fractal calculation
    if (high_line->size() < params.period) return;
    
    int middle = (params.period - 1) / 2;  // For period=5, middle=2
    
    // Check for up fractal (high in the middle is higher than surrounding highs)
    double middle_high = (*high_line)[-middle];
    bool is_up_fractal = true;
    
    for (int i = 0; i < params.period; ++i) {
        if (i == middle) continue;  // Skip the middle bar
        int offset = i - middle;
        if ((*high_line)[offset] >= middle_high) {
            is_up_fractal = false;
            break;
        }
    }
    
    if (is_up_fractal) {
        up_line->set(-middle, middle_high);
    }
    
    // Check for down fractal (low in the middle is lower than surrounding lows)
    double middle_low = (*low_line)[-middle];
    bool is_down_fractal = true;
    
    for (int i = 0; i < params.period; ++i) {
        if (i == middle) continue;  // Skip the middle bar
        int offset = i - middle;
        if ((*low_line)[offset] <= middle_low) {
            is_down_fractal = false;
            break;
        }
    }
    
    if (is_down_fractal) {
        down_line->set(-middle, middle_low);
    }
}

void Fractal::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);  // high is usually line 1
    auto low_line = datas[0]->lines->getline(2);   // low is usually line 2
    auto up_line = lines->getline(up);
    auto down_line = lines->getline(down);
    
    if (!high_line || !low_line || !up_line || !down_line) return;
    
    for (int i = start; i < end; ++i) {
        // Initialize with NaN
        up_line->set(i, std::numeric_limits<double>::quiet_NaN());
        down_line->set(i, std::numeric_limits<double>::quiet_NaN());
        
        // Need at least enough bars for fractal calculation
        if (i < params.period - 1) continue;
        
        int middle = (params.period - 1) / 2;  // For period=5, middle=2
        int fractal_bar = i - middle;
        
        // Check for up fractal
        double middle_high = (*high_line)[fractal_bar];
        bool is_up_fractal = true;
        
        for (int j = 0; j < params.period; ++j) {
            if (j == middle) continue;
            int check_bar = fractal_bar - middle + j;
            if (check_bar < 0 || check_bar >= end) continue;
            if ((*high_line)[check_bar] >= middle_high) {
                is_up_fractal = false;
                break;
            }
        }
        
        if (is_up_fractal) {
            up_line->set(fractal_bar, middle_high);
        }
        
        // Check for down fractal
        double middle_low = (*low_line)[fractal_bar];
        bool is_down_fractal = true;
        
        for (int j = 0; j < params.period; ++j) {
            if (j == middle) continue;
            int check_bar = fractal_bar - middle + j;
            if (check_bar < 0 || check_bar >= end) continue;
            if ((*low_line)[check_bar] <= middle_low) {
                is_down_fractal = false;
                break;
            }
        }
        
        if (is_down_fractal) {
            down_line->set(fractal_bar, middle_low);
        }
    }
}

} // namespace indicators
} // namespace backtrader