#pragma once

// Include the full prettygoodoscillator header
#include "prettygoodoscillator.h"

namespace backtrader {
namespace indicators {

// Re-export the PrettyGoodOscillator class for test compatibility
using PrettyGoodOscillator = backtrader::PrettyGoodOscillator;
using PGO = PrettyGoodOscillator;
using PrettyGoodOsc = PrettyGoodOscillator;

} // namespace indicators
} // namespace backtrader