#include "filters/datafilter.h"

namespace backtrader {
namespace filters {

DataFilter::DataFilter(std::shared_ptr<AbstractDataBase> dataname, const Params& params)
    : AbstractDataBase(), p(params), dataname_(dataname) {
}

void DataFilter::preload() {
    if (!dataname_) {
        return;
    }
    
    // Check if data is not preloaded and preload it
    if (dataname_->size() == dataname_->buflen()) {
        dataname_->start();
        dataname_->preload();
        dataname_->home();
    }
    
    // Copy timeframe info from underlying data after start
    copy_timeframe_info();
    
    // Call parent preload
    AbstractDataBase::preload();
}

bool DataFilter::next() {
    return _load();
}

void DataFilter::start() {
    AbstractDataBase::start();
    if (dataname_) {
        dataname_->start();
    }
}

void DataFilter::stop() {
    if (dataname_) {
        dataname_->stop();
    }
    AbstractDataBase::stop();
}

size_t DataFilter::size() const {
    if (dataname_) {
        return dataname_->size();
    }
    return 0;
}

void DataFilter::home() {
    if (dataname_) {
        dataname_->home();
    }
    AbstractDataBase::home();
}

bool DataFilter::_load() {
    if (!dataname_) {
        return false;
    }
    
    if (dataname_->size() == 0) {
        dataname_->start();  // start data if not done somewhere else
    }
    
    // Tell underlying source to get next data
    while (dataname_->next()) {
        // Try to load the data from the underlying source
        if (p.funcfilter && !p.funcfilter(dataname_)) {
            continue;  // Skip this bar
        }
        
        // Data is allowed - Copy data from underlying source
        // This would need proper implementation based on the line structure
        // For now, we'll assume the data copying mechanism exists
        copy_lines_from(dataname_);
        
        return true;
    }
    
    return false;  // no more data from underlying source
}

void DataFilter::copy_timeframe_info() {
    if (dataname_) {
        // Copy timeframe information from underlying data source
        // This would need to be implemented based on the timeframe structure
        timeframe_ = dataname_->get_timeframe();
        compression_ = dataname_->get_compression();
    }
}

} // namespace filters
} // namespace backtrader