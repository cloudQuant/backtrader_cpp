#include "../../include/feeds/oanda.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <sstream>

namespace backtrader {
namespace feeds {

OandaData::OandaData(const Params& params) : p(params), is_live_(false) {
    if (!p.store) {
        throw std::invalid_argument("Oanda store is required");
    }
    
    store_ = p.store;
    instrument_ = p.instrument;
    granularity_ = p.granularity;
    
    // Initialize data lines (datetime, open, high, low, close, volume)
    lines.resize(6);
    
    // Set up parameters
    params["instrument"] = std::any(instrument_);
    params["granularity"] = std::any(granularity_);
}

bool OandaData::start() {
    try {
        if (p.historical) {
            // Load historical data
            load_historical_data();
        }
        
        if (p.live) {
            // Start live data streaming
            is_live_ = true;
            start_live_data();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to start Oanda feed: " << e.what() << std::endl;
        }
        return false;
    }
}

void OandaData::stop() {
    is_live_ = false;
    
    if (live_thread_.joinable()) {
        live_thread_.join();
    }
}

bool OandaData::load() {
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
            const auto& tick = live_data_queue_.front();
            live_data_queue_.pop();
            
            // Convert tick to OHLCV bar (simplified)
            double price = (tick.bid + tick.ask) / 2.0;
            
            lines[0].push_back(tick.timestamp);  // datetime
            lines[1].push_back(price);           // open
            lines[2].push_back(price);           // high
            lines[3].push_back(price);           // low
            lines[4].push_back(price);           // close
            lines[5].push_back(1.0);             // volume (not available for ticks)
            
            return true;
        }
    }
    
    return false; // No more data
}

void OandaData::load_historical_data() {
    try {
        // Build parameters for historical data request
        std::map<std::string, std::any> params;
        params["instrument"] = instrument_;
        params["granularity"] = granularity_;
        
        if (p.count > 0) {
            params["count"] = p.count;
        }
        
        if (!p.from.empty()) {
            params["from"] = p.from;
        }
        
        if (!p.to.empty()) {
            params["to"] = p.to;
        }
        
        params["price"] = p.price_component; // "M" for mid, "B" for bid, "A" for ask
        params["includeFirst"] = p.include_first;
        params["dailyAlignment"] = p.daily_alignment;
        params["alignmentTimezone"] = p.alignment_timezone;
        params["weeklyAlignment"] = p.weekly_alignment;
        
        // Request historical data from Oanda
        auto result = store_->get_history(params);
        
        if (result.find("candles") != result.end()) {
            auto candles = std::any_cast<std::vector<std::any>>(result["candles"]);
            
            historical_data_.clear();
            historical_data_.reserve(candles.size());
            
            for (const auto& candle_any : candles) {
                auto candle = std::any_cast<std::map<std::string, std::any>>(candle_any);
                
                // Skip incomplete candles if requested
                if (!p.include_incomplete && 
                    candle.find("complete") != candle.end() && 
                    !std::any_cast<bool>(candle["complete"])) {
                    continue;
                }
                
                OHLCVBar bar;
                
                // Parse timestamp
                if (candle.find("time") != candle.end()) {
                    std::string time_str = std::any_cast<std::string>(candle["time"]);
                    bar.timestamp = parse_oanda_time(time_str);
                }
                
                // Parse price data based on component
                if (candle.find(p.price_component) != candle.end()) {
                    auto price_data = std::any_cast<std::map<std::string, std::any>>(
                        candle[p.price_component]);
                    
                    bar.open = std::any_cast<double>(price_data["o"]);
                    bar.high = std::any_cast<double>(price_data["h"]);
                    bar.low = std::any_cast<double>(price_data["l"]);
                    bar.close = std::any_cast<double>(price_data["c"]);
                }
                
                // Volume (tick count for Oanda)
                if (candle.find("volume") != candle.end()) {
                    bar.volume = std::any_cast<double>(candle["volume"]);
                } else {
                    bar.volume = 1.0; // Default
                }
                
                historical_data_.push_back(bar);
            }
        }
        
        historical_index_ = 0;
        
        if (p.debug) {
            std::cout << "Loaded " << historical_data_.size() 
                      << " historical candles for " << instrument_ << std::endl;
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error loading historical data: " << e.what() << std::endl;
        }
    }
}

void OandaData::start_live_data() {
    live_thread_ = std::thread([this]() {
        live_data_loop();
    });
}

