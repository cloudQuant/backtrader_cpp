/**
 * @file LineSeries.h
 * @brief Line Series - Multi-line data structure for OHLCV and indicator data
 * 
 * This file implements the LineSeries functionality from backtrader's Python codebase,
 * providing multi-line data management with named access (open, high, low, close, volume),
 * automatic synchronization, and efficient bulk operations for technical analysis.
 */

#pragma once

#include "LineBuffer.h"
#include "LineIterator.h"
#include "MetaClass.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <concepts>
#include <ranges>
#include <span>
#include <optional>

namespace backtrader {
namespace line {

// Forward declarations
class LineSeries;
class LineGroup;

/**
 * @brief Line specification for multi-line data structures
 */
struct LineSpec {
    std::string name;           ///< Line name (e.g., "open", "high", "close")
    std::string alias;          ///< Alternative name/alias
    int index;                  ///< Position in line array
    bool is_required = true;    ///< Whether line is required
    
    LineSpec() = default;
    LineSpec(const std::string& n, int idx, bool required = true)
        : name(n), alias(n), index(idx), is_required(required) {}
    LineSpec(const std::string& n, const std::string& a, int idx, bool required = true)
        : name(n), alias(a), index(idx), is_required(required) {}
};

/**
 * @brief Common line specifications for OHLCV data
 */
namespace Lines {
    inline const LineSpec Open{"open", 0};
    inline const LineSpec High{"high", 1};
    inline const LineSpec Low{"low", 2};
    inline const LineSpec Close{"close", 3};
    inline const LineSpec Volume{"volume", 4};
    inline const LineSpec OpenInterest{"openinterest", 5};
    
    // Convenience arrays
    inline const std::vector<LineSpec> OHLC = {Open, High, Low, Close};
    inline const std::vector<LineSpec> OHLCV = {Open, High, Low, Close, Volume};
    inline const std::vector<LineSpec> OHLCVI = {Open, High, Low, Close, Volume, OpenInterest};
}

/**
 * @brief Core LineSeries class - manages multiple synchronized LineBuffers
 * 
 * LineSeries provides:
 * - Named access to multiple data lines (open, high, low, close, etc.)
 * - Automatic synchronization between lines
 * - Bulk operations for performance
 * - Iterator support and range-based operations
 * - Integration with LineIterator for execution
 */
class LineSeries : public meta::MetaParams<LineSeries> {
private:
    std::vector<std::shared_ptr<LineBuffer>> lines_;
    std::unordered_map<std::string, int> name_to_index_;
    std::unordered_map<std::string, int> alias_to_index_;
    std::vector<LineSpec> line_specs_;
    
    std::string series_name_;
    int minimum_period_;
    bool auto_sync_;
    
    // Performance optimization
    mutable std::vector<std::vector<double>> bulk_cache_;
    bool enable_bulk_cache_;
    
    // Validation and integrity
    void validate_line_consistency() const;
    void ensure_line_capacity(int capacity);
    void sync_all_lines();
    
protected:
    void initializeParams() override;
    
public:
    /**
     * @brief Default constructor
     */
    LineSeries();
    
    /**
     * @brief Constructor with line specifications
     * @param specs Vector of line specifications
     */
    explicit LineSeries(const std::vector<LineSpec>& specs);
    
    /**
     * @brief Constructor with series name and line specs
     * @param name Series name
     * @param specs Vector of line specifications
     */
    LineSeries(const std::string& name, const std::vector<LineSpec>& specs);
    
    /**
     * @brief Destructor
     */
    virtual ~LineSeries() = default;
    
    // ========== Configuration ==========
    
    /**
     * @brief Set series name
     * @param name Name for this series
     */
    void setName(const std::string& name) { series_name_ = name; }
    
    /**
     * @brief Get series name
     */
    const std::string& getName() const { return series_name_; }
    
    /**
     * @brief Set minimum period for valid data
     * @param period Minimum number of data points
     */
    void setMinimumPeriod(int period) { minimum_period_ = period; }
    
    /**
     * @brief Get minimum period
     */
    int getMinimumPeriod() const { return minimum_period_; }
    
    /**
     * @brief Enable/disable automatic synchronization
     * @param enable Whether to automatically sync lines
     */
    void setAutoSync(bool enable) { auto_sync_ = enable; }
    
    /**
     * @brief Check if auto-sync is enabled
     */
    bool isAutoSyncEnabled() const { return auto_sync_; }
    
    // ========== Line Management ==========
    
    /**
     * @brief Add line with specification
     * @param spec Line specification
     * @return Index of added line
     */
    int addLine(const LineSpec& spec);
    
    /**
     * @brief Add line with name
     * @param name Line name
     * @return Index of added line
     */
    int addLine(const std::string& name);
    
