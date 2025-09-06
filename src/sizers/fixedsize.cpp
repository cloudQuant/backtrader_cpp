#include "../../include/sizers/fixedsize.h"
#include <algorithm>
#include <stdexcept>

namespace backtrader {
namespace sizers {

// FixedSize implementation
FixedSize::FixedSize(const Params& params) : p(params) {
    if (p.stake <= 0) {
        throw std::invalid_argument("Stake must be positive");
    }
    if (p.tranches <= 0) {
        throw std::invalid_argument("Tranches must be positive");
    }
}

int FixedSize::_getsizing(std::shared_ptr<CommInfo> comminfo, double cash, 
                         std::shared_ptr<DataSeries> data, bool isbuy) {
    // Calculate the effective stake size based on tranches
    int effective_stake = p.stake / p.tranches;
    
    // Ensure we have at least 1 unit per tranche
    if (effective_stake < 1) {
        effective_stake = 1;
    }
    
    // For buying, return positive size; for selling, return negative size
    return isbuy ? effective_stake : -effective_stake;
}

void FixedSize::setsizing(int stake) {
    if (stake <= 0) {
        throw std::invalid_argument("Stake must be positive");
    }
    p.stake = stake;
}

// FixedReverser implementation
FixedReverser::FixedReverser(const Params& params) : p(params) {
    if (p.stake <= 0) {
        throw std::invalid_argument("Stake must be positive");
    }
}

int FixedReverser::_getsizing(std::shared_ptr<CommInfo> comminfo, double cash,
                             std::shared_ptr<DataSeries> data, bool isbuy) {
    
    // Get current position for this data
    auto position = get_position(data);
    
    if (!position || position->size == 0) {
        // No position exists - return normal stake
        return isbuy ? p.stake : -p.stake;
    } else {
        // Position exists - return double stake to reverse
        // If current position is long and we want to sell (or vice versa)
        // we need 2x stake to first close the position and then open opposite
        int reverse_size = 2 * p.stake;
        
        // Check if this is actually a reversal
        bool current_is_long = position->size > 0;
        bool want_to_reverse = (current_is_long && !isbuy) || (!current_is_long && isbuy);
        
        if (want_to_reverse) {
            return isbuy ? reverse_size : -reverse_size;
        } else {
            // Same direction as current position - just add normal stake
            return isbuy ? p.stake : -p.stake;
        }
    }
}

// FixedSizeTarget implementation
FixedSizeTarget::FixedSizeTarget(const Params& params) : p(params) {
    if (p.stake <= 0) {
        throw std::invalid_argument("Stake must be positive");
    }
    if (p.tranches <= 0) {
        throw std::invalid_argument("Tranches must be positive");
    }
}

int FixedSizeTarget::_getsizing(std::shared_ptr<CommInfo> comminfo, double cash,
                               std::shared_ptr<DataSeries> data, bool isbuy) {
    
    // Calculate target position size
    int target_size = p.stake / p.tranches;
    if (target_size < 1) {
        target_size = 1;
    }
    
    // Make target negative for short positions
    if (!isbuy) {
        target_size = -target_size;
    }
    
    // Get current position
    auto position = get_position(data);
    int current_size = (position) ? static_cast<int>(position->size) : 0;
    
    // Calculate the difference to reach target
    int size_diff = target_size - current_size;
    
    return size_diff;
}

void FixedSizeTarget::setsizing(int stake) {
    if (stake <= 0) {
        throw std::invalid_argument("Stake must be positive");
    }
    p.stake = stake;
}

} // namespace sizers
} // namespace backtrader