#pragma once

#include <map>
#include <functional>
#include <memory>
#include <stdexcept>

namespace backtrader {
namespace utils {

/**
 * OrderedDefaultDict - Ordered dictionary with default factory
 * 
 * A combination of std::map (ordered) with default value generation functionality.
 * When a key is accessed that doesn't exist, the default factory function
 * is called to create a default value.
 * 
 * Template parameters:
 *   - Key: Key type (must be comparable)
 *   - Value: Value type
 */
template<typename Key, typename Value>
class OrderedDefaultDict {
public:
    using DefaultFactory = std::function<Value()>;
    using Container = std::map<Key, Value>;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;

    // Constructors
    OrderedDefaultDict() : default_factory_(nullptr) {}
    
    explicit OrderedDefaultDict(DefaultFactory factory)
        : default_factory_(factory) {}
    
    OrderedDefaultDict(const OrderedDefaultDict& other) = default;
    OrderedDefaultDict(OrderedDefaultDict&& other) = default;
    
    // Assignment operators
    OrderedDefaultDict& operator=(const OrderedDefaultDict& other) = default;
    OrderedDefaultDict& operator=(OrderedDefaultDict&& other) = default;
    
    // Destructor
    virtual ~OrderedDefaultDict() = default;

    // Access operators
    Value& operator[](const Key& key) {
        auto it = container_.find(key);
        if (it != container_.end()) {
            return it->second;
        }
        
        // Key not found - create default value if factory is available
        if (!default_factory_) {
            throw std::out_of_range("Key not found and no default factory set");
        }
        
        auto result = container_.emplace(key, default_factory_());
        return result.first->second;
    }
    
    const Value& operator[](const Key& key) const {
        auto it = container_.find(key);
        if (it != container_.end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found");
    }

    // Access methods
    Value& at(const Key& key) {
        return container_.at(key);
    }
    
    const Value& at(const Key& key) const {
        return container_.at(key);
    }

    // Capacity
    bool empty() const noexcept {
        return container_.empty();
    }
    
    size_type size() const noexcept {
        return container_.size();
    }
    
    size_type max_size() const noexcept {
        return container_.max_size();
    }

    // Modifiers
    void clear() noexcept {
        container_.clear();
    }
    
    std::pair<iterator, bool> insert(const value_type& value) {
        return container_.insert(value);
    }
    
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return container_.emplace(std::forward<Args>(args)...);
    }
    
    iterator erase(const_iterator pos) {
        return container_.erase(pos);
    }
    
    size_type erase(const Key& key) {
        return container_.erase(key);
    }
    
    void swap(OrderedDefaultDict& other) noexcept {
        container_.swap(other.container_);
        std::swap(default_factory_, other.default_factory_);
    }

    // Lookup
    size_type count(const Key& key) const {
        return container_.count(key);
    }
    
    iterator find(const Key& key) {
        return container_.find(key);
    }
    
    const_iterator find(const Key& key) const {
        return container_.find(key);
    }
    
    bool contains(const Key& key) const {
        return container_.find(key) != container_.end();
    }

    // Iterators
    iterator begin() noexcept {
        return container_.begin();
    }
    
    const_iterator begin() const noexcept {
        return container_.begin();
    }
    
    const_iterator cbegin() const noexcept {
        return container_.cbegin();
    }
    
    iterator end() noexcept {
        return container_.end();
    }
    
    const_iterator end() const noexcept {
        return container_.end();
    }
    
    const_iterator cend() const noexcept {
        return container_.cend();
    }

    // Default factory management
    void set_default_factory(DefaultFactory factory) {
        default_factory_ = factory;
    }
    
    DefaultFactory get_default_factory() const {
        return default_factory_;
    }
    
    bool has_default_factory() const {
        return static_cast<bool>(default_factory_);
    }

private:
    Container container_;
    DefaultFactory default_factory_;
};

// Type aliases for common use cases
using StringDefaultDict = OrderedDefaultDict<std::string, std::string>;
using IntDefaultDict = OrderedDefaultDict<int, int>;
using DoubleDefaultDict = OrderedDefaultDict<std::string, double>;

} // namespace utils
} // namespace backtrader