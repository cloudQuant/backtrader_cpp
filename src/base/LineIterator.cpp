/**
 * @file LineIterator.cpp
 * @brief Implementation of LineIterator class
 */

#include "base/LineIterator.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <thread>

namespace backtrader {
namespace line {

// ========== Constructor ==========

LineIterator::LineIterator()
    : mode_(IteratorMode::Next)
    , minimum_period_(1)
    , current_period_(0)
    , is_running_(false)
    , warmup_complete_(false)
    , enable_caching_(true) {
    
    sync_state_.reset();
    stats_ = {};
}

// ========== Protected Methods ==========

void LineIterator::initializeParams() {
    DEFINE_PARAMETER(int, minperiod, 1, "Minimum period before execution");
    DEFINE_PARAMETER(bool, enable_cache, true, "Enable performance caching");
    DEFINE_PARAMETER(int, batch_size, 1, "Batch processing size");
}

// ========== Data Management ==========

void LineIterator::addDataBuffer(std::shared_ptr<LineBuffer> buffer) {
    if (buffer) {
        auto it = std::find(data_buffers_.begin(), data_buffers_.end(), buffer);
        if (it == data_buffers_.end()) {
            data_buffers_.push_back(buffer);
            sync_state_.data_positions.push_back(0);
        }
    }
}

void LineIterator::removeDataBuffer(std::shared_ptr<LineBuffer> buffer) {
    auto it = std::find(data_buffers_.begin(), data_buffers_.end(), buffer);
    if (it != data_buffers_.end()) {
        size_t index = std::distance(data_buffers_.begin(), it);
        data_buffers_.erase(it);
        if (index < sync_state_.data_positions.size()) {
            sync_state_.data_positions.erase(sync_state_.data_positions.begin() + index);
        }
    }
}

// ========== Execution Control ==========

bool LineIterator::start() {
    if (is_running_) {
        return false; // Already running
    }
    
    if (data_buffers_.empty()) {
        std::cerr << "Warning: No data buffers registered" << std::endl;
        return false;
    }
    
    // Reset state
    reset();
    is_running_ = true;
    warmup_complete_ = false;
    
    // Initialize statistics
    stats_ = {};
    last_step_time_ = std::chrono::steady_clock::now();
    
    return true;
}

void LineIterator::stop() {
    is_running_ = false;
}

bool LineIterator::step() {
    if (!is_running_) {
        return false;
    }
    
    auto step_start = std::chrono::steady_clock::now();
    
    try {
        // Check if we've reached maximum iterations
        if (max_iterations_.has_value() && current_period_ >= max_iterations_.value()) {
            stop();
            return false;
        }
        
        // Synchronize all data sources
        synchronize_data();
        
        if (!sync_state_.is_synchronized) {
            // No more data available
            stop();
            return false;
        }
        
        // Check minimum period requirement
        if (!warmup_complete_ && check_minimum_period()) {
            warmup_complete_ = true;
        }
        
        // Execute appropriate stage based on mode and warmup status
        if (mode_ == IteratorMode::Once && !warmup_complete_) {
            execute_stage_callbacks(IterationStage::PreOnce);
            execute_stage_callbacks(IterationStage::Once);
            execute_stage_callbacks(IterationStage::PostOnce);
        } else if (warmup_complete_) {
            execute_stage_callbacks(IterationStage::PreNext);
            execute_stage_callbacks(IterationStage::Next);
            execute_stage_callbacks(IterationStage::PostNext);
        }
        
        // Advance all buffers to next position
        advance_all_buffers();
        current_period_++;
        
        // Update statistics
        auto step_end = std::chrono::steady_clock::now();
        auto step_duration = std::chrono::duration_cast<std::chrono::milliseconds>(step_end - step_start);
        
        stats_.total_iterations++;
        stats_.successful_steps++;
        stats_.total_time += step_duration;
        stats_.avg_step_time = stats_.total_time / stats_.total_iterations;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in iteration step: " << e.what() << std::endl;
        stats_.failed_steps++;
        return false;
    }
}

int LineIterator::run() {
    if (!start()) {
        return 0;
    }
    
    int iterations = 0;
    while (step()) {
        iterations++;
        
        // Yield control periodically for responsiveness
        if (iterations % 1000 == 0) {
            std::this_thread::yield();
        }
    }
    
    return iterations;
}

void LineIterator::reset() {
    current_period_ = 0;
    warmup_complete_ = false;
    sync_state_.reset();
    
    // Reset all data buffers to beginning
    for (auto& buffer : data_buffers_) {
        if (buffer) {
            buffer->home();
        }
    }
    
    // Clear cache
    cache_buffer_.clear();
}

// ========== Synchronization ==========

void LineIterator::synchronize_data() {
    if (data_buffers_.empty()) {
        sync_state_.is_synchronized = false;
        return;
    }
    
    // Find minimum available data across all buffers
    int min_available = std::numeric_limits<int>::max();
    bool has_data = false;
    
    for (size_t i = 0; i < data_buffers_.size(); ++i) {
        auto& buffer = data_buffers_[i];
        if (buffer && !buffer->empty()) {
            min_available = std::min(min_available, buffer->size());
            has_data = true;
        }
    }
    
    if (!has_data || min_available <= current_period_) {
        sync_state_.is_synchronized = false;
        return;
    }
    
    // Update synchronization state
    sync_state_.current_bar = current_period_;
    sync_state_.is_synchronized = true;
    
    // Update positions for each data buffer
    for (size_t i = 0; i < data_buffers_.size() && i < sync_state_.data_positions.size(); ++i) {
        sync_state_.data_positions[i] = current_period_;
    }
}

bool LineIterator::check_minimum_period() const {
    return current_period_ >= minimum_period_;
}

void LineIterator::advance_all_buffers() {
    for (auto& buffer : data_buffers_) {
        if (buffer) {
            // This would advance the logical position in the buffer
            // The actual implementation depends on how data is fed into buffers
            // For now, we assume buffers are automatically advanced
        }
    }
}

void LineIterator::execute_stage_callbacks(IterationStage stage) {
    std::vector<std::function<void()>>* callbacks = nullptr;
    
    switch (stage) {
        case IterationStage::PreNext:
            callbacks = &prenext_callbacks_;
            break;
        case IterationStage::Next:
            callbacks = &next_callbacks_;
            break;
        case IterationStage::PostNext:
            callbacks = &postnext_callbacks_;
            break;
        case IterationStage::PreOnce:
        case IterationStage::PostOnce:
            // Handle once-related callbacks if needed
            break;
        case IterationStage::Once:
            callbacks = &once_callbacks_;
            break;
    }
    
    if (callbacks) {
        for (auto& callback : *callbacks) {
            try {
                callback();
            } catch (const std::exception& e) {
                std::cerr << "Error in stage callback: " << e.what() << std::endl;
            }
        }
    }
}

// ========== Advanced Features ==========

int LineIterator::runTimeRange(std::chrono::system_clock::time_point start_time,
                               std::chrono::system_clock::time_point end_time) {
    // This would require timestamp support in LineBuffer
    // For now, return standard run
    return run();
}

int LineIterator::runBatch(int batch_size) {
    if (!start()) {
        return 0;
    }
    
    int batches_processed = 0;
    std::vector<bool> batch_results;
    batch_results.reserve(batch_size);
    
    while (is_running_) {
        batch_results.clear();
        
        // Process a batch of steps
        for (int i = 0; i < batch_size && is_running_; ++i) {
            batch_results.push_back(step());
        }
        
        // Check if any step in the batch succeeded
        if (std::any_of(batch_results.begin(), batch_results.end(), [](bool result) { return result; })) {
            batches_processed++;
        } else {
            break; // No successful steps in batch
        }
        
        // Yield control after each batch
        std::this_thread::yield();
    }
    
    return batches_processed;
}

const std::vector<double>& LineIterator::getCachedData(int buffer_index, int periods) const {
    if (!enable_caching_ || buffer_index >= static_cast<int>(data_buffers_.size())) {
        static const std::vector<double> empty_cache;
        return empty_cache;
    }
    
    // Simple caching implementation
    cache_buffer_.clear();
    
    auto& buffer = data_buffers_[buffer_index];
    if (buffer) {
        cache_buffer_.reserve(periods);
        for (int i = 0; i < periods; ++i) {
            try {
                cache_buffer_.push_back((*buffer)[-i]);
            } catch (const std::exception&) {
                cache_buffer_.push_back(NAN_VALUE);
            }
        }
    }
    
    return cache_buffer_;
}

// ========== Utility Methods ==========

void LineIterator::printState() const {
    std::cout << "LineIterator State:" << std::endl;
    std::cout << "  Mode: " << (mode_ == IteratorMode::Next ? "Next" : "Once") << std::endl;
    std::cout << "  Running: " << (is_running_ ? "Yes" : "No") << std::endl;
    std::cout << "  Current Period: " << current_period_ << std::endl;
    std::cout << "  Minimum Period: " << minimum_period_ << std::endl;
    std::cout << "  Warmup Complete: " << (warmup_complete_ ? "Yes" : "No") << std::endl;
    std::cout << "  Synchronized: " << (sync_state_.is_synchronized ? "Yes" : "No") << std::endl;
    std::cout << "  Data Buffers: " << data_buffers_.size() << std::endl;
    std::cout << "  Next Callbacks: " << next_callbacks_.size() << std::endl;
    std::cout << "  Once Callbacks: " << once_callbacks_.size() << std::endl;
    
    if (max_iterations_.has_value()) {
        double progress = getProgress();
        std::cout << "  Progress: " << std::fixed << std::setprecision(1) 
                  << (progress * 100.0) << "%" << std::endl;
    }
    
    // Print statistics
    auto stats = getStats();
    std::cout << "  Statistics:" << std::endl;
    std::cout << "    Total Iterations: " << stats.total_iterations << std::endl;
    std::cout << "    Successful Steps: " << stats.successful_steps << std::endl;
    std::cout << "    Failed Steps: " << stats.failed_steps << std::endl;
    std::cout << "    Total Time: " << stats.total_time.count() << "ms" << std::endl;
    std::cout << "    Avg Step Time: " << stats.avg_step_time.count() << "ms" << std::endl;
}

LineIterator::IterationStats LineIterator::getStats() const {
    return stats_;
}

// Note: Coroutine implementation removed for compatibility
// Can be re-added when full C++20 coroutine support is available

} // namespace line
} // namespace backtrader