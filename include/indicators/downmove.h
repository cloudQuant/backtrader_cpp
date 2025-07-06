#pragma once

#include "upmove.h"

namespace backtrader {
namespace indicators {

// Re-export DownMove from upmove.h for backward compatibility
using DownMove = ::backtrader::indicators::DownMove;

} // namespace indicators
} // namespace backtrader