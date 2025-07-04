#include "comminfo.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace backtrader {

CommInfo::CommInfo(double commission_val, 
                   double margin_val, 
                   double mult_val, 
                   bool percabs_val,
                   bool stocklike_val)
    : commission(commission_val), margin(margin_val), mult(mult_val), 
      percabs(percabs_val), stocklike(stocklike_val) {
}

double CommInfo::getcommission(double size, double price) const {
    return _getcommission(size, price, false);
}

double CommInfo::getcommissioninfo(double size, double price) const {
    return getcommission(size, price);
}

double CommInfo::getmargin(double price) const {
    return margin * mult;
}

double CommInfo::getoperationcost(double size, double price) const {
    return getcommission(size, price) + getmargin(price);
}

double CommInfo::getsize(double price, double cash) const {
    if (price <= 0.0) {
        return 0.0;
    }
    
    if (stocklike) {
        // For stocks, cash determines how many shares can be bought
        return std::floor(cash / price);
    } else {
        // For futures/forex, margin determines position size
        double margin_req = getmargin(price);
        if (margin_req <= 0.0) {
            return 0.0;
        }
        return std::floor(cash / margin_req);
    }
}

double CommInfo::getvalue(double size, double price) const {
    return getvaluesize(size, price);
}

double CommInfo::getvaluesize(double size, double price) const {
    return std::abs(size) * price * mult;
}

double CommInfo::profitandloss(double size, double price, double newprice) const {
    return _profitandloss(size, price, newprice);
}

double CommInfo::cashadjust(double size, double price, double newprice) const {
    if (stocklike) {
        return 0.0; // No cash adjustment for stocks
    }
    return profitandloss(size, price, newprice);
}

double CommInfo::get_credit_interest(double data, double pos, double dt) const {
    // Placeholder for interest calculation
    return 0.0;
}

std::shared_ptr<CommInfo> CommInfo::clone() const {
    auto cloned = std::make_shared<CommInfo>();
    cloned->commission = commission;
    cloned->margin = margin;
    cloned->mult = mult;
    cloned->percabs = percabs;
    cloned->stocklike = stocklike;
    cloned->commtype = commtype;
    cloned->interest = interest;
    cloned->interest_long = interest_long;
    cloned->interest_short = interest_short;
    cloned->leverage = leverage;
    return cloned;
}

std::string CommInfo::to_string() const {
    std::ostringstream oss;
    oss << "CommInfo[";
    oss << "commission=" << std::fixed << std::setprecision(4) << commission;
    oss << ", margin=" << margin;
    oss << ", mult=" << mult;
    oss << ", percabs=" << (percabs ? "True" : "False");
    oss << ", stocklike=" << (stocklike ? "True" : "False");
    oss << "]";
    return oss.str();
}

double CommInfo::_getcommission(double size, double price, bool pseudoexec) const {
    if (size == 0.0 || price <= 0.0) {
        return 0.0;
    }
    
    double abs_size = std::abs(size);
    
    if (percabs) {
        // Percentage commission
        if (commtype) {
            // Percentage of size
            return abs_size * commission;
        } else {
            // Percentage of value
            return abs_size * price * mult * commission;
        }
    } else {
        // Absolute commission
        return commission;
    }
}

double CommInfo::_profitandloss(double size, double price, double newprice) const {
    if (size == 0.0) {
        return 0.0;
    }
    
    double pricediff = newprice - price;
    if (stocklike) {
        return size * pricediff;
    } else {
        return size * pricediff * mult;
    }
}

// CommInfoStock implementation
CommInfoStock::CommInfoStock(double commission_val, bool percabs_val)
    : CommInfo(commission_val, 0.0, 1.0, percabs_val, true) {
}

double CommInfoStock::getcommission(double size, double price) const {
    return _getcommission(size, price, false);
}

// CommInfoFutures implementation
CommInfoFutures::CommInfoFutures(double commission_val, double margin_val, double mult_val)
    : CommInfo(commission_val, margin_val, mult_val, true, false) {
}

double CommInfoFutures::getcommission(double size, double price) const {
    return _getcommission(size, price, false);
}

double CommInfoFutures::getmargin(double price) const {
    return margin * mult;
}

double CommInfoFutures::profitandloss(double size, double price, double newprice) const {
    return _profitandloss(size, price, newprice);
}

// CommInfoForex implementation
CommInfoForex::CommInfoForex(double commission_val, double margin_val, double mult_val, double leverage_val)
    : CommInfo(commission_val, margin_val, mult_val, false, false) {
    leverage = leverage_val;
}

double CommInfoForex::getcommission(double size, double price) const {
    return _getcommission(size, price, false);
}

double CommInfoForex::getmargin(double price) const {
    if (leverage <= 0.0) {
        return margin * mult;
    }
    return (margin * mult) / leverage;
}

// Factory functions
std::shared_ptr<CommInfo> create_comminfo(double commission, double margin, double mult, bool percabs, bool stocklike) {
    return std::make_shared<CommInfo>(commission, margin, mult, percabs, stocklike);
}

std::shared_ptr<CommInfoStock> create_stock_comminfo(double commission, bool percabs) {
    return std::make_shared<CommInfoStock>(commission, percabs);
}

std::shared_ptr<CommInfoFutures> create_futures_comminfo(double commission, double margin, double mult) {
    return std::make_shared<CommInfoFutures>(commission, margin, mult);
}

std::shared_ptr<CommInfoForex> create_forex_comminfo(double commission, double margin, double mult, double leverage) {
    return std::make_shared<CommInfoForex>(commission, margin, mult, leverage);
}

} // namespace backtrader