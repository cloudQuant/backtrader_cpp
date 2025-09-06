#pragma once

#include "../indicator.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Smoothed Moving Average (Wilder's Moving Average)
class SmoothedMovingAverage : public Indicator {
public:
    struct Params {
        int period = 30;  // Period for SMMA calculation
    } params;
    
    // Lines
    enum Lines { 
        smma = 0
    };
    
    SmoothedMovingAverage();
    SmoothedMovingAverage(std::shared_ptr<LineSeries> data_source);  // Test framework constructor
    SmoothedMovingAverage(std::shared_ptr<LineSeries> data_source, int period);
    // Constructor for test framework compatibility
    SmoothedMovingAverage(std::shared_ptr<DataSeries> data_source, int period = 30);
    virtual ~SmoothedMovingAverage() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // SMMA calculation parameters
    double alpha_;        // Smoothing factor (1.0 / period)
    double alpha1_;       // 1.0 - alpha
    double prev_smma_;    // Previous SMMA value
    bool initialized_;    // Whether SMMA is initialized
    
    // Initial seed calculation
    std::vector<double> seed_values_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using SMMA = SmoothedMovingAverage;
using WilderMA = SmoothedMovingAverage;
using MovingAverageSmoothed = SmoothedMovingAverage;
using MovingAverageWilder = SmoothedMovingAverage;
using ModifiedMovingAverage = SmoothedMovingAverage;

} // namespace indicators
} // namespace backtrader