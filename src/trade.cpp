#include "trade.h"
#include <sstream>
#include <iomanip>

namespace backtrader {

int Trade::next_ref_ = 1;

// TradeHistory implementation
TradeHistory::TradeHistory(TradeStatus status_val,
                          std::chrono::system_clock::time_point dt,
                          int barlen,
                          double size,
                          double price,
                          double value,
                          double pnl,
                          double pnlcomm,
                          const std::string& tz) {
    status.status = status_val;
    status.dt = dt;
    status.barlen = barlen;
    status.size = size;
    status.price = price;
    status.value = value;
    status.pnl = pnl;
    status.pnlcomm = pnlcomm;
    status.tz = tz;
}

void TradeHistory::doupdate(std::shared_ptr<Order> order,
                           double size,
                           double price,
                           double commission) {
    event.order = order;
    event.size = size;
    event.price = price;
    event.commission = commission;
}

std::string TradeHistory::to_string() const {
    std::ostringstream oss;
    oss << "TradeHistory[";
    oss << "status=" << (status.status == TradeStatus::Created ? "Created" : 
                         status.status == TradeStatus::Open ? "Open" : "Closed");
    oss << ", size=" << std::fixed << std::setprecision(2) << status.size;
    oss << ", price=" << std::fixed << std::setprecision(4) << status.price;
    oss << ", pnl=" << std::fixed << std::setprecision(2) << status.pnl;
    oss << "]";
    return oss.str();
}

// Trade implementation
Trade::Trade() : ref(next_ref_++), just_opened_(false), status(TradeStatus::Created), historynotify(true) {
}

Trade::Trade(std::shared_ptr<DataSeries> data_val) 
    : Trade() {
    data = data_val;
}

void Trade::update(std::shared_ptr<Order> order,
                  double size_change,
                  double price_val,
                  double value_change,
                  double commission_val,
                  double pnl_val,
                  std::chrono::system_clock::time_point dt) {
    
    // Update trade totals
    size += size_change;
    commission += commission_val;
    value += value_change;
    pnl = pnl_val;
    pnlcomm = pnl - commission;
    
    // Determine old size before update
    double old_size = size - size_change;
    
    // Update pricing and status
    if (size != 0.0) {
        if (status == TradeStatus::Created) {
            // First time opening the trade
            price = price_val;
            dtopen = dt;
            status = TradeStatus::Open;
            just_opened_ = true;
        } else if (status == TradeStatus::Open) {
            // Check if we're adding to position (same sign) or reducing (opposite sign)
            bool adding_to_position = (old_size * size_change > 0);
            
            if (adding_to_position) {
                // For open trades, maintain weighted average price when adding
                price = (price * old_size + price_val * size_change) / size;
            }
            // When reducing position, keep the original price
            just_opened_ = false;
        }
    } else {
        // Trade is closed (size is 0)
        status = TradeStatus::Closed;
        dtclose = dt;
        just_opened_ = false;
    }
    
    // Update bar counts
    if (data && data->size() > 0) {
        if (status == TradeStatus::Open && baropen == 0) {
            baropen = static_cast<int>(data->size());
        }
        if (status == TradeStatus::Closed) {
            barclose = static_cast<int>(data->size());
        }
        barlen = static_cast<int>(data->size()) - baropen + 1;
    }
    
    // Add to history if requested
    if (historynotify) {
        _addhistory(status, dt, barlen, size, price, value, pnl, pnlcomm);
        
        // Update the last history entry with event details
        if (!history.empty()) {
            history.back().doupdate(order, size_change, price_val, commission_val);
        }
    }
}

bool Trade::isopen() const {
    return status == TradeStatus::Open;
}

bool Trade::isclosed() const {
    return status == TradeStatus::Closed;
}

bool Trade::justopened() const {
    return just_opened_;
}

double Trade::pnl_unrealized(double current_price) const {
    if (size == 0.0) {
        return 0.0;
    }
    
    return size * (current_price - price);
}

double Trade::pnl_realized() const {
    if (isclosed()) {
        return pnl;
    }
    return 0.0;
}

bool Trade::long_() const {
    return size > 0.0;
}

bool Trade::short_() const {
    return size < 0.0;
}

std::string Trade::to_string() const {
    std::ostringstream oss;
    oss << "Trade[" << ref << "] ";
    oss << (status == TradeStatus::Created ? "CREATED" : 
            status == TradeStatus::Open ? "OPEN" : "CLOSED") << " ";
    oss << "Size: " << std::fixed << std::setprecision(2) << size << " @ " << std::fixed << std::setprecision(4) << price;
    if (isclosed()) {
        oss << " PnL: " << std::fixed << std::setprecision(2) << pnlcomm;
    }
    return oss.str();
}

std::shared_ptr<Trade> Trade::clone() const {
    auto cloned = std::make_shared<Trade>();
    
    cloned->ref = ref;
    cloned->status = status;
    cloned->tradeid = tradeid;
    cloned->size = size;
    cloned->price = price;
    cloned->value = value;
    cloned->commission = commission;
    cloned->pnl = pnl;
    cloned->pnlcomm = pnlcomm;
    cloned->barlen = barlen;
    cloned->dtopen = dtopen;
    cloned->dtclose = dtclose;
    cloned->baropen = baropen;
    cloned->barclose = barclose;
    cloned->data = data;
    cloned->history = history;
    cloned->historynotify = historynotify;
    cloned->just_opened_ = just_opened_;
    
    return cloned;
}

bool Trade::operator==(const Trade& other) const {
    return ref == other.ref;
}

bool Trade::operator!=(const Trade& other) const {
    return !(*this == other);
}

void Trade::_addhistory(TradeStatus status_val,
                       std::chrono::system_clock::time_point dt,
                       int barlen_val,
                       double size_val,
                       double price_val,
                       double value_val,
                       double pnl_val,
                       double pnlcomm_val) {
    history.emplace_back(status_val, dt, barlen_val, size_val, 
                        price_val, value_val, pnl_val, pnlcomm_val, "");
}

// Factory function
std::shared_ptr<Trade> create_trade(std::shared_ptr<DataSeries> data) {
    return std::make_shared<Trade>(data);
}

} // namespace backtrader