#pragma once

#include "envelope.h"

namespace backtrader {
namespace indicators {

// Re-export SMAEnvelope from envelope.h for backward compatibility
using SMAEnvelope = SimpleMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader