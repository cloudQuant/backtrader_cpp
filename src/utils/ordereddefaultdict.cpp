#include "../../include/utils/ordereddefaultdict.h"
#include <stdexcept>

namespace backtrader {
namespace utils {

// OrderedDefaultDict<T> implementation
template<typename T>
OrderedDefaultDict<T>::OrderedDefaultDict() : default_factory_(nullptr) {}

template<typename T>
OrderedDefaultDict<T>::OrderedDefaultDict(std::function<T()> factory) 
    : default_factory_(std::move(factory)) {}

template<typename T>
T& OrderedDefaultDict<T>::operator[](const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        // Key doesn't exist, create with default value
        T default_value = default_factory_ ? default_factory_() : T{};
        auto result = data_.emplace(key, default_value);
        
        // Add to insertion order if it's a new key
        if (result.second) {
            insertion_order_.push_back(key);
        }
        
        return result.first->second;
    }
    return it->second;
}

template<typename T>
const T& OrderedDefaultDict<T>::operator[](const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        throw std::out_of_range("Key not found: " + key);
    }
    return it->second;
}

template<typename T>
T& OrderedDefaultDict<T>::at(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        throw std::out_of_range("Key not found: " + key);
    }
    return it->second;
}

template<typename T>
const T& OrderedDefaultDict<T>::at(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        throw std::out_of_range("Key not found: " + key);
    }
    return it->second;
}

template<typename T>
bool OrderedDefaultDict<T>::contains(const std::string& key) const {
    return data_.find(key) != data_.end();
}

template<typename T>
size_t OrderedDefaultDict<T>::size() const {
    return data_.size();
}

template<typename T>
bool OrderedDefaultDict<T>::empty() const {
    return data_.empty();
}

template<typename T>
void OrderedDefaultDict<T>::clear() {
    data_.clear();
    insertion_order_.clear();
}

template<typename T>
bool OrderedDefaultDict<T>::erase(const std::string& key) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        data_.erase(it);
        
        // Remove from insertion order
        auto order_it = std::find(insertion_order_.begin(), insertion_order_.end(), key);
        if (order_it != insertion_order_.end()) {
            insertion_order_.erase(order_it);
        }
        
        return true;
    }
    return false;
}

template<typename T>
void OrderedDefaultDict<T>::insert(const std::string& key, const T& value) {
    auto result = data_.emplace(key, value);
    
    // Add to insertion order if it's a new key
    if (result.second) {
        insertion_order_.push_back(key);
    } else {
        // Update existing value
        result.first->second = value;
    }
}

template<typename T>
std::vector<std::string> OrderedDefaultDict<T>::keys() const {
    return insertion_order_;
}

template<typename T>
std::vector<T> OrderedDefaultDict<T>::values() const {
    std::vector<T> result;
    result.reserve(insertion_order_.size());
    
    for (const auto& key : insertion_order_) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

template<typename T>
std::vector<std::pair<std::string, T>> OrderedDefaultDict<T>::items() const {
    std::vector<std::pair<std::string, T>> result;
    result.reserve(insertion_order_.size());
    
    for (const auto& key : insertion_order_) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            result.emplace_back(key, it->second);
        }
    }
    
    return result;
}

template<typename T>
void OrderedDefaultDict<T>::set_default_factory(std::function<T()> factory) {
    default_factory_ = std::move(factory);
}

template<typename T>
T OrderedDefaultDict<T>::get(const std::string& key, const T& default_value) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return default_value;
}

template<typename T>
void OrderedDefaultDict<T>::update(const std::map<std::string, T>& other) {
    for (const auto& pair : other) {
        insert(pair.first, pair.second);
    }
}

template<typename T>
void OrderedDefaultDict<T>::update(const OrderedDefaultDict<T>& other) {
    for (const auto& key : other.keys()) {
        insert(key, other[key]);
    }
}

template<typename T>
typename OrderedDefaultDict<T>::iterator OrderedDefaultDict<T>::begin() {
    return data_.begin();
}

template<typename T>
typename OrderedDefaultDict<T>::iterator OrderedDefaultDict<T>::end() {
    return data_.end();
}

template<typename T>
typename OrderedDefaultDict<T>::const_iterator OrderedDefaultDict<T>::begin() const {
    return data_.begin();
}

template<typename T>
typename OrderedDefaultDict<T>::const_iterator OrderedDefaultDict<T>::end() const {
    return data_.end();
}

template<typename T>
typename OrderedDefaultDict<T>::const_iterator OrderedDefaultDict<T>::cbegin() const {
    return data_.cbegin();
}

template<typename T>
typename OrderedDefaultDict<T>::const_iterator OrderedDefaultDict<T>::cend() const {
    return data_.cend();
}

// Explicit template instantiations for common types
template class OrderedDefaultDict<int>;
template class OrderedDefaultDict<double>;
template class OrderedDefaultDict<std::string>;
template class OrderedDefaultDict<std::vector<int>>;
template class OrderedDefaultDict<std::vector<double>>;
template class OrderedDefaultDict<std::map<std::string, std::string>>;

} // namespace utils
} // namespace backtrader