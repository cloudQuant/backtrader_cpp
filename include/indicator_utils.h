#pragma once

#include "linebuffer.h"
#include "lineseries.h"
#include <memory>
#include <limits>

namespace backtrader {
namespace utils {

/**
 * Get the actual data size from a LineSeries or DataSeries
 * This handles the case where LineBuffer::size() returns 0 when _idx is -1
 * In streaming mode, returns the current position (effective data size)
 */
inline size_t getDataSize(const std::shared_ptr<LineSeries>& data_source) {
    if (!data_source || !data_source->lines || data_source->lines->size() == 0) return 0;
    
    // Get the first line to determine actual data size
    auto first_line = data_source->lines->getline(0);
    if (!first_line) return 0;
    
    // Try to get LineBuffer to access array size and current position
    auto buffer = std::dynamic_pointer_cast<LineBuffer>(first_line);
    if (buffer) {
        // In streaming mode, the effective data size is determined by _idx position
        // _idx starts at -1 (no data), 0 (first data point), etc.
        int idx = buffer->get_idx();
        if (idx >= 0) {
            // When _idx is valid, the effective data size is idx + 1
            // This represents how much data is "available" at current position
            return static_cast<size_t>(idx + 1);
        } else {
            // When _idx is -1, no data is available yet in streaming mode
            // But we still return the total array size for batch mode compatibility
            return buffer->array().size();
        }
    }
    
    // Fallback to regular size() method
    return first_line->size();
}

/**
 * Safely access data from a LineBuffer using array() method
 * This handles the case where size() returns 0 but data exists in array()
 */
inline double getBufferValue(const std::shared_ptr<LineBuffer>& buffer, int index, size_t data_size) {
    if (!buffer || index < 0 || index >= static_cast<int>(data_size)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return buffer->array()[index];
}

/**
 * Get data line from LineSeries/DataSeries
 * Handles both single-line LineSeries and multi-line DataSeries (OHLCV)
 */
inline std::shared_ptr<LineSingle> getDataLine(const std::shared_ptr<LineSeries>& data_source, int default_line = 0) {
    if (!data_source || !data_source->lines) return nullptr;
    
    if (data_source->lines->size() >= 5) {
        // DataSeries with OHLCV - use close price at index 4
        // Standard order: DateTime=0, Open=1, High=2, Low=3, Close=4, Volume=5, OpenInterest=6
        return data_source->lines->getline(4);
    } else if (data_source->lines->size() > default_line) {
        // LineSeries - use specified line (default 0)
        return data_source->lines->getline(default_line);
    }
    
    return nullptr;
}

/**
 * Setup LineBuffer to correct position after batch calculation
 */
inline void finalizeLineBuffer(const std::shared_ptr<LineBuffer>& buffer) {
    if (buffer && buffer->size() > 0) {
        buffer->set_idx(buffer->size() - 1);
    }
}

/**
 * Initialize a value to NaN for proper indicator behavior
 */
inline double initNaN() {
    return std::numeric_limits<double>::quiet_NaN();
}

/**
 * Check if a value is valid (not NaN or infinity)
 */
inline bool isValidValue(double value) {
    return !std::isnan(value) && std::isfinite(value);
}

} // namespace utils
} // namespace backtrader