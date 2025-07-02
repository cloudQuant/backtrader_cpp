/**
 * @file MetaClass.h
 * @brief Metaclass system simulation for C++ - emulates Python's metabase.py functionality
 * 
 * This file implements the core metaclass functionality from backtrader's Python codebase,
 * providing parameter management, lifecycle control, and dynamic class creation capabilities
 * that are essential for the backtrader framework.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <type_traits>
#include <functional>
#include <any>
#include <variant>
#include <typeindex>
#include <iostream>
#include <stdexcept>

namespace backtrader {
namespace meta {

// Forward declarations
class AutoInfoClass;
template<typename T> class MetaParams;

/**
 * @brief Parameter value type that can hold various data types
 * 
 * Simulates Python's dynamic typing for parameter values
 */
using ParameterValue = std::variant<
    bool, int, long, float, double, std::string,
    std::vector<int>, std::vector<double>, std::vector<std::string>
>;

/**
 * @brief Parameter definition with name, default value, and type information
 */
struct ParameterDef {
    std::string name;
    ParameterValue default_value;
    std::type_index type_info;
    std::string description;
    bool is_required;

    // Default constructor
    ParameterDef() : name(""), default_value(0), type_info(std::type_index(typeid(int))), 
                     description(""), is_required(false) {}

    template<typename T>
    ParameterDef(const std::string& n, const T& def_val, const std::string& desc = "", bool required = false)
        : name(n), default_value(def_val), type_info(std::type_index(typeid(T))), 
          description(desc), is_required(required) {}
};

/**
 * @brief Ordered parameter collection that maintains insertion order
 * 
 * Equivalent to Python's OrderedDict for parameters
 */
class ParameterCollection {
private:
    std::vector<std::string> names_;
    std::unordered_map<std::string, ParameterValue> values_;
    std::unordered_map<std::string, ParameterDef> definitions_;

public:
    ParameterCollection() = default;
    ParameterCollection(const ParameterCollection& other) = default;
    ParameterCollection& operator=(const ParameterCollection& other) = default;

    // Add parameter definition
    void addParameter(const ParameterDef& param_def);
    
    // Set parameter value
    template<typename T>
    void setValue(const std::string& name, const T& value);
    
    // Get parameter value
    template<typename T>
    T getValue(const std::string& name) const;
    
    // Get parameter value with default
    template<typename T>
    T getValue(const std::string& name, const T& default_val) const;
    
    // Get raw parameter value
    ParameterValue getRawValue(const std::string& name) const;
    
    // Check if parameter exists
    bool hasParameter(const std::string& name) const;
    
    // Check if value is default
    bool isDefault(const std::string& name) const;
    
    // Get all parameter names in order
    const std::vector<std::string>& getNames() const { return names_; }
    
    // Get parameter count
    size_t size() const { return names_.size(); }
    
    // Copy parameters from another collection
    void update(const ParameterCollection& other);
    
    // Clear all parameters
    void clear();
    
    // Get parameter definition
    const ParameterDef& getDefinition(const std::string& name) const;
    
    // Validate all parameters
    bool validate() const;
    
    // Print parameters for debugging
    void print() const;
};

/**
 * @brief AutoInfoClass equivalent - base class for parameter management
 * 
 * Provides the core functionality for parameter handling, similar to Python's AutoInfoClass
 */
class AutoInfoClass {
protected:
    ParameterCollection params_;
    bool recurse_;

public:
    AutoInfoClass(bool recurse = false);
    virtual ~AutoInfoClass() = default;

    // Parameter access methods
    template<typename T>
    T getParam(const std::string& name) const;
    
    template<typename T>
    T getParam(const std::string& name, const T& default_val) const;
    
    template<typename T>
    void setParam(const std::string& name, const T& value);
    
    // Parameter state methods
    bool isDefault(const std::string& name) const;
    bool notDefault(const std::string& name) const;
    
    // Derived class creation (simulates _derive method)
    static std::shared_ptr<AutoInfoClass> derive(
        const std::string& name,
        const ParameterCollection& new_params,
        const std::vector<std::shared_ptr<AutoInfoClass>>& other_bases,
        bool recurse = false
    );
    
    // Get parameter collections
    virtual ParameterCollection getParams() const;
    virtual ParameterCollection getParamsBase() const;
    virtual bool getRecurse() const { return recurse_; }
    
    // Parameter names and values
    std::vector<std::string> getKeys() const;
    std::vector<ParameterValue> getValues() const;
    std::vector<std::pair<std::string, ParameterValue>> getItems() const;
    
    // Validation
    virtual bool validateParams() const;
};

/**
 * @brief Lifecycle stages for metaclass simulation
 * 
 * Corresponds to the 5-stage initialization process in Python's MetaBase
 */
enum class LifecycleStage {
    PreNew,     // doprenew
    New,        // donew  
    PreInit,    // dopreinit
    Init,       // doinit
    PostInit    // dopostinit
};

/**
 * @brief Base metaclass functionality
 * 
 * Simulates Python's MetaBase class with 5-stage lifecycle management
 */
