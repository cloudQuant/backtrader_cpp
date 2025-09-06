#pragma once

#include "../sizer.h"

namespace backtrader {
namespace sizers {

/**
 * FixedSize - This sizer simply returns a fixed size for any operation
 * 
 * Size can be controlled by number of tranches that a system wishes to use
 * to scale into trades by specifying the tranches parameter.
 * 
 * Params:
 *   - stake (default: 1) - Fixed stake size
 *   - tranches (default: 1) - Number of tranches to divide stake
 */
class FixedSize : public Sizer {
public:
    // Parameters structure
    struct Params {
        int stake = 1;
        int tranches = 1;
    };
    
    FixedSize(const Params& params = Params{});
    virtual ~FixedSize() = default;
    
    // Override sizer interface
    int _getsizing(std::shared_ptr<CommInfo> comminfo, double cash, 
                   std::shared_ptr<DataSeries> data, bool isbuy) override;
    
    void setsizing(int stake);
    
protected:
    Params p;  // Parameters
};

/**
 * SizerFix - Alias for FixedSize (for compatibility)
 */
using SizerFix = FixedSize;

/**
 * FixedReverser - Returns fixed size to reverse an open position or open one
 * 
 * - To open a position: return the param stake
 * - To reverse a position: return 2 * stake
 * 
 * Params:
 *   - stake (default: 1) - Fixed stake size
 */
class FixedReverser : public Sizer {
public:
    // Parameters structure
    struct Params {
        int stake = 1;
    };
    
    FixedReverser(const Params& params = Params{});
    virtual ~FixedReverser() = default;
    
    // Override sizer interface
    int _getsizing(std::shared_ptr<CommInfo> comminfo, double cash,
                   std::shared_ptr<DataSeries> data, bool isbuy) override;
    
protected:
    Params p;  // Parameters
};

/**
 * FixedSizeTarget - Returns a fixed target size
 * 
 * Useful when coupled with Target Orders and specifically cerebro.target_order_size().
 * Size can be controlled by number of tranches.
 * 
 * Params:
 *   - stake (default: 1) - Fixed target stake size
 *   - tranches (default: 1) - Number of tranches to divide stake
 */
class FixedSizeTarget : public Sizer {
public:
    // Parameters structure
    struct Params {
        int stake = 1;
        int tranches = 1;
    };
    
    FixedSizeTarget(const Params& params = Params{});
    virtual ~FixedSizeTarget() = default;
    
    // Override sizer interface
    int _getsizing(std::shared_ptr<CommInfo> comminfo, double cash,
                   std::shared_ptr<DataSeries> data, bool isbuy) override;
    
    void setsizing(int stake);
    
protected:
    Params p;  // Parameters
};

} // namespace sizers
} // namespace backtrader