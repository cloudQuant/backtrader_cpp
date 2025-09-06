#include "lineiterator.h"
#include "dataseries.h"
#include "indicators/crossover.h"
#include "indicators/sma.h"
#include "indicators/closeline.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cstdio>

namespace backtrader {

// Static member initialization
size_t LineIterator::global_bar_count_ = 0;

LineIterator::LineIterator() : LineSeries() {
    // Don't set _ltype here - let derived classes set it
    minperiod_ = 1; // Initialize inherited minperiod_ from LineRoot
}

void LineIterator::_next() {
    // Prevent multiple executions within the same global bar
    if (!should_execute_for_current_bar()) {
        return;  // Already executed for this bar
    }
    
    // Update clock
    _clk_update();
    
    // Execute child indicators
    static int debug_count = 0;
    debug_count++;
    if (debug_count <= 10) {
        std::cerr << "LineIterator::_next() - this=" << this 
                  << ", global_bar=" << global_bar_count_
                  << ", Executing " << _lineiterators[static_cast<int>(LineIterator::IndType)].size() 
                  << " indicators" << std::endl;
    }
    for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
        if (debug_count <= 10) {
            std::cerr << "LineIterator::_next() - Calling _next() on indicator " << indicator.get() 
                      << " with type=" << static_cast<int>(indicator->_ltype) 
                      << " (is CrossOver? " << (dynamic_cast<CrossOver*>(indicator.get()) ? "yes" : "no") << ")"
                      << std::endl;
        }
        indicator->_next();
    }
    
    // Call notification
    _notify();
    
    // DEBUG: Check if we reach this point for strategies
    if (debug_count <= 10) {
        std::cerr << "DEBUG_AFTER_INDICATORS LineIterator::_next() - About to check _ltype for this=" << this << std::endl;
    }
    
    // Debug: check _ltype for strategies
    static int ltype_debug_count = 0;
    if (ltype_debug_count++ < 10) {
        std::cerr << "DEBUG_LTYPE LineIterator::_next() - this=" << this << ", _ltype=" << static_cast<int>(_ltype) 
                  << " (StratType=" << static_cast<int>(LineRoot::IndType::StratType) 
                  << ", IndType=" << static_cast<int>(LineRoot::IndType::IndType) << ")" << std::endl;
    }
    
