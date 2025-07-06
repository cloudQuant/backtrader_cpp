#pragma once

#include "envelope.h"

namespace backtrader {
namespace indicators {

// Re-export DEMAEnvelope from envelope.h for backward compatibility
using DEMAEnvelope = DoubleExponentialMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader