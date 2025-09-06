#include "../../include/utils/py3.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace backtrader {
namespace utils {
namespace py3 {

// String utilities
std::string string_types::strip(const std::string& str, const std::string& chars) {
    if (str.empty()) {
        return str;
    }
    
    size_t start = str.find_first_not_of(chars);
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(chars);
    return str.substr(start, end - start + 1);
}

std::string string_types::lstrip(const std::string& str, const std::string& chars) {
    if (str.empty()) {
        return str;
    }
    
    size_t start = str.find_first_not_of(chars);
    if (start == std::string::npos) {
        return "";
    }
    
    return str.substr(start);
}

std::string string_types::rstrip(const std::string& str, const std::string& chars) {
    if (str.empty()) {
        return str;
    }
    
    size_t end = str.find_last_not_of(chars);
    if (end == std::string::npos) {
        return "";
    }
    
    return str.substr(0, end + 1);
}

std::vector<std::string> string_types::split(const std::string& str, const std::string& delimiter, int maxsplit) {
    std::vector<std::string> tokens;
    
    if (str.empty()) {
        return tokens;
    }
    
    size_t start = 0;
    size_t pos = 0;
    int splits = 0;
    
    while ((pos = str.find(delimiter, start)) != std::string::npos && 
           (maxsplit == -1 || splits < maxsplit)) {
        tokens.push_back(str.substr(start, pos - start));
        start = pos + delimiter.length();
        splits++;
    }
    
    // Add the remaining part
    tokens.push_back(str.substr(start));
    
    return tokens;
}

std::string string_types::join(const std::vector<std::string>& strings, const std::string& separator) {
    if (strings.empty()) {
        return "";
    }
    
    std::ostringstream result;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) {
            result << separator;
        }
        result << strings[i];
    }
    
    return result.str();
}

std::string string_types::replace(const std::string& str, const std::string& old_val, const std::string& new_val, int count) {
    if (str.empty() || old_val.empty()) {
        return str;
    }
    
    std::string result = str;
    size_t pos = 0;
    int replacements = 0;
    
    while ((pos = result.find(old_val, pos)) != std::string::npos && 
           (count == -1 || replacements < count)) {
        result.replace(pos, old_val.length(), new_val);
        pos += new_val.length();
        replacements++;
    }
    
    return result;
}

std::string string_types::upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string string_types::lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool string_types::startswith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }
    return str.substr(0, prefix.length()) == prefix;
}

bool string_types::endswith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

int string_types::find(const std::string& str, const std::string& substr, size_t start) {
    if (start >= str.length()) {
        return -1;
    }
    
    size_t pos = str.find(substr, start);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
}

int string_types::count(const std::string& str, const std::string& substr) {
    if (str.empty() || substr.empty()) {
        return 0;
    }
    
    int count = 0;
    size_t pos = 0;
    
    while ((pos = str.find(substr, pos)) != std::string::npos) {
        count++;
        pos += substr.length();
    }
    
    return count;
}

// Numeric utilities
bool numeric_types::isfinite(double value) {
    return std::isfinite(value);
}

bool numeric_types::isinf(double value) {
    return std::isinf(value);
}

bool numeric_types::isnan(double value) {
    return std::isnan(value);
}

double numeric_types::abs(double value) {
    return std::abs(value);
}

double numeric_types::round(double value, int decimals) {
    double factor = std::pow(10.0, decimals);
    return std::round(value * factor) / factor;
}

double numeric_types::floor(double value) {
    return std::floor(value);
}

double numeric_types::ceil(double value) {
    return std::ceil(value);
}

double numeric_types::pow(double base, double exponent) {
    return std::pow(base, exponent);
}

double numeric_types::sqrt(double value) {
    return std::sqrt(value);
}

double numeric_types::log(double value) {
    return std::log(value);
}

double numeric_types::log10(double value) {
    return std::log10(value);
}

double numeric_types::exp(double value) {
    return std::exp(value);
}

double numeric_types::sin(double value) {
    return std::sin(value);
}

double numeric_types::cos(double value) {
    return std::cos(value);
}

double numeric_types::tan(double value) {
    return std::tan(value);
}

// Container utilities
bool container_types::any(const std::vector<bool>& container) {
    return std::any_of(container.begin(), container.end(), [](bool b) { return b; });
}

bool container_types::all(const std::vector<bool>& container) {
    return std::all_of(container.begin(), container.end(), [](bool b) { return b; });
}

template<typename T>
std::vector<T> container_types::filter(const std::vector<T>& container, std::function<bool(const T&)> predicate) {
    std::vector<T> result;
    std::copy_if(container.begin(), container.end(), std::back_inserter(result), predicate);
    return result;
}

template<typename T, typename U>
std::vector<U> container_types::map(const std::vector<T>& container, std::function<U(const T&)> func) {
    std::vector<U> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(), std::back_inserter(result), func);
    return result;
}

