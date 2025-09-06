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
    
    // Constructor for DataSeries compatibility 
    WilliamsR(std::shared_ptr<DataSeries> data_source, int period = 14);

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
    
    // Size method for test framework compatibility
    size_t size() const override;
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    std::shared_ptr<LineSeries> data_source_;

    bool using_line_roots_;
    
    // Helper methods
    double get_highest(int period, int start_ago = 0);
    double get_lowest(int period, int start_ago = 0);
    double get_highest_at_index(int period, int index);
    double get_lowest_at_index(int period, int index);
};

} // namespace indicators
} // namespace backtrader