    // Execute strategy/indicator logic based on type
    if (_ltype == LineRoot::IndType::StratType) {
        // Strategy execution with minimum period status
        int minperstatus = _getminperstatus();
        static int strategy_debug_count = 0;
        if (strategy_debug_count++ < 50) {
            std::cerr << "LineIterator::_next() - STRATEGY #" << strategy_debug_count 
                      << " minperstatus=" << minperstatus << std::endl;
        }
        if (minperstatus < 0) {
            if (strategy_debug_count <= 20) {
                std::cerr << "LineIterator::_next() - CALLING STRATEGY NEXT() #" << strategy_debug_count << std::endl;
            }
            next();
        } else if (minperstatus == 0) {
            if (strategy_debug_count <= 20) {
                std::cerr << "LineIterator::_next() - CALLING STRATEGY NEXTSTART() #" << strategy_debug_count << std::endl;
            }
            nextstart();
        } else {
            if (strategy_debug_count <= 20) {
                std::cerr << "LineIterator::_next() - CALLING STRATEGY PRENEXT() #" << strategy_debug_count 
                          << " (minperstatus=" << minperstatus << ")" << std::endl;
            }
            prenext();
        }
    } else if (static_cast<int>(_ltype) == 0) {  // IndType = 0
        // Indicator execution - ALL indicators should execute prenext/nextstart/next
        if (debug_count <= 30) {
            std::cerr << "LineIterator::_next() - ENTERING INDICATOR BRANCH for " << this 
                      << ", _ltype=" << static_cast<int>(_ltype) << std::endl;
        }
        size_t current_len = 0;
        
        // For indicators, we need the actual bar count, not the total line size
        // The clock should be a DataSeries that tracks the current bar position
        if (_clock) {
            // Use clock's size() which should return current position after forward() calls
            current_len = _clock->size();
            
            // Extra debug for specific indicators
            if ((dynamic_cast<indicators::SMA*>(this) || dynamic_cast<indicators::CloseLine*>(this)) && debug_count <= 20) {
                std::cerr << "LineIterator::_next() - Indicator clock size: " << current_len 
                          << ", clock ptr=" << _clock.get() 
                          << ", indicator type=" << (dynamic_cast<indicators::SMA*>(this) ? "SMA" : "CloseLine") << std::endl;
            }
        } else {
            // Fallback to own size if no clock
            current_len = size();
            
            // If we still don't have a clock but we have an owner, try to get it from there
            if (current_len == 0 && _owner) {
                if (auto owner_iter = dynamic_cast<LineIterator*>(_owner)) {
                    if (owner_iter->_clock) {
                        current_len = owner_iter->_clock->size();
                    }
                }
            }
        }
        
        bool is_crossover = dynamic_cast<CrossOver*>(this) != nullptr;
        bool is_crossbase = dynamic_cast<CrossBase*>(this) != nullptr;
        bool is_nzd = dynamic_cast<NonZeroDifference*>(this) != nullptr;
        bool is_sma = dynamic_cast<indicators::SMA*>(this) != nullptr;
        
        // Extra debug for SMA type checking
        if (debug_count <= 20) {
            std::string type_name = "unknown";
            if (dynamic_cast<indicators::SMA*>(this)) {
                type_name = "SMA";
            } else if (dynamic_cast<CrossOver*>(this)) {
                type_name = "CrossOver";
            } else if (dynamic_cast<NonZeroDifference*>(this)) {
                type_name = "NonZeroDifference";
            } else if (dynamic_cast<IndicatorBase*>(this)) {
                type_name = "IndicatorBase (generic)";
            }
            std::cerr << "LineIterator::_next() - Type check for " << this 
                      << ": type_name=" << type_name 
                      << ", _ltype=" << static_cast<int>(_ltype) 
                      << " (IndType=" << static_cast<int>(LineRoot::IndType::IndType) << ")" << std::endl;
        }
        
        if ((debug_count <= 10 && _ltype == LineRoot::IndType::IndType) || is_crossover || is_crossbase || is_nzd || is_sma) {
            std::cerr << "LineIterator::_next() - Indicator " << this 
                      << ": current_len=" << current_len << ", minperiod_=" << minperiod_ 
                      << ", is_crossover=" << is_crossover 
                      << ", is_crossbase=" << is_crossbase
                      << ", is_nzd=" << is_nzd
                      << ", is_sma=" << is_sma
                      << ", _clock=" << (_clock ? "set" : "null") << std::endl;
        }
        // Debug transition logic
        if (is_sma && debug_count <= 30) {
            std::cerr << "LineIterator::_next() - SMA transition check: current_len=" << current_len 
                      << ", minperiod_=" << minperiod_ 
                      << ", condition: ";
            if (current_len > minperiod_) {
                std::cerr << "current_len > minperiod_ -> calling next()";
            } else if (current_len == minperiod_) {
                std::cerr << "current_len == minperiod_ -> calling nextstart()";
            } else if (current_len > 0) {
                std::cerr << "current_len > 0 -> calling prenext()";
            } else {
                std::cerr << "current_len <= 0 -> skipping";
            }
            std::cerr << std::endl;
        }
        
        // Call prenext/nextstart/next for ALL indicators based on minperiod
        bool is_closeline = dynamic_cast<indicators::CloseLine*>(this) != nullptr;
        
        // Extra debug for specific indicators
        bool is_minperiod_one = (minperiod_ == 1);
        
        if (is_minperiod_one && current_len == 1) {
            std::cerr << "DEBUG: Indicator with minperiod=1 at " << this 
                      << ", current_len=" << current_len 
                      << " -> should call nextstart()" << std::endl;
        }
        
        if (current_len > minperiod_) {
            if (debug_count <= 20 || is_closeline || is_minperiod_one) {
                std::cerr << "LineIterator::_next() - Calling next() for indicator " << this 
                          << " (current_len=" << current_len << " > minperiod_=" << minperiod_ << ")"
                          << (is_minperiod_one ? " [minperiod=1]" : "") << std::endl;
            }
            next();
        } else if (current_len == minperiod_) {
            if (debug_count <= 20 || is_closeline || is_minperiod_one) {
                std::cerr << "LineIterator::_next() - Calling nextstart() for indicator " << this 
                          << " (current_len=" << current_len << " == minperiod_=" << minperiod_ << ")"
                          << (is_minperiod_one ? " [minperiod=1]" : "") << std::endl;
            }
            nextstart();
        } else if (current_len > 0) {
            if (debug_count <= 20 || is_closeline || is_minperiod_one) {
                std::cerr << "LineIterator::_next() - Calling prenext() for indicator " << this 
                          << " (current_len=" << current_len << " < minperiod_=" << minperiod_ << ")"
                          << (is_minperiod_one ? " [minperiod=1]" : "") << std::endl;
            }
            prenext();
        }
    }
}

