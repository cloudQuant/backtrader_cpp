#pragma once

#include "envelope.h"

namespace backtrader {
namespace indicators {

// Re-export EMAEnvelope from envelope.h for backward compatibility
using EMAEnvelope = ExponentialMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader