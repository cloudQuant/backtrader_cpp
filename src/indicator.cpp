#include "indicator.h"
#include <algorithm>

namespace backtrader {

Indicator::Indicator() : IndicatorBase() {
    // Line type is already set as static constexpr in header
}

void Indicator::advance(size_t advance_size) {
    // Supporting datas with different lengths (timeframes)
    if (size() < (_clock ? _clock->size() : 0)) {
        LineMultiple::advance(advance_size);
    }
}

void Indicator::preonce_via_prenext(int start, int end) {
    // Generic implementation if prenext is overridden but preonce is not
    for (int i = start; i < end; ++i) {
        // Advance all data sources
        for (auto& data : datas) {
            data->advance();
        }
        
        // Advance all indicators
        for (auto& indicator : _lineiterators[LineIterator::IndType]) {
            indicator->advance();
        }
        
        // Advance self
        advance();
        
        // Call prenext
        prenext();
    }
}

void Indicator::oncestart_via_nextstart(int start, int end) {
    // nextstart has been overridden, but oncestart has not
    for (int i = start; i < end; ++i) {
        // Advance all data sources
        for (auto& data : datas) {
            data->advance();
        }
        
        // Advance all indicators
        for (auto& indicator : _lineiterators[LineIterator::IndType]) {
            indicator->advance();
        }
        
        // Advance self
        advance();
        
        // Call nextstart
        nextstart();
    }
}

void Indicator::once_via_next(int start, int end) {
    // next has been overridden, but once has not
    for (int i = start; i < end; ++i) {
        // Advance all data sources
        for (auto& data : datas) {
            data->advance();
        }
        
        // Advance all indicators
        for (auto& indicator : _lineiterators[LineIterator::IndType]) {
            indicator->advance();
        }
        
        // Advance self
        advance();
        
        // Call next
        next();
    }
}

} // namespace backtrader