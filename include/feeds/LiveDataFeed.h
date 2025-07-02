#pragma once

#include "DataFeed.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <future>
#include <random>
#include <iomanip>
#include <sstream>

namespace backtrader {
namespace data {

/**
 * @brief 实时数据连接状态
 */
enum class ConnectionStatus {
    DISCONNECTED,   // 未连接
    CONNECTING,     // 连接中
    CONNECTED,      // 已连接
    RECONNECTING,   // 重连中
    ERROR          // 错误状态
};

/**
 * @brief 数据质量指标
 */
struct DataQuality {
    size_t total_ticks;          // 总tick数
    size_t missing_ticks;        // 丢失tick数
    size_t duplicate_ticks;      // 重复tick数
    double avg_latency_ms;       // 平均延迟(毫秒)
    double max_latency_ms;       // 最大延迟(毫秒)
    std::chrono::system_clock::time_point last_update;
    
    DataQuality() : total_ticks(0), missing_ticks(0), duplicate_ticks(0),
                   avg_latency_ms(0.0), max_latency_ms(0.0) {}
};

/**
 * @brief 实时数据提供者接口
 */
class LiveDataProvider {
public:
    virtual ~LiveDataProvider() = default;
    
    /**
     * @brief 连接到数据源
     * @return 是否成功连接
     */
    virtual bool connect() = 0;
    
    /**
     * @brief 断开连接
     */
    virtual void disconnect() = 0;
    
    /**
     * @brief 订阅品种
     * @param symbol 品种代码
     * @return 是否成功订阅
     */
    virtual bool subscribe(const std::string& symbol) = 0;
    
    /**
     * @brief 取消订阅
     * @param symbol 品种代码
     */
    virtual void unsubscribe(const std::string& symbol) = 0;
    
    /**
     * @brief 检查连接状态
     * @return 连接状态
     */
    virtual ConnectionStatus getStatus() const = 0;
    
    /**
     * @brief 设置数据回调
     * @param callback 数据回调函数
     */
    virtual void setDataCallback(std::function<void(const BarData&)> callback) = 0;
    
    /**
     * @brief 设置状态回调
     * @param callback 状态回调函数
     */
    virtual void setStatusCallback(std::function<void(ConnectionStatus)> callback) = 0;
};

/**
 * @brief 模拟实时数据提供者（用于测试）
 */
class SimulatedLiveProvider : public LiveDataProvider {
private:
    ConnectionStatus status_;
    std::thread data_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> should_stop_;
    
    std::function<void(const BarData&)> data_callback_;
    std::function<void(ConnectionStatus)> status_callback_;
    
    std::string symbol_;
    double base_price_;
    double volatility_;
    std::chrono::milliseconds interval_;
    
    // 随机数生成
    mutable std::mt19937 rng_;
    mutable std::normal_distribution<double> normal_dist_;
    
public:
    /**
     * @brief 构造函数
     * @param base_price 基础价格
     * @param volatility 波动率
     * @param interval_ms 数据间隔(毫秒)
     */
    explicit SimulatedLiveProvider(double base_price = 100.0, 
                                  double volatility = 0.02,
                                  int interval_ms = 1000)
        : status_(ConnectionStatus::DISCONNECTED),
          running_(false),
          should_stop_(false),
          base_price_(base_price),
          volatility_(volatility),
          interval_(interval_ms),
          rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
          normal_dist_(0.0, 1.0) {}
    
    ~SimulatedLiveProvider() {
        disconnect();
    }
    
    bool connect() override {
        if (status_ == ConnectionStatus::CONNECTED) {
            return true;
        }
        
        status_ = ConnectionStatus::CONNECTING;
        if (status_callback_) {
            status_callback_(status_);
        }
        
        // 模拟连接延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        status_ = ConnectionStatus::CONNECTED;
        if (status_callback_) {
            status_callback_(status_);
        }
        
        return true;
    }
    
    void disconnect() override {
        should_stop_ = true;
        
        if (data_thread_.joinable()) {
            data_thread_.join();
        }
        
        running_ = false;
        status_ = ConnectionStatus::DISCONNECTED;
        
        if (status_callback_) {
            status_callback_(status_);
        }
    }
    
