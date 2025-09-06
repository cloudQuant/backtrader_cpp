#include "analyzers/timereturn.h"
#include "strategy.h"
#include <sstream>
#include <iomanip>

namespace backtrader {

TimeReturn::TimeReturn() : TimeFrameAnalyzerBase(), value_start_(0.0), last_value_(0.0), 
                          current_value_(0.0), fundmode_(false) {
    // Initialize with default parameters
}

TimeReturn::TimeReturn(const std::string& name) : TimeFrameAnalyzerBase(), value_start_(0.0), last_value_(0.0), 
                          current_value_(0.0), fundmode_(false) {
    // Initialize with default parameters, name parameter is for compatibility
    (void)name; // Suppress unused parameter warning
}

TimeReturn::TimeReturn(const std::string& name, TimeFrame timeframe) : TimeFrameAnalyzerBase(), value_start_(0.0), last_value_(0.0), 
                          current_value_(0.0), fundmode_(false) {
    // Initialize with specified timeframe
    (void)name; // Suppress unused parameter warning for now
    params.timeframe = static_cast<int>(timeframe);
    // Also set the parent class's timeframe
    p.timeframe = timeframe;
    this->timeframe = timeframe;
}

void TimeReturn::start() {
    // std::cerr << "TimeReturn::start() - entry, strategy=" << strategy.get() << std::endl;
    TimeFrameAnalyzerBase::start();
    
    // Auto-detect fund mode if needed
    if (params.auto_fund && strategy) {
        // fundmode_ = strategy->broker->get_fundmode(); // TODO: implement when broker methods are available
        fundmode_ = params.fund; // For now, use the default
    } else {
        fundmode_ = params.fund;
    }
    
    // Initialize values
    value_start_ = 0.0;
    returns_.clear();
    
    // Set initial portfolio value from broker (matches Python lines 106-111)
    if (strategy && strategy->broker) {
        if (!fundmode_) {
            // Keep the initial portfolio value if not tracking data
            last_value_ = strategy->broker->getvalue();
            current_value_ = last_value_;
            // Don't set value_start here - it will be set in on_dt_over
        } else {
            // TODO: implement fundvalue when available
            last_value_ = strategy->broker->getvalue();  
            current_value_ = last_value_;
            // Don't set value_start here - it will be set in on_dt_over
        }
    } else {
        last_value_ = 0.0;
        current_value_ = 0.0;
    }
    std::cerr << "TimeReturn::start() - completed, initial value=" << current_value_ 
              << ", value_start=" << value_start_ << ", last_value=" << last_value_ << std::endl;
}

void TimeReturn::next() {
    TimeFrameAnalyzerBase::next();
    
    next_call_count_++;
    
    // Calculate return for current period
    // This matches Python: self.rets[self.dtkey] = (self._value / self._value_start) - 1.0
    std::string date_key = get_current_date_key();
    std::cerr << "TimeReturn::next() - value_start_=" << value_start_ 
              << ", current_value_=" << current_value_ 
              << ", date_key=" << date_key << std::endl;
    if (value_start_ > 0.0 && current_value_ > 0.0) {
        double return_value = (current_value_ / value_start_) - 1.0;
        returns_[date_key] = return_value;
        std::cerr << "TimeReturn::next() - Added return " << return_value 
                  << " for date " << date_key 
                  << " (current=" << current_value_ 
                  << ", start=" << value_start_ << ")" << std::endl;
    } else {
        std::cerr << "TimeReturn::next() - SKIPPED return calculation!" << std::endl;
    }
    
    // Update last value - Python: self._lastvalue = self._value
    last_value_ = current_value_;
}

void TimeReturn::notify_fund(double cash, double value, double fundvalue, double shares) {
    notify_fund_call_count_++;
    
    std::cerr << "TimeReturn::notify_fund() called - cash=" << cash 
              << ", value=" << value << ", fundvalue=" << fundvalue << std::endl;
    
    // Update current value based on fund mode - matches Python logic
    if (!fundmode_) {
        current_value_ = value;  // Portfolio value if tracking no data
    } else {
        current_value_ = fundvalue;  // Fund value if in fund mode
    }
    
    // std::cerr << "TimeReturn::notify_fund() - current_value_ set to " << current_value_ << std::endl;
}

void TimeReturn::on_dt_over() {
    on_dt_over_call_count_++;
    
    std::cerr << "TimeReturn::on_dt_over() called - last_value_=" << last_value_ 
              << ", current_value_=" << current_value_ << std::endl;
    
    // Called when timeframe period ends - update value_start for next period
    // Python: if self._lastvalue is not None: self._value_start = self._lastvalue
    // Only update if we have a valid last_value
    if (last_value_ > 0.0) {
        value_start_ = last_value_;
    }
    
    std::cerr << "TimeReturn::on_dt_over() - set value_start_ to " << value_start_ << std::endl;
}

AnalysisResult TimeReturn::get_analysis() const {
    AnalysisResult result;
    result["returns"] = returns_;
    return result;
}

std::vector<double> TimeReturn::get_returns() const {
    std::vector<double> returns_vector;
    returns_vector.reserve(returns_.size());
    
    for (const auto& pair : returns_) {
        returns_vector.push_back(pair.second);
    }
    
    return returns_vector;
}

std::string TimeReturn::get_current_date_key() const {
    // Generate date key from the actual datetime
    // Try to get data from analyzer's datas or strategy's datas
    std::shared_ptr<DataSeries> data_series;
    
    if (!datas.empty()) {
        data_series = datas[0];
    } else if (strategy) {
        if (!strategy->datas.empty()) {
            // Try to cast strategy's data to DataSeries
            data_series = std::dynamic_pointer_cast<DataSeries>(strategy->datas[0]);
        }
    }
    
    if (data_series) {
            double dt_double = data_series->datetime(0);
            auto time_t = static_cast<std::time_t>(dt_double);
            
            // Check if time_t is valid before using localtime
            if (time_t <= 0) {
                // Use fallback counter method
                static int period_counter = 0;
                period_counter++;
                std::stringstream ss;
                ss << "period_" << std::setfill('0') << std::setw(4) << period_counter;
                return ss.str();
            }
            
            auto tm_ptr = std::localtime(&time_t);
            if (!tm_ptr) {
                // localtime failed, use fallback
                static int period_counter = 0;
                period_counter++;
                std::stringstream ss;
                ss << "period_" << std::setfill('0') << std::setw(4) << period_counter;
                return ss.str();
            }
            auto tm = *tm_ptr;
            
            std::stringstream ss;
            // Format based on timeframe
            TimeFrame tf = static_cast<TimeFrame>(params.timeframe);
            if (tf == TimeFrame::NoTimeFrame && p.timeframe != TimeFrame::NoTimeFrame) {
                tf = p.timeframe;
            }
            
            switch (tf) {
                case TimeFrame::Years:
                    ss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900);
                    break;
                case TimeFrame::Months:
                    ss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)
                       << "-" << std::setw(2) << (tm.tm_mon + 1);
                    break;
                case TimeFrame::Weeks:
                    // For weeks, use year-week format
                    ss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)
                       << "-W" << std::setw(2) << (tm.tm_yday / 7 + 1);
                    break;
                case TimeFrame::Days:
                default:
                    ss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)
                       << "-" << std::setw(2) << (tm.tm_mon + 1)
                       << "-" << std::setw(2) << tm.tm_mday;
                    break;
            }
            return ss.str();
    }
    
    // Fallback
    static int period_counter = 0;
    period_counter++;
    std::stringstream ss;
    ss << "period_" << std::setfill('0') << std::setw(4) << period_counter;
    return ss.str();
}

} // namespace backtrader