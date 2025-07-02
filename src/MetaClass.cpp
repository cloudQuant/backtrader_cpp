/**
 * @file MetaClass.cpp
 * @brief Implementation of metaclass system simulation for C++
 */

#include "MetaClass.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace backtrader {
namespace meta {

// ParameterCollection implementation
void ParameterCollection::addParameter(const ParameterDef& param_def) {
    if (definitions_.find(param_def.name) == definitions_.end()) {
        names_.push_back(param_def.name);
    }
    definitions_[param_def.name] = param_def;
    values_[param_def.name] = param_def.default_value;
}

bool ParameterCollection::hasParameter(const std::string& name) const {
    return definitions_.find(name) != definitions_.end();
}

bool ParameterCollection::isDefault(const std::string& name) const {
    auto value_it = values_.find(name);
    auto def_it = definitions_.find(name);
    
    if (value_it == values_.end() || def_it == definitions_.end()) {
        return false;
    }
    
    return value_it->second == def_it->second.default_value;
}

void ParameterCollection::update(const ParameterCollection& other) {
    // Add new parameter definitions
    for (const auto& name : other.names_) {
        if (definitions_.find(name) == definitions_.end()) {
            names_.push_back(name);
        }
        definitions_[name] = other.definitions_.at(name);
    }
    
    // Update values
    for (const auto& pair : other.values_) {
        values_[pair.first] = pair.second;
    }
}

void ParameterCollection::clear() {
    names_.clear();
    values_.clear();
    definitions_.clear();
}

const ParameterDef& ParameterCollection::getDefinition(const std::string& name) const {
    auto it = definitions_.find(name);
    if (it == definitions_.end()) {
        throw std::runtime_error("Parameter definition for '" + name + "' not found");
    }
    return it->second;
}

bool ParameterCollection::validate() const {
    for (const auto& name : names_) {
        const auto& def = definitions_.at(name);
        if (def.is_required) {
            auto value_it = values_.find(name);
            if (value_it == values_.end() || value_it->second == def.default_value) {
                return false;
            }
        }
    }
    return true;
}

void ParameterCollection::print() const {
    auto printValue = [](const ParameterValue& value) {
        std::visit([](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::vector<int>>) {
                std::cout << "[";
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << v[i];
                }
                std::cout << "]";
            } else if constexpr (std::is_same_v<T, std::vector<double>>) {
                std::cout << "[";
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << v[i];
                }
                std::cout << "]";
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                std::cout << "[";
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << "\"" << v[i] << "\"";
                }
                std::cout << "]";
            } else {
                std::cout << v;
            }
        }, value);
    };

    std::cout << "Parameters (" << names_.size() << "):" << std::endl;
    for (const auto& name : names_) {
        const auto& def = definitions_.at(name);
        auto value_it = values_.find(name);
        
        std::cout << "  " << name << ": ";
        
        if (value_it != values_.end()) {
            // Print current value
            printValue(value_it->second);
        }
        
        std::cout << " (default: ";
        printValue(def.default_value);
        std::cout << ")";
        
        if (def.is_required) {
            std::cout << " [REQUIRED]";
        }
        
        if (!def.description.empty()) {
            std::cout << " - " << def.description;
        }
        
        std::cout << std::endl;
    }
}

// AutoInfoClass implementation
AutoInfoClass::AutoInfoClass(bool recurse) : recurse_(recurse) {
    // Initialize with empty parameter collection
}

bool AutoInfoClass::isDefault(const std::string& name) const {
    return params_.isDefault(name);
}

bool AutoInfoClass::notDefault(const std::string& name) const {
    return !params_.isDefault(name);
}

std::shared_ptr<AutoInfoClass> AutoInfoClass::derive(
    const std::string& /*name*/,
    const ParameterCollection& new_params,
    const std::vector<std::shared_ptr<AutoInfoClass>>& other_bases,
    bool recurse) {
    
    auto derived = std::make_shared<AutoInfoClass>(recurse);
    
    // Start with current parameters
    derived->params_ = new_params;
    
    // Merge parameters from other bases
    for (const auto& base : other_bases) {
        if (base) {
            derived->params_.update(base->getParams());
        }
    }
    
    return derived;
}

ParameterCollection AutoInfoClass::getParams() const {
    return params_;
}

ParameterCollection AutoInfoClass::getParamsBase() const {
    // Return a copy of the base parameters
    return params_;
}

std::vector<std::string> AutoInfoClass::getKeys() const {
    return params_.getNames();
}

ParameterValue ParameterCollection::getRawValue(const std::string& name) const {
    auto it = values_.find(name);
    if (it != values_.end()) {
        return it->second;
    }
    
    // Return default value
    auto def_it = definitions_.find(name);
    if (def_it != definitions_.end()) {
        return def_it->second.default_value;
    }
    
    throw std::runtime_error("Parameter '" + name + "' not found");
}

std::vector<ParameterValue> AutoInfoClass::getValues() const {
    std::vector<ParameterValue> values;
    for (const auto& name : params_.getNames()) {
        try {
            values.push_back(params_.getRawValue(name));
        } catch (const std::exception&) {
            // If we can't get the value, skip it
            continue;
        }
    }
    return values;
}

std::vector<std::pair<std::string, ParameterValue>> AutoInfoClass::getItems() const {
    std::vector<std::pair<std::string, ParameterValue>> items;
    for (const auto& name : params_.getNames()) {
        try {
            items.emplace_back(name, params_.getRawValue(name));
        } catch (const std::exception&) {
            // If we can't get the value, skip it
            continue;
        }
    }
    return items;
}

bool AutoInfoClass::validateParams() const {
    return params_.validate();
}

// MetaBase implementation
void MetaBase::executeLifecycle() {
    // Execute the 5-stage lifecycle
    doPreNew();
    onStageComplete(LifecycleStage::PreNew);
    
    doNew();
    onStageComplete(LifecycleStage::New);
    
    doPreInit();
    onStageComplete(LifecycleStage::PreInit);
    
    doInit();
    onStageComplete(LifecycleStage::Init);
    
    doPostInit();
    onStageComplete(LifecycleStage::PostInit);
}

} // namespace meta
} // namespace backtrader