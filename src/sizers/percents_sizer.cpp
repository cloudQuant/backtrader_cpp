#include "../../include/sizers/percents_sizer.h"
#include <cmath>
#include <stdexcept>

namespace backtrader {
namespace sizers {

// PercentSizer implementation
PercentSizer::PercentSizer(const Params& params) : p(params) {
    if (p.percents <= 0.0 || p.percents > 100.0) {
        throw std::invalid_argument("Percents must be between 0 and 100");
    }
}

int PercentSizer::_getsizing(std::shared_ptr<CommInfo> comminfo, double cash,
                            std::shared_ptr<DataSeries> data, bool isbuy) {
    
    // Get current position for this data
    auto position = get_position(data);
    
    double size = 0.0;
    
    if (!position || position->size == 0) {
        // No position exists - calculate based on cash percentage
        if (data && data->lines.size() > 0) {
            // Get current price (assuming close price is at index 4)
            double current_price = data->lines[4][0]; // close price
            
            if (current_price > 0.0) {
                // Calculate position size based on cash percentage
                double cash_to_use = cash * (p.percents / 100.0);
                size = cash_to_use / current_price;
            }
        }
    } else {
        // Position exists - use existing position size
        size = std::abs(position->size);
    }
    
    // Convert to integer if requested
    if (p.retint) {
        size = std::floor(size);
    }
    
    // Return appropriate sign based on buy/sell
    return isbuy ? static_cast<int>(size) : -static_cast<int>(size);
}

// AllInSizer implementation
AllInSizer::AllInSizer() : PercentSizer(Params{100.0, false}) {
    // Uses 100% of available cash
}

// PercentSizerInt implementation
PercentSizerInt::PercentSizerInt(const Params& params) 
    : PercentSizer(PercentSizer::Params{params.percents, true}) {
    // Always returns integer values
}

// AllInSizerInt implementation
AllInSizerInt::AllInSizerInt() 
    : PercentSizerInt(PercentSizerInt::Params{100.0, true}) {
    // Uses 100% of available cash and returns integer
}

} // namespace sizers
} // namespace backtrader