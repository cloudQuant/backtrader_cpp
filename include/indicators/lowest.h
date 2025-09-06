#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "basicops.h"
#include <memory>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// Calculate lowest value over period
class Lowest : public OperationN {
public:
    // Line indices
    enum LineIndex { 
        lowest = 0 
    };
    
    Lowest();
    Lowest(std::shared_ptr<LineSeries> data_source, int period = 14);
    // DataSeries constructors for disambiguation
    Lowest(std::shared_ptr<DataSeries> data_source);
    Lowest(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~Lowest() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    // Override to use close line (index 4) for DataSeries - matches Python behavior
    int get_dataseries_line_index() const override { return 4; }
    
private:
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    void setup_lines();
};

// Aliases
using Low = Lowest;
using MinN = Lowest;

} // namespace indicators
} // namespace backtrader