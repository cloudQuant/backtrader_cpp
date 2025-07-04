#include "../../include/utils/autodict.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace backtrader {
namespace utils {

// Explicit template instantiations for common types
template class AutoDictList<std::string, std::any>;
template class AutoDictList<std::string, double>;
template class AutoDictList<std::string, int>;
template class AutoDictList<int, std::any>;

template class DotDict<std::string, std::any>;
template class DotDict<std::string, double>;
template class DotDict<std::string, int>;
template class DotDict<std::string, std::string>;

template class AutoOrderedDict<std::string, std::any>;
template class AutoOrderedDict<std::string, double>;
template class AutoOrderedDict<std::string, int>;
template class AutoOrderedDict<int, std::any>;

// Utility functions for AutoDictList
namespace auto_dict_utils {

/**
 * Create an AutoDictList with string keys and any values
 */
AutoDictList<std::string, std::any> create_string_any_list() {
    return AutoDictList<std::string, std::any>();
}

/**
 * Create an AutoDictList with string keys and double values
 */
AutoDictList<std::string, double> create_string_double_list() {
    return AutoDictList<std::string, double>();
}

/**
 * Print contents of an AutoDictList for debugging
 */
template<typename Key, typename Value>
void print_auto_dict_list(const AutoDictList<Key, Value>& dict, const std::string& name = "AutoDictList") {
    std::cout << name << " contents:" << std::endl;
    for (const auto& pair : dict) {
        std::cout << "  Key: " << pair.first << ", Values: [";
        for (size_t i = 0; i < pair.second.size(); ++i) {
            if (i > 0) std::cout << ", ";
            try {
                if constexpr (std::is_same_v<Value, std::any>) {
                    std::cout << "any_value";
                } else if constexpr (std::is_same_v<Value, std::string>) {
                    std::cout << "\"" << pair.second[i] << "\"";
                } else {
                    std::cout << pair.second[i];
                }
            } catch (const std::exception& e) {
                std::cout << "error";
            }
        }
        std::cout << "]" << std::endl;
    }
}

// Explicit instantiations for print function
template void print_auto_dict_list<std::string, std::any>(const AutoDictList<std::string, std::any>&, const std::string&);
template void print_auto_dict_list<std::string, double>(const AutoDictList<std::string, double>&, const std::string&);
template void print_auto_dict_list<std::string, int>(const AutoDictList<std::string, int>&, const std::string&);

} // namespace auto_dict_utils

// Utility functions for DotDict
namespace dot_dict_utils {

/**
 * Create a DotDict with string keys and any values
 */
DotDict<std::string, std::any> create_string_any_dict() {
    return DotDict<std::string, std::any>();
}

/**
 * Create a DotDict with string keys and double values
 */
DotDict<std::string, double> create_string_double_dict() {
    return DotDict<std::string, double>();
}

/**
 * Convert a DotDict to a string representation
 */
template<typename Key, typename Value>
std::string to_string(const DotDict<Key, Value>& dict) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& pair : dict) {
        if (!first) oss << ", ";
        oss << pair.first << ": ";
        
        try {
            if constexpr (std::is_same_v<Value, std::any>) {
                oss << "any_value";
            } else if constexpr (std::is_same_v<Value, std::string>) {
                oss << "\"" << pair.second << "\"";
            } else {
                oss << pair.second;
            }
        } catch (const std::exception& e) {
            oss << "error";
        }
        
        first = false;
    }
    oss << "}";
    return oss.str();
}

/**
 * Merge two DotDict instances
 */
template<typename Key, typename Value>
DotDict<Key, Value> merge(const DotDict<Key, Value>& dict1, const DotDict<Key, Value>& dict2) {
    DotDict<Key, Value> result = dict1;
    for (const auto& pair : dict2) {
        result[pair.first] = pair.second;
    }
    return result;
}

/**
 * Filter DotDict by keys
 */
template<typename Key, typename Value>
DotDict<Key, Value> filter_by_keys(const DotDict<Key, Value>& dict, const std::vector<Key>& keys) {
    DotDict<Key, Value> result;
    for (const auto& key : keys) {
        if (dict.has_key(key)) {
            result[key] = dict.at(key);
        }
    }
    return result;
}

