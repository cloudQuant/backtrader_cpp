#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <queue>
#include <any>
#include <functional>

namespace backtrader {
namespace utils {

/**
 * py3 - Python 3 compatibility utilities for C++
 * 
 * Provides C++ equivalents of Python 3 functions and types
 * to ease porting from Python backtrader code.
 */
namespace py3 {

// Type aliases for Python-like types
using bytes = std::vector<uint8_t>;
using str = std::string;
using queue = std::queue<std::any>;

// String functions
inline std::string bstr(const std::string& s) {
    return s;  // In C++, strings are already byte strings
}

template<typename T>
inline std::string str(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else {
        return std::string(value);
    }
}

inline long long_cast(const std::string& s) {
    return std::stoll(s);
}

inline long long_cast(double d) {
    return static_cast<long>(d);
}

// Iterator functions (Python-like)
template<typename Container>
auto items(const Container& container) {
    std::vector<std::pair<typename Container::key_type, typename Container::mapped_type>> result;
    for (const auto& item : container) {
        result.emplace_back(item.first, item.second);
    }
    return result;
}

template<typename Container>
auto keys(const Container& container) {
    std::vector<typename Container::key_type> result;
    for (const auto& item : container) {
        result.push_back(item.first);
    }
    return result;
}

template<typename Container>
auto values(const Container& container) {
    std::vector<typename Container::mapped_type> result;
    for (const auto& item : container) {
        result.push_back(item.second);
    }
    return result;
}

// Range function (Python-like)
inline std::vector<int> range(int stop) {
    std::vector<int> result;
    for (int i = 0; i < stop; ++i) {
        result.push_back(i);
    }
    return result;
}

inline std::vector<int> range(int start, int stop) {
    std::vector<int> result;
    for (int i = start; i < stop; ++i) {
        result.push_back(i);
    }
    return result;
}

inline std::vector<int> range(int start, int stop, int step) {
    std::vector<int> result;
    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            result.push_back(i);
        }
    } else if (step < 0) {
        for (int i = start; i > stop; i += step) {
            result.push_back(i);
        }
    }
    return result;
}

// Zip function (Python-like)
template<typename Container1, typename Container2>
auto zip(const Container1& c1, const Container2& c2) {
    using T1 = typename Container1::value_type;
    using T2 = typename Container2::value_type;
    
    std::vector<std::pair<T1, T2>> result;
    auto it1 = c1.begin();
    auto it2 = c2.begin();
    
    while (it1 != c1.end() && it2 != c2.end()) {
        result.emplace_back(*it1, *it2);
        ++it1;
        ++it2;
    }
    
    return result;
}

// Enumerate function (Python-like)
template<typename Container>
auto enumerate(const Container& container) {
    using T = typename Container::value_type;
    
    std::vector<std::pair<size_t, T>> result;
    size_t index = 0;
    
    for (const auto& item : container) {
        result.emplace_back(index++, item);
    }
    
    return result;
}

// Filter function (Python-like)
template<typename Container, typename Predicate>
auto filter(Predicate pred, const Container& container) {
    Container result;
    std::copy_if(container.begin(), container.end(), 
                 std::back_inserter(result), pred);
    return result;
}

// Map function (Python-like)
template<typename Container, typename Function>
auto map(Function func, const Container& container) {
    using ReturnType = decltype(func(*container.begin()));
    std::vector<ReturnType> result;
    
    std::transform(container.begin(), container.end(),
                   std::back_inserter(result), func);
    return result;
}

// String operations
namespace string {
    inline std::vector<std::string> split(const std::string& str, char delimiter = ' ') {
        std::vector<std::string> result;
        std::string current;
        
        for (char ch : str) {
            if (ch == delimiter) {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += ch;
            }
        }
        
        if (!current.empty()) {
            result.push_back(current);
        }
        
        return result;
    }
    
    inline std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
        if (strings.empty()) return "";
        
        std::string result = strings[0];
        for (size_t i = 1; i < strings.size(); ++i) {
            result += delimiter + strings[i];
        }
        return result;
    }
    
    inline std::string strip(const std::string& str) {
        auto start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        
        auto end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
}

} // namespace py3
} // namespace utils
} // namespace backtrader