void LineIterator::_once() {
    std::cerr << "LineIterator::_once() called on " << this << std::endl;
    // Forward all lines to full length
    forward(buflen());
    
    // Execute child indicators in once mode
    for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
        std::cerr << "LineIterator::_once() calling child indicator->_once() on " << indicator.get() << std::endl;
        indicator->_once();
    }
    
    // Forward observers
    for (auto& observer : _lineiterators[ObsType]) {
        observer->forward(buflen());
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
    
    // Execute the three phases of once processing
    preonce(0, minperiod_ - 1);
    oncestart(minperiod_ - 1, minperiod_);
    once(minperiod_, buflen());
    
    // Execute binding synchronization
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

void LineIterator::_periodrecalc() {
    // Recalculate minimum period based on child indicators
    auto indicators = _lineiterators[IndType];
    std::vector<size_t> indperiods;
    
    for (auto& ind : indicators) {
        indperiods.push_back(ind->_minperiod());
    }
    
    size_t indminperiod = indperiods.empty() ? minperiod_ : 
                         *std::max_element(indperiods.begin(), indperiods.end());
    
    updateminperiod(indminperiod);
}

void LineIterator::_stage1() {
    LineSeries::_stage1();
    
    // Stage1 for all data sources
    for (auto& data : datas) {
        data->_stage1();
    }
    
    // Stage1 for all line iterators
    for (auto& lineiterators : _lineiterators) {
        for (auto& lineiterator : lineiterators.second) {
            lineiterator->_stage1();
        }
    }
}

void LineIterator::_stage2() {
    LineSeries::_stage2();
    
    // Stage2 for all data sources
    for (auto& data : datas) {
        data->_stage2();
    }
    
    // Stage2 for all line iterators
    for (auto& lineiterators : _lineiterators) {
        for (auto& lineiterator : lineiterators.second) {
            lineiterator->_stage2();
        }
    }
}

std::vector<std::shared_ptr<LineIterator>> LineIterator::getindicators() {
    return _lineiterators[IndType];
}

std::vector<std::shared_ptr<LineIterator>> LineIterator::getindicators_lines() {
    std::vector<std::shared_ptr<LineIterator>> result;
    for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
        if (indicator->lines && indicator->lines->get_aliases().size() > 0) {
            result.push_back(indicator);
        }
    }
    return result;
}

std::vector<std::shared_ptr<LineIterator>> LineIterator::getobservers() {
    return _lineiterators[static_cast<int>(LineIterator::ObsType)];
}

void LineIterator::addindicator(std::shared_ptr<LineIterator> indicator) {
    std::cerr << "LineIterator::addindicator - Adding indicator " << indicator.get() 
              << " with _ltype=" << static_cast<int>(indicator->_ltype) 
              << " to strategy " << this << std::endl;
    _lineiterators[static_cast<int>(indicator->_ltype)].push_back(indicator);
    std::cerr << "LineIterator::addindicator - _lineiterators[" << static_cast<int>(indicator->_ltype) << "] now has " 
              << _lineiterators[static_cast<int>(indicator->_ltype)].size() << " indicators" << std::endl;
    
    // Set the owner of the indicator
    indicator->_owner = this;
    
    // Set clock if not already set
    std::cerr << "LineIterator::addindicator - indicator->_clock=" << indicator->_clock.get() 
              << ", this->_clock=" << _clock.get() << std::endl;
    if (!indicator->_clock) {
        if (_clock) {
            indicator->_clock = _clock;
            std::cerr << "LineIterator::addindicator - Set indicator clock to strategy clock: " 
                      << _clock.get() << std::endl;
        } else if (!datas.empty()) {
            indicator->_clock = datas[0];
            std::cerr << "LineIterator::addindicator - Set indicator clock to datas[0]: " 
                      << datas[0].get() << std::endl;
        }
    } else {
        std::cerr << "LineIterator::addindicator - Indicator already has clock: " 
                  << indicator->_clock.get() << std::endl;
    }
    
    // Handle nextforce flag
    if (indicator->_nextforce) {
        auto o = this;
        while (o != nullptr) {
            if (o->_ltype == LineRoot::IndType::StratType) {
                // Disable runonce mode (would need cerebro reference)
                break;
            }
            o = dynamic_cast<LineIterator*>(o->_owner);
        }
    }
}

LineIterator& LineIterator::bindlines(const std::vector<int>& owner, 
                                     const std::vector<int>& own) {
    std::vector<int> owner_indices = owner.empty() ? std::vector<int>{0} : owner;
    std::vector<int> own_indices = own.empty() ? 
        std::vector<int>(owner_indices.size()) : own;
    
    // Fill own_indices with sequential values if empty
    if (own.empty()) {
        std::iota(own_indices.begin(), own_indices.end(), 0);
    }
    
    for (size_t i = 0; i < owner_indices.size() && i < own_indices.size(); ++i) {
        int owner_idx = owner_indices[i];
        int own_idx = own_indices[i];
        
        if (_owner && owner_idx < static_cast<int>(_owner->lines->size()) &&
            own_idx < static_cast<int>(lines->size())) {
            
            auto owner_line = _owner->lines->getline(owner_idx);
            auto own_line = lines->getline(own_idx);
            
            own_line->addbinding(owner_line);
        }
    }
    
    return *this;
}

LineIterator& LineIterator::bind2lines(const std::vector<int>& owner, 
                                      const std::vector<int>& own) {
    return bindlines(owner, own);
}

LineIterator& LineIterator::bind2line(const std::vector<int>& owner, 
                                     const std::vector<int>& own) {
    return bindlines(owner, own);
}

void LineIterator::_clk_update() {
    if (!_clock) {
        return;
    }
    
    size_t clock_len = _clock->size();
    if (clock_len != size()) {
        forward();
    }
}

void LineIterator::qbuffer(size_t savemem) {
    if (savemem > 0) {
        for (auto& line : lines_) {
            if (auto buffer = dynamic_cast<LineBuffer*>(line.get())) {
                buffer->qbuffer();
            }
        }
    }
    
    // Propagate to child indicators
    for (auto& obj : _lineiterators[IndType]) {
        obj->qbuffer(1);
    }
    
    // Tell data sources to adjust buffer
    for (auto& data : datas) {
        if (auto buffer = dynamic_cast<LineBuffer*>(data.get())) {
            buffer->minbuffer(minperiod_);
        }
    }
}

int LineIterator::_getminperstatus() {
    if (datas.empty()) {
        return -1;
    }
    
    // For strategies, we need to check the current bar number (how many times forward was called)
    // not the total data size
    size_t current_bar = 0;
    if (datas[0]) {
        // Get the current position in the data
        // This is a simplified approach - in reality we'd track the bar count
        auto lines_obj = std::dynamic_pointer_cast<LineSeries>(datas[0]);
        if (lines_obj && lines_obj->lines && lines_obj->lines->size() > 0) {
            auto first_line = lines_obj->lines->getline(0);
            if (auto buffer = std::dynamic_pointer_cast<LineBuffer>(first_line)) {
                current_bar = buffer->get_idx() + 1; // idx is 0-based, we want 1-based bar count
            }
        }
    }
    
    if (current_bar < minperiod_) {
        return 1; // prenext
    } else if (current_bar == minperiod_) {
        return 0; // nextstart
    } else {
        return -1; // next
    }
}

