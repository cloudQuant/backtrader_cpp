#include "indicators/Ichimoku.h"
#include "Common.h"
#include <stdexcept>
#include <algorithm>

namespace backtrader {

Ichimoku::Ichimoku(std::shared_ptr<LineRoot> high_input,
                   std::shared_ptr<LineRoot> low_input,
                   std::shared_ptr<LineRoot> close_input,
                   size_t tenkan_period, size_t kijun_period,
                   size_t senkou_b_period, size_t displacement)
    : IndicatorBase(high_input, "Ichimoku"),
      tenkan_period_(tenkan_period),
      kijun_period_(kijun_period),
      senkou_b_period_(senkou_b_period),
      displacement_(displacement),
      high_buffer_tenkan_(tenkan_period),
      low_buffer_tenkan_(tenkan_period),
      high_buffer_kijun_(kijun_period),
      low_buffer_kijun_(kijun_period),
      high_buffer_senkou_b_(senkou_b_period),
      low_buffer_senkou_b_(senkou_b_period),
      close_buffer_(displacement + 1) {
    
    if (tenkan_period == 0 || kijun_period == 0 || senkou_b_period == 0 || displacement == 0) {
        throw std::invalid_argument("Ichimoku periods must be greater than 0");
    }
    
    setInputs(high_input, low_input, close_input);
    setParam("tenkan_period", static_cast<double>(tenkan_period));
    setParam("kijun_period", static_cast<double>(kijun_period));
    setParam("senkou_b_period", static_cast<double>(senkou_b_period));
    setParam("displacement", static_cast<double>(displacement));
    
    setMinPeriod(std::max({tenkan_period, kijun_period, senkou_b_period}) + displacement);
    
    // Create output lines for all components
    addOutputLine();  // Kijun-sen (index 1)
    addOutputLine();  // Senkou Span A (index 2)
    addOutputLine();  // Senkou Span B (index 3)
    addOutputLine();  // Chikou Span (index 4)
}

void Ichimoku::setInputs(std::shared_ptr<LineRoot> high_input,
                         std::shared_ptr<LineRoot> low_input,
                         std::shared_ptr<LineRoot> close_input) {
    if (!high_input || !low_input || !close_input) {
        throw std::invalid_argument("Ichimoku requires valid high, low, and close inputs");
    }
    
    addInput(low_input);
    addInput(close_input);
}

void Ichimoku::reset() {
    IndicatorBase::reset();
    high_buffer_tenkan_.clear();
    low_buffer_tenkan_.clear();
    high_buffer_kijun_.clear();
    low_buffer_kijun_.clear();
    high_buffer_senkou_b_.clear();
    low_buffer_senkou_b_.clear();
    close_buffer_.clear();
}

void Ichimoku::calculate() {
    if (inputs_.size() < 3) {
        setOutput(0, NaN);  // Tenkan-sen
        setOutput(1, NaN);  // Kijun-sen
        setOutput(2, NaN);  // Senkou Span A
        setOutput(3, NaN);  // Senkou Span B
        setOutput(4, NaN);  // Chikou Span
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    
    if (!high_input || !low_input || !close_input) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        setOutput(2, NaN);
        setOutput(3, NaN);
        setOutput(4, NaN);
        return;
    }
    
    double current_high = high_input->get(0);
    double current_low = low_input->get(0);
    double current_close = close_input->get(0);
    
    if (isNaN(current_high) || isNaN(current_low) || isNaN(current_close)) {
        setOutput(0, NaN);
        setOutput(1, NaN);
        setOutput(2, NaN);
        setOutput(3, NaN);
        setOutput(4, NaN);
        return;
    }
    
    // Update all buffers
    high_buffer_tenkan_.push_back(current_high);
    low_buffer_tenkan_.push_back(current_low);
    high_buffer_kijun_.push_back(current_high);
    low_buffer_kijun_.push_back(current_low);
    high_buffer_senkou_b_.push_back(current_high);
    low_buffer_senkou_b_.push_back(current_low);
    close_buffer_.push_back(current_close);
    
    // Calculate Tenkan-sen (Conversion Line)
    double tenkan_sen = NaN;
    if (high_buffer_tenkan_.size() >= tenkan_period_) {
        tenkan_sen = calculateMidpoint(high_buffer_tenkan_, low_buffer_tenkan_);
    }
    setOutput(0, tenkan_sen);
    
    // Calculate Kijun-sen (Base Line)
    double kijun_sen = NaN;
    if (high_buffer_kijun_.size() >= kijun_period_) {
        kijun_sen = calculateMidpoint(high_buffer_kijun_, low_buffer_kijun_);
    }
    setOutput(1, kijun_sen);
    
    // Calculate Senkou Span A (Leading Span A)
    double senkou_span_a = NaN;
    if (!isNaN(tenkan_sen) && !isNaN(kijun_sen)) {
        senkou_span_a = (tenkan_sen + kijun_sen) / 2.0;
    }
    setOutput(2, senkou_span_a);
    
    // Calculate Senkou Span B (Leading Span B)
    double senkou_span_b = NaN;
    if (high_buffer_senkou_b_.size() >= senkou_b_period_) {
        senkou_span_b = calculateMidpoint(high_buffer_senkou_b_, low_buffer_senkou_b_);
    }
    setOutput(3, senkou_span_b);
    
    // Calculate Chikou Span (Lagging Span) - current close
    setOutput(4, current_close);
}

void Ichimoku::calculateBatch(size_t start, size_t end) {
    if (inputs_.size() < 3) {
        return;
    }
    
    auto high_input = getInput(0);
    auto low_input = getInput(1);
    auto close_input = getInput(2);
    
    if (!high_input || !low_input || !close_input) {
        return;
    }
    
    for (size_t i = start; i < end; ++i) {
        calculate();
        
        if (i < end - 1) {
            high_input->forward();
            low_input->forward();
            close_input->forward();
        }
    }
}

double Ichimoku::getTenkanSen() const {
    return get(0);
}

double Ichimoku::getKijunSen() const {
    return getOutput(1)->get(0);
}

double Ichimoku::getSenkouSpanA() const {
    return getOutput(2)->get(0);
}

double Ichimoku::getSenkouSpanB() const {
    return getOutput(3)->get(0);
}

double Ichimoku::getChikouSpan() const {
    return getOutput(4)->get(0);
}

double Ichimoku::getCloudTop() const {
    double span_a = getSenkouSpanA();
    double span_b = getSenkouSpanB();
    
    if (isNaN(span_a) || isNaN(span_b)) {
        return NaN;
    }
    
    return std::max(span_a, span_b);
}

double Ichimoku::getCloudBottom() const {
    double span_a = getSenkouSpanA();
    double span_b = getSenkouSpanB();
    
    if (isNaN(span_a) || isNaN(span_b)) {
        return NaN;
    }
    
    return std::min(span_a, span_b);
}

double Ichimoku::getCloudThickness() const {
    double span_a = getSenkouSpanA();
    double span_b = getSenkouSpanB();
    
    if (isNaN(span_a) || isNaN(span_b)) {
        return NaN;
    }
    
    return std::abs(span_a - span_b);
}

double Ichimoku::getTrendSignal() const {
    double tenkan = getTenkanSen();
    double kijun = getKijunSen();
    double cloud_top = getCloudTop();
    double cloud_bottom = getCloudBottom();
    
    if (isNaN(tenkan) || isNaN(kijun) || isNaN(cloud_top) || isNaN(cloud_bottom)) {
        return 0.0;
    }
    
    // Strong bullish: Tenkan > Kijun and both above cloud
    if (tenkan > kijun && tenkan > cloud_top && kijun > cloud_top) {
        return 1.0;
    }
    
    // Strong bearish: Tenkan < Kijun and both below cloud
    if (tenkan < kijun && tenkan < cloud_bottom && kijun < cloud_bottom) {
        return -1.0;
    }
    
    return 0.0;  // Neutral/mixed signals
}

double Ichimoku::getTKCrossSignal() const {
    try {
        double tenkan_current = getTenkanSen();
        double kijun_current = getKijunSen();
        double tenkan_prev = getOutput(0)->get(-1);
        double kijun_prev = getOutput(1)->get(-1);
        
        if (isNaN(tenkan_current) || isNaN(kijun_current) ||
            isNaN(tenkan_prev) || isNaN(kijun_prev)) {
            return 0.0;
        }
        
        // Bullish crossover: Tenkan crosses above Kijun
        if (tenkan_prev <= kijun_prev && tenkan_current > kijun_current) {
            return 1.0;
        }
        
        // Bearish crossover: Tenkan crosses below Kijun
        if (tenkan_prev >= kijun_prev && tenkan_current < kijun_current) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double Ichimoku::getPriceCloudSignal() const {
    try {
        if (inputs_.size() < 3) {
            return 0.0;
        }
        
        auto close_input = getInput(2);
        if (!close_input) {
            return 0.0;
        }
        
        double current_price = close_input->get(0);
        double prev_price = close_input->get(-1);
        double cloud_top = getCloudTop();
        double cloud_bottom = getCloudBottom();
        
        if (isNaN(current_price) || isNaN(prev_price) ||
            isNaN(cloud_top) || isNaN(cloud_bottom)) {
            return 0.0;
        }
        
        // Bullish breakout: price breaks above cloud
        if (prev_price <= cloud_top && current_price > cloud_top) {
            return 1.0;
        }
        
        // Bearish breakdown: price breaks below cloud
        if (prev_price >= cloud_bottom && current_price < cloud_bottom) {
            return -1.0;
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

double Ichimoku::getChikouSignal() const {
    try {
        if (inputs_.size() < 3) {
            return 0.0;
        }
        
        auto close_input = getInput(2);
        if (!close_input) {
            return 0.0;
        }
        
        double current_close = close_input->get(0);
        double displaced_close = close_input->get(-static_cast<int>(displacement_));
        
        if (isNaN(current_close) || isNaN(displaced_close)) {
            return 0.0;
        }
        
        if (current_close > displaced_close) {
            return 1.0;   // Bullish
        } else if (current_close < displaced_close) {
            return -1.0;  // Bearish
        }
        
        return 0.0;
        
    } catch (...) {
        return 0.0;
    }
}

bool Ichimoku::isBullishCloud() const {
    double span_a = getSenkouSpanA();
    double span_b = getSenkouSpanB();
    
    if (isNaN(span_a) || isNaN(span_b)) {
        return false;
    }
    
    return span_a > span_b;
}

bool Ichimoku::isPriceAboveCloud() const {
    try {
        if (inputs_.size() < 3) {
            return false;
        }
        
        auto close_input = getInput(2);
        if (!close_input) {
            return false;
        }
        
        double current_price = close_input->get(0);
        double cloud_top = getCloudTop();
        
        if (isNaN(current_price) || isNaN(cloud_top)) {
            return false;
        }
        
        return current_price > cloud_top;
        
    } catch (...) {
        return false;
    }
}

double Ichimoku::calculateMidpoint(const CircularBuffer<double>& high_buffer,
                                   const CircularBuffer<double>& low_buffer) const {
    if (high_buffer.empty() || low_buffer.empty()) {
        return NaN;
    }
    
    double highest = *std::max_element(high_buffer.begin(), high_buffer.end());
    double lowest = *std::min_element(low_buffer.begin(), low_buffer.end());
    
    return (highest + lowest) / 2.0;
}

} // namespace backtrader