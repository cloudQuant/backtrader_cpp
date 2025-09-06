#pragma once

#include <string>
#include <vector>

namespace backtrader {

// Version information
extern const std::string VERSION;
extern const std::vector<int> VERSION_TUPLE;

// Version functions
std::string get_version();
std::vector<int> get_version_tuple();

} // namespace backtrader