#pragma once

#include "../indicator.h"
#include "../lineiterator.h"
#include "wma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// WMA Envelope indicator
class WMAEnvelope : public IndicatorBase {
public:
    struct Params {
        int period = 30;        // WMA period
        double perc = 2.5;      // Envelope percentage
    } params;
    
    // Lines
    enum Lines {
        mid = 0,    // WMA line (middle)
        upper = 1,  // Upper envelope
        lower = 2   // Lower envelope
    };
    
    // Default constructor
    WMAEnvelope();
    
    // Constructor with parameters (LineIterator)
    WMAEnvelope(std::shared_ptr<LineIterator> data_line, 
                int period = 30, double perc = 2.5);
    
    // Constructor with parameters (LineRoot)
    WMAEnvelope(std::shared_ptr<LineRoot> data_line, 
                int period = 30, double perc = 2.5);
    
    virtual ~WMAEnvelope() = default;
    
    // Get method for accessing the indicator value (defaults to mid line)
    double get(int ago = 0) const;
    
    // Get specific line values
    double getMid(int ago = 0) const;
    double getUpper(int ago = 0) const;
    double getLower(int ago = 0) const;
    
    // Get minimum period required
    int getMinPeriod() const { return params.period; }
    
    // Calculate method for manual testing
    void calculate();
    
protected:
    
private:
    std::shared_ptr<WMA> wma_;
    std::vector<double> upper_data_;
    std::vector<double> lower_data_;
    int current_index_;
};

} // namespace indicators
} // namespace backtrader