    bool subscribe(const std::string& symbol) override {
        if (status_ != ConnectionStatus::CONNECTED) {
            return false;
        }
        
        symbol_ = symbol;
        should_stop_ = false;
        running_ = true;
        
        // 启动数据生成线程
        data_thread_ = std::thread([this]() {
            generateData();
        });
        
        return true;
    }
    
    void unsubscribe(const std::string& symbol) override {
        should_stop_ = true;
        
        if (data_thread_.joinable()) {
            data_thread_.join();
        }
        
        running_ = false;
    }
    
    ConnectionStatus getStatus() const override {
        return status_;
    }
    
    void setDataCallback(std::function<void(const BarData&)> callback) override {
        data_callback_ = callback;
    }
    
    void setStatusCallback(std::function<void(ConnectionStatus)> callback) override {
        status_callback_ = callback;
    }
    
private:
    void generateData() {
        double current_price = base_price_;
        
        while (!should_stop_) {
            // 生成随机价格变动
            double change = normal_dist_(rng_) * volatility_ * current_price;
            current_price += change;
            current_price = std::max(0.01, current_price); // 确保价格为正
            
            // 生成OHLC数据
            double high = current_price * (1.0 + std::abs(normal_dist_(rng_)) * 0.01);
            double low = current_price * (1.0 - std::abs(normal_dist_(rng_)) * 0.01);
            double open = low + (high - low) * (0.2 + 0.6 * std::uniform_real_distribution<double>(0, 1)(rng_));
            double close = current_price;
            double volume = 1000 + std::abs(normal_dist_(rng_)) * 500;
            
            // 创建BarData
            BarData bar;
            bar.timestamp = std::chrono::system_clock::now();
            bar.open = open;
            bar.high = high;
            bar.low = low;
            bar.close = close;
            bar.volume = volume;
            
            if (data_callback_) {
                data_callback_(bar);
            }
            
            std::this_thread::sleep_for(interval_);
        }
    }
};

/**
 * @brief 实时数据源
 * 
 * 支持实时数据接收、缓存、质量监控和自动重连
 */
class LiveDataFeed : public DataFeed {
private:
    std::unique_ptr<LiveDataProvider> provider_;
    std::string symbol_;
    
    // 数据缓存
    std::queue<BarData> data_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable data_available_;
    size_t max_buffer_size_;
    
    // 连接管理
    ConnectionStatus status_;
    std::atomic<bool> auto_reconnect_;
    std::chrono::seconds reconnect_interval_;
    std::thread reconnect_thread_;
    std::atomic<bool> should_stop_;
    
    // 数据质量监控
    DataQuality quality_stats_;
    mutable std::mutex quality_mutex_;
    std::chrono::system_clock::time_point last_data_time_;
    std::chrono::system_clock::time_point connection_start_time_;
    
    // 回调函数
    std::function<void(const std::string&)> error_callback_;
    std::function<void(ConnectionStatus)> status_callback_;
    
    // 性能统计
    std::atomic<size_t> total_received_;
    std::atomic<size_t> total_processed_;
    
public:
    /**
     * @brief 构造函数
     * @param provider 数据提供者
     * @param symbol 品种代码
     * @param name 数据源名称
     */
    explicit LiveDataFeed(std::unique_ptr<LiveDataProvider> provider,
                         const std::string& symbol,
                         const std::string& name = "LiveDataFeed")
        : DataFeed(name),
          provider_(std::move(provider)),
          symbol_(symbol),
          max_buffer_size_(10000),
          status_(ConnectionStatus::DISCONNECTED),
          auto_reconnect_(true),
          reconnect_interval_(5),
          should_stop_(false),
          total_received_(0),
          total_processed_(0) {
        
        if (provider_) {
            // 设置回调函数
            provider_->setDataCallback([this](const BarData& bar) {
                onDataReceived(bar);
            });
            
            provider_->setStatusCallback([this](ConnectionStatus status) {
                onStatusChanged(status);
            });
        }
    }
    
    ~LiveDataFeed() {
        stop();
    }
    