// Explicit instantiations for utility functions
template std::string to_string<std::string, std::any>(const DotDict<std::string, std::any>&);
template std::string to_string<std::string, double>(const DotDict<std::string, double>&);
template std::string to_string<std::string, std::string>(const DotDict<std::string, std::string>&);

template DotDict<std::string, std::any> merge<std::string, std::any>(const DotDict<std::string, std::any>&, const DotDict<std::string, std::any>&);
template DotDict<std::string, double> merge<std::string, double>(const DotDict<std::string, double>&, const DotDict<std::string, double>&);

template DotDict<std::string, std::any> filter_by_keys<std::string, std::any>(const DotDict<std::string, std::any>&, const std::vector<std::string>&);
template DotDict<std::string, double> filter_by_keys<std::string, double>(const DotDict<std::string, double>&, const std::vector<std::string>&);

} // namespace dot_dict_utils

// Utility functions for AutoOrderedDict
namespace auto_ordered_dict_utils {

/**
 * Create an AutoOrderedDict with default factory for std::any
 */
AutoOrderedDict<std::string, std::any> create_string_any_ordered_dict() {
    return AutoOrderedDict<std::string, std::any>([]() { return std::any{}; });
}

/**
 * Create an AutoOrderedDict with default factory for double
 */
AutoOrderedDict<std::string, double> create_string_double_ordered_dict(double default_value = 0.0) {
    return AutoOrderedDict<std::string, double>([default_value]() { return default_value; });
}

/**
 * Create an AutoOrderedDict with default factory for vector<double>
 */
AutoOrderedDict<std::string, std::vector<double>> create_string_vector_ordered_dict() {
    return AutoOrderedDict<std::string, std::vector<double>>([]() { return std::vector<double>{}; });
}

/**
 * Convert AutoOrderedDict to string representation
 */
template<typename Key, typename Value>
std::string to_string(const AutoOrderedDict<Key, Value>& dict) {
    std::ostringstream oss;
    oss << "OrderedDict([";
    bool first = true;
    for (const auto& pair : dict) {
        if (!first) oss << ", ";
        oss << "(" << pair.first << ", ";
        
        try {
            if constexpr (std::is_same_v<Value, std::any>) {
                oss << "any_value";
            } else if constexpr (std::is_same_v<Value, std::string>) {
                oss << "\"" << pair.second << "\"";
            } else if constexpr (std::is_same_v<Value, std::vector<double>>) {
                oss << "[";
                for (size_t i = 0; i < pair.second.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << pair.second[i];
                }
                oss << "]";
            } else {
                oss << pair.second;
            }
        } catch (const std::exception& e) {
            oss << "error";
        }
        
        oss << ")";
        first = false;
    }
    oss << "])";
    return oss.str();
}

/**
 * Get values as a vector from AutoOrderedDict
 */
template<typename Key, typename Value>
std::vector<Value> get_values_vector(const AutoOrderedDict<Key, Value>& dict) {
    return dict.values();
}

/**
 * Get keys as a vector from AutoOrderedDict
 */
template<typename Key, typename Value>
std::vector<Key> get_keys_vector(const AutoOrderedDict<Key, Value>& dict) {
    return dict.keys();
}

/**
 * Reverse the order of items in AutoOrderedDict
 */
template<typename Key, typename Value>
AutoOrderedDict<Key, Value> reverse(const AutoOrderedDict<Key, Value>& dict) {
    AutoOrderedDict<Key, Value> result;
    auto keys = dict.keys();
    std::reverse(keys.begin(), keys.end());
    
    for (const auto& key : keys) {
        result[key] = dict.at(key);
    }
    
    return result;
}

/**
 * Sort AutoOrderedDict by keys
 */
template<typename Key, typename Value>
AutoOrderedDict<Key, Value> sort_by_keys(const AutoOrderedDict<Key, Value>& dict) {
    AutoOrderedDict<Key, Value> result;
    auto keys = dict.keys();
    std::sort(keys.begin(), keys.end());
    
    for (const auto& key : keys) {
        result[key] = dict.at(key);
    }
    
    return result;
}

// Explicit instantiations for utility functions
template std::string to_string<std::string, std::any>(const AutoOrderedDict<std::string, std::any>&);
template std::string to_string<std::string, double>(const AutoOrderedDict<std::string, double>&);
template std::string to_string<std::string, std::vector<double>>(const AutoOrderedDict<std::string, std::vector<double>>&);

template std::vector<std::any> get_values_vector<std::string, std::any>(const AutoOrderedDict<std::string, std::any>&);
template std::vector<double> get_values_vector<std::string, double>(const AutoOrderedDict<std::string, double>&);

template std::vector<std::string> get_keys_vector<std::string, std::any>(const AutoOrderedDict<std::string, std::any>&);
template std::vector<std::string> get_keys_vector<std::string, double>(const AutoOrderedDict<std::string, double>&);

template AutoOrderedDict<std::string, std::any> reverse<std::string, std::any>(const AutoOrderedDict<std::string, std::any>&);
template AutoOrderedDict<std::string, double> reverse<std::string, double>(const AutoOrderedDict<std::string, double>&);

template AutoOrderedDict<std::string, std::any> sort_by_keys<std::string, std::any>(const AutoOrderedDict<std::string, std::any>&);
template AutoOrderedDict<std::string, double> sort_by_keys<std::string, double>(const AutoOrderedDict<std::string, double>&);

} // namespace auto_ordered_dict_utils

