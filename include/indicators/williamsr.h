#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>
#include <algorithm>

namespace backtrader {
namespace indicators {

// Williams %R indicator
class WilliamsR : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for calculation
        double upperband = -20.0; // Upper band level
        double lowerband = -80.0; // Lower band level
    } params;
    
    // Default constructor for test framework compatibility
    explicit WilliamsR(int period = 14);
    
    // Constructor with data source
    WilliamsR(std::shared_ptr<LineSeries> data_source, int period = 14);
    
    // Constructor with LineRoot objects for manual testing
    WilliamsR(std::shared_ptr<LineRoot> close_line, 
              std::shared_ptr<LineRoot> high_line, 
              std::shared_ptr<LineRoot> low_line, 
              int period = 14);
    
    virtual ~WilliamsR() = default;
    
    // Indicator implementation
    void next() override;
    void once(int start, int end) override;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const override;
    
    // Get minimum period required
    int getMinPeriod() const override { return params.period; }
    
    // Calculate method for manual testing
    void calculate() override;
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    std::shared_ptr<LineSeries> data_source_;
    
    // LineRoot sources for manual testing
    std::shared_ptr<LineRoot> close_line_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    bool using_line_roots_;
    
    // Helper methods
    double get_highest(int period, int start_ago = 0);
    double get_lowest(int period, int start_ago = 0);
};

} // namespace indicators
} // namespace backtrader