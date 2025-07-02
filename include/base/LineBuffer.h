/**
 * @file LineBuffer.h
 * @brief Line Buffer System - Core data structure for time series with negative indexing
 * 
 * This file implements the Line Buffer functionality from backtrader's Python codebase,
 * providing efficient circular buffer management with historical data access through
 * negative indexing. This is the foundation of backtrader's data access system.
 */

#pragma once

#include <vector>
#include <deque>
#include <memory>
#include <limits>
#include <stdexcept>
#include <cmath>

namespace backtrader {
namespace line {

// Forward declarations
class LineBuffer;

// Constants
constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

/**
 * @brief Core LineBuffer class - implements circular buffer with negative indexing
 * 
 * The LineBuffer is the heart of backtrader's data access system. It provides:
 * - Negative indexing: data[0] = current, data[-1] = previous, data[-2] = two periods ago
 * - Two memory modes: UnBounded (unlimited growth) and QBuffer (circular buffer)
 * - Forward/backward navigation for backtesting replay and resampling
 * - Efficient memory management for both backtesting and live trading
 */
class LineBuffer {
public:
    /**
     * @brief Memory management modes
     */
    enum Mode {
        UnBounded = 0,  ///< Unlimited growth using std::vector
        QBuffer = 1     ///< Fixed-size circular buffer using std::deque
    };

private:
    Mode mode_;                          ///< Current memory management mode
    std::vector<double> unbounded_array_; ///< Array for UnBounded mode
    std::deque<double> circular_array_;   ///< Deque for QBuffer mode
    
    int idx_;           ///< Current logical position (starts at -1)
    int lencount_;      ///< Number of valid data points
    int extension_;     ///< Extension buffer size
    int maxlen_;        ///< Maximum length for QBuffer mode
    int extrasize_;     ///< Extra buffer size for operations
    int lenmark_;       ///< Length marker for operations
    bool useislice_;    ///< Whether to use slicing operations
    
    std::vector<LineBuffer*> bindings_;  ///< Bound line buffers
    
    // Helper methods
    void ensure_capacity(int size);
    void propagate_value(double value, int ago);
    
public:
    /**
     * @brief Default constructor
     */
    LineBuffer();
    
    /**
     * @brief Copy constructor
     */
    LineBuffer(const LineBuffer& other);
    
    /**
     * @brief Assignment operator
     */
    LineBuffer& operator=(const LineBuffer& other);
    
    /**
     * @brief Destructor
     */
    virtual ~LineBuffer() = default;
    
    // ========== Mode Management ==========
    
    /**
     * @brief Configure QBuffer mode with memory limits
     * @param savemem Maximum memory periods to save (0 = unlimited)
     * @param extrasize Extra buffer size for operations
     */
    void qbuffer(int savemem = 0, int extrasize = 0);
    
    /**
     * @brief Set minimum buffer size
     * @param size Minimum buffer size
     */
    void minbuffer(int size);
    
    /**
     * @brief Reset buffer to initial state
     */
    virtual void reset();
    
    /**
     * @brief Get current mode
     */
    Mode getMode() const { return mode_; }
    
    // ========== Index Management ==========
    
    /**
     * @brief Get current logical index
     */
    int getIdx() const { return idx_; }
    
    /**
     * @brief Set logical index (internal use)
     * @param idx New index value
     * @param force Force setting even if out of bounds
     */
    void setIdx(int idx, bool force = false);
    
    // ========== Data Access ==========
    
    /**
     * @brief Access data with negative indexing
     * @param ago Number of periods ago (0 = current, -1 = previous, etc.)
     * @return Data value at specified offset
     */
    virtual double operator[](int ago) const;
    
    /**
     * @brief Set data value at offset
     * @param value Value to set
     * @param ago Offset from current position (0 = current, -1 = previous)
     */
    void set(double value, int ago = 0);
    
    /**
     * @brief Get data value (alias for operator[])
     * @param ago Number of periods ago
     * @return Data value
     */
    double get(int ago = 0) const { return (*this)[ago]; }
    
    // ========== Navigation ==========
    
    /**
     * @brief Move forward and add new value
     * @param value Value to add (default: NAN)
     * @param size Number of periods to advance
     */
    virtual void forward(double value = NAN_VALUE, int size = 1);
    
    /**
     * @brief Move backward and remove values
     * @param size Number of periods to move back
     * @param force Force operation even if it would cause data loss
     */
    virtual void backwards(int size = 1, bool force = false);
    
    /**
     * @brief Rewind index without changing buffer
     * @param size Number of periods to rewind
     */
    void rewind(int size = 1);
    
    /**
     * @brief Advance index without changing buffer
     * @param size Number of periods to advance
     */
    void advance(int size = 1);
    
