#include "../../include/feeds/ibdata.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <sstream>

namespace backtrader {
namespace feeds {

IBData::IBData(const Params& params) : p(params), is_live_(false), req_id_(0) {
    if (!p.store) {
        throw std::invalid_argument("IB store is required");
    }
    
    store_ = p.store;
    symbol_ = p.symbol;
    sectype_ = p.sectype;
    exchange_ = p.exchange;
    currency_ = p.currency;
    
    // Initialize data lines (datetime, open, high, low, close, volume)
    lines.resize(6);
    
    // Set up parameters
    params["symbol"] = std::any(symbol_);
    params["sectype"] = std::any(sectype_);
    params["exchange"] = std::any(exchange_);
    params["currency"] = std::any(currency_);
    
    // Create contract
    contract_ = create_contract();
}

bool IBData::start() {
    try {
        if (!store_->isConnected()) {
            throw std::runtime_error("IB store is not connected");
        }
        
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
            std::cerr << "Failed to start IB feed: " << e.what() << std::endl;
        }
        return false;
    }
}

void IBData::stop() {
    is_live_ = false;
    
    if (req_id_ > 0) {
        if (p.live) {
            store_->cancelRealTimeData(req_id_);
        }
        if (p.historical) {
            store_->cancelHistoricalData(req_id_);
        }
    }
    
    if (live_thread_.joinable()) {
        live_thread_.join();
    }
}

bool IBData::load() {
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

IBData::Contract IBData::create_contract() const {
    Contract contract;
    contract.symbol = symbol_;
    contract.secType = sectype_;
    contract.exchange = exchange_;
    contract.currency = currency_;
    
    // Set additional fields based on security type
    if (sectype_ == "STK") {
        // Stock specific settings
        contract.primaryExchange = p.primary_exchange.empty() ? exchange_ : p.primary_exchange;
    } else if (sectype_ == "FUT") {
        // Futures specific settings
        contract.lastTradeDateOrContractMonth = p.expiry;
        contract.multiplier = std::to_string(static_cast<int>(p.multiplier));
    } else if (sectype_ == "OPT") {
        // Options specific settings
        contract.lastTradeDateOrContractMonth = p.expiry;
        contract.strike = p.strike;
        contract.right = p.right; // "C" for call, "P" for put
        contract.multiplier = std::to_string(static_cast<int>(p.multiplier));
    } else if (sectype_ == "FOP") {
        // Futures Options specific settings
        contract.lastTradeDateOrContractMonth = p.expiry;
        contract.strike = p.strike;
        contract.right = p.right;
        contract.multiplier = std::to_string(static_cast<int>(p.multiplier));
    } else if (sectype_ == "CASH") {
        // Forex specific settings
        // For forex pairs like "EUR.USD", IB expects symbol="EUR", currency="USD"
        if (symbol_.find('.') != std::string::npos) {
            size_t dot_pos = symbol_.find('.');
            contract.symbol = symbol_.substr(0, dot_pos);
            contract.currency = symbol_.substr(dot_pos + 1);
        }
    }
    
    return contract;
}

void IBData::load_historical_data() {
    try {
        req_id_ = store_->getNextReqId();
        
        // Build query time string
        std::string query_time = format_ib_datetime(p.todate > 0 ? p.todate : 
                                                   std::chrono::system_clock::to_time_t(
                                                       std::chrono::system_clock::now()));
        
        // Build duration string
        std::string duration = calculate_duration();
        
        // Build bar size string
        std::string bar_size = get_ib_bar_size();
        
        // What to show
        std::string what_to_show = get_what_to_show();
        
        // Request historical data
        auto request = create_historical_request(query_time, duration, bar_size, what_to_show);
        auto result = store_->request_historical_data(req_id_, contract_, request);
        
        // Process result (this would typically be done via callbacks in real IB API)
        if (result.find("bars") != result.end()) {
            auto bars_data = std::any_cast<std::vector<std::any>>(result["bars"]);
            
            historical_data_.clear();
            historical_data_.reserve(bars_data.size());
            
            for (const auto& bar_any : bars_data) {
                auto bar_data = std::any_cast<std::map<std::string, std::any>>(bar_any);
                
                OHLCVBar bar;
                bar.timestamp = std::any_cast<double>(bar_data["timestamp"]);
                bar.open = std::any_cast<double>(bar_data["open"]);
                bar.high = std::any_cast<double>(bar_data["high"]);
                bar.low = std::any_cast<double>(bar_data["low"]);
                bar.close = std::any_cast<double>(bar_data["close"]);
                bar.volume = std::any_cast<double>(bar_data["volume"]);
                
                // Apply date filtering
                if (p.fromdate > 0 && bar.timestamp < p.fromdate) {
                    continue;
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

void IBData::start_live_data() {
    try {
        req_id_ = store_->getNextReqId();
        
        // Request real-time bars
        auto request = create_realtime_request();
        auto result = store_->request_realtime_bars(req_id_, contract_, request);
        
        // Start processing thread
        live_thread_ = std::thread([this]() {
            live_data_loop();
        });
        
        if (p.debug) {
            std::cout << "Started live data for " << symbol_ << std::endl;
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error starting live data: " << e.what() << std::endl;
        }
    }
}

void IBData::live_data_loop() {
    while (is_live_) {
        try {
            // In real implementation, this would receive data via IB callbacks
            // For now, simulate periodic data updates
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Simulate receiving a new bar
            if (is_live_) {
                OHLCVBar bar = simulate_live_bar();
                
                {
                    std::lock_guard<std::mutex> lock(live_data_mutex_);
                    live_data_queue_.push(bar);
                }
                
                if (p.debug) {
                    std::cout << "Received live bar for " << symbol_ 
                              << " at " << bar.timestamp << std::endl;
                }
            }
            
        } catch (const std::exception& e) {
            if (p.debug) {
                std::cerr << "Error in live data loop: " << e.what() << std::endl;
            }
            
            // Wait before retrying
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

std::string IBData::format_ib_datetime(time_t timestamp) const {
    std::tm* tm = std::gmtime(&timestamp);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y%m%d %H:%M:%S");
    return oss.str();
}

std::string IBData::calculate_duration() const {
    if (p.historical_days > 0) {
        return std::to_string(p.historical_days) + " D";
    }
    
    // Calculate based on fromdate and todate
    if (p.fromdate > 0 && p.todate > 0) {
        int days = static_cast<int>((p.todate - p.fromdate) / 86400);
        return std::to_string(days) + " D";
    }
    
    // Default to 30 days
    return "30 D";
}

std::string IBData::get_ib_bar_size() const {
    // Map compression and timeframe to IB bar sizes
    if (p.compression == 1) {
        switch (p.timeframe) {
            case 1: return "1 sec";   // Seconds
            case 2: return "1 min";   // Minutes
            case 3: return "1 hour";  // Hours
            case 4: return "1 day";   // Days
            case 5: return "1 week";  // Weeks
            case 6: return "1 month"; // Months
        }
    } else {
        switch (p.timeframe) {
            case 1: return std::to_string(p.compression) + " secs";
            case 2: return std::to_string(p.compression) + " mins";
            case 3: return std::to_string(p.compression) + " hours";
            case 4: return std::to_string(p.compression) + " days";
            case 5: return std::to_string(p.compression) + " weeks";
            case 6: return std::to_string(p.compression) + " months";
        }
    }
    
    return "1 min"; // Default
}

std::string IBData::get_what_to_show() const {
    if (sectype_ == "STK") {
        return "TRADES";
    } else if (sectype_ == "CASH") {
        return "MIDPOINT";
    } else if (sectype_ == "FUT" || sectype_ == "FOP") {
        return "TRADES";
    } else if (sectype_ == "OPT") {
        return "OPTION_IMPLIED_VOLATILITY";
    }
    
    return "TRADES"; // Default
}

std::map<std::string, std::any> IBData::create_historical_request(
    const std::string& query_time, const std::string& duration,
    const std::string& bar_size, const std::string& what_to_show) const {
    
    std::map<std::string, std::any> request;
    request["endDateTime"] = query_time;
    request["durationStr"] = duration;
    request["barSizeSetting"] = bar_size;
    request["whatToShow"] = what_to_show;
    request["useRTH"] = p.rtvolume ? 1 : 0; // Regular trading hours
    request["formatDate"] = 2; // Unix timestamp
    request["keepUpToDate"] = false;
    
    return request;
}

std::map<std::string, std::any> IBData::create_realtime_request() const {
    std::map<std::string, std::any> request;
    request["barSize"] = 5; // 5 seconds for real-time bars
    request["whatToShow"] = get_what_to_show();
    request["useRTH"] = p.rtvolume ? 1 : 0;
    
    return request;
}

IBData::OHLCVBar IBData::simulate_live_bar() const {
    // This is a simulation for demonstration
    // In real implementation, data would come from IB callbacks
    
    OHLCVBar bar;
    auto now = std::chrono::system_clock::now();
    bar.timestamp = std::chrono::system_clock::to_time_t(now);
    
    // Use last close as base for simulation
    double last_close = 100.0; // Default
    if (!lines.empty() && !lines[4].empty()) {
        last_close = lines[4].back();
    }
    
    // Simulate small price movements
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(-0.01, 0.01);
    
    double change = dis(gen);
    bar.open = last_close;
    bar.close = last_close * (1.0 + change);
    bar.high = std::max(bar.open, bar.close) * (1.0 + std::abs(change) * 0.5);
    bar.low = std::min(bar.open, bar.close) * (1.0 - std::abs(change) * 0.5);
    bar.volume = 1000.0 + dis(gen) * 500.0;
    
    return bar;
}

bool IBData::islive() const {
    return p.live && is_live_;
}

std::string IBData::get_symbol() const {
    return symbol_;
}

std::string IBData::get_sectype() const {
    return sectype_;
}

std::string IBData::get_exchange() const {
    return exchange_;
}

std::string IBData::get_currency() const {
    return currency_;
}

IBData::Contract IBData::get_contract() const {
    return contract_;
}

size_t IBData::get_historical_size() const {
    return historical_data_.size();
}

size_t IBData::get_live_queue_size() const {
    std::lock_guard<std::mutex> lock(live_data_mutex_);
    return live_data_queue_.size();
}

double IBData::get_last_price() const {
    if (!lines.empty() && !lines[4].empty()) {
        return lines[4].back(); // Last close price
    }
    return 0.0;
}

IBData::OHLCVBar IBData::get_last_bar() const {
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

void IBData::set_live_mode(bool live) {
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

std::map<std::string, double> IBData::get_market_data() const {
    std::map<std::string, double> market_data;
    
    // In real implementation, this would return current bid/ask/last from IB
    // For now, return simulated data
    double last_price = get_last_price();
    market_data["last"] = last_price;
    market_data["bid"] = last_price * 0.999;
    market_data["ask"] = last_price * 1.001;
    market_data["volume"] = 1000.0;
    
    return market_data;
}

} // namespace feeds
} // namespace backtrader