#include "flt.h"
#include "dataseries.h"

namespace backtrader {

Filter::Filter(std::shared_ptr<DataSeries> data) {
    // Base constructor - can be overridden by derived classes
}

void Filter::operator()(std::shared_ptr<DataSeries> data) {
    // Handle first time logic
    if (first_time_) {
        nextstart(data);
        first_time_ = false;
    }
    
    // Call next method
    next(data);
}

void Filter::nextstart(std::shared_ptr<DataSeries> data) {
    // Default implementation does nothing
    // Override in derived classes for first-time initialization
}

void Filter::next(std::shared_ptr<DataSeries> data) {
    // Default implementation does nothing
    // Override in derived classes for data processing
}

} // namespace backtrader