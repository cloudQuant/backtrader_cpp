#pragma once

#include "dema.h"

namespace backtrader {
namespace indicators {

// Re-export TEMA from dema.h for backward compatibility
using TEMA = TripleExponentialMovingAverage;

} // namespace indicators
} // namespace backtrader