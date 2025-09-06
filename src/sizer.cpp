#include "sizer.h"
#include "strategy.h"
#include "broker.h"
#include "dataseries.h"
#include "comminfo.h"
#include <cmath>
#include <algorithm>

namespace backtrader {

// Sizer implementation
double Sizer::getsizing(std::shared_ptr<DataSeries> data, bool isbuy) {
    if (!broker) {
        return 0.0;
    }
    
    auto comminfo = broker->getcommissioninfo(data);
    double cash = broker->getcash();
    
    return _getsizing(comminfo, cash, data, isbuy);
}

void Sizer::set(std::shared_ptr<Strategy> strategy_ref, std::shared_ptr<Broker> broker_ref) {
    strategy = strategy_ref;
    broker = broker_ref;
}

// FixedSize implementation
FixedSize::FixedSize(double size) : Sizer(), stake(size) {
}

double FixedSize::_getsizing(std::shared_ptr<CommInfo> comminfo,
                            double cash,
                            std::shared_ptr<DataSeries> data,
                            bool isbuy) {
    return stake;
}

// AllInSizer implementation
double AllInSizer::_getsizing(std::shared_ptr<CommInfo> comminfo,
                             double cash,
                             std::shared_ptr<DataSeries> data,
                             bool isbuy) {
    if (!data || data->empty() || !comminfo) {
        return 0.0;
    }
    
    // Get current price
    double price = (*data->lines->getline(DataSeries::Close))[0];
    if (price <= 0.0) {
        return 0.0;
    }
    
    // Calculate maximum size based on available cash
    double size = comminfo->getsize(price, cash);
    return std::floor(size);
}

// PercentSizer implementation
PercentSizer::PercentSizer(double percent) : Sizer(), percents(percent) {
}

double PercentSizer::_getsizing(std::shared_ptr<CommInfo> comminfo,
                               double cash,
                               std::shared_ptr<DataSeries> data,
                               bool isbuy) {
    if (!data || data->empty() || !comminfo) {
        return 0.0;
    }
    
    // Get current price
    double price = (*data->lines->getline(DataSeries::Close))[0];
    if (price <= 0.0) {
        return 0.0;
    }
    
    // Calculate percentage of available cash to use
    double available_cash = cash * (percents / 100.0);
    
    // Calculate size
    double size = comminfo->getsize(price, available_cash);
    
    if (retint) {
        return std::floor(size);
    }
    
    return size;
}

} // namespace backtrader