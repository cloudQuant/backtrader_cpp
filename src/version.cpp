#include "version.h"
#include <sstream>

namespace backtrader {

// Version constants
const std::string VERSION = "1.9.76.123";
const std::vector<int> VERSION_TUPLE = {1, 9, 76, 123};

std::string get_version() {
    return VERSION;
}

std::vector<int> get_version_tuple() {
    return VERSION_TUPLE;
}

} // namespace backtrader