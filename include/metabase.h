#pragma once

#include <vector>
#include <memory>
#include <typeinfo>
#include <string>
#include <map>

namespace backtrader {

// Forward declarations
template<typename T>
T* findowner(void* owned);

// Template function to find owner of specific type
template<typename T>
T* findowner(void* owned, T* /* type_hint */ = nullptr) {
    // This is a simplified implementation
    // In Python, this uses frame inspection which is not directly available in C++
    // We would need a different approach, possibly using a registry pattern
    return nullptr;
}

// Base metaclass functionality
class MetaBase {
public:
    virtual ~MetaBase() = default;
    
    // Virtual methods that can be overridden by derived metaclasses
    virtual void doprenew() {}
    virtual void donew() {}
    virtual void dopreinit() {}
    virtual void doinit() {}
    virtual void dopostinit() {}
    
    // Object creation pipeline
    template<typename T, typename... Args>
    std::shared_ptr<T> create(Args&&... args) {
        doprenew();
        auto obj = std::make_shared<T>(std::forward<Args>(args)...);
        dopreinit();
        doinit();
        dopostinit();
        return obj;
    }
};

// Auto info class for parameter management
class AutoInfoClass {
public:
    AutoInfoClass() = default;
    virtual ~AutoInfoClass() = default;
    
    // Base methods for parameter handling
    virtual std::map<std::string, std::string> _getpairsbase() const {
        return std::map<std::string, std::string>();
    }
    
    virtual std::map<std::string, std::string> _getpairs() const {
        return std::map<std::string, std::string>();
    }
    
    virtual bool _getrecurse() const {
        return false;
    }
    
    // Derive method for creating new classes with additional info
    virtual std::shared_ptr<AutoInfoClass> _derive(
        const std::string& name,
        const std::map<std::string, std::string>& info,
        const std::vector<std::shared_ptr<AutoInfoClass>>& otherbases = {},
        bool recurse = false) const;
    
    // String representation
    virtual std::string to_string() const;
    
protected:
    std::map<std::string, std::string> info_pairs_;
};

// Item collection class for managing collections of objects
class ItemCollection {
public:
    ItemCollection() = default;
    virtual ~ItemCollection() = default;
    
    // Add item to collection
    void add(const std::string& name, std::shared_ptr<void> item);
    
    // Get item by name
    std::shared_ptr<void> get(const std::string& name) const;
    
    // Check if item exists
    bool has(const std::string& name) const;
    
    // Get all names
    std::vector<std::string> get_names() const;
    
    // Get all items
    std::vector<std::shared_ptr<void>> get_items() const;
    
    // Size
    size_t size() const;
    
    // Clear all items
    void clear();
    
private:
    std::map<std::string, std::shared_ptr<void>> items_;
};

// Parameters management class
template<typename T>
class MetaParams : public MetaBase {
public:
    MetaParams() = default;
    virtual ~MetaParams() = default;
    
    // Parameter management
    virtual void set_param(const std::string& name, const T& value) {
        params_[name] = value;
    }
    
    virtual T get_param(const std::string& name, const T& default_value = T{}) const {
        auto it = params_.find(name);
        if (it != params_.end()) {
            return it->second;
        }
        return default_value;
    }
    
    virtual bool has_param(const std::string& name) const {
        return params_.find(name) != params_.end();
    }
    
    virtual std::map<std::string, T> get_all_params() const {
        return params_;
    }
    
    virtual void clear_params() {
        params_.clear();
    }
    
protected:
    std::map<std::string, T> params_;
};

// Utility functions for finding class hierarchies
template<typename T>
std::vector<const std::type_info*> findbases() {
    std::vector<const std::type_info*> bases;
    // This would need to be implemented using RTTI or other mechanisms
    // C++ doesn't have Python's introspection capabilities built-in
    return bases;
}

// Type registry for runtime type management
class TypeRegistry {
public:
    static TypeRegistry& instance() {
        static TypeRegistry registry;
        return registry;
    }
    
    void register_type(const std::string& name, const std::type_info& type_info) {
        types_[name] = &type_info;
    }
    
    const std::type_info* get_type(const std::string& name) const {
        auto it = types_.find(name);
        if (it != types_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    std::vector<std::string> get_type_names() const {
        std::vector<std::string> names;
        for (const auto& pair : types_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    std::map<std::string, const std::type_info*> types_;
};

// Macro for registering types
#define REGISTER_TYPE(TypeName) \
    namespace { \
        struct TypeName##Registrar { \
            TypeName##Registrar() { \
                TypeRegistry::instance().register_type(#TypeName, typeid(TypeName)); \
            } \
        }; \
        static TypeName##Registrar global_##TypeName##_registrar; \
    }

} // namespace backtrader