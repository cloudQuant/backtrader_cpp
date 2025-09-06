#pragma once

// Forwarding header for TEMAEnvelope
// The actual TEMAEnvelope class is defined in envelope.h

#include "envelope.h"

namespace backtrader {
namespace indicators {

// TEMAEnvelope is already defined in envelope.h
// This header provides an alias for consistency with the test framework
using TEMAEnvelope = TripleExponentialMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader