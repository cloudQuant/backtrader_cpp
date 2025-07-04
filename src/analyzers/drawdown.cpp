#include "analyzers/drawdown.h"
#include "strategy.h"
#include "broker.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace analyzers {

// DrawDown implementation
DrawDown::DrawDown(const Params& params) : Analyzer(), p(params) {
    create_analysis();
}

void DrawDown::start() {
    Analyzer::start();
    
    // Auto-detect fund mode if needed
    if (p.auto_fund && strategy && strategy->get_broker()) {
        _fundmode = strategy->get_broker()->get_fundmode();
    } else {
        _fundmode = p.fund;
    }
}

void DrawDown::create_analysis() {
    // Initialize analysis results (matching Python AutoOrderedDict behavior)
    rets = DrawDownResults{};
    rets.len = 0;
    rets.drawdown = 0.0;
    rets.moneydown = 0.0;
    
    rets.max.len = 0;
    rets.max.drawdown = 0.0;
    rets.max.moneydown = 0.0;
    
    _maxvalue = std::numeric_limits<double>::lowest(); // any value will outdo it
}

void DrawDown::stop() {
    rets._closed = true; // . notation cannot create more keys (Python equivalent)
}

void DrawDown::notify_fund(double cash, double value, double fundvalue, double shares) {
    if (!_fundmode) {
        _value = value; // record current value
        _maxvalue = std::max(_maxvalue, value); // update peak value
    } else {
        _value = fundvalue; // record current value
        _maxvalue = std::max(_maxvalue, fundvalue); // update peak
    }
}

void DrawDown::next() {
    update_drawdown_stats();
}

void DrawDown::update_drawdown_stats() {
    // Calculate current drawdown values
    rets.moneydown = _maxvalue - _value;
    rets.drawdown = (_maxvalue != 0.0) ? (100.0 * rets.moneydown / _maxvalue) : 0.0;
    
    // Maximum drawdown values
    rets.max.moneydown = std::max(rets.max.moneydown, rets.moneydown);
    rets.max.drawdown = std::max(rets.max.drawdown, rets.drawdown);
    
    // Drawdown length tracking
    rets.len = (rets.drawdown > 0.0) ? rets.len + 1 : 0;
    rets.max.len = std::max(rets.max.len, rets.len);
}

AnalysisResult DrawDown::get_analysis() const {
    AnalysisResult result;
    
    // Current values
    result["drawdown"] = rets.drawdown;
    result["moneydown"] = rets.moneydown;
    result["len"] = static_cast<double>(rets.len);
    
    // Maximum values in nested structure
    std::map<std::string, double> max_values;
    max_values["drawdown"] = rets.max.drawdown;
    max_values["moneydown"] = rets.max.moneydown;
    max_values["len"] = static_cast<double>(rets.max.len);
    result["max"] = max_values;
    
    return result;
}

// TimeDrawDown implementation
TimeDrawDown::TimeDrawDown(const Params& params) 
    : TimeFrameAnalyzerBase(static_cast<const TimeFrameAnalyzerBase::Params&>(params)), p(params) {
    dd = 0.0;
    maxdd = 0.0;
    maxddlen = 0;
    peak = std::numeric_limits<double>::lowest();
    ddlen = 0;
}

void TimeDrawDown::start() {
    TimeFrameAnalyzerBase::start();
    
    // Auto-detect fund mode if needed
    if (p.auto_fund && strategy && strategy->get_broker()) {
        _fundmode = strategy->get_broker()->get_fundmode();
    } else {
        _fundmode = p.fund;
    }
    
    // Initialize parameters
    dd = 0.0;
    maxdd = 0.0;
    maxddlen = 0;
    peak = std::numeric_limits<double>::lowest();
    ddlen = 0;
}

void TimeDrawDown::on_dt_over() {
    double value = get_current_value();
    
    // Update the maximum seen peak
    if (value > peak) {
        peak = value;
        ddlen = 0; // start of streak
    }
    
    // Calculate the current drawdown
    dd = (peak != 0.0) ? (100.0 * (peak - value) / peak) : 0.0;
    ddlen += (dd > 0.0) ? 1 : 0; // if peak == value -> dd = 0
    
    // Update the maxdrawdown if needed
    maxdd = std::max(maxdd, dd);
    maxddlen = std::max(maxddlen, ddlen);
}

void TimeDrawDown::stop() {
    // Add final results to analysis
    rets["maxdrawdown"] = maxdd;
    rets["maxdrawdownperiod"] = static_cast<double>(maxddlen);
    
    TimeFrameAnalyzerBase::stop();
}

AnalysisResult TimeDrawDown::get_analysis() const {
    AnalysisResult result = TimeFrameAnalyzerBase::get_analysis();
    
    // Add time-based drawdown results
    result["maxdrawdown"] = maxdd;
    result["maxdrawdownperiod"] = static_cast<double>(maxddlen);
    result["currentdrawdown"] = dd;
    result["currentdrawdownlen"] = static_cast<double>(ddlen);
    
    return result;
}

double TimeDrawDown::get_current_value() const {
    if (!strategy || !strategy->get_broker()) {
        return 0.0;
    }
    
    if (!_fundmode) {
        return strategy->get_broker()->get_value();
    } else {
        return strategy->get_broker()->get_fundvalue();
    }
}

} // namespace analyzers
} // namespace backtrader