#pragma once

#include "envelope.h"

namespace backtrader {
namespace indicators {

// Re-export SMMAEnvelope from envelope.h for backward compatibility
using SMMAEnvelope = SmoothedMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader