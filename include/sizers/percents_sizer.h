#pragma once

#include "../sizer.h"
#include <cmath>

namespace backtrader {
namespace sizers {

/**
 * PercentSizer - Returns percents of available cash
 * 
 * This sizer calculates position size based on a percentage of available cash.
 * If no position exists, it calculates based on cash percentage.
 * If a position exists, it uses the existing position size.
 * 
 * Params:
 *   - percents: Percentage of cash to use (default: 20)
 *   - retint: Return integer size instead of float (default: false)
 */
class PercentSizer : public Sizer {
public:
    // Parameters structure
    struct Params {
        double percents = 20.0;
        bool retint = false;
    };
    
    PercentSizer(const Params& params = Params{});
    virtual ~PercentSizer() = default;
    
    // Override sizer interface
    int _getsizing(std::shared_ptr<CommInfo> comminfo, double cash, 
                   std::shared_ptr<DataSeries> data, bool isbuy) override;

protected:
    Params p;  // Parameters
};

/**
 * AllInSizer - Returns all available cash of broker
 * 
 * Uses 100% of available cash for position sizing.
 * 
 * Params:
 *   - percents: Fixed at 100%
 */
class AllInSizer : public PercentSizer {
public:
    AllInSizer();
    virtual ~AllInSizer() = default;
};

/**
 * PercentSizerInt - Returns percents of available cash as integer
 * 
 * Same as PercentSizer but truncates the result to an integer.
 * 
 * Params:
 *   - percents: Percentage of cash to use (default: 20)
 *   - retint: Fixed at true
 */
class PercentSizerInt : public PercentSizer {
public:
    // Parameters structure
    struct Params {
        double percents = 20.0;
        bool retint = true;
    };
    
    PercentSizerInt(const Params& params = Params{});
    virtual ~PercentSizerInt() = default;
};

/**
 * AllInSizerInt - Returns all available cash with size truncated to int
 * 
 * Uses 100% of available cash and returns integer size.
 * 
 * Params:
 *   - percents: Fixed at 100%
 *   - retint: Fixed at true
 */
class AllInSizerInt : public PercentSizerInt {
public:
    AllInSizerInt();
    virtual ~AllInSizerInt() = default;
};

} // namespace sizers
} // namespace backtrader