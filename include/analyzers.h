#pragma once

// Include only the essential analyzer classes to avoid compilation errors
#include "analyzers/sqn.h"
#include "analyzers/timereturn.h"

namespace backtrader {

// Analyzer namespace alias for convenience
namespace analyzers {
    using SQN = backtrader::SQN;
    using SystemQualityNumber = backtrader::SystemQualityNumber;
    using TimeReturn = backtrader::TimeReturn;
}

} // namespace backtrader