    /**
     * @brief Reset to beginning position
     */
    void home();
    
    /**
     * @brief Extend buffer with values
     * @param value Value to add (default: NAN)
     * @param size Number of values to add (0 = just move index)
     */
    void extend(double value = NAN_VALUE, int size = 0);
    
    // ========== Data Retrieval ==========
    
    /**
     * @brief Get multiple data points
     * @param ago Starting offset
     * @param size Number of data points
     * @return Vector of data values
     */
    std::vector<double> getRange(int ago = 0, int size = 1) const;
    
    /**
     * @brief Get current value or zero if invalid
     * @param idx Index to check (relative to current)
     * @return Data value or 0.0
     */
    double getZeroVal(int idx = 0) const;
    
    /**
     * @brief Get range of values with zero fallback
     * @param idx Starting index
     * @param size Number of values
     * @return Vector of values (zero for invalid indices)
     */
    std::vector<double> getZero(int idx = 0, int size = 1) const;
    
    // ========== Buffer Information ==========
    
    /**
     * @brief Get number of valid data points
     */
    int size() const { return lencount_; }
    
    /**
     * @brief Get buffer capacity
     */
    int buflen() const;
    
    /**
     * @brief Check if buffer is empty
     */
    bool empty() const { return lencount_ == 0; }
    
    /**
     * @brief Get maximum length (for QBuffer mode)
     */
    int getMaxLen() const { return maxlen_; }
    
    /**
     * @brief Get extra size
     */
    int getExtraSize() const { return extrasize_; }
    
    // ========== Binding System ==========
    
    /**
     * @brief Add a binding to another LineBuffer
     * @param binding LineBuffer to bind to this one
     */
    void addBinding(LineBuffer* binding);
    
    /**
     * @brief Remove a binding
     * @param binding LineBuffer to unbind
     */
    void removeBinding(LineBuffer* binding);
    
    /**
     * @brief Get all bindings
     */
    const std::vector<LineBuffer*>& getBindings() const { return bindings_; }
    
    // ========== Utility Methods ==========
    
    /**
     * @brief Check if index is valid
     * @param ago Offset to check
     * @return true if valid index
     */
    bool isValidIndex(int ago) const;
    
    /**
     * @brief Get actual array index for logical offset
     * @param ago Logical offset
     * @return Physical array index
     */
    int getArrayIndex(int ago) const;
    
    /**
     * @brief Print buffer state for debugging
     */
    void print() const;
    
    /**
     * @brief Clear all data
     */
    void clear();
    
    // ========== Iterator Support ==========
    
    /**
     * @brief Iterator for LineBuffer
     */
    class Iterator {
    private:
        const LineBuffer* buffer_;
        int current_ago_;
        
    public:
        Iterator(const LineBuffer* buffer, int ago) : buffer_(buffer), current_ago_(ago) {}
        
        double operator*() const { return (*buffer_)[current_ago_]; }
        Iterator& operator++() { --current_ago_; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; --current_ago_; return tmp; }
        bool operator!=(const Iterator& other) const { return current_ago_ != other.current_ago_; }
        bool operator==(const Iterator& other) const { return current_ago_ == other.current_ago_; }
    };
    
    /**
     * @brief Get iterator to current position
     */
    Iterator begin() const { return Iterator(this, 0); }
    
    /**
     * @brief Get iterator to end (oldest available data)
     */
    Iterator end() const { return Iterator(this, -lencount_); }
};

// ========== Inline Implementations ==========

inline double LineBuffer::operator[](int ago) const {
    if (!isValidIndex(ago)) {
        throw std::out_of_range("LineBuffer index out of range: " + std::to_string(ago));
    }
    
    if (mode_ == UnBounded) {
        return unbounded_array_[idx_ + ago];
    } else {
        // QBuffer mode - circular access
        int actual_idx = getArrayIndex(ago);
        return circular_array_[actual_idx];
    }
}

inline bool LineBuffer::isValidIndex(int ago) const {
    return ago <= 0 && ago >= -lencount_;
}

inline int LineBuffer::getArrayIndex(int ago) const {
    if (mode_ == UnBounded) {
        return idx_ + ago;
    } else {
        // For circular buffer, wrap around
        int size = static_cast<int>(circular_array_.size());
        if (size == 0) return 0;
        return (idx_ + ago + size) % size;
    }
}

inline int LineBuffer::buflen() const {
    if (mode_ == UnBounded) {
        return static_cast<int>(unbounded_array_.size()) - extension_;
    } else {
        return static_cast<int>(circular_array_.size()) - extension_;
    }
}

} // namespace line
} // namespace backtrader