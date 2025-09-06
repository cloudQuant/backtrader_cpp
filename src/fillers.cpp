#include "fillers.h"
#include "order.h"
#include "dataseries.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// FixedSize implementation
FixedSize::FixedSize(double size) {
    params.size = size;
}

double FixedSize::operator()(std::shared_ptr<Order> order, double price, int ago) {
    if (!order || !order->data) {
        return 0.0;
    }
    
    // Get the maximum size limit (0 means unlimited)
    double size_limit = (params.size > 0.0) ? params.size : std::numeric_limits<double>::max();
    
    // Get current volume from data
    double volume = 0.0;
    if (order->data->lines && order->data->lines->size() > static_cast<size_t>(DataSeries::Volume)) {
        auto volume_line = order->data->lines->getline(DataSeries::Volume);
        if (volume_line && volume_line->size() > 0) {
            volume = (*volume_line)[ago];
        }
    }
    
    // Get remaining order size
    double rem_size = std::abs(order->executed.remsize);
    
    // Return minimum of volume, remaining size, and size limit
    return std::min({volume, rem_size, size_limit});
}

// FixedBarPerc implementation
FixedBarPerc::FixedBarPerc(double perc) {
    params.perc = std::clamp(perc, 0.0, 100.0);
}

double FixedBarPerc::operator()(std::shared_ptr<Order> order, double price, int ago) {
    if (!order || !order->data) {
        return 0.0;
    }
    
    // Get current volume from data
    double volume = 0.0;
    if (order->data->lines && order->data->lines->size() > static_cast<size_t>(DataSeries::Volume)) {
        auto volume_line = order->data->lines->getline(DataSeries::Volume);
        if (volume_line && volume_line->size() > 0) {
            volume = (*volume_line)[ago];
        }
    }
    
    // Calculate percentage of volume
    double max_size = std::floor((volume * params.perc) / 100.0);
    
    // Get remaining order size
    double rem_size = std::abs(order->executed.remsize);
    
    // Return minimum of calculated size and remaining order size
    return std::min(max_size, rem_size);
}

// BarPointPerc implementation
BarPointPerc::BarPointPerc(double minmov, double perc) {
    params.minmov = minmov;
    params.perc = std::clamp(perc, 0.0, 100.0);
}

double BarPointPerc::calculate_parts(double high, double low, double minmov) const {
    if (minmov <= 0.0) {
        return 1.0;
    }
    
    // Calculate number of price parts in the high-low range
    return std::floor((high - low + minmov) / minmov);
}

double BarPointPerc::operator()(std::shared_ptr<Order> order, double price, int ago) {
    if (!order || !order->data) {
        return 0.0;
    }
    
    auto data = order->data;
    
    // Get OHLCV data
    double high = 0.0, low = 0.0, volume = 0.0;
    
    if (data->lines) {
        if (auto high_line = data->lines->getline(DataSeries::High)) {
            if (high_line->size() > 0) high = (*high_line)[ago];
        }
        if (auto low_line = data->lines->getline(DataSeries::Low)) {
            if (low_line->size() > 0) low = (*low_line)[ago];
        }
        if (auto volume_line = data->lines->getline(DataSeries::Volume)) {
            if (volume_line->size() > 0) volume = (*volume_line)[ago];
        }
    }
    
    // Calculate number of parts
    double parts = calculate_parts(high, low, params.minmov);
    
    // Calculate allocated volume for this price point
    double alloc_vol = std::floor(((volume / parts) * params.perc) / 100.0);
    
    // Get remaining order size
    double rem_size = std::abs(order->executed.remsize);
    
    // Return minimum of allocated volume and remaining order size
    return std::min(alloc_vol, rem_size);
}

// Factory functions
std::shared_ptr<FillerBase> create_fixed_size_filler(double size) {
    return std::make_shared<FixedSize>(size);
}

std::shared_ptr<FillerBase> create_fixed_bar_perc_filler(double perc) {
    return std::make_shared<FixedBarPerc>(perc);
}

std::shared_ptr<FillerBase> create_bar_point_perc_filler(double minmov, double perc) {
    return std::make_shared<BarPointPerc>(minmov, perc);
}

} // namespace backtrader