    /**
     * @brief Remove line by name
     * @param name Line name to remove
     */
    void removeLine(const std::string& name);
    
    /**
     * @brief Remove line by index
     * @param index Line index to remove
     */
    void removeLine(int index);
    
    /**
     * @brief Get number of lines
     */
    int getLineCount() const { return static_cast<int>(lines_.size()); }
    
    /**
     * @brief Check if line exists
     * @param name Line name
     * @return true if line exists
     */
    bool hasLine(const std::string& name) const;
    
    /**
     * @brief Get line index by name
     * @param name Line name
     * @return Line index, or -1 if not found
     */
    int getLineIndex(const std::string& name) const;
    
    /**
     * @brief Get line names
     */
    std::vector<std::string> getLineNames() const;
    
    /**
     * @brief Get line specifications
     */
    const std::vector<LineSpec>& getLineSpecs() const { return line_specs_; }
    
    // ========== Data Access ==========
    
    /**
     * @brief Get LineBuffer by name
     * @param name Line name
     * @return Shared pointer to LineBuffer
     */
    std::shared_ptr<LineBuffer> getLine(const std::string& name) const;
    
    /**
     * @brief Get LineBuffer by index
     * @param index Line index
     * @return Shared pointer to LineBuffer
     */
    std::shared_ptr<LineBuffer> getLine(int index) const;
    
    /**
     * @brief Get all LineBuffers
     */
    const std::vector<std::shared_ptr<LineBuffer>>& getLines() const { return lines_; }
    
    /**
     * @brief Access line data with negative indexing
     * @param line_name Line name
     * @param ago Periods ago (0 = current, -1 = previous)
     * @return Data value
     */
    double operator()(const std::string& line_name, int ago = 0) const;
    
    /**
     * @brief Access line data by index
     * @param line_index Line index
     * @param ago Periods ago
     * @return Data value
     */
    double operator()(int line_index, int ago = 0) const;
    
    /**
     * @brief Set line data value
     * @param line_name Line name
     * @param value Value to set
     * @param ago Periods ago (default: current)
     */
    void set(const std::string& line_name, double value, int ago = 0);
    
    /**
     * @brief Set line data value by index
     * @param line_index Line index
     * @param value Value to set
     * @param ago Periods ago
     */
    void set(int line_index, double value, int ago = 0);
    
    // ========== Convenience Accessors for OHLCV ==========
    
    /**
     * @brief Get open price
     * @param ago Periods ago
     */
    double open(int ago = 0) const { return (*this)("open", ago); }
    
    /**
     * @brief Get high price
     * @param ago Periods ago
     */
    double high(int ago = 0) const { return (*this)("high", ago); }
    
    /**
     * @brief Get low price
     * @param ago Periods ago
     */
    double low(int ago = 0) const { return (*this)("low", ago); }
    
    /**
     * @brief Get close price
     * @param ago Periods ago
     */
    double close(int ago = 0) const { return (*this)("close", ago); }
    
    /**
     * @brief Get volume
     * @param ago Periods ago
     */
    double volume(int ago = 0) const { return (*this)("volume", ago); }
    
    /**
     * @brief Get open interest
     * @param ago Periods ago
     */
    double openInterest(int ago = 0) const { return (*this)("openinterest", ago); }
    
    // ========== Bulk Operations ==========
    
    /**
     * @brief Add multiple data points at once
     * @param data Map of line names to values
     */
    void addData(const std::unordered_map<std::string, double>& data);
    
    /**
     * @brief Add OHLCV data point
     * @param o Open price
     * @param h High price
     * @param l Low price
     * @param c Close price
     * @param v Volume (optional)
     */
    void addOHLCV(double o, double h, double l, double c, double v = 0.0);
    
    /**
     * @brief Get multiple values from a line
     * @param line_name Line name
     * @param periods Number of periods to retrieve
     * @param start_ago Starting offset (default: current)
     * @return Vector of values
     */
    std::vector<double> getValues(const std::string& line_name, int periods, int start_ago = 0) const;
    
    /**
     * @brief Get all line values for current period
     * @return Map of line names to current values
     */
    std::unordered_map<std::string, double> getCurrentValues() const;
    
    /**
     * @brief Get values for all lines over multiple periods
     * @param periods Number of periods
     * @return Vector of maps (oldest to newest)
     */
    std::vector<std::unordered_map<std::string, double>> getBulkValues(int periods) const;
    
    // ========== Navigation ==========
    
    /**
     * @brief Move all lines forward and add new data
     * @param data Map of line names to values
     */
    void forward(const std::unordered_map<std::string, double>& data);
    
    /**
     * @brief Move all lines forward (with NaN values)
     */
    void forward();
    
    /**
     * @brief Move all lines backward
     * @param periods Number of periods to move back
     */
    void backward(int periods = 1);
    