template<typename T>
std::vector<T> container_types::reversed(const std::vector<T>& container) {
    std::vector<T> result(container.rbegin(), container.rend());
    return result;
}

template<typename T>
std::vector<T> container_types::sorted(const std::vector<T>& container) {
    std::vector<T> result = container;
    std::sort(result.begin(), result.end());
    return result;
}

template<typename T>
std::vector<T> container_types::sorted(const std::vector<T>& container, std::function<bool(const T&, const T&)> compare) {
    std::vector<T> result = container;
    std::sort(result.begin(), result.end(), compare);
    return result;
}

template<typename T>
T container_types::max(const std::vector<T>& container) {
    if (container.empty()) {
        throw std::invalid_argument("Container is empty");
    }
    return *std::max_element(container.begin(), container.end());
}

template<typename T>
T container_types::min(const std::vector<T>& container) {
    if (container.empty()) {
        throw std::invalid_argument("Container is empty");
    }
    return *std::min_element(container.begin(), container.end());
}

template<typename T>
double container_types::sum(const std::vector<T>& container) {
    return std::accumulate(container.begin(), container.end(), 0.0);
}

template<typename T>
size_t container_types::len(const std::vector<T>& container) {
    return container.size();
}

template<typename T>
std::vector<std::pair<size_t, T>> container_types::enumerate(const std::vector<T>& container, size_t start) {
    std::vector<std::pair<size_t, T>> result;
    result.reserve(container.size());
    
    for (size_t i = 0; i < container.size(); ++i) {
        result.emplace_back(start + i, container[i]);
    }
    
    return result;
}

// Range utilities
std::vector<int> range_types::range(int stop) {
    return range(0, stop, 1);
}

std::vector<int> range_types::range(int start, int stop) {
    return range(start, stop, 1);
}

std::vector<int> range_types::range(int start, int stop, int step) {
    std::vector<int> result;
    
    if (step == 0) {
        throw std::invalid_argument("Step cannot be zero");
    }
    
    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            result.push_back(i);
        }
    } else {
        for (int i = start; i > stop; i += step) {
            result.push_back(i);
        }
    }
    
    return result;
}

// Explicit template instantiations for common types
template std::vector<int> container_types::filter<int>(const std::vector<int>&, std::function<bool(const int&)>);
template std::vector<double> container_types::filter<double>(const std::vector<double>&, std::function<bool(const double&)>);
template std::vector<std::string> container_types::filter<std::string>(const std::vector<std::string>&, std::function<bool(const std::string&)>);

template std::vector<int> container_types::map<int, int>(const std::vector<int>&, std::function<int(const int&)>);
template std::vector<double> container_types::map<double, double>(const std::vector<double>&, std::function<double(const double&)>);
template std::vector<std::string> container_types::map<std::string, std::string>(const std::vector<std::string>&, std::function<std::string(const std::string&)>);

template std::vector<int> container_types::reversed<int>(const std::vector<int>&);
template std::vector<double> container_types::reversed<double>(const std::vector<double>&);
template std::vector<std::string> container_types::reversed<std::string>(const std::vector<std::string>&);

template std::vector<int> container_types::sorted<int>(const std::vector<int>&);
template std::vector<double> container_types::sorted<double>(const std::vector<double>&);
template std::vector<std::string> container_types::sorted<std::string>(const std::vector<std::string>&);

template std::vector<int> container_types::sorted<int>(const std::vector<int>&, std::function<bool(const int&, const int&)>);
template std::vector<double> container_types::sorted<double>(const std::vector<double>&, std::function<bool(const double&, const double&)>);
template std::vector<std::string> container_types::sorted<std::string>(const std::vector<std::string>&, std::function<bool(const std::string&, const std::string&)>);

template int container_types::max<int>(const std::vector<int>&);
template double container_types::max<double>(const std::vector<double>&);
template std::string container_types::max<std::string>(const std::vector<std::string>&);

template int container_types::min<int>(const std::vector<int>&);
template double container_types::min<double>(const std::vector<double>&);
template std::string container_types::min<std::string>(const std::vector<std::string>&);

template double container_types::sum<int>(const std::vector<int>&);
template double container_types::sum<double>(const std::vector<double>&);

template size_t container_types::len<int>(const std::vector<int>&);
template size_t container_types::len<double>(const std::vector<double>&);
template size_t container_types::len<std::string>(const std::vector<std::string>&);

template std::vector<std::pair<size_t, int>> container_types::enumerate<int>(const std::vector<int>&, size_t);
template std::vector<std::pair<size_t, double>> container_types::enumerate<double>(const std::vector<double>&, size_t);
template std::vector<std::pair<size_t, std::string>> container_types::enumerate<std::string>(const std::vector<std::string>&, size_t);

} // namespace py3
} // namespace utils
} // namespace backtrader