void OandaData::live_data_loop() {
    try {
        // Open streaming connection
        std::map<std::string, std::any> stream_params;
        stream_params["instruments"] = instrument_;
        stream_params["snapshot"] = p.snapshot;
        
        auto stream = store_->create_stream(stream_params);
        
        if (p.debug) {
            std::cout << "Started live streaming for " << instrument_ << std::endl;
        }
        
        while (is_live_) {
            try {
                // Get next price update from stream
                auto tick_data = store_->get_next_tick(stream);
                
                if (tick_data.find("type") != tick_data.end()) {
                    std::string type = std::any_cast<std::string>(tick_data["type"]);
                    
                    if (type == "PRICE") {
                        TickData tick;
                        
                        // Parse timestamp
                        if (tick_data.find("time") != tick_data.end()) {
                            std::string time_str = std::any_cast<std::string>(tick_data["time"]);
                            tick.timestamp = parse_oanda_time(time_str);
                        }
                        
                        // Parse bid/ask prices
                        if (tick_data.find("bids") != tick_data.end()) {
                            auto bids = std::any_cast<std::vector<std::any>>(tick_data["bids"]);
                            if (!bids.empty()) {
                                auto bid_data = std::any_cast<std::map<std::string, std::any>>(bids[0]);
                                tick.bid = std::any_cast<double>(bid_data["price"]);
                            }
                        }
                        
                        if (tick_data.find("asks") != tick_data.end()) {
                            auto asks = std::any_cast<std::vector<std::any>>(tick_data["asks"]);
                            if (!asks.empty()) {
                                auto ask_data = std::any_cast<std::map<std::string, std::any>>(asks[0]);
                                tick.ask = std::any_cast<double>(ask_data["price"]);
                            }
                        }
                        
                        // Add to queue
                        {
                            std::lock_guard<std::mutex> lock(live_data_mutex_);
                            live_data_queue_.push(tick);
                        }
                        
                        if (p.debug && live_data_queue_.size() % 100 == 0) {
                            std::cout << "Received " << live_data_queue_.size() 
                                      << " ticks for " << instrument_ << std::endl;
                        }
                    }
                }
                
            } catch (const std::exception& e) {
                if (p.debug) {
                    std::cerr << "Error in live data stream: " << e.what() << std::endl;
                }
                
                // Wait before retrying
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        
        // Close stream
        store_->close_stream(stream);
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error starting live data stream: " << e.what() << std::endl;
        }
    }
}

double OandaData::parse_oanda_time(const std::string& time_str) const {
    // Parse Oanda RFC3339 timestamp format
    // Example: "2023-01-15T10:30:00.000000000Z"
    
    std::tm tm = {};
    std::istringstream ss(time_str);
    
    // Parse the main part (without nanoseconds and timezone)
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        // Fallback to current time
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }
    
    // Convert to timestamp
    return static_cast<double>(std::mktime(&tm));
}

std::string OandaData::format_oanda_time(double timestamp) const {
    std::time_t time_t_val = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::gmtime(&time_t_val);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S.000000000Z");
    return oss.str();
}

bool OandaData::islive() const {
    return p.live && is_live_;
}

std::string OandaData::get_instrument() const {
    return instrument_;
}

std::string OandaData::get_granularity() const {
    return granularity_;
}

size_t OandaData::get_historical_size() const {
    return historical_data_.size();
}

size_t OandaData::get_live_queue_size() const {
    std::lock_guard<std::mutex> lock(live_data_mutex_);
    return live_data_queue_.size();
}

double OandaData::get_last_price() const {
    if (!lines.empty() && !lines[4].empty()) {
        return lines[4].back(); // Last close price
    }
    return 0.0;
}

