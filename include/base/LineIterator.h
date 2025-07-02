/**
 * @file LineIterator.h
 * @brief Line Iterator Engine - Core iteration and synchronization system
 * 
 * This file implements the Line Iterator functionality from backtrader's Python codebase,
 * providing dual execution modes (Next/Once), minimum period handling, and synchronization
 * mechanisms for multi-data operations. This is the engine that drives backtrader's
 * time-based execution.
 */

#pragma once

#include "base/LineBuffer.h"
#include "base/MetaClass.h"
#include <vector>
#include <memory>
#include <functional>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <chrono>
#include <optional>

namespace backtrader {
namespace line {

// Forward declarations
class LineIterator;
class LineSeries;

/**
 * @brief Execution modes for LineIterator
 */
enum class IteratorMode {
    Next = 0,   ///< Standard next() execution mode
    Once = 1    ///< Once-only execution mode for initialization
};

/**
 * @brief Iteration stages for lifecycle management
 */
enum class IterationStage {
    PreNext,    ///< Before next() execution
    Next,       ///< During next() execution  
    PostNext,   ///< After next() execution
    PreOnce,    ///< Before once() execution
    Once,       ///< During once() execution
    PostOnce    ///< After once() execution
};

/**
 * @brief Synchronization state for multi-data scenarios
 */
struct SyncState {
    std::chrono::system_clock::time_point current_time;
    int current_bar = -1;
    bool is_synchronized = false;
    std::vector<int> data_positions;
    
    void reset() {
        current_bar = -1;
        is_synchronized = false;
        data_positions.clear();
    }
};

/**
 * @brief Core LineIterator class - manages iteration and synchronization
 * 
 * The LineIterator is responsible for:
 * - Dual execution modes: Next (normal) and Once (initialization)
 * - Minimum period handling before strategy execution begins
 * - Multi-data synchronization ensuring all data feeds are aligned
 * - Performance optimization through batch processing and caching
 */
class LineIterator : public meta::MetaParams<LineIterator> {
private:
    IteratorMode mode_;
    int minimum_period_;
    int current_period_;
    SyncState sync_state_;
    
    std::vector<std::shared_ptr<LineBuffer>> data_buffers_;
    std::vector<std::function<void()>> next_callbacks_;
    std::vector<std::function<void()>> once_callbacks_;
    std::vector<std::function<void()>> prenext_callbacks_;
    std::vector<std::function<void()>> postnext_callbacks_;
    
    bool is_running_;
    bool warmup_complete_;
    std::optional<int> max_iterations_;
    
    // Performance optimization
    mutable std::vector<double> cache_buffer_;
    bool enable_caching_;
    
    // Synchronization helpers
    void synchronize_data();
    bool check_minimum_period() const;
    void advance_all_buffers();
    void execute_stage_callbacks(IterationStage stage);
    
protected:
    void initializeParams() override;
    
public:
    /**
     * @brief Constructor
     */
    LineIterator();
    
    /**
     * @brief Destructor
     */
    virtual ~LineIterator() = default;
    
    // ========== Configuration ==========
    
    /**
     * @brief Set execution mode
     * @param mode Next or Once mode
     */
    void setMode(IteratorMode mode) { mode_ = mode; }
    
    /**
     * @brief Get current execution mode
     */
    IteratorMode getMode() const { return mode_; }
    
    /**
     * @brief Set minimum period before execution begins
     * @param period Minimum number of data points required
     */
    void setMinimumPeriod(int period) { minimum_period_ = period; }
    
    /**
     * @brief Get minimum period
     */
    int getMinimumPeriod() const { return minimum_period_; }
    
    /**
     * @brief Enable/disable performance caching
     * @param enable Whether to enable caching
     */
    void enableCaching(bool enable) { enable_caching_ = enable; }
    
    /**
     * @brief Set maximum iterations (for testing/debugging)
     * @param max_iter Maximum number of iterations, nullopt for unlimited
     */
    void setMaxIterations(std::optional<int> max_iter) { max_iterations_ = max_iter; }
    
    // ========== Data Management ==========
    
    /**
     * @brief Add data buffer to iteration
     * @param buffer LineBuffer to add
     */
    void addDataBuffer(std::shared_ptr<LineBuffer> buffer);
    
    /**
     * @brief Remove data buffer
     * @param buffer LineBuffer to remove
     */
    void removeDataBuffer(std::shared_ptr<LineBuffer> buffer);
    
    /**
     * @brief Get all data buffers
     */
    const std::vector<std::shared_ptr<LineBuffer>>& getDataBuffers() const { return data_buffers_; }
    
    /**
     * @brief Clear all data buffers
     */
    void clearDataBuffers() { data_buffers_.clear(); }
    
    // ========== Callback Registration ==========
    
