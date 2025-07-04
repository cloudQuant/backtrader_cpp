#pragma once

#include "metabase.h"
#include <memory>
#include <limits>

namespace backtrader {

// Forward declarations
class Order;
class DataSeries;

// Base class for all fillers
class FillerBase {
public:
    virtual ~FillerBase() = default;
    
    // Main function that returns execution size for an order
    virtual double operator()(std::shared_ptr<Order> order, double price, int ago) = 0;
};

// Fixed size filler - returns minimum of fixed size, remaining order size, and volume
class FixedSize : public FillerBase {
public:
    struct Params {
        double size = 0.0; // Maximum size to be executed (0 means no limit)
    } params;
    
    FixedSize(double size = 0.0);
    virtual ~FixedSize() = default;
    
    double operator()(std::shared_ptr<Order> order, double price, int ago) override;
};

// Fixed bar percentage filler - uses percentage of bar volume
class FixedBarPerc : public FillerBase {
public:
    struct Params {
        double perc = 100.0; // Percentage of volume bar to use (0.0 - 100.0)
    } params;
    
    FixedBarPerc(double perc = 100.0);
    virtual ~FixedBarPerc() = default;
    
    double operator()(std::shared_ptr<Order> order, double price, int ago) override;
};

// Bar point percentage filler - distributes volume across price range
class BarPointPerc : public FillerBase {
public:
    struct Params {
        double minmov = 0.01; // Minimum price movement
        double perc = 100.0;  // Percentage of allocated volume to use
    } params;
    
    BarPointPerc(double minmov = 0.01, double perc = 100.0);
    virtual ~BarPointPerc() = default;
    
    double operator()(std::shared_ptr<Order> order, double price, int ago) override;

private:
    double calculate_parts(double high, double low, double minmov) const;
};

// Factory functions for creating fillers
std::shared_ptr<FillerBase> create_fixed_size_filler(double size = 0.0);
std::shared_ptr<FillerBase> create_fixed_bar_perc_filler(double perc = 100.0);
std::shared_ptr<FillerBase> create_bar_point_perc_filler(double minmov = 0.01, double perc = 100.0);

} // namespace backtrader