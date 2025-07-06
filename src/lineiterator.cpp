#include "lineiterator.h"
#include "dataseries.h"
#include <algorithm>
#include <stdexcept>

namespace backtrader {

LineIterator::LineIterator() : LineSeries(), minperiod_(1) {
    _ltype = static_cast<LineRoot::IndType>(LineRoot::IndType::IndType);
}

void LineIterator::_next() {
    // Update clock
    _clk_update();
    
    // Execute child indicators
    for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
        indicator->_next();
    }
    
    // Call notification
    _notify();
    
    // Execute strategy/indicator logic based on type
    if (_ltype == LineRoot::IndType::StratType) {
        // Strategy execution with minimum period status
        int minperstatus = _getminperstatus();
        if (minperstatus < 0) {
            next();
        } else if (minperstatus == 0) {
            nextstart();
        } else {
            prenext();
        }
    } else {
        // Indicator/Observer execution
        size_t current_len = _clock ? _clock->size() : size();
        if (current_len > minperiod_) {
            next();
        } else if (current_len == minperiod_) {
            nextstart();
        } else if (current_len > 0) {
            prenext();
        }
    }
}

void LineIterator::_once() {
    // Forward all lines to full length
    forward(buflen());
    
    // Execute child indicators in once mode
    for (auto& indicator : _lineiterators[static_cast<int>(LineIterator::IndType)]) {
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
    _lineiterators[static_cast<int>(indicator->_ltype)].push_back(indicator);
    
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
    
    size_t data_len = datas[0]->size();
    if (data_len < minperiod_) {
        return 1; // prenext
    } else if (data_len == minperiod_) {
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

std::shared_ptr<LineSingle> IndicatorBase::getLine(size_t idx) {
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

} // namespace backtrader