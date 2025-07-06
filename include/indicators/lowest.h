#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace indicators {

// Calculate lowest value over period
class Lowest : public Indicator {
public:
    struct Params {
        int period = 14;
    } params;
    
    // Line indices
    enum LineIndex { 
        lowest = 0 
    };
    
    Lowest();
    Lowest(std::shared_ptr<LineSeries> data_source, int period = 14);
    Lowest(std::shared_ptr<LineRoot> data_source, int period = 14);
    virtual ~Lowest() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    std::shared_ptr<LineRoot> root_data_source_;
    size_t current_index_;
};

// Aliases
using Low = Lowest;
using MinN = Lowest;

} // namespace indicators
} // namespace backtrader