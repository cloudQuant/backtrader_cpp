#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "sma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Commodity Channel Index (CCI)
class CommodityChannelIndex : public Indicator {
public:
    struct Params {
        int period = 20;           // Period for calculation
        double factor = 0.015;     // Factor for CCI calculation
        double upperband = 100.0;  // Upper band level
        double lowerband = -100.0; // Lower band level
    } params;
    
    // Lines
    enum Lines { 
        cci = 0  // CCI line
    };
    
    // Constructor with default parameters (original)
    CommodityChannelIndex();
    // Constructor for test framework compatibility (single parameter)
    CommodityChannelIndex(std::shared_ptr<LineSeries> data_source);
    // Constructor with parameters
    CommodityChannelIndex(std::shared_ptr<LineSeries> data_source, int period, double factor = 0.015);
    virtual ~CommodityChannelIndex() = default;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const override;
    void calculate() override;
    
    // Size method
    size_t size() const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double calculate_typical_price(int offset = 0);
    double calculate_mean_deviation(const std::vector<double>& tp_values, double mean);
    
    // Sub-indicators
    std::shared_ptr<indicators::SMA> tp_sma_;  // SMA of typical price
    
    // Circular buffers for typical price values
    std::vector<double> tp_values_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using CCI = CommodityChannelIndex;

} // namespace indicators
} // namespace backtrader