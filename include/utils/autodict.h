#pragma once

#include <map>
#include <vector>
#include <string>
#include <any>
#include <functional>

namespace backtrader {
namespace utils {

/**
 * AutoDictList - Dictionary that auto-creates empty lists for missing keys
 * 
 * This class inherits from map and automatically generates an empty vector
 * when accessing a missing key
 */
template<typename Key = std::string, typename Value = std::any>
class AutoDictList : public std::map<Key, std::vector<Value>> {
public:
    using Base = std::map<Key, std::vector<Value>>;
    
    // Override operator[] to auto-create empty vectors
    std::vector<Value>& operator[](const Key& key) {
        auto it = this->find(key);
        if (it == this->end()) {
            // Create empty vector for missing key
            auto result = this->insert({key, std::vector<Value>{}});
            return result.first->second;
        }
        return it->second;
    }
    
    const std::vector<Value>& at(const Key& key) const {
        return Base::at(key);
    }
};

/**
 * DotDict - Dictionary with dot notation access
 * 
 * This class allows accessing dictionary values using dot notation
 * similar to Python's attribute access
 */
template<typename Key = std::string, typename Value = std::any>
class DotDict : public std::map<Key, Value> {
public:
    using Base = std::map<Key, Value>;
    
    // Get value by key with optional default
    Value get(const Key& key, const Value& default_value = Value{}) const {
        auto it = this->find(key);
        if (it != this->end()) {
            return it->second;
        }
        return default_value;
    }
    
    // Check if key exists
    bool has_key(const Key& key) const {
        return this->find(key) != this->end();
    }
    
    // Set value (chainable)
    DotDict& set(const Key& key, const Value& value) {
        (*this)[key] = value;
        return *this;
    }
};

/**
 * AutoOrderedDict - Auto-creating ordered dictionary
 * 
 * Similar to Python's collections.defaultdict but maintains insertion order
 */
template<typename Key = std::string, typename Value = std::any>
class AutoOrderedDict {
public:
    using Container = std::vector<std::pair<Key, Value>>;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    
    // Constructor with default factory function
    AutoOrderedDict(std::function<Value()> default_factory = []() { return Value{}; })
        : default_factory_(default_factory) {}
    
    // Access operators
    Value& operator[](const Key& key) {
        auto it = find_key(key);
        if (it != data_.end()) {
            return it->second;
        }
        
        // Create new entry with default value
        data_.emplace_back(key, default_factory_());
        return data_.back().second;
    }
    
    const Value& at(const Key& key) const {
        auto it = find_key(key);
        if (it != data_.end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found");
    }
    
    // Check if key exists
    bool contains(const Key& key) const {
        return find_key(key) != data_.end();
    }
    
    // Size and empty checks
    size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    // Iterators
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.begin(); }
    const_iterator cend() const { return data_.end(); }
    
    // Get all keys
    std::vector<Key> keys() const {
        std::vector<Key> result;
        result.reserve(data_.size());
        for (const auto& pair : data_) {
            result.push_back(pair.first);
        }
        return result;
    }
    
    // Get all values
    std::vector<Value> values() const {
        std::vector<Value> result;
        result.reserve(data_.size());
        for (const auto& pair : data_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    // Clear all data
    void clear() {
        data_.clear();
    }
    
    // Erase by key
    bool erase(const Key& key) {
        auto it = find_key(key);
        if (it != data_.end()) {
            data_.erase(it);
            return true;
        }
        return false;
    }
    
private:
    Container data_;
    std::function<Value()> default_factory_;
    
    // Helper to find key
    auto find_key(const Key& key) -> decltype(data_.begin()) {
        return std::find_if(data_.begin(), data_.end(),
            [&key](const auto& pair) { return pair.first == key; });
    }
    
    auto find_key(const Key& key) const -> decltype(data_.begin()) {
        return std::find_if(data_.begin(), data_.end(),
            [&key](const auto& pair) { return pair.first == key; });
    }
};

} // namespace utils
} // namespace backtrader