#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "sma.h"

namespace backtrader {
namespace indicators {

// Detrended Price Oscillator
class DetrendedPriceOscillator : public Indicator {
public:
    struct Params {
        int period = 20;
    } params;
    
    // Lines
    enum Lines { 
        dpo = 0  // Detrended Price Oscillator line
    };
    
    DetrendedPriceOscillator();
    DetrendedPriceOscillator(std::shared_ptr<LineSeries> data_source, int period = 20);
    // DataSeries constructors for disambiguation
    DetrendedPriceOscillator(std::shared_ptr<DataSeries> data_source);
    DetrendedPriceOscillator(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~DetrendedPriceOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Moving average for calculation
    std::shared_ptr<indicators::SMA> sma_;
    
    // Helper method to calculate DPO value
    double calculate_dpo(int index);
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using DPO = DetrendedPriceOscillator;

} // namespace indicators
} // namespace backtrader