    /**
     * @brief Register next() callback
     * @param callback Function to call during next() execution
     */
    void addNextCallback(std::function<void()> callback) {
        next_callbacks_.push_back(std::move(callback));
    }
    
    /**
     * @brief Register once() callback
     * @param callback Function to call during once() execution
     */
    void addOnceCallback(std::function<void()> callback) {
        once_callbacks_.push_back(std::move(callback));
    }
    
    /**
     * @brief Register prenext() callback
     * @param callback Function to call before next() execution
     */
    void addPreNextCallback(std::function<void()> callback) {
        prenext_callbacks_.push_back(std::move(callback));
    }
    
    /**
     * @brief Register postnext() callback
     * @param callback Function to call after next() execution
     */
    void addPostNextCallback(std::function<void()> callback) {
        postnext_callbacks_.push_back(std::move(callback));
    }
    
    /**
     * @brief Clear all callbacks
     */
    void clearCallbacks() {
        next_callbacks_.clear();
        once_callbacks_.clear();
        prenext_callbacks_.clear();
        postnext_callbacks_.clear();
    }
    
    // ========== Execution Control ==========
    
    /**
     * @brief Start iteration process
     * @return true if started successfully
     */
    bool start();
    
    /**
     * @brief Stop iteration process
     */
    void stop();
    
    /**
     * @brief Execute single iteration step
     * @return true if step executed, false if finished
     */
    bool step();
    
    /**
     * @brief Run complete iteration until finished
     * @return Number of iterations executed
     */
    int run();
    
    /**
     * @brief Reset iterator to initial state
     */
    void reset();
    
    /**
     * @brief Check if iterator is currently running
     */
    bool isRunning() const { return is_running_; }
    
    /**
     * @brief Check if warmup period is complete
     */
    bool isWarmupComplete() const { return warmup_complete_; }
    
    // ========== State Information ==========
    
    /**
     * @brief Get current period/bar number
     */
    int getCurrentPeriod() const { return current_period_; }
    
    /**
     * @brief Get synchronization state
     */
    const SyncState& getSyncState() const { return sync_state_; }
    
    /**
     * @brief Check if all data is synchronized
     */
    bool isSynchronized() const { return sync_state_.is_synchronized; }
    
    /**
     * @brief Get progress percentage (0.0 - 1.0)
     * @return Progress as fraction, or -1.0 if undetermined
     */
    double getProgress() const;
    
    // ========== Advanced Features ==========
    
    /**
     * @brief Execute with custom time range
     * @param start_time Start time for execution
     * @param end_time End time for execution
     * @return Number of iterations executed
     */
    int runTimeRange(std::chrono::system_clock::time_point start_time,
                     std::chrono::system_clock::time_point end_time);
    
    /**
     * @brief Batch process multiple periods at once
     * @param batch_size Number of periods to process together
     * @return Number of batches processed
     */
    int runBatch(int batch_size = 10);
    
    /**
     * @brief Get cached data for performance optimization
     * @param buffer_index Index of data buffer
     * @param periods Number of historical periods
     * @return Cached data vector
     */
    const std::vector<double>& getCachedData(int buffer_index, int periods) const;
    
    // ========== Utility Methods ==========
    
    /**
     * @brief Print iterator state for debugging
     */
    void printState() const;
    
    /**
     * @brief Get statistics about iteration performance
     */
    struct IterationStats {
        int total_iterations = 0;
        int successful_steps = 0;
        int failed_steps = 0;
        std::chrono::milliseconds total_time{0};
        std::chrono::milliseconds avg_step_time{0};
    };
    
    IterationStats getStats() const;
    
    // ========== C++20 Features ==========
    
    /**
     * @brief Add data source with concept checking
     * @param data Data source
     */
    template<typename T>
    void addDataSource(std::shared_ptr<T> data) {
        static_assert(std::is_base_of_v<LineBuffer, T>, "Data must derive from LineBuffer");
        addDataBuffer(std::static_pointer_cast<LineBuffer>(data));
    }
    
    /**
     * @brief Range-based iteration over data buffers
     */
    auto getDataRange() const {
        return data_buffers_ | std::views::filter([](const auto& buf) { return buf != nullptr; });
    }
    
    /**
     * @brief Simple async iteration support (placeholder for future coroutine implementation)
     */
    std::function<bool()> getAsyncIterator() {
        return [this]() -> bool {
            return step();
        };
    }

private:
    // Statistics tracking
    mutable IterationStats stats_;
    mutable std::chrono::steady_clock::time_point last_step_time_;
};

// ========== Template Implementations ==========

inline double LineIterator::getProgress() const {
    if (!max_iterations_.has_value()) {
        return -1.0; // Undetermined
    }
    
    return static_cast<double>(current_period_) / static_cast<double>(max_iterations_.value());
}

} // namespace line
} // namespace backtrader