/**
 * @file LineSeries.cpp
 * @brief Implementation of LineSeries class
 */

#include "LineSeries.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace backtrader {
namespace line {

// ========== Constructors ==========

LineSeries::LineSeries()
    : minimum_period_(1)
    , auto_sync_(true)
    , enable_bulk_cache_(true) {
}

LineSeries::LineSeries(const std::vector<LineSpec>& specs)
    : LineSeries() {
    for (const auto& spec : specs) {
        addLine(spec);
    }
}

LineSeries::LineSeries(const std::string& name, const std::vector<LineSpec>& specs)
    : LineSeries(specs) {
    series_name_ = name;
}

// ========== Protected Methods ==========

void LineSeries::initializeParams() {
    DEFINE_PARAMETER(int, minperiod, 1, "Minimum period for valid data");
    DEFINE_PARAMETER(bool, autosync, true, "Automatically synchronize lines");
    DEFINE_PARAMETER(bool, bulk_cache, true, "Enable bulk operation caching");
}

// ========== Line Management ==========

int LineSeries::addLine(const LineSpec& spec) {
    // Check if line already exists
    if (hasLine(spec.name)) {
        throw std::runtime_error("Line already exists: " + spec.name);
    }
    
    // Create new LineBuffer
    auto line_buffer = std::make_shared<LineBuffer>();
    
    // Determine index
    int index = spec.index;
    if (index < 0 || index >= static_cast<int>(lines_.size())) {
        index = static_cast<int>(lines_.size());
    }
    
    // Insert at specified position
    if (index >= static_cast<int>(lines_.size())) {
        lines_.resize(index + 1);
        line_specs_.resize(index + 1);
    }
    
    lines_[index] = line_buffer;
    line_specs_[index] = spec;
    line_specs_[index].index = index; // Ensure index is correct
    
    // Update name mappings
    name_to_index_[spec.name] = index;
    if (!spec.alias.empty() && spec.alias != spec.name) {
        alias_to_index_[spec.alias] = index;
    }
    
    return index;
}

int LineSeries::addLine(const std::string& name) {
    LineSpec spec(name, static_cast<int>(lines_.size()));
    return addLine(spec);
}

void LineSeries::removeLine(const std::string& name) {
    int index = getLineIndex(name);
    if (index >= 0) {
        removeLine(index);
    }
}

void LineSeries::removeLine(int index) {
    if (index < 0 || index >= static_cast<int>(lines_.size())) {
        return;
    }
    
    // Remove from name mappings
    if (index < static_cast<int>(line_specs_.size())) {
        const auto& spec = line_specs_[index];
        name_to_index_.erase(spec.name);
        if (!spec.alias.empty() && spec.alias != spec.name) {
            alias_to_index_.erase(spec.alias);
        }
    }
    
    // Remove line
    lines_.erase(lines_.begin() + index);
    if (index < static_cast<int>(line_specs_.size())) {
        line_specs_.erase(line_specs_.begin() + index);
    }
    
    // Update indices in mappings
    for (auto& pair : name_to_index_) {
        if (pair.second > index) {
            pair.second--;
        }
    }
    for (auto& pair : alias_to_index_) {
        if (pair.second > index) {
            pair.second--;
        }
    }
    
    // Update line specs indices
    for (size_t i = index; i < line_specs_.size(); ++i) {
        line_specs_[i].index = static_cast<int>(i);
    }
}

std::vector<std::string> LineSeries::getLineNames() const {
    std::vector<std::string> names;
    names.reserve(line_specs_.size());
    
    for (const auto& spec : line_specs_) {
        if (!spec.name.empty()) {
            names.push_back(spec.name);
        }
    }
    
    return names;
}

// ========== Data Access ==========

std::shared_ptr<LineBuffer> LineSeries::getLine(const std::string& name) const {
    int index = getLineIndex(name);
    if (index >= 0) {
        return getLine(index);
    }
    return nullptr;
}

std::shared_ptr<LineBuffer> LineSeries::getLine(int index) const {
    if (index >= 0 && index < static_cast<int>(lines_.size())) {
        return lines_[index];
    }
    return nullptr;
}

void LineSeries::set(const std::string& line_name, double value, int ago) {
    auto line = getLine(line_name);
    if (line) {
        line->set(value, ago);
        if (auto_sync_) {
            sync_all_lines();
        }
    } else {
        throw std::runtime_error("Line not found: " + line_name);
    }
}

void LineSeries::set(int line_index, double value, int ago) {
    auto line = getLine(line_index);
    if (line) {
        line->set(value, ago);
        if (auto_sync_) {
            sync_all_lines();
        }
    } else {
        throw std::out_of_range("Line index out of range: " + std::to_string(line_index));
    }
}

// ========== Bulk Operations ==========

void LineSeries::addData(const std::unordered_map<std::string, double>& data) {
    // First, advance all lines
    for (auto& line : lines_) {
        if (line) {
            line->forward(NAN_VALUE);
        }
    }
    
    // Then set the provided values
    for (const auto& pair : data) {
        try {
            set(pair.first, pair.second, 0);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to set " << pair.first << ": " << e.what() << std::endl;
        }
    }
}

void LineSeries::addOHLCV(double o, double h, double l, double c, double v) {
    std::unordered_map<std::string, double> data = {
        {"open", o},
        {"high", h},
        {"low", l},
        {"close", c}
    };
    
    if (v > 0.0) {
        data["volume"] = v;
    }
    
    addData(data);
}

std::vector<double> LineSeries::getValues(const std::string& line_name, int periods, int start_ago) const {
    auto line = getLine(line_name);
    if (!line) {
        throw std::runtime_error("Line not found: " + line_name);
    }
    
    return line->getRange(start_ago, periods);
}

std::unordered_map<std::string, double> LineSeries::getCurrentValues() const {
    std::unordered_map<std::string, double> values;
    
    for (const auto& spec : line_specs_) {
        if (!spec.name.empty() && spec.index < static_cast<int>(lines_.size())) {
            auto line = lines_[spec.index];
            if (line) {
                try {
                    values[spec.name] = (*line)[0];
                } catch (const std::exception&) {
                    values[spec.name] = NAN_VALUE;
                }
            }
        }
    }
    
    return values;
}

std::vector<std::unordered_map<std::string, double>> LineSeries::getBulkValues(int periods) const {
    std::vector<std::unordered_map<std::string, double>> result;
    result.reserve(periods);
    
    for (int i = -(periods - 1); i <= 0; ++i) {
        std::unordered_map<std::string, double> period_data;
        
        for (const auto& spec : line_specs_) {
            if (!spec.name.empty() && spec.index < static_cast<int>(lines_.size())) {
                auto line = lines_[spec.index];
                if (line) {
                    try {
                        period_data[spec.name] = (*line)[i];
                    } catch (const std::exception&) {
                        period_data[spec.name] = NAN_VALUE;
                    }
                }
            }
        }
        
        result.push_back(std::move(period_data));
    }
    
    return result;
}

// ========== Navigation ==========

void LineSeries::forward(const std::unordered_map<std::string, double>& data) {
    addData(data);
}

void LineSeries::forward() {
    for (auto& line : lines_) {
        if (line) {
            line->forward(NAN_VALUE);
        }
    }
}

void LineSeries::backward(int periods) {
    for (auto& line : lines_) {
        if (line) {
            line->backwards(periods);
        }
    }
}

void LineSeries::home() {
    for (auto& line : lines_) {
        if (line) {
            line->home();
        }
    }
}

void LineSeries::rewind(int periods) {
    for (auto& line : lines_) {
        if (line) {
            line->rewind(periods);
        }
    }
}

// ========== State Information ==========

int LineSeries::size() const {
    if (lines_.empty()) {
        return 0;
    }
    
    int min_size = std::numeric_limits<int>::max();
    bool has_data = false;
    
    for (const auto& line : lines_) {
        if (line && !line->empty()) {
            min_size = std::min(min_size, line->size());
            has_data = true;
        }
    }
    
    return has_data ? min_size : 0;
}

bool LineSeries::empty() const {
    return size() == 0;
}

int LineSeries::getCurrentBar() const {
    if (lines_.empty()) {
        return -1;
    }
    
    for (const auto& line : lines_) {
        if (line) {
            return line->getIdx() + 1; // Convert from 0-based to 1-based
        }
    }
    
    return -1;
}

bool LineSeries::isReady() const {
    return size() >= minimum_period_;
}

// ========== Iterator Support ==========

std::unordered_map<std::string, double> LineSeries::Iterator::operator*() const {
    std::unordered_map<std::string, double> values;
    
    for (const auto& spec : series_->line_specs_) {
        if (!spec.name.empty() && spec.index < static_cast<int>(series_->lines_.size())) {
            auto line = series_->lines_[spec.index];
            if (line) {
                try {
                    values[spec.name] = (*line)[current_ago_];
                } catch (const std::exception&) {
                    values[spec.name] = NAN_VALUE;
                }
            }
        }
    }
    
    return values;
}

// ========== C++20 Features ==========

std::optional<std::vector<double>> LineSeries::getDataSpan(const std::string& line_name, int periods) const {
    // C++17 compatible implementation - return vector instead of span
    auto line = getLine(line_name);
    if (!line || periods <= 0) {
        return std::nullopt;
    }
    
    std::vector<double> result;
    result.reserve(periods);
    
    for (int i = periods - 1; i >= 0; --i) {
        if (line->size() > i) {
            result.push_back(line->get(-i));
        }
    }
    
    return result;
}

// ========== Utility Methods ==========

void LineSeries::print() const {
    std::cout << "LineSeries: " << series_name_ << std::endl;
    std::cout << "  Lines: " << lines_.size() << std::endl;
    std::cout << "  Size: " << size() << std::endl;
    std::cout << "  Current Bar: " << getCurrentBar() << std::endl;
    std::cout << "  Min Period: " << minimum_period_ << std::endl;
    std::cout << "  Ready: " << (isReady() ? "Yes" : "No") << std::endl;
    std::cout << "  Auto Sync: " << (auto_sync_ ? "Yes" : "No") << std::endl;
    
    std::cout << "  Line Specifications:" << std::endl;
    for (const auto& spec : line_specs_) {
        if (!spec.name.empty()) {
            std::cout << "    " << spec.index << ": " << spec.name;
            if (!spec.alias.empty() && spec.alias != spec.name) {
                std::cout << " (alias: " << spec.alias << ")";
            }
            if (spec.is_required) {
                std::cout << " [REQUIRED]";
            }
            std::cout << std::endl;
        }
    }
    
    if (!empty()) {
        std::cout << "  Current Values:" << std::endl;
        auto current = getCurrentValues();
        for (const auto& pair : current) {
            std::cout << "    " << pair.first << ": " 
                      << std::fixed << std::setprecision(4) << pair.second << std::endl;
        }
    }
}

void LineSeries::clear() {
    for (auto& line : lines_) {
        if (line) {
            line->clear();
        }
    }
}

std::unique_ptr<LineSeries> LineSeries::clone() const {
    auto cloned = std::make_unique<LineSeries>(line_specs_);
    cloned->series_name_ = series_name_ + "_clone";
    cloned->minimum_period_ = minimum_period_;
    cloned->auto_sync_ = auto_sync_;
    cloned->enable_bulk_cache_ = enable_bulk_cache_;
    
    return cloned;
}

void LineSeries::merge(const LineSeries& other, const std::string& strategy) {
    // Simple merge implementation - add lines from other series
    for (const auto& spec : other.line_specs_) {
        if (!spec.name.empty() && !hasLine(spec.name)) {
            addLine(spec);
        }
    }
    
    // Copy data would require more sophisticated logic
    // This is a placeholder implementation
}

bool LineSeries::validate() const {
    try {
        validate_line_consistency();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// ========== Static Factory Methods ==========

std::unique_ptr<LineSeries> LineSeries::createOHLC(const std::string& name) {
    return std::make_unique<LineSeries>(name, Lines::OHLC);
}

std::unique_ptr<LineSeries> LineSeries::createOHLCV(const std::string& name) {
    return std::make_unique<LineSeries>(name, Lines::OHLCV);
}

std::unique_ptr<LineSeries> LineSeries::createCustom(const std::string& name, 
                                                    const std::vector<std::string>& line_names) {
    std::vector<LineSpec> specs;
    specs.reserve(line_names.size());
    
    for (size_t i = 0; i < line_names.size(); ++i) {
        specs.emplace_back(line_names[i], static_cast<int>(i));
    }
    
    return std::make_unique<LineSeries>(name, specs);
}

// ========== Helper Methods ==========

void LineSeries::validate_line_consistency() const {
    if (lines_.empty()) {
        return;
    }
    
    // Check that all non-null lines have the same current index
    int expected_idx = -1;
    bool first_line = true;
    
    for (const auto& line : lines_) {
        if (line) {
            if (first_line) {
                expected_idx = line->getIdx();
                first_line = false;
            } else if (line->getIdx() != expected_idx) {
                throw std::runtime_error("Line indices are not synchronized");
            }
        }
    }
}

void LineSeries::ensure_line_capacity(int capacity) {
    for (auto& line : lines_) {
        if (line) {
            line->minbuffer(capacity);
        }
    }
}

void LineSeries::sync_all_lines() {
    if (!auto_sync_ || lines_.empty()) {
        return;
    }
    
    // Find the line with the maximum index
    int max_idx = -1;
    for (const auto& line : lines_) {
        if (line) {
            max_idx = std::max(max_idx, line->getIdx());
        }
    }
    
    // Advance all lines to match the maximum index
    for (auto& line : lines_) {
        if (line) {
            while (line->getIdx() < max_idx) {
                line->forward(NAN_VALUE);
            }
        }
    }
}

} // namespace line
} // namespace backtrader