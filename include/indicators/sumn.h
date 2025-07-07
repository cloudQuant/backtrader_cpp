#pragma once

// Forwarding header for SumN
// The actual SumN class is defined in basicops.h

#include "basicops.h"

namespace backtrader {
namespace indicators {

// SumN is already defined in basicops.h in this namespace
// This header provides an alias for consistency with the test framework
using Sum = SumN;

} // namespace indicators
} // namespace backtrader