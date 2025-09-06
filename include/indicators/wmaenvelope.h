#pragma once

#include "../indicator.h"
#include "../lineiterator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "../lineroot.h"
#include "wma.h"
#include <memory>

namespace backtrader {
namespace indicators {

// WMA Envelope indicator
class WMAEnvelope : public Indicator {
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
    
    // Default constructor (for test framework compatibility)
    WMAEnvelope();
    
    // Constructor with parameters (LineIterator)
    WMAEnvelope(std::shared_ptr<LineIterator> data_line, 
                int period = 30, double perc = 2.5);
    
    // Constructor with parameters (LineSeries)
    WMAEnvelope(std::shared_ptr<LineSeries> data_source, 
                int period = 30, double perc = 2.5);
    
    // Constructor with parameters (DataSeries)
    WMAEnvelope(std::shared_ptr<DataSeries> data_source, 
                int period = 30, double perc = 2.5);
    
    virtual ~WMAEnvelope() = default;
    
    // Get method for accessing the indicator value (defaults to mid line)
    double get(int ago = 0) const;
    
    // Get specific line values
    double getMid(int ago = 0) const;
    double getUpper(int ago = 0) const;
    double getLower(int ago = 0) const;
    
    // Get line by index (for test compatibility)
    std::shared_ptr<LineSingle> getLine(size_t idx) const;
    
    // Get minimum period required
    int getMinPeriod() const override { return params.period; }
    
    // Size method
    size_t size() const override;
    
    // Calculate method for manual testing
    void calculate();
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    std::shared_ptr<WMA> wma_;
    int current_index_;
};

} // namespace indicators
} // namespace backtrader