    /**
     * @brief 启动实时数据接收
     * @return 是否成功启动
     */
    bool start() {
        if (!provider_) {
            return false;
        }
        
        should_stop_ = false;
        connection_start_time_ = std::chrono::system_clock::now();
        
        if (!provider_->connect()) {
            return false;
        }
        
        if (!provider_->subscribe(symbol_)) {
            provider_->disconnect();
            return false;
        }
        
        // 启动重连线程
        if (auto_reconnect_) {
            reconnect_thread_ = std::thread([this]() {
                reconnectLoop();
            });
        }
        
        return true;
    }
    
    /**
     * @brief 停止数据接收
     */
    void stop() {
        should_stop_ = true;
        
        if (provider_) {
            provider_->unsubscribe(symbol_);
            provider_->disconnect();
        }
        
        data_available_.notify_all();
        
        if (reconnect_thread_.joinable()) {
            reconnect_thread_.join();
        }
    }
    
    /**
     * @brief 检查是否有下一个数据
     * @return true if has next
     */
    bool hasNext() const override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return !data_queue_.empty();
    }
    
    /**
     * @brief 获取下一个数据
     * @return true if successful
     */
    bool next() override {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // 等待数据可用或超时
        if (data_queue_.empty()) {
            data_available_.wait_for(lock, std::chrono::seconds(1), [this]() {
                return !data_queue_.empty() || should_stop_;
            });
        }
        
        if (data_queue_.empty()) {
            return false;
        }
        
        BarData bar = data_queue_.front();
        data_queue_.pop();
        lock.unlock();
        
        // 更新数据线
        updateDataLines(bar);
        total_processed_++;
        
        return true;
    }
    
    /**
     * @brief 等待数据（阻塞）
     * @param timeout_ms 超时时间(毫秒)
     * @return true if data available
     */
    bool waitForData(int timeout_ms = 5000) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return data_available_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
            return !data_queue_.empty() || should_stop_;
        });
    }
    
    /**
     * @brief 获取连接状态
     * @return 连接状态
     */
    ConnectionStatus getConnectionStatus() const {
        return status_;
    }
    
    /**
     * @brief 获取数据质量统计
     * @return 数据质量信息
     */
    DataQuality getDataQuality() const {
        std::lock_guard<std::mutex> lock(quality_mutex_);
        return quality_stats_;
    }
    
    /**
     * @brief 设置自动重连
     * @param enable 是否启用
     * @param interval_seconds 重连间隔(秒)
     */
    void setAutoReconnect(bool enable, int interval_seconds = 5) {
        auto_reconnect_ = enable;
        reconnect_interval_ = std::chrono::seconds(interval_seconds);
    }
    
    /**
     * @brief 设置缓存大小
     * @param size 最大缓存数量
     */
    void setMaxBufferSize(size_t size) {
        max_buffer_size_ = size;
    }
    
    /**
     * @brief 设置错误回调
     * @param callback 错误回调函数
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        error_callback_ = callback;
    }
    
    /**
     * @brief 设置状态回调
     * @param callback 状态回调函数
     */
    void setStatusCallback(std::function<void(ConnectionStatus)> callback) {
        status_callback_ = callback;
    }
    
    /**
     * @brief 获取缓存队列大小
     * @return 队列大小
     */
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return data_queue_.size();
    }
    
    /**
     * @brief 获取性能统计
     * @return 统计信息
     */
    std::map<std::string, double> getPerformanceStats() const {
        std::map<std::string, double> stats;
        stats["total_received"] = static_cast<double>(total_received_.load());
        stats["total_processed"] = static_cast<double>(total_processed_.load());
        stats["queue_size"] = static_cast<double>(getQueueSize());
        stats["processing_rate"] = (total_received_ > 0) ? 
            static_cast<double>(total_processed_) / total_received_ : 0.0;
        
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - connection_start_time_).count();
        
        if (duration > 0) {
            stats["throughput_per_second"] = static_cast<double>(total_processed_) / duration;
        }
        
        return stats;
    }
    
    /**
     * @brief 清空缓存
     */
    void clearBuffer() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!data_queue_.empty()) {
            data_queue_.pop();
        }
    }
    
    /**
     * @brief 手动重连
     * @return 是否成功重连
     */
    bool reconnect() {
        if (provider_) {
            provider_->disconnect();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (provider_->connect()) {
                return provider_->subscribe(symbol_);
            }
        }
        return false;
    }
    
