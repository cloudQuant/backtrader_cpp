#include "indicators/williamsad.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace indicators {

WilliamsAccumulationDistribution::WilliamsAccumulationDistribution() 
    : Indicator(), data_source_(nullptr), current_index_(0), ad_value_(0.0) {
    setup_lines();
}

WilliamsAccumulationDistribution::WilliamsAccumulationDistribution(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0), ad_value_(0.0) {
    setup_lines();
}

WilliamsAccumulationDistribution::WilliamsAccumulationDistribution(std::shared_ptr<backtrader::LineRoot> high, 
                                                                   std::shared_ptr<backtrader::LineRoot> low, 
                                                                   std::shared_ptr<backtrader::LineRoot> close, 
                                                                   std::shared_ptr<backtrader::LineRoot> volume)
    : Indicator(), data_source_(nullptr), current_index_(0), 
      high_line_(high), low_line_(low), close_line_(close), volume_line_(volume), ad_value_(0.0) {
    setup_lines();
}

void WilliamsAccumulationDistribution::setup_lines() {
    if (!lines) {
        lines = std::make_shared<Lines>();
        // Create 1 line for accumulation/distribution
        lines->add_line(std::make_shared<LineBuffer>());
    }
    // Add alias for line name
    lines->add_alias("ad", 0);
}

double WilliamsAccumulationDistribution::get(int ago) const {
    if (!lines || ago < 0) return 0.0;
    auto line = lines->getline(0);
    if (!line) return 0.0;
    return (*line)[ago];
}

int WilliamsAccumulationDistribution::getMinPeriod() const {
    return 1; // Williams A/D has no minimum period
}

void WilliamsAccumulationDistribution::calculate() {
    double high, low, close, volume = 1.0;
    
    // Get OHLC data from appropriate source
    if (data_source_) {
        if (current_index_ >= data_source_->size()) return;
        // Using single data source (close prices for all)
        high = (*data_source_)[current_index_];
        low = (*data_source_)[current_index_];
        close = (*data_source_)[current_index_];
    } else if (high_line_ && low_line_ && close_line_) {
        // Using separate H/L/C/V lines
        auto high_single = std::dynamic_pointer_cast<LineSingle>(high_line_);
        auto low_single = std::dynamic_pointer_cast<LineSingle>(low_line_);
        auto close_single = std::dynamic_pointer_cast<LineSingle>(close_line_);
        auto volume_single = volume_line_ ? std::dynamic_pointer_cast<LineSingle>(volume_line_) : nullptr;
        
        if (!high_single || !low_single || !close_single) return;
        if (current_index_ >= high_single->size()) return;
        
        high = (*high_single)[current_index_];
        low = (*low_single)[current_index_];
        close = (*close_single)[current_index_];
        if (volume_single && current_index_ < volume_single->size()) {
            volume = (*volume_single)[current_index_];
        }
    } else {
        return;
    }
    
    // Williams A/D calculation: AD = AD_prev + Volume * (Close - TrueRangeLow) / TrueRange
    // Simplified version using (Close - Low) * Volume
    ad_value_ += (close - low) * volume;
    
    // Set the calculated value
    if (lines) {
        auto ad_line = lines->getline(0);
        if (ad_line) ad_line->set(0, ad_value_);
    }
    
    current_index_++;
}

void WilliamsAccumulationDistribution::next() {
    calculate();
}

void WilliamsAccumulationDistribution::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        current_index_ = i;
        calculate();
    }
}

} // namespace indicators
} // namespace backtrader