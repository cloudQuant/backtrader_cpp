#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <memory>
#include <deque>

namespace backtrader {
namespace indicators {

// Rate of Change (ROC) Indicator
class RateOfChange : public Indicator {
public:
    struct Params {
        int period = 12;   // Period for ROC calculation
    } params;
    
    // Lines
    enum Lines { 
        roc = 0  // Rate of Change line
    };
    
    RateOfChange();
    RateOfChange(std::shared_ptr<LineSeries> data_source);
    RateOfChange(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructors
    RateOfChange(std::shared_ptr<DataSeries> data_source);
    RateOfChange(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~RateOfChange() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // Price buffer for period calculation
    std::deque<double> price_buffer_;
};

// Aliases
using ROC = RateOfChange;
using RateOfChangeIndicator = RateOfChange;

} // namespace indicators
} // namespace backtrader