private:
    /**
     * @brief 数据接收回调
     * @param bar 接收到的数据
     */
    void onDataReceived(const BarData& bar) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // 检查缓存大小
        if (data_queue_.size() >= max_buffer_size_) {
            data_queue_.pop(); // 移除最老的数据
        }
        
        data_queue_.push(bar);
        total_received_++;
        
        // 更新质量统计
        updateQualityStats(bar);
        
        data_available_.notify_one();
    }
    
    /**
     * @brief 状态变化回调
     * @param status 新状态
     */
    void onStatusChanged(ConnectionStatus status) {
        status_ = status;
        
        if (status_callback_) {
            status_callback_(status);
        }
        
        if (status == ConnectionStatus::ERROR && error_callback_) {
            error_callback_("Connection error occurred");
        }
    }
    
    /**
     * @brief 更新质量统计
     * @param bar 数据
     */
    void updateQualityStats(const BarData& bar) {
        std::lock_guard<std::mutex> lock(quality_mutex_);
        
        quality_stats_.total_ticks++;
        quality_stats_.last_update = bar.timestamp;
        
        // 计算延迟
        auto now = std::chrono::system_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - bar.timestamp).count();
        
        if (latency > 0) {
            double latency_ms = static_cast<double>(latency);
            quality_stats_.avg_latency_ms = 
                (quality_stats_.avg_latency_ms * (quality_stats_.total_ticks - 1) + latency_ms) 
                / quality_stats_.total_ticks;
            quality_stats_.max_latency_ms = std::max(quality_stats_.max_latency_ms, latency_ms);
        }
        
        // 检查数据间隔（简化的丢失检测）
        if (last_data_time_ != std::chrono::system_clock::time_point{}) {
            auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(
                bar.timestamp - last_data_time_).count();
            
            // 如果间隔超过预期的2倍，可能有丢失
            if (gap > 2000) { // 假设期望间隔为1秒
                quality_stats_.missing_ticks++;
            }
        }
        
        last_data_time_ = bar.timestamp;
    }
    
    /**
     * @brief 重连循环
     */
    void reconnectLoop() {
        while (!should_stop_) {
            std::this_thread::sleep_for(reconnect_interval_);
            
            if (should_stop_) break;
            
            if (status_ == ConnectionStatus::ERROR || 
                status_ == ConnectionStatus::DISCONNECTED) {
                
                if (error_callback_) {
                    error_callback_("Attempting to reconnect...");
                }
                
                if (reconnect()) {
                    if (error_callback_) {
                        error_callback_("Reconnection successful");
                    }
                }
            }
        }
    }
    
    /**
     * @brief 更新数据线
     * @param bar 数据
     */
    void updateDataLines(const BarData& bar) {
        if (open_line_) open_line_->forward(bar.open);
        if (high_line_) high_line_->forward(bar.high);
        if (low_line_) low_line_->forward(bar.low);
        if (close_line_) close_line_->forward(bar.close);
        if (volume_line_) volume_line_->forward(bar.volume);
        
        current_data_ = bar;
    }
};

/**
 * @brief 实时数据源工厂
 */
class LiveDataFeedFactory {
public:
    /**
     * @brief 创建模拟实时数据源
     * @param symbol 品种代码
     * @param base_price 基础价格
     * @param volatility 波动率
     * @param interval_ms 数据间隔(毫秒)
     * @return 实时数据源
     */
    static std::unique_ptr<LiveDataFeed> createSimulated(
        const std::string& symbol,
        double base_price = 100.0,
        double volatility = 0.02,
        int interval_ms = 1000) {
        
        auto provider = std::make_unique<SimulatedLiveProvider>(
            base_price, volatility, interval_ms);
        
        return std::make_unique<LiveDataFeed>(
            std::move(provider), symbol, "SimulatedLive_" + symbol);
    }
};

} // namespace data
} // namespace backtrader