OandaData::OHLCVBar OandaData::get_last_bar() const {
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

OandaData::TickData OandaData::get_last_tick() const {
    std::lock_guard<std::mutex> lock(live_data_mutex_);
    
    if (!live_data_queue_.empty()) {
        return live_data_queue_.back();
    }
    
    TickData tick;
    tick.timestamp = 0.0;
    tick.bid = 0.0;
    tick.ask = 0.0;
    return tick;
}

void OandaData::set_live_mode(bool live) {
    if (live && !is_live_) {
        // Start live mode
        try {
            start_live_data();
            is_live_ = true;
        } catch (const std::exception& e) {
            if (p.debug) {
                std::cerr << "Failed to start live mode: " << e.what() << std::endl;
            }
        }
    } else if (!live && is_live_) {
        // Stop live mode
        stop();
    }
}

std::map<std::string, double> OandaData::get_current_prices() const {
    try {
        std::map<std::string, std::any> params;
        params["instruments"] = instrument_;
        
        auto result = store_->get_pricing(params);
        
        std::map<std::string, double> prices;
        
        if (result.find("prices") != result.end()) {
            auto prices_list = std::any_cast<std::vector<std::any>>(result["prices"]);
            
            if (!prices_list.empty()) {
                auto price_data = std::any_cast<std::map<std::string, std::any>>(prices_list[0]);
                
                if (price_data.find("bids") != price_data.end()) {
                    auto bids = std::any_cast<std::vector<std::any>>(price_data["bids"]);
                    if (!bids.empty()) {
                        auto bid = std::any_cast<std::map<std::string, std::any>>(bids[0]);
                        prices["bid"] = std::any_cast<double>(bid["price"]);
                    }
                }
                
                if (price_data.find("asks") != price_data.end()) {
                    auto asks = std::any_cast<std::vector<std::any>>(price_data["asks"]);
                    if (!asks.empty()) {
                        auto ask = std::any_cast<std::map<std::string, std::any>>(asks[0]);
                        prices["ask"] = std::any_cast<double>(ask["price"]);
                    }
                }
                
                if (prices.find("bid") != prices.end() && prices.find("ask") != prices.end()) {
                    prices["mid"] = (prices["bid"] + prices["ask"]) / 2.0;
                }
                
                if (price_data.find("closeoutBid") != price_data.end()) {
                    prices["closeout_bid"] = std::any_cast<double>(price_data["closeoutBid"]);
                }
                
                if (price_data.find("closeoutAsk") != price_data.end()) {
                    prices["closeout_ask"] = std::any_cast<double>(price_data["closeoutAsk"]);
                }
            }
        }
        
        return prices;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error getting current prices: " << e.what() << std::endl;
        }
        return {};
    }
}

std::vector<std::string> OandaData::get_available_instruments() const {
    try {
        auto result = store_->get_instruments();
        
        std::vector<std::string> instruments;
        
        if (result.find("instruments") != result.end()) {
            auto instruments_list = std::any_cast<std::vector<std::any>>(result["instruments"]);
            
            for (const auto& inst_any : instruments_list) {
                auto inst_data = std::any_cast<std::map<std::string, std::any>>(inst_any);
                
                if (inst_data.find("name") != inst_data.end()) {
                    instruments.push_back(std::any_cast<std::string>(inst_data["name"]));
                }
            }
        }
        
        return instruments;
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error getting available instruments: " << e.what() << std::endl;
        }
        return {};
    }
}

double OandaData::get_conversion_rate(const std::string& from_currency, 
                                     const std::string& to_currency) const {
    if (from_currency == to_currency) {
        return 1.0;
    }
    
    try {
        std::string conversion_instrument = from_currency + "_" + to_currency;
        
        std::map<std::string, std::any> params;
        params["instruments"] = conversion_instrument;
        
        auto result = store_->get_pricing(params);
        
        if (result.find("prices") != result.end()) {
            auto prices_list = std::any_cast<std::vector<std::any>>(result["prices"]);
            
            if (!prices_list.empty()) {
                auto price_data = std::any_cast<std::map<std::string, std::any>>(prices_list[0]);
                
                // Use mid price for conversion
                if (price_data.find("bids") != price_data.end() && 
                    price_data.find("asks") != price_data.end()) {
                    
                    auto bids = std::any_cast<std::vector<std::any>>(price_data["bids"]);
                    auto asks = std::any_cast<std::vector<std::any>>(price_data["asks"]);
                    
                    if (!bids.empty() && !asks.empty()) {
                        auto bid = std::any_cast<std::map<std::string, std::any>>(bids[0]);
                        auto ask = std::any_cast<std::map<std::string, std::any>>(asks[0]);
                        
                        double bid_price = std::any_cast<double>(bid["price"]);
                        double ask_price = std::any_cast<double>(ask["price"]);
                        
                        return (bid_price + ask_price) / 2.0;
                    }
                }
            }
        }
        
        return 1.0; // Fallback
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error getting conversion rate: " << e.what() << std::endl;
        }
        return 1.0;
    }
}

} // namespace feeds
} // namespace backtrader