#pragma once

// Include the full priceoscillator header
#include "priceoscillator.h"

namespace backtrader {
namespace indicators {

// Re-export the PercentagePriceOscillator class for test compatibility
using PercentagePriceOscillator = backtrader::PercentagePriceOscillator;
using PPO = PercentagePriceOscillator;
using PercPriceOsc = PercentagePriceOscillator;

} // namespace indicators
} // namespace backtrader