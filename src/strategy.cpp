#include "strategy.h"
#include "cerebro.h"
#include "feed.h"
#include "order.h"
#include "position.h"
#include "trade.h"
#include "comminfo.h"
#include "indicators/sma.h"
#include "indicators/crossover.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <map>

namespace backtrader {

Strategy::Strategy() : StrategyBase() {
    // StrategyBase constructor already sets _ltype = LineRoot::IndType::StratType
    // so we don't need to set it again here
    
    // Initialize bar counter
    current_bar_ = 0;
    
    // Initialize default values
    _setup_default_sizer();
    
    // Initialize minperiods vector
    _minperiods.clear();
}

std::shared_ptr<Order> Strategy::buy(std::shared_ptr<DataSeries> data,
                                    double size,
                                    double price,
                                    std::string order_type) {
    // std::cerr << "Strategy::buy called, broker=" << broker.get() << std::endl << std::flush;
    // Use the first data if none specified
    if (!data && !datas.empty()) {
        data = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    }
    
    if (!broker) {
        // std::cerr << "Strategy::buy - NO BROKER!" << std::endl;
        return nullptr;
    }
    
    // Create a buy order
    auto order = std::make_shared<Order>();
    std::cerr << "Strategy::buy - Created new order at " << order.get() << std::endl;
    order->data = data;  // IMPORTANT: Set the data reference!
    // std::cerr << "Strategy::buy - order->data set to " << order->data.get() << std::endl;
    order->type = order_type == "Market" ? OrderType::Market : OrderType::Limit;
    order->size = size > 0 ? size : 1.0;  // Default to 1 unit if size not specified
    order->price = price;
    order->status = OrderStatus::Submitted;
    // For now, use current time - in full implementation would convert from data->datetime(0)
    order->created.dt = std::chrono::system_clock::now();
    order->created.price = data ? data->close(0) : price;
    order->created.size = order->size;
    
    // Submit to broker
    if (broker) {
        broker->submit(order);
    }
    
    // Don't add to _orderspending here - it will be added when notification arrives
    // This avoids duplicate notifications
    
    return order;
}

std::shared_ptr<Order> Strategy::sell(std::shared_ptr<DataSeries> data,
                                     double size,
                                     double price,
                                     std::string order_type) {
    // Use the first data if none specified
    if (!data && !datas.empty()) {
        data = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    }
    
    if (!broker) {
        return nullptr;
    }
    
    // Create a sell order
    auto order = std::make_shared<Order>();
    order->data = data;  // IMPORTANT: Set the data reference!
    order->type = order_type == "Market" ? OrderType::Market : OrderType::Limit;
    order->size = -(size > 0 ? size : 1.0);  // Negative size for sell
    order->price = price;
    order->status = OrderStatus::Submitted;
    // For now, use current time - in full implementation would convert from data->datetime(0)
    order->created.dt = std::chrono::system_clock::now();
    order->created.price = data ? data->close(0) : price;
    order->created.size = order->size;
    
    // Submit to broker
    if (broker) {
        broker->submit(order);
    }
    
    // Don't add to _orderspending here - it will be added when notification arrives
    // This avoids duplicate notifications
    
    return order;
}

std::shared_ptr<Order> Strategy::close(std::shared_ptr<DataSeries> data,
                                      double size) {
    // Use the first data if none specified
    if (!data && !datas.empty()) {
        data = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    }
    
    if (!broker) {
        return nullptr;
    }
    
    // Get current position
    double current_pos = getposition(data);
    std::cerr << "Strategy::close - current position=" << current_pos << std::endl;
    
    if (current_pos == 0.0) {
        return nullptr;  // No position to close
    }
    
    // Create opposing order to close position
    double close_size = size > 0 ? size : std::abs(current_pos);
    std::cerr << "Strategy::close - close_size=" << close_size << std::endl;
    
    if (current_pos > 0) {
        // Long position, sell to close
        std::cerr << "Strategy::close - Calling sell with size=" << close_size << std::endl;
        return sell(data, close_size, 0.0, "Market");
    } else {
        // Short position, buy to close
        std::cerr << "Strategy::close - Calling buy with size=" << close_size << std::endl;
        return buy(data, close_size, 0.0, "Market");
    }
}

std::shared_ptr<Order> Strategy::cancel(std::shared_ptr<Order> order) {
    // This would cancel an existing order
    // Would interact with broker to cancel the order
    return nullptr;
}

double Strategy::getposition(std::shared_ptr<DataSeries> data) const {
    // Use the first data if none specified
    if (!data && !datas.empty()) {
        data = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    }
    
    if (!broker || !data) {
        return 0.0;
    }
    
    // Get position from broker
    auto pos = broker->getposition(data);
    if (pos) {
        return pos->size;
    }
    
    return 0.0;
}

double Strategy::getpositionbyname(const std::string& name) const {
    // This would return the current position size for the named data
    // Would find the data by name and return its position
    return 0.0;
}

std::shared_ptr<Position> Strategy::position(std::shared_ptr<DataSeries> data) const {
    // Use the first data if none specified
    if (!data && !datas.empty()) {
        data = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    }
    
    if (!broker || !data) {
        return nullptr;
    }
    
    // Get position from broker
    return broker->getposition(data);
}

double Strategy::getcash() const {
    if (broker) {
        return broker->getcash();
    }
    return 0.0;
}

double Strategy::getvalue() const {
    if (broker) {
        return broker->getvalue();
    }
    return 0.0;
}

std::shared_ptr<LineSeries> Strategy::data(int idx) const {
    if (idx >= 0 && idx < static_cast<int>(datas.size())) {
        return datas[idx];
    }
    return nullptr;
}

size_t Strategy::len() const {
    return current_bar_;
}

size_t Strategy::datas_count() const {
    return datas.size();
}

std::shared_ptr<void> Strategy::getanalyzer(const std::string& name) const {
    auto it = _analyzer_instances.find(name);
    if (it != _analyzer_instances.end()) {
        return std::static_pointer_cast<void>(it->second);
    }
    return nullptr;
}

void Strategy::log(const std::string& message, bool doprint) {
    if (doprint) {
        std::cout << "[Strategy] " << message << std::endl;
    }
    
    // Store the message for later retrieval if needed
    _notifications.push_back(message);
}

void Strategy::_addnotification(const std::string& type, const std::string& msg) {
    std::ostringstream oss;
    oss << "[" << type << "] " << msg;
    _notifications.push_back(oss.str());
}

void Strategy::_addnotification(std::shared_ptr<Order> order) {
    if (!order) return;
    
    std::cerr << "Strategy::_addnotification - Order status: " << static_cast<int>(order->status) 
              << ", size: " << order->size << ", executed.size: " << order->executed.size << std::endl;
    
    // Add order to pending notifications
    _orderspending.push_back(order);
    
    // Check if this order completes a trade
    if (order->status == Order::Status::Completed || 
        order->status == Order::Status::Partial) {
        
        // std::cerr << "Strategy::_addnotification - Order is completed/partial, creating trade" << std::endl;
        
        // Get the trade for this order
        auto& trade_list = _trades[order->data->_name];
        
        // Find or create trade
        std::shared_ptr<Trade> trade = nullptr;
        for (auto& t : trade_list) {
            if (t->status == TradeStatus::Open) {
                trade = t;
                break;
            }
        }
        
        if (!trade) {
            // Create new trade
            trade = std::make_shared<Trade>(order->data);
            trade_list.push_back(trade);
        }
        
        // Update trade with order execution
        double exec_size = order->executed.size;
        double exec_price = order->executed.price;
        double exec_value = order->executed.value;
        double exec_comm = order->executed.comm;
        
        // Calculate PnL for closing trades using CommInfo (with multiplier support)
        double pnl = 0.0;
        std::cerr << "PnL calc: trade->size=" << trade->size << ", order->isbuy()=" << (order->isbuy() ? "true" : "false") << std::endl;
        if (trade->size != 0.0 && ((trade->size > 0 && !order->isbuy()) || 
                                   (trade->size < 0 && order->isbuy()))) {
            std::cerr << "This is a closing trade" << std::endl;
            // This is a closing trade
            // Use broker's CommInfo to calculate PnL with proper multiplier
            if (broker && trade->data) {
                auto comminfo = broker->getcommissioninfo(trade->data);
                if (comminfo) {
                    // Debug: check CommInfo settings
                    std::cerr << "CommInfo found - stocklike=" << (comminfo->stocklike ? "true" : "false") 
                              << ", mult=" << comminfo->mult 
                              << ", commission=" << comminfo->commission << std::endl;
                    
                    // Use absolute size for PnL calculation
                    double close_size = std::abs(exec_size);
                    if (trade->size > 0) {
                        // Long position: use CommInfo profitandloss
                        pnl = comminfo->profitandloss(close_size, trade->price, exec_price);
                        std::cerr << "Long PnL: size=" << close_size << ", entry=" << trade->price 
                                  << ", exit=" << exec_price << ", pnl=" << pnl << std::endl;
                    } else {
                        // Short position: use CommInfo profitandloss with negative size
                        pnl = comminfo->profitandloss(-close_size, trade->price, exec_price);
                        std::cerr << "Short PnL: size=" << -close_size << ", entry=" << trade->price 
                                  << ", exit=" << exec_price << ", pnl=" << pnl << std::endl;
                    }
                } else {
                    // Fallback to simple calculation if no CommInfo
                    double close_size = std::abs(exec_size);
                    if (trade->size > 0) {
                        pnl = (exec_price - trade->price) * close_size;
                    } else {
                        pnl = (trade->price - exec_price) * close_size;
                    }
                }
            } else {
                // Fallback to simple calculation if no broker
                double close_size = std::abs(exec_size);
                if (trade->size > 0) {
                    pnl = (exec_price - trade->price) * close_size;
                } else {
                    pnl = (trade->price - exec_price) * close_size;
                }
            }
        }
        
        // Update trade
        // exec_size is already signed (positive for buy, negative for sell)
        trade->update(order, exec_size, 
                     exec_price, exec_value, exec_comm, pnl, order->executed.dt);
        
        // Add to pending trades for notification
        _tradespending.push_back(trade);
    }
}

void Strategy::_notify() {
    // Process pending order notifications
    std::cerr << "Strategy::_notify - _orderspending.size()=" << _orderspending.size() 
              << ", _tradespending.size()=" << _tradespending.size() << std::endl;
              
    for (auto& order : _orderspending) {
        notify_order(order);  // Call the shared_ptr version only
        // Note: The const ref version is for backward compatibility but should not be called here
    }
    _orderspending.clear();
    
    // Process pending trade notifications
    for (auto& trade : _tradespending) {
        // std::cerr << "Strategy::_notify - Notifying trade to analyzers" << std::endl;
        
        // Notify the strategy
        notify_trade(trade);  // Call the shared_ptr version
        if (trade) {
            notify_trade(*trade);  // Also call the const ref version
        }
        
        // Notify all analyzers
        for (auto& [name, analyzer] : _analyzer_instances) {
            if (analyzer) {
                // std::cerr << "Strategy::_notify - Calling analyzer '" << name << "' _notify_trade" << std::endl;
                analyzer->_notify_trade(trade);
            }
        }
    }
    _tradespending.clear();
    
    LineIterator::_notify();
}

void Strategy::_setup_default_sizer() {
    // Set up a default fixed size sizer
    // In a full implementation, this would create a FixedSize sizer
    // _sizer = std::make_shared<FixedSize>();
}

void Strategy::_periodset() {
    // Calculate minimum periods for each data source
    // This is crucial for multi-timeframe synchronization
    
    
    _minperiods.clear();
    
    // Map to store minimum periods for each data/clock
    std::map<void*, std::vector<size_t>> _dminperiods;
    
    // Get all indicators
    auto indicators = _lineiterators[IndType];
    
    
    // For each indicator, find its clock (data source) and track its minperiod
    for (auto& indicator : indicators) {
        // Get the clock (data source) for this indicator
        auto clk = indicator->_clock;
        if (!clk) {
            clk = indicator->data;
        }
        if (!clk) {
            continue;
        }
        
        // Store the indicator's minimum period for this clock
        void* clk_ptr = clk.get();
        
        // Try to use getMinPeriod() method if available (more reliable than _minperiod())
        size_t ind_minperiod = indicator->_minperiod();
        if (auto ind_base = dynamic_cast<IndicatorBase*>(indicator.get())) {
            ind_minperiod = static_cast<size_t>(ind_base->getMinPeriod());
            }
        
        _dminperiods[clk_ptr].push_back(ind_minperiod);
        
        // Check if this is an SMA or CrossOver
        bool is_sma = (dynamic_cast<indicators::SMA*>(indicator.get()) != nullptr);
        bool is_crossover = (dynamic_cast<CrossOver*>(indicator.get()) != nullptr);
        
        
    }
    
    // Calculate minimum period for each data source
    for (auto& data : datas) {
        void* data_ptr = data.get();
        
        // Get all minimum periods associated with this data
        auto& dlminperiods = _dminperiods[data_ptr];
        
        // Find the maximum minimum period for this data
        size_t dminperiod = data->_minperiod();
        if (!dlminperiods.empty()) {
            dminperiod = std::max(dminperiod, *std::max_element(dlminperiods.begin(), dlminperiods.end()));
        }
        
        _minperiods.push_back(dminperiod);
        
    }
    
    // Update strategy's overall minimum period
    std::vector<size_t> indminperiods;
    for (auto& ind : indicators) {
        indminperiods.push_back(ind->_minperiod());
    }
    
    if (!indminperiods.empty()) {
        size_t max_ind_minperiod = *std::max_element(indminperiods.begin(), indminperiods.end());
        updateminperiod(max_ind_minperiod);
    }
    
}

int Strategy::_getminperstatus() {
    // Calculate minimum period status based on all data sources
    // This determines whether to call prenext, nextstart, or next
    
    if (_minperiods.empty() || datas.empty()) {
        std::cerr << "Strategy::_getminperstatus() - Using base class, _minperiods.empty()=" 
                  << _minperiods.empty() << ", datas.empty()=" << datas.empty() << std::endl;
        return LineIterator::_getminperstatus();
    }
    
    // Calculate the "distance" for each data source
    // (minimum period - current length)
    std::vector<int> dlens;
    
    for (size_t i = 0; i < datas.size() && i < _minperiods.size(); ++i) {
        auto data = datas[i];
        if (!data) continue;
        
        // Get current bar count for this data
        // Use the data's size() method which tracks current position
        size_t current_len = data->size();
        
        // Calculate distance (minperiod - current length)
        int distance = static_cast<int>(_minperiods[i]) - static_cast<int>(current_len);
        dlens.push_back(distance);
    }
    
    if (dlens.empty()) {
        return -1;
    }
    
    // Return the maximum distance
    // If > 0: some data doesn't have enough bars yet (prenext)
    // If = 0: all data just reached minimum bars (nextstart)
    // If < 0: all data have enough bars (next)
    int max_distance = *std::max_element(dlens.begin(), dlens.end());
    
    static int debug_count = 0;
    if (debug_count++ < 50) {  // Increase debug count to see more transitions
        std::cerr << "Strategy::_getminperstatus() #" << debug_count << " - max_distance=" << max_distance 
                  << ", dlens=[";
        for (size_t i = 0; i < dlens.size(); ++i) {
            if (i > 0) std::cerr << ", ";
            std::cerr << dlens[i];
        }
        std::cerr << "]" << std::endl;
    }
    
    return max_distance;
}

void Strategy::_next() {
    // Increment current bar counter before calling parent _next
    current_bar_++;
    
    // Call parent implementation to handle indicators and next() logic
    LineIterator::_next();
}

void Strategy::_once() {
    // For strategies, we need to use the data's buflen, not the strategy's buflen
    // because strategies don't have their own line buffers
    
    
    size_t data_buflen = 0;
    if (!datas.empty() && datas[0]) {
        data_buflen = datas[0]->buflen();
    }
    
    
    // Forward all lines to full length
    forward(data_buflen);
    
    // Execute child indicators in once mode
    auto& indicators = _lineiterators[static_cast<int>(LineIterator::IndType)];
    for (auto& indicator : indicators) {
        indicator->_once();
    }
    
    // Forward observers
    for (auto& observer : _lineiterators[ObsType]) {
        observer->forward(data_buflen);
    }
    
    // Reset position for all data sources
    for (auto& data : datas) {
        data->home();
    }
    
    // Reset position for indicators
    for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
        indicator->home();
    }
    