void LineIterator::_setup_data_aliases() {
    if (!datas.empty()) {
        data = datas[0];
        
        // Set up data line aliases (data_open, data_high, etc.)
        // This would be implemented with proper string handling
        // For now, we'll skip the complex string alias setup
    }
}

// DataAccessor implementation
DataAccessor::DataAccessor() : LineIterator() {
}

// IndicatorBase implementation
IndicatorBase::IndicatorBase() : DataAccessor() {
    _ltype = LineRoot::IndType::IndType;
}

size_t IndicatorBase::size() const {
    if (lines_.empty()) {
        return 0;
    }
    return lines_[0]->size();
}

std::shared_ptr<LineSingle> IndicatorBase::getLine(size_t idx) const {
    if (idx >= lines_.size()) {
        throw std::out_of_range("Line index out of range");
    }
    return lines_[idx];
}

double IndicatorBase::get(int ago) const {
    if (lines_.empty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines_[0];
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int IndicatorBase::getMinPeriod() const {
    return static_cast<int>(minperiod_);
}

// ObserverBase implementation
ObserverBase::ObserverBase() : DataAccessor() {
    _ltype = LineRoot::IndType::ObsType;
}

// StrategyBase implementation
StrategyBase::StrategyBase() : DataAccessor() {
    _ltype = LineRoot::IndType::StratType;
}

// SingleCoupler implementation
SingleCoupler::SingleCoupler(std::shared_ptr<LineSeries> cdata, 
                            std::shared_ptr<LineSeries> clock)
    : LineActions(), cdata_(cdata), _clock(clock ? clock : cdata), 
      dlen_(0), val_(std::numeric_limits<double>::quiet_NaN()) {
}

void SingleCoupler::next() {
    if (cdata_->size() > dlen_) {
        val_ = (*cdata_)[0];
        dlen_++;
    }
    
    set(0, val_);
}

// MultiCoupler implementation
MultiCoupler::MultiCoupler() : LineIterator(), dlen_(0), dsize_(0) {
    _ltype = LineRoot::IndType::IndType;
    dsize_ = fullsize();
    dvals_.resize(dsize_, std::numeric_limits<double>::quiet_NaN());
}

void MultiCoupler::next() {
    if (!data || data->size() <= dlen_) {
        return;
    }
    
    dlen_++;
    
    for (size_t i = 0; i < dsize_; ++i) {
        if (i < data->lines->size()) {
            dvals_[i] = (*data->lines->getline(i))[0];
        }
    }
    
    for (size_t i = 0; i < dsize_ && i < lines_.size(); ++i) {
        lines_[i]->set(0, dvals_[i]);
    }
}

// Factory function
std::shared_ptr<LineIterator> LinesCoupler(std::shared_ptr<LineSeries> cdata,
                                          std::shared_ptr<LineSeries> clock) {
    // For now, always return MultiCoupler until we fix SingleCoupler inheritance
    auto coupler = std::make_shared<MultiCoupler>();
    coupler->datas.push_back(cdata);
    coupler->data = cdata;
    coupler->_clock = clock ? clock : cdata;
    
    return coupler;
}

// Global bar management methods
void LineIterator::increment_global_bar() {
    global_bar_count_++;
}

size_t LineIterator::get_global_bar_count() {
    return global_bar_count_;
}

void LineIterator::reset_global_bar_count() {
    global_bar_count_ = 0;
}

bool LineIterator::should_execute_for_current_bar() {
    // Check if we've already executed for the current global bar
    if (last_executed_bar_ == global_bar_count_) {
        return false;  // Already executed for this bar
    }
    
    // Mark this bar as executed
    last_executed_bar_ = global_bar_count_;
    return true;  // Should execute
}

} // namespace backtrader