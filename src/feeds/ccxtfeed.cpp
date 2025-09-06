#include "../../include/feeds/ccxtfeed.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>

namespace backtrader {
namespace feeds {

CCXTFeed::CCXTFeed(const Params& params) : p(params), is_live_(false) {
    if (!p.store) {
        throw std::invalid_argument("CCXT store is required");
    }
    
    store_ = p.store;
    symbol_ = p.symbol;
    timeframe_ = p.timeframe;
    compression_ = p.compression;
    
    // Initialize data lines (datetime, open, high, low, close, volume)
    lines.resize(6);
    
    // Set up parameters
    params["symbol"] = std::any(symbol_);
    params["timeframe"] = std::any(timeframe_);
    params["compression"] = std::any(compression_);
}

bool CCXTFeed::start() {
    try {
        // Get timeframe string for CCXT
        std::string tf_str = store_->get_granularity(
            static_cast<TimeFrame>(timeframe_), compression_);
        
        if (p.historical) {
            // Load historical data
            load_historical_data(tf_str);
        }
        
        if (p.live) {
            // Start live data streaming
            is_live_ = true;
            start_live_data(tf_str);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to start CCXT feed: " << e.what() << std::endl;
        }
        return false;
    }
}

void CCXTFeed::stop() {
    is_live_ = false;
    
    if (live_thread_.joinable()) {
        live_thread_.join();
    }
}

bool CCXTFeed::load() {
    if (p.historical && historical_index_ < historical_data_.size()) {
        // Load next historical bar
        const auto& bar = historical_data_[historical_index_++];
        
        lines[0].push_back(bar.timestamp);   // datetime
        lines[1].push_back(bar.open);        // open
        lines[2].push_back(bar.high);        // high
        lines[3].push_back(bar.low);         // low
        lines[4].push_back(bar.close);       // close
        lines[5].push_back(bar.volume);      // volume
        
        return true;
    }
    
    if (p.live && !live_data_queue_.empty()) {
        // Load next live bar
        std::lock_guard<std::mutex> lock(live_data_mutex_);
        
        if (!live_data_queue_.empty()) {
            const auto& bar = live_data_queue_.front();
            live_data_queue_.pop();
            
            lines[0].push_back(bar.timestamp);   // datetime
            lines[1].push_back(bar.open);        // open
            lines[2].push_back(bar.high);        // high
            lines[3].push_back(bar.low);         // low
            lines[4].push_back(bar.close);       // close
            lines[5].push_back(bar.volume);      // volume
            
            return true;
        }
    }
    
    return false; // No more data
}

void CCXTFeed::load_historical_data(const std::string& timeframe_str) {
    try {
        // Calculate since timestamp
        int64_t since = 0;
        if (p.fromdate > 0) {
            since = static_cast<int64_t>(p.fromdate * 1000); // Convert to milliseconds
        }
        
        // Fetch historical OHLCV data
        auto ohlcv_data = store_->fetch_ohlcv(symbol_, timeframe_str, since, p.limit);
        
        historical_data_.clear();
        historical_data_.reserve(ohlcv_data.size());
        
        for (const auto& ohlcv : ohlcv_data) {
            if (ohlcv.size() >= 6) {
                OHLCVBar bar;
                bar.timestamp = ohlcv[0] / 1000.0;  // Convert from milliseconds to seconds
                bar.open = ohlcv[1];
                bar.high = ohlcv[2];
                bar.low = ohlcv[3];
                bar.close = ohlcv[4];
                bar.volume = ohlcv[5];
                
                // Apply date filtering
                if (p.todate > 0 && bar.timestamp > p.todate) {
                    break;
                }
                
                historical_data_.push_back(bar);
            }
        }
        
        historical_index_ = 0;
        
        if (p.debug) {
            std::cout << "Loaded " << historical_data_.size() 
                      << " historical bars for " << symbol_ << std::endl;
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error loading historical data: " << e.what() << std::endl;
        }
    }
}

void CCXTFeed::start_live_data(const std::string& timeframe_str) {
    live_thread_ = std::thread([this, timeframe_str]() {
        live_data_loop(timeframe_str);
    });
}

void CCXTFeed::live_data_loop(const std::string& timeframe_str) {
    auto last_fetch = std::chrono::system_clock::now();
    int64_t last_timestamp = 0;
    
    while (is_live_) {
        try {
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_fetch);
            
            // Fetch new data periodically
            if (elapsed.count() >= p.update_interval) {
                // Get latest bars
                auto ohlcv_data = store_->fetch_ohlcv(symbol_, timeframe_str, 
                                                     last_timestamp * 1000, 10);
                
                for (const auto& ohlcv : ohlcv_data) {
                    if (ohlcv.size() >= 6) {
                        int64_t timestamp_ms = static_cast<int64_t>(ohlcv[0]);
                        
                        // Only add new bars
                        if (timestamp_ms > last_timestamp * 1000) {
                            OHLCVBar bar;
                            bar.timestamp = timestamp_ms / 1000.0;
                            bar.open = ohlcv[1];
                            bar.high = ohlcv[2];
                            bar.low = ohlcv[3];
                            bar.close = ohlcv[4];
                            bar.volume = ohlcv[5];
                            
                            {
                                std::lock_guard<std::mutex> lock(live_data_mutex_);
                                live_data_queue_.push(bar);
                            }
                            
                            last_timestamp = static_cast<int64_t>(bar.timestamp);
                            
                            if (p.debug) {
                                std::cout << "Received live bar for " << symbol_ 
                                          << " at " << bar.timestamp << std::endl;
                            }
                        }
                    }
                }
                
                last_fetch = now;
            }
            
            // Sleep before next check
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
        } catch (const std::exception& e) {
            if (p.debug) {
                std::cerr << "Error in live data loop: " << e.what() << std::endl;
            }
            
            // Wait before retrying
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

bool CCXTFeed::islive() const {
    return p.live && is_live_;
}

std::string CCXTFeed::get_symbol() const {
    return symbol_;
}

int CCXTFeed::get_timeframe() const {
    return timeframe_;
}

int CCXTFeed::get_compression() const {
    return compression_;
}

size_t CCXTFeed::get_historical_size() const {
    return historical_data_.size();
}

size_t CCXTFeed::get_live_queue_size() const {
    std::lock_guard<std::mutex> lock(live_data_mutex_);
    return live_data_queue_.size();
}

double CCXTFeed::get_last_price() const {
    if (!lines.empty() && !lines[4].empty()) {
        return lines[4].back(); // Last close price
    }
    return 0.0;
}

CCXTFeed::OHLCVBar CCXTFeed::get_last_bar() const {
    OHLCVBar bar;
    
    if (!lines.empty() && !lines[0].empty()) {
        size_t last_index = lines[0].size() - 1;
        
        bar.timestamp = lines[0][last_index];
        bar.open = lines[1][last_index];
        bar.high = lines[2][last_index];
        bar.low = lines[3][last_index];
        bar.close = lines[4][last_index];
        bar.volume = lines[5][last_index];
    }
    
    return bar;
}

void CCXTFeed::set_live_mode(bool live) {
    if (live && !is_live_) {
        // Start live mode
        try {
            std::string tf_str = store_->get_granularity(
                static_cast<TimeFrame>(timeframe_), compression_);
            start_live_data(tf_str);
            is_live_ = true;
        } catch (const std::exception& e) {
            if (p.debug) {
                std::cerr << "Failed to start live mode: " << e.what() << std::endl;
            }
        }
    } else if (!live && is_live_) {
        // Stop live mode
        is_live_ = false;
        if (live_thread_.joinable()) {
            live_thread_.join();
        }
    }
}

std::map<std::string, std::any> CCXTFeed::get_ticker() const {
    try {
        return store_->fetch_ticker(symbol_);
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error fetching ticker: " << e.what() << std::endl;
        }
        return {};
    }
}

std::vector<std::vector<double>> CCXTFeed::get_order_book() const {
    // Not implemented in base CCXT store, would need exchange-specific implementation
    return {};
}

void CCXTFeed::resample_data(int new_timeframe, int new_compression) {
    // This would implement data resampling to different timeframes
    // For now, it's a placeholder
    
    if (p.debug) {
        std::cout << "Resampling data from " << timeframe_ << ":" << compression_
                  << " to " << new_timeframe << ":" << new_compression << std::endl;
    }
    
    // Implementation would:
    // 1. Group existing bars by new timeframe
    // 2. Aggregate OHLCV data
    // 3. Replace current data with resampled data
}

void CCXTFeed::apply_filter(std::function<bool(const OHLCVBar&)> filter) {
    if (!p.historical) {
        return;
    }
    
    // Filter historical data
    auto filtered_data = std::vector<OHLCVBar>();
    filtered_data.reserve(historical_data_.size());
    
    for (const auto& bar : historical_data_) {
        if (filter(bar)) {
            filtered_data.push_back(bar);
        }
    }
    
    historical_data_ = std::move(filtered_data);
    historical_index_ = 0;
    
    if (p.debug) {
        std::cout << "Applied filter, " << historical_data_.size() 
                  << " bars remaining" << std::endl;
    }
}

} // namespace feeds
} // namespace backtrader