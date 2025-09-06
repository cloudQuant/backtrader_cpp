#pragma once

#include "metabase.h"
#include <memory>

namespace backtrader {

// Forward declaration
class DataSeries;

// Base filter class
class Filter {
public:
    Filter(std::shared_ptr<DataSeries> data);
    virtual ~Filter() = default;
    
    // Main call operator - handles first time logic
    void operator()(std::shared_ptr<DataSeries> data);
    
protected:
    // First time initialization
    virtual void nextstart(std::shared_ptr<DataSeries> data);
    
    // Called on each data point
    virtual void next(std::shared_ptr<DataSeries> data);
    
private:
    bool first_time_ = true;
};

} // namespace backtrader