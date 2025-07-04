#include "metabase.h"
#include <sstream>
#include <algorithm>

namespace backtrader {

// AutoInfoClass implementation
std::shared_ptr<AutoInfoClass> AutoInfoClass::_derive(
    const std::string& name,
    const std::map<std::string, std::string>& info,
    const std::vector<std::shared_ptr<AutoInfoClass>>& otherbases,
    bool recurse) const {
    
    auto derived = std::make_shared<AutoInfoClass>();
    
    // Copy current info pairs
    derived->info_pairs_ = info_pairs_;
    
    // Add new info
    for (const auto& pair : info) {
        derived->info_pairs_[pair.first] = pair.second;
    }
    
    // If recursing, merge info from other bases
    if (recurse) {
        for (const auto& base : otherbases) {
            if (base) {
                auto base_pairs = base->_getpairs();
                for (const auto& pair : base_pairs) {
                    if (derived->info_pairs_.find(pair.first) == derived->info_pairs_.end()) {
                        derived->info_pairs_[pair.first] = pair.second;
                    }
                }
            }
        }
    }
    
    return derived;
}

std::string AutoInfoClass::to_string() const {
    std::ostringstream oss;
    oss << "AutoInfoClass[";
    bool first = true;
    for (const auto& pair : info_pairs_) {
        if (!first) oss << ", ";
        oss << pair.first << "=" << pair.second;
        first = false;
    }
    oss << "]";
    return oss.str();
}

// ItemCollection implementation
void ItemCollection::add(const std::string& name, std::shared_ptr<void> item) {
    items_[name] = item;
}

std::shared_ptr<void> ItemCollection::get(const std::string& name) const {
    auto it = items_.find(name);
    if (it != items_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ItemCollection::has(const std::string& name) const {
    return items_.find(name) != items_.end();
}

std::vector<std::string> ItemCollection::get_names() const {
    std::vector<std::string> names;
    for (const auto& pair : items_) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<std::shared_ptr<void>> ItemCollection::get_items() const {
    std::vector<std::shared_ptr<void>> items;
    for (const auto& pair : items_) {
        items.push_back(pair.second);
    }
    return items;
}

size_t ItemCollection::size() const {
    return items_.size();
}

void ItemCollection::clear() {
    items_.clear();
}

} // namespace backtrader