#include "indicators/dm.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace indicators {

DirectionalMovement::DirectionalMovement() 
    : Indicator(), data_source_(nullptr), current_index_(0), 
      prev_high_(0.0), prev_low_(0.0), prev_close_(0.0), first_run_(true) {
    setup_lines();
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<LineSeries> data_source, int period)
    : Indicator(), data_source_(data_source), current_index_(0), 
      prev_high_(0.0), prev_low_(0.0), prev_close_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<backtrader::LineRoot> high, std::shared_ptr<backtrader::LineRoot> low, 
                                         std::shared_ptr<backtrader::LineRoot> close, int period)
    : Indicator(), data_source_(nullptr), current_index_(0),
      high_line_(high), low_line_(low), close_line_(close),
      prev_high_(0.0), prev_low_(0.0), prev_close_(0.0), first_run_(true) {
    params.period = period;
    setup_lines();
}

DirectionalMovement::DirectionalMovement(std::shared_ptr<LineRoot> data)
    : Indicator(), data_source_(nullptr), current_index_(0),
      prev_high_(0.0), prev_low_(0.0), prev_close_(0.0), first_run_(true) {
    // For test framework compatibility, we assume data is a close line
    close_line_ = data;
    setup_lines();
    _minperiod(params.period);
}

void DirectionalMovement::setup_lines() {
    if (!lines) {
        lines = std::make_shared<Lines>();
        // Create 4 lines for Plus DM, Minus DM, Plus DI, Minus DI
        for (int i = 0; i < 4; ++i) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
    }
    // Add aliases for line names
    lines->add_alias("plusDM", 0);
    lines->add_alias("minusDM", 1);
    lines->add_alias("plusDI", 2);
    lines->add_alias("minusDI", 3);
}

double DirectionalMovement::get(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(2); // Return Plus DI by default
    if (!line) return 0.0;
    return (*line)[ago];
}

int DirectionalMovement::getMinPeriod() const {
    return params.period;
}

void DirectionalMovement::calculate() {
    double high, low, close;
    
    // Get OHLC data from appropriate source
    if (data_source_) {
        if (current_index_ >= data_source_->size()) return;
        // Using single data source (close prices for all)
        high = (*data_source_)[current_index_];
        low = (*data_source_)[current_index_];
        close = (*data_source_)[current_index_];
    } else if (high_line_ && low_line_ && close_line_) {
        // Using separate H/L/C lines
        auto high_single = std::dynamic_pointer_cast<LineSingle>(high_line_);
        auto low_single = std::dynamic_pointer_cast<LineSingle>(low_line_);
        auto close_single = std::dynamic_pointer_cast<LineSingle>(close_line_);
        
        if (!high_single || !low_single || !close_single) return;
        if (current_index_ >= high_single->size()) return;
        
        high = (*high_single)[current_index_];
        low = (*low_single)[current_index_];
        close = (*close_single)[current_index_];
    } else {
        return;
    }
    
    if (first_run_) {
        prev_high_ = high;
        prev_low_ = low;
        prev_close_ = close;
        first_run_ = false;
        return;
    }
    
    // Calculate directional movements
    double plus_dm = std::max(0.0, high - prev_high_);
    double minus_dm = std::max(0.0, prev_low_ - low);
    
    // Set the calculated values
    if (lines) {
        auto plusDM_line = lines->getline(0);
        auto minusDM_line = lines->getline(1);
        
        if (plusDM_line) plusDM_line->set(0, plus_dm);
        if (minusDM_line) minusDM_line->set(0, minus_dm);
    }
    
    prev_high_ = high;
    prev_low_ = low;
    prev_close_ = close;
    current_index_++;
}

void DirectionalMovement::next() {
    calculate();
}

void DirectionalMovement::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

double DirectionalMovement::getDIPlus(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(2); // Plus DI
    if (!line) return 0.0;
    return (*line)[ago];
}

double DirectionalMovement::getDIMinus(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(3); // Minus DI
    if (!line) return 0.0;
    return (*line)[ago];
}

double DirectionalMovement::getDX(int ago) const {
    // DX = abs(DI+ - DI-) / (DI+ + DI-) * 100
    double diplus = getDIPlus(ago);
    double diminus = getDIMinus(ago);
    if (diplus + diminus == 0.0) return 0.0;
    return std::abs(diplus - diminus) / (diplus + diminus) * 100.0;
}

double DirectionalMovement::getADX(int ago) const {
    // Simplified ADX - in real implementation this would be a smoothed version of DX
    // For now, return DX
    return getDX(ago);
}

} // namespace indicators
} // namespace backtrader