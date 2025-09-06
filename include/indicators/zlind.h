#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"

namespace backtrader {
namespace indicators {

// Zero Lag Indicator (Error Correcting)
class ZeroLagIndicator : public Indicator {
public:
    struct Params {
        int period = 30;
        int gainlimit = 50;
    } params;
    
    // Line indices
    enum LineIndex { 
        ec = 0  // Error correcting line
    };
    
    ZeroLagIndicator();
    ZeroLagIndicator(std::shared_ptr<LineSeries> data_source, int period = 30);
    ZeroLagIndicator(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~ZeroLagIndicator() = default;
    
    // Utility methods
    double get(int ago = 0) const override;
    int getMinPeriod() const override;
    void calculate() override;
    size_t size() const override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Alpha values for calculation
    double alpha_;
    double alpha1_;
    
    // EMA state for persistent calculation
    double ema_value_;
    bool ema_initialized_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using ZLIndicator = ZeroLagIndicator;
using ZLInd = ZeroLagIndicator;
using EC = ZeroLagIndicator;
using ErrorCorrecting = ZeroLagIndicator;

} // namespace indicators
} // namespace backtrader