// Global utility functions for creating and working with auto dictionaries

/**
 * Factory function to create different types of auto dictionaries
 */
template<typename ContainerType>
ContainerType create_auto_dict() {
    return ContainerType();
}

// Explicit instantiations for factory function
template AutoDictList<std::string, std::any> create_auto_dict<AutoDictList<std::string, std::any>>();
template AutoDictList<std::string, double> create_auto_dict<AutoDictList<std::string, double>>();
template DotDict<std::string, std::any> create_auto_dict<DotDict<std::string, std::any>>();
template DotDict<std::string, double> create_auto_dict<DotDict<std::string, double>>();
template AutoOrderedDict<std::string, std::any> create_auto_dict<AutoOrderedDict<std::string, std::any>>();
template AutoOrderedDict<std::string, double> create_auto_dict<AutoOrderedDict<std::string, double>>();

/**
 * Helper function to demonstrate usage of auto dictionaries
 */
void demonstrate_auto_dicts() {
    std::cout << "=== AutoDict Demonstration ===" << std::endl;
    
    // AutoDictList demonstration
    std::cout << "\n1. AutoDictList Example:" << std::endl;
    AutoDictList<std::string, double> price_history;
    price_history["AAPL"].push_back(150.0);
    price_history["AAPL"].push_back(152.5);
    price_history["GOOGL"].push_back(2800.0);  // Auto-creates vector
    
    auto_dict_utils::print_auto_dict_list(price_history, "Price History");
    
    // DotDict demonstration
    std::cout << "\n2. DotDict Example:" << std::endl;
    DotDict<std::string, double> config;
    config.set("learning_rate", 0.01)
          .set("batch_size", 32)
          .set("epochs", 100);
    
    std::cout << "Config: " << dot_dict_utils::to_string(config) << std::endl;
    std::cout << "Learning rate: " << config.get("learning_rate", 0.1) << std::endl;
    std::cout << "Momentum (default): " << config.get("momentum", 0.9) << std::endl;
    
    // AutoOrderedDict demonstration
    std::cout << "\n3. AutoOrderedDict Example:" << std::endl;
    auto metrics = auto_ordered_dict_utils::create_string_double_ordered_dict(0.0);
    metrics["accuracy"] = 0.95;
    metrics["precision"] = 0.92;
    metrics["recall"] = 0.88;
    metrics["f1_score"] = 0.90;
    
    std::cout << "Metrics: " << auto_ordered_dict_utils::to_string(metrics) << std::endl;
    
    auto keys = metrics.keys();
    std::cout << "Metric names: [";
    for (size_t i = 0; i < keys.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << keys[i];
    }
    std::cout << "]" << std::endl;
    
    // Demonstrate auto-creation
    std::cout << "\n4. Auto-creation Example:" << std::endl;
    auto time_series = auto_ordered_dict_utils::create_string_vector_ordered_dict();
    time_series["prices"].push_back(100.0);  // Auto-creates vector
    time_series["volumes"].push_back(1000.0); // Auto-creates vector
    
    std::cout << "Time series: " << auto_ordered_dict_utils::to_string(time_series) << std::endl;
    
    std::cout << "\n=== End Demonstration ===" << std::endl;
}

} // namespace utils
} // namespace backtrader