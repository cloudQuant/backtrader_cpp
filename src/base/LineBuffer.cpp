/**
 * @file LineBuffer.cpp
 * @brief Implementation of LineBuffer class
 */

#include "base/LineBuffer.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace backtrader {
namespace line {

// ========== Constructors and Destructors ==========

LineBuffer::LineBuffer() 
    : mode_(UnBounded)
    , idx_(-1)
    , lencount_(0)
    , extension_(0)
    , maxlen_(0)
    , extrasize_(0)
    , lenmark_(0)
    , useislice_(false) {
    reset();
}

LineBuffer::LineBuffer(const LineBuffer& other)
    : mode_(other.mode_)
    , unbounded_array_(other.unbounded_array_)
    , circular_array_(other.circular_array_)
    , idx_(other.idx_)
    , lencount_(other.lencount_)
    , extension_(other.extension_)
    , maxlen_(other.maxlen_)
    , extrasize_(other.extrasize_)
    , lenmark_(other.lenmark_)
    , useislice_(other.useislice_)
    , bindings_(other.bindings_) {
}

LineBuffer& LineBuffer::operator=(const LineBuffer& other) {
    if (this != &other) {
        mode_ = other.mode_;
        unbounded_array_ = other.unbounded_array_;
        circular_array_ = other.circular_array_;
        idx_ = other.idx_;
        lencount_ = other.lencount_;
        extension_ = other.extension_;
        maxlen_ = other.maxlen_;
        extrasize_ = other.extrasize_;
        lenmark_ = other.lenmark_;
        useislice_ = other.useislice_;
        bindings_ = other.bindings_;
    }
    return *this;
}

// ========== Mode Management ==========

void LineBuffer::qbuffer(int savemem, int extrasize) {
    mode_ = QBuffer;
    maxlen_ = savemem;
    extrasize_ = extrasize;
    useislice_ = true;
    reset();
}

void LineBuffer::minbuffer(int size) {
    if (mode_ == QBuffer) {
        maxlen_ = std::max(maxlen_, size);
    }
    // For UnBounded mode, reserve capacity if needed
    if (mode_ == UnBounded) {
        unbounded_array_.reserve(size);
    }
}

void LineBuffer::reset() {
    if (mode_ == QBuffer) {
        // Circular buffer mode - limited memory
        int total_size = maxlen_ + extrasize_;
        circular_array_.clear();
        if (total_size > 0) {
            // Pre-allocate deque (note: deque doesn't have reserve, but we can pre-fill)
            circular_array_.resize(total_size, NAN_VALUE);
            circular_array_.clear(); // Clear but keep capacity
        }
        useislice_ = true;
    } else {
        // Unbounded mode - unlimited growth
        unbounded_array_.clear();
        useislice_ = false;
    }
    
    lencount_ = 0;
    idx_ = -1;
    extension_ = 0;
    lenmark_ = 0;
}

// ========== Index Management ==========

void LineBuffer::setIdx(int idx, bool force) {
    if (!force) {
        // Validate index bounds
        if (mode_ == UnBounded) {
            int max_idx = static_cast<int>(unbounded_array_.size()) - 1;
            if (idx > max_idx || idx < -1) {
                throw std::out_of_range("Index out of valid range");
            }
        } else {
            int max_idx = static_cast<int>(circular_array_.size()) - 1;
            if (idx > max_idx || idx < -1) {
                throw std::out_of_range("Index out of valid range");
            }
        }
    }
    idx_ = idx;
}

// ========== Data Access ==========

void LineBuffer::set(double value, int ago) {
    if (!isValidIndex(ago)) {
        throw std::out_of_range("Cannot set value at invalid index: " + std::to_string(ago));
    }
    
    if (mode_ == UnBounded) {
        unbounded_array_[idx_ + ago] = value;
    } else {
        int actual_idx = getArrayIndex(ago);
        circular_array_[actual_idx] = value;
    }
    
    // Propagate to bindings
    propagate_value(value, ago);
}

// ========== Navigation ==========

void LineBuffer::forward(double value, int size) {
    for (int i = 0; i < size; ++i) {
        idx_++;
        lencount_++;
        
        if (mode_ == UnBounded) {
            unbounded_array_.push_back(value);
        } else {
            // QBuffer mode - manage circular buffer
            if (static_cast<int>(circular_array_.size()) < maxlen_ + extrasize_) {
                circular_array_.push_back(value);
            } else {
                // Circular overwrite
                int actual_idx = getArrayIndex(0);
                circular_array_[actual_idx] = value;
            }
        }
    }
}

void LineBuffer::backwards(int size, bool force) {
    if (!force && size > lencount_) {
        throw std::runtime_error("Cannot move backwards more than available data");
    }
    
    for (int i = 0; i < size; ++i) {
        if (lencount_ > 0) {
            lencount_--;
            idx_--;
            
            if (mode_ == UnBounded) {
                if (!unbounded_array_.empty()) {
                    unbounded_array_.pop_back();
                }
            } else {
                if (!circular_array_.empty()) {
                    circular_array_.pop_back();
                }
            }
        }
    }
}

void LineBuffer::rewind(int size) {
    idx_ -= size;
    lencount_ -= size;
    if (lencount_ < 0) lencount_ = 0;
    if (idx_ < -1) idx_ = -1;
}

void LineBuffer::advance(int size) {
    idx_ += size;
    lencount_ += size;
    
    // Ensure we don't advance beyond available data
    int max_len = (mode_ == UnBounded) ? 
        static_cast<int>(unbounded_array_.size()) : 
        static_cast<int>(circular_array_.size());
        
    if (idx_ >= max_len) {
        idx_ = max_len - 1;
        lencount_ = max_len;
    }
}

void LineBuffer::home() {
    idx_ = -1;
    lencount_ = 0;
}

void LineBuffer::extend(double value, int size) {
    if (size == 0) {
        // Just move index
        idx_++;
        lencount_++;
        return;
    }
    
    for (int i = 0; i < size; ++i) {
        forward(value, 1);
    }
}

// ========== Data Retrieval ==========

std::vector<double> LineBuffer::getRange(int ago, int size) const {
    std::vector<double> result;
    result.reserve(size);
    
    for (int i = 0; i < size; ++i) {
        int offset = ago - i;
        if (isValidIndex(offset)) {
            result.push_back((*this)[offset]);
        } else {
            result.push_back(NAN_VALUE);
        }
    }
    
    return result;
}

double LineBuffer::getZeroVal(int idx) const {
    if (isValidIndex(idx)) {
        return (*this)[idx];
    }
    return 0.0;
}

std::vector<double> LineBuffer::getZero(int idx, int size) const {
    std::vector<double> result;
    result.reserve(size);
    
    for (int i = 0; i < size; ++i) {
        int offset = idx - i;
        if (isValidIndex(offset)) {
            result.push_back((*this)[offset]);
        } else {
            result.push_back(0.0);
        }
    }
    
    return result;
}

// ========== Binding System ==========

void LineBuffer::addBinding(LineBuffer* binding) {
    if (binding && std::find(bindings_.begin(), bindings_.end(), binding) == bindings_.end()) {
        bindings_.push_back(binding);
    }
}

void LineBuffer::removeBinding(LineBuffer* binding) {
    auto it = std::find(bindings_.begin(), bindings_.end(), binding);
    if (it != bindings_.end()) {
        bindings_.erase(it);
    }
}

// ========== Utility Methods ==========

void LineBuffer::clear() {
    unbounded_array_.clear();
    circular_array_.clear();
    bindings_.clear();
    reset();
}

void LineBuffer::print() const {
    std::cout << "LineBuffer state:" << std::endl;
    std::cout << "  Mode: " << (mode_ == UnBounded ? "UnBounded" : "QBuffer") << std::endl;
    std::cout << "  Index: " << idx_ << std::endl;
    std::cout << "  Length: " << lencount_ << std::endl;
    std::cout << "  Buffer size: " << buflen() << std::endl;
    std::cout << "  Max length: " << maxlen_ << std::endl;
    std::cout << "  Extra size: " << extrasize_ << std::endl;
    std::cout << "  Bindings: " << bindings_.size() << std::endl;
    
    if (lencount_ > 0) {
        std::cout << "  Data (recent to old): ";
        int show_count = std::min(10, lencount_);
        for (int i = 0; i >= -show_count + 1; --i) {
            if (isValidIndex(i)) {
                std::cout << std::fixed << std::setprecision(2) << (*this)[i];
                if (i > -show_count + 1) std::cout << ", ";
            }
        }
        if (lencount_ > show_count) {
            std::cout << "... (" << (lencount_ - show_count) << " more)";
        }
        std::cout << std::endl;
    }
}

// ========== Helper Methods ==========

void LineBuffer::ensure_capacity(int size) {
    if (mode_ == UnBounded) {
        if (static_cast<int>(unbounded_array_.capacity()) < size) {
            unbounded_array_.reserve(size);
        }
    }
    // For QBuffer mode, capacity is managed by maxlen_
}

void LineBuffer::propagate_value(double value, int ago) {
    // Propagate value to all bound LineBuffers
    for (LineBuffer* binding : bindings_) {
        if (binding) {
            try {
                binding->set(value, ago);
            } catch (const std::exception& e) {
                // Log error but continue with other bindings
                std::cerr << "Warning: Failed to propagate value to binding: " << e.what() << std::endl;
            }
        }
    }
}

} // namespace line
} // namespace backtrader