    /**
     * @brief Reset all lines to beginning
     */
    void home();
    
    /**
     * @brief Rewind all lines
     * @param periods Number of periods to rewind
     */
    void rewind(int periods = 1);
    
    // ========== State Information ==========
    
    /**
     * @brief Get current data length (minimum across all lines)
     */
    int size() const;
    
    /**
     * @brief Check if series is empty
     */
    bool empty() const;
    
    /**
     * @brief Get current bar/period index
     */
    int getCurrentBar() const;
    
    /**
     * @brief Check if minimum period is satisfied
     */
    bool isReady() const;
    
    // ========== Iterator Support ==========
    
    /**
     * @brief Iterator for LineSeries data
     */
    class Iterator {
    private:
        const LineSeries* series_;
        int current_ago_;
        
    public:
        Iterator(const LineSeries* series, int ago) : series_(series), current_ago_(ago) {}
        
        std::unordered_map<std::string, double> operator*() const;
        Iterator& operator++() { --current_ago_; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; --current_ago_; return tmp; }
        bool operator!=(const Iterator& other) const { return current_ago_ != other.current_ago_; }
        bool operator==(const Iterator& other) const { return current_ago_ == other.current_ago_; }
    };
    
    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, -size()); }
    
    // ========== C++20 Features ==========
    
    /**
     * @brief Range-based access to line data
     */
    // C++17 compatible version without std::views
    std::vector<std::shared_ptr<LineBuffer>> getLineRange() const {
        std::vector<std::shared_ptr<LineBuffer>> result;
        std::copy_if(lines_.begin(), lines_.end(), std::back_inserter(result),
                     [](const auto& line) { return line != nullptr; });
        return result;
    }
    
    /**
     * @brief Add data from any container
     * @param line_name Line name to add data to
     * @param data Container with data
     */
    template<typename T>
    void addLineData(const std::string& line_name, const T& data) {
        auto line = getLine(line_name);
        if (line) {
            for (const auto& value : data) {
                line->forward(static_cast<double>(value));
            }
        }
    }
    
    /**
     * @brief Get data as span for zero-copy access
     * @param line_name Line name
     * @param periods Number of periods
     * @return Span of data (if possible)
     */
    // C++17 compatible version - return vector instead of span
    std::optional<std::vector<double>> getDataSpan(const std::string& line_name, int periods) const;
    
    // ========== Utility Methods ==========
    
    /**
     * @brief Print series state for debugging
     */
    void print() const;
    
    /**
     * @brief Clear all data
     */
    void clear();
    
    /**
     * @brief Clone this series
     * @return New LineSeries with same structure
     */
    std::unique_ptr<LineSeries> clone() const;
    
    /**
     * @brief Merge with another series
     * @param other Series to merge
     * @param strategy Merge strategy ("outer", "inner", "left", "right")
     */
    void merge(const LineSeries& other, const std::string& strategy = "outer");
    
    /**
     * @brief Validate data integrity
     * @return true if all lines are consistent
     */
    bool validate() const;
    
    // ========== Static Factory Methods ==========
    
    /**
     * @brief Create OHLC series
     * @param name Series name
     * @return New OHLC LineSeries
     */
    static std::unique_ptr<LineSeries> createOHLC(const std::string& name = "OHLC");
    
    /**
     * @brief Create OHLCV series
     * @param name Series name
     * @return New OHLCV LineSeries
     */
    static std::unique_ptr<LineSeries> createOHLCV(const std::string& name = "OHLCV");
    
    /**
     * @brief Create custom series
     * @param name Series name
     * @param line_names Vector of line names
     * @return New custom LineSeries
     */
    static std::unique_ptr<LineSeries> createCustom(const std::string& name, 
                                                   const std::vector<std::string>& line_names);
};

// ========== Inline Implementations ==========

inline bool LineSeries::hasLine(const std::string& name) const {
    return name_to_index_.find(name) != name_to_index_.end() ||
           alias_to_index_.find(name) != alias_to_index_.end();
}

inline int LineSeries::getLineIndex(const std::string& name) const {
    auto it = name_to_index_.find(name);
    if (it != name_to_index_.end()) {
        return it->second;
    }
    
    auto alias_it = alias_to_index_.find(name);
    if (alias_it != alias_to_index_.end()) {
        return alias_it->second;
    }
    
    return -1;
}

inline double LineSeries::operator()(const std::string& line_name, int ago) const {
    auto line = getLine(line_name);
    if (line) {
        return (*line)[ago];
    }
    throw std::runtime_error("Line not found: " + line_name);
}

inline double LineSeries::operator()(int line_index, int ago) const {
    auto line = getLine(line_index);
    if (line) {
        return (*line)[ago];
    }
    throw std::out_of_range("Line index out of range: " + std::to_string(line_index));
}

} // namespace line
} // namespace backtrader