class MetaBase {
public:
    virtual ~MetaBase() = default;

    // Lifecycle stage handlers
    virtual void doPreNew() {}
    virtual void doNew() {}
    virtual void doPreInit() {}
    virtual void doInit() {}
    virtual void doPostInit() {}
    
    // Main lifecycle execution (equivalent to __call__)
    void executeLifecycle();
    
    // Stage notification
    virtual void onStageComplete(LifecycleStage /*stage*/) {}
};

/**
 * @brief Package import simulation
 * 
 * Simulates Python's dynamic package importing
 */
struct PackageInfo {
    std::string package_name;
    std::string alias;
    std::vector<std::string> from_imports;
    
    PackageInfo(const std::string& name, const std::string& alias_name = "")
        : package_name(name), alias(alias_name.empty() ? name : alias_name) {}
    
    PackageInfo(const std::string& name, const std::vector<std::string>& imports)
        : package_name(name), alias(name), from_imports(imports) {}
};

/**
 * @brief MetaParams equivalent - parameter metaclass
 * 
 * Provides the core parameter management functionality similar to Python's MetaParams
 */
template<typename Derived>
class MetaParams : public MetaBase, public AutoInfoClass {
private:
    std::vector<PackageInfo> packages_;
    std::vector<PackageInfo> from_packages_;
    
    // Static registry for derived classes
    static std::unordered_map<std::string, std::function<std::unique_ptr<Derived>()>>& getRegistry() {
        static std::unordered_map<std::string, std::function<std::unique_ptr<Derived>()>> registry;
        return registry;
    }

protected:
    // Parameter initialization
    virtual void initializeParams() {}
    
    // Package management
    void addPackage(const PackageInfo& package);
    void addFromPackage(const PackageInfo& package);
    
    // Parameter derivation
    void deriveParams(const std::string& class_name,
                     const ParameterCollection& new_params,
                     const std::vector<std::shared_ptr<AutoInfoClass>>& base_params);

public:
    MetaParams();
    virtual ~MetaParams() = default;

    // Initialize parameters after construction (to handle virtual dispatch properly)
    void initialize() {
        initializeParams();
        executeLifecycle();
    }

    // Lifecycle overrides
    void doPreNew() override;
    void doNew() override;
    void doPreInit() override;
    void doInit() override;
    void doPostInit() override;
    
    // Factory method registration
    template<typename ConcreteClass>
    static void registerClass(const std::string& name) {
        getRegistry()[name] = []() -> std::unique_ptr<Derived> {
            return std::make_unique<ConcreteClass>();
        };
    }
    
    // Factory method creation
    static std::unique_ptr<Derived> create(const std::string& name);
    
    // Package access
    const std::vector<PackageInfo>& getPackages() const { return packages_; }
    const std::vector<PackageInfo>& getFromPackages() const { return from_packages_; }
    
    // Parameter shortcuts (equivalent to Python's p attribute)
    template<typename T>
    T p(const std::string& name) const { return getParam<T>(name); }
    
    template<typename T>
    void setP(const std::string& name, const T& value) { setParam(name, value); }
};

/**
 * @brief ParamsBase equivalent - base class for parameterized objects
 * 
 * Provides a convenient base class for objects that need parameter management
 */
class ParamsBase : public MetaParams<ParamsBase> {
public:
    ParamsBase() = default;
    virtual ~ParamsBase() = default;
};

/**
 * @brief ItemCollection equivalent - named item collection
 * 
 * Provides index and name-based access to collections
 */
template<typename T>
class ItemCollection {
private:
    std::vector<T> items_;
    std::vector<std::string> names_;
    std::unordered_map<std::string, size_t> name_to_index_;

public:
    ItemCollection() = default;
    
    // Add item with optional name
    void append(const T& item, const std::string& name = "");
    
    // Access by index
    const T& operator[](size_t index) const;
    T& operator[](size_t index);
    
    // Access by name
    const T& getByName(const std::string& name) const;
    T& getByName(const std::string& name);
    
    // Size
    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    
    // Names
    const std::vector<std::string>& getNames() const { return names_; }
    
    // Items
    const std::vector<T>& getItems() const { return items_; }
    
    // Iterator support
    typename std::vector<T>::iterator begin() { return items_.begin(); }
    typename std::vector<T>::iterator end() { return items_.end(); }
    typename std::vector<T>::const_iterator begin() const { return items_.begin(); }
    typename std::vector<T>::const_iterator end() const { return items_.end(); }
    
    // Clear
    void clear();
};

/**
 * @brief Helper macros for parameter definition
 */
#define DEFINE_PARAMETER(type, name, default_value, description) \
    params_.addParameter(backtrader::meta::ParameterDef(#name, static_cast<type>(default_value), description))

#define DEFINE_REQUIRED_PARAMETER(type, name, description) \
    params_.addParameter(backtrader::meta::ParameterDef(#name, type{}, description, true))

#define GET_PARAM(type, name) getParam<type>(#name)
#define SET_PARAM(type, name, value) setParam<type>(#name, value)

// Template implementations
template<typename T>
void ParameterCollection::setValue(const std::string& name, const T& value) {
    if (definitions_.find(name) == definitions_.end()) {
        throw std::runtime_error("Parameter '" + name + "' not defined");
    }
    values_[name] = value;
}

template<typename T>
T ParameterCollection::getValue(const std::string& name) const {
    auto it = values_.find(name);
    if (it != values_.end()) {
        try {
            return std::get<T>(it->second);
        } catch (const std::bad_variant_access& e) {
            throw std::runtime_error("Type mismatch for parameter '" + name + "'");
        }
    }
    
    // Return default value
    auto def_it = definitions_.find(name);
    if (def_it != definitions_.end()) {
        try {
            return std::get<T>(def_it->second.default_value);
        } catch (const std::bad_variant_access& e) {
            throw std::runtime_error("Type mismatch for default value of parameter '" + name + "'");
        }
    }
    
    throw std::runtime_error("Parameter '" + name + "' not found");
}

template<typename T>
T ParameterCollection::getValue(const std::string& name, const T& default_val) const {
    try {
        return getValue<T>(name);
    } catch (const std::runtime_error&) {
        return default_val;
    }
}

template<typename T>
T AutoInfoClass::getParam(const std::string& name) const {
    return params_.getValue<T>(name);
}

template<typename T>
T AutoInfoClass::getParam(const std::string& name, const T& default_val) const {
    return params_.getValue<T>(name, default_val);
}

template<typename T>
void AutoInfoClass::setParam(const std::string& name, const T& value) {
    params_.setValue<T>(name, value);
}

template<typename T>
void ItemCollection<T>::append(const T& item, const std::string& name) {
    items_.push_back(item);
    if (!name.empty()) {
        names_.push_back(name);
        name_to_index_[name] = items_.size() - 1;
    }
}

template<typename T>
const T& ItemCollection<T>::operator[](size_t index) const {
    if (index >= items_.size()) {
        throw std::out_of_range("Index out of range");
    }
    return items_[index];
}

template<typename T>
T& ItemCollection<T>::operator[](size_t index) {
    if (index >= items_.size()) {
        throw std::out_of_range("Index out of range");
    }
    return items_[index];
}

template<typename T>
const T& ItemCollection<T>::getByName(const std::string& name) const {
    auto it = name_to_index_.find(name);
    if (it == name_to_index_.end()) {
        throw std::runtime_error("Item with name '" + name + "' not found");
    }
    return items_[it->second];
}

template<typename T>
T& ItemCollection<T>::getByName(const std::string& name) {
    auto it = name_to_index_.find(name);
    if (it == name_to_index_.end()) {
        throw std::runtime_error("Item with name '" + name + "' not found");
    }
    return items_[it->second];
}

template<typename T>
void ItemCollection<T>::clear() {
    items_.clear();
    names_.clear();
    name_to_index_.clear();
}

// MetaParams template implementations
template<typename Derived>
MetaParams<Derived>::MetaParams() : AutoInfoClass(false) {
    // Note: Cannot call virtual initializeParams() in constructor
    // Parameters must be initialized after construction is complete
}

template<typename Derived>
void MetaParams<Derived>::doPreNew() {
    // Stage 1: Pre-creation setup
    MetaBase::doPreNew();
}

template<typename Derived>
void MetaParams<Derived>::doNew() {
    // Stage 2: Object creation
    MetaBase::doNew();
}

template<typename Derived>
void MetaParams<Derived>::doPreInit() {
    // Stage 3: Pre-initialization
    MetaBase::doPreInit();
    // Initialize parameters here when virtual dispatch works properly
    initializeParams();
}

template<typename Derived>
void MetaParams<Derived>::doInit() {
    // Stage 4: Initialization
    MetaBase::doInit();
}

template<typename Derived>
void MetaParams<Derived>::doPostInit() {
    // Stage 5: Post-initialization
    MetaBase::doPostInit();
}

template<typename Derived>
void MetaParams<Derived>::addPackage(const PackageInfo& package) {
    packages_.push_back(package);
}

template<typename Derived>
void MetaParams<Derived>::addFromPackage(const PackageInfo& package) {
    from_packages_.push_back(package);
}

template<typename Derived>
void MetaParams<Derived>::deriveParams(const std::string& class_name,
                                      const ParameterCollection& new_params,
                                      const std::vector<std::shared_ptr<AutoInfoClass>>& base_params) {
    // Merge parameters from base classes
    for (const auto& base : base_params) {
        if (base) {
            params_.update(base->getParams());
        }
    }
    
    // Add new parameters
    params_.update(new_params);
}

template<typename Derived>
std::unique_ptr<Derived> MetaParams<Derived>::create(const std::string& name) {
    auto& registry = getRegistry();
    auto it = registry.find(name);
    if (it != registry.end()) {
        return it->second();
    }
    throw std::runtime_error("Class '" + name + "' not registered");
}

} // namespace meta
} // namespace backtrader