    // Reset position for observers
    for (auto& observer : _lineiterators[ObsType]) {
        observer->home();
    }
    
    // Reset own position
    home();
    
    // Execute the three phases of once processing with data's buflen
    // Handle case where data_buflen < minperiod_
    if (data_buflen < minperiod_) {
        // Not enough data for the strategy
        preonce(0, data_buflen);
    } else {
        preonce(0, minperiod_ - 1);
        oncestart(minperiod_ - 1, minperiod_);
        once(minperiod_, data_buflen);
    }
    
    // Execute binding synchronization
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

void Strategy::once(int start, int end) {
    // In runonce mode, iterate through each bar and call next()
    // This allows custom strategies to collect data in their next() methods
    
    // std::cerr << "Strategy::once(" << start << ", " << end << ") called!" << std::endl;
    
    // IMPORTANT: In Python backtrader, indicators move together with data
    // We need to synchronize their positions
    
    for (int i = start; i < end; ++i) {
        // std::cerr << "Strategy::once() - Processing bar " << i << "/" << (end-1) << std::endl;
        
        // Set current position for all data sources to bar i
        for (auto& data : datas) {
            if (i < static_cast<int>(data->buflen())) {
                // Reset to home and forward to position i
                data->home();
                data->forward(i);  // Forward directly to position i
                // std::cerr << "  Data positioned at: size=" << data->size() << ", buflen=" << data->buflen() << std::endl;
            }
        }
        
        // ALSO position indicators to the same bar
        // Indicators have pre-calculated values, we just need to set their index
        // Use a recursive lambda to position all indicators and their children
        std::function<void(std::shared_ptr<LineIterator>)> position_indicator;
        position_indicator = [&](std::shared_ptr<LineIterator> indicator) {
            // Position this indicator's lines
            if (indicator->lines && indicator->lines->size() > 0) {
                for (size_t j = 0; j < indicator->lines->size(); ++j) {
                    auto line = indicator->lines->getline(j);
                    if (auto buffer = std::dynamic_pointer_cast<LineBuffer>(line)) {
                        // Set the buffer index to the current bar position
                        buffer->set_idx(i);
                        // std::cerr << "  Indicator line " << j << " positioned at index " << i << std::endl;
                    }
                }
            }
            
            // Position child indicators recursively
            auto& child_indicators = indicator->_lineiterators[static_cast<int>(LineIterator::IndType)];
            for (auto& child : child_indicators) {
                position_indicator(child);
            }
        };
        
        // Position all top-level indicators
        for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
            position_indicator(indicator);
        }
        
        // Update current bar counter
        current_bar_ = i + 1;
        
        // Call the custom strategy's next() method
        int minperstatus = _getminperstatus();
        if (minperstatus < 0) {
            // std::cerr << "Strategy::once() - Calling next() for bar " << i << std::endl;
            next();
        } else if (minperstatus == 0) {
            // std::cerr << "Strategy::once() - Calling nextstart() for bar " << i << std::endl;
            nextstart();
        } else {
            // std::cerr << "Strategy::once() - Calling prenext() for bar " << i << std::endl;
            prenext();
        }
        
        // CRITICAL: In runonce mode, broker needs to process orders after each bar
        if (broker) {
            broker->next();
        }
        
        // Process notifications after broker has processed orders
        _notify();
    }
}


} // namespace backtrader