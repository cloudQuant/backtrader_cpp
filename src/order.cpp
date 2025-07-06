#include "order.h"
#include <sstream>
#include <iomanip>

namespace backtrader {

int Order::next_ref_ = 1;

// OrderExecutionBit implementation
OrderExecutionBit::OrderExecutionBit(std::chrono::system_clock::time_point dt_val,
                                   double size_val,
                                   double price_val,
                                   double closed_val,
                                   double closedvalue_val,
                                   double closedcomm_val,
                                   double opened_val,
                                   double openedvalue_val,
                                   double openedcomm_val,
                                   double pnl_val,
                                   double psize_val,
                                   double pprice_val)
    : dt(dt_val), size(size_val), price(price_val), closed(closed_val),
      opened(opened_val), closedvalue(closedvalue_val), openedvalue(openedvalue_val),
      closedcomm(closedcomm_val), openedcomm(openedcomm_val), pnl(pnl_val),
      psize(psize_val), pprice(pprice_val) {
    
    value = closedvalue + openedvalue;
    comm = closedcomm + openedcomm;
}

// OrderData implementation
void OrderData::addbit(const OrderExecutionBit& bit) {
    exbits.push_back(bit);
    
    // Update totals
    size += bit.size;
    value += bit.value;
    comm += bit.comm;
    pnl += bit.pnl;
    
    // Update position info from latest bit
    psize = bit.psize;
    pprice = bit.pprice;
    
    // Update remaining size
    remsize = size; // This would be calculated based on original order size
}

// Order implementation
bool Order::isbuy() const {
    return size > 0.0;
}

bool Order::issell() const {
    return size < 0.0;
}

bool Order::alive() const {
    return status == OrderStatus::Created ||
           status == OrderStatus::Submitted ||
           status == OrderStatus::Accepted ||
           status == OrderStatus::Partial;
}

bool Order::iscompleted() const {
    return status == OrderStatus::Completed;
}

bool Order::ispartial() const {
    return status == OrderStatus::Partial;
}

bool Order::isaccepted() const {
    return status == OrderStatus::Accepted;
}

bool Order::issubmitted() const {
    return status == OrderStatus::Submitted;
}

double Order::remaining() const {
    return size - executed.size;
}

std::string Order::to_string() const {
    std::ostringstream oss;
    oss << "Order[" << ref << "] ";
    oss << (isbuy() ? "BUY" : "SELL") << " ";
    oss << std::fixed << std::setprecision(2) << std::abs(size) << " @ ";
    
    switch (type) {
        case OrderType::Market:
            oss << "Market";
            break;
        case OrderType::Limit:
            oss << "Limit " << price;
            break;
        case OrderType::Stop:
            oss << "Stop " << price;
            break;
        case OrderType::StopLimit:
            oss << "StopLimit " << price << "/" << pricelimit;
            break;
        case OrderType::Close:
            oss << "Close";
            break;
    }
    
    oss << " Status: ";
    switch (status) {
        case OrderStatus::Created:
            oss << "Created";
            break;
        case OrderStatus::Submitted:
            oss << "Submitted";
            break;
        case OrderStatus::Accepted:
            oss << "Accepted";
            break;
        case OrderStatus::Partial:
            oss << "Partial";
            break;
        case OrderStatus::Completed:
            oss << "Completed";
            break;
        case OrderStatus::Canceled:
            oss << "Canceled";
            break;
        case OrderStatus::Expired:
            oss << "Expired";
            break;
        case OrderStatus::Margin:
            oss << "Margin";
            break;
        case OrderStatus::Rejected:
            oss << "Rejected";
            break;
    }
    
    if (executed.size > 0.0) {
        oss << " Executed: " << executed.size << "/" << size;
    }
    
    return oss.str();
}

std::shared_ptr<Order> Order::clone() const {
    auto cloned = std::make_shared<Order>();
    
    // Copy basic properties
    cloned->data = data;
    cloned->size = size;
    cloned->price = price;
    cloned->pricelimit = pricelimit;
    cloned->trailamount = trailamount;
    cloned->trailpercent = trailpercent;
    cloned->type = type;
    cloned->transmit = transmit;
    cloned->parent = parent;
    cloned->simulated = simulated;
    
    // Copy state
    cloned->status = status;
    cloned->owner = owner;
    cloned->ref = ++next_ref_;
    cloned->info = info;
    
    // Copy execution data
    cloned->created = created;
    cloned->executed = executed;
    
    return cloned;
}

std::string Order::status_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::Created: return "Created";
        case OrderStatus::Submitted: return "Submitted";
        case OrderStatus::Accepted: return "Accepted";
        case OrderStatus::Partial: return "Partial";
        case OrderStatus::Completed: return "Completed";
        case OrderStatus::Canceled: return "Canceled";
        case OrderStatus::Expired: return "Expired";
        case OrderStatus::Margin: return "Margin";
        case OrderStatus::Rejected: return "Rejected";
        default: return "Unknown";
    }
}

// BuyOrder implementation
BuyOrder::BuyOrder(std::shared_ptr<DataSeries> data_val,
                   double size_val,
                   double price_val,
                   OrderType type_val) {
    data = data_val;
    size = std::abs(size_val); // Ensure positive for buy
    price = price_val;
    type = type_val;
    ref = ++next_ref_;
}

// SellOrder implementation
SellOrder::SellOrder(std::shared_ptr<DataSeries> data_val,
                     double size_val,
                     double price_val,
                     OrderType type_val) {
    data = data_val;
    size = -std::abs(size_val); // Ensure negative for sell
    price = price_val;
    type = type_val;
    ref = ++next_ref_;
}

// Factory functions
std::shared_ptr<Order> create_order(std::shared_ptr<DataSeries> data,
                                   double size,
                                   double price,
                                   OrderType type,
                                   bool is_buy) {
    if (is_buy) {
        return std::make_shared<BuyOrder>(data, size, price, type);
    } else {
        return std::make_shared<SellOrder>(data, size, price, type);
    }
}

std::shared_ptr<BuyOrder> create_buy_order(std::shared_ptr<DataSeries> data,
                                          double size,
                                          double price,
                                          OrderType type) {
    return std::make_shared<BuyOrder>(data, size, price, type);
}

std::shared_ptr<SellOrder> create_sell_order(std::shared_ptr<DataSeries> data,
                                            double size,
                                            double price,
                                            OrderType type) {
    return std::make_shared<SellOrder>(data, size, price, type);
}

} // namespace backtrader