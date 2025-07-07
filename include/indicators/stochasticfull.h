#pragma once

// Forwarding header for StochasticFull
// The actual StochasticFull class is defined in stochastic.h

#include "stochastic.h"

namespace backtrader {
namespace indicators {

// StochasticFull is already defined in stochastic.h
// This header provides an alias for consistency with the test framework
using StochasticFull = backtrader::StochasticFull;

} // namespace indicators
} // namespace backtrader