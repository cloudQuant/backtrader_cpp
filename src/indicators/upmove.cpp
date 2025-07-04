#include "indicators/upmove.h"
#include <limits>

namespace backtrader {
namespace indicators {

// UpMove implementation
UpMove::UpMove() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(2);
}

UpMove::UpMove(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    _minperiod(2);
}

double UpMove::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::upmove);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int UpMove::getMinPeriod() const {
    return 2;
}

void UpMove::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void UpMove::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void UpMove::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto upmove_line = lines->getline(Lines::upmove);
    
    if (!data_line || !upmove_line) return;
    
    // UpMove = max(high - high[-1], 0)
    double current_high = (*data_line)[0];
    double prev_high = (*data_line)[-1];
    double upmove = std::max(current_high - prev_high, 0.0);
    
    upmove_line->set(0, upmove);
}

void UpMove::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto upmove_line = lines->getline(Lines::upmove);
    
    if (!data_line || !upmove_line) return;
    
    for (int i = start; i < end; ++i) {
        if (i > 0) {
            double current_high = (*data_line)[i];
            double prev_high = (*data_line)[i-1];
            double upmove = std::max(current_high - prev_high, 0.0);
            upmove_line->set(i, upmove);
        } else {
            upmove_line->set(i, 0.0);
        }
    }
}

// DownMove implementation
DownMove::DownMove() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(2);
}

DownMove::DownMove(std::shared_ptr<LineSeries> data_source) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    setup_lines();
    _minperiod(2);
}

double DownMove::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::downmove);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int DownMove::getMinPeriod() const {
    return 2;
}

void DownMove::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void DownMove::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void DownMove::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto downmove_line = lines->getline(Lines::downmove);
    
    if (!data_line || !downmove_line) return;
    
    // DownMove = max(low[-1] - low, 0)
    double current_low = (*data_line)[0];
    double prev_low = (*data_line)[-1];
    double downmove = std::max(prev_low - current_low, 0.0);
    
    downmove_line->set(0, downmove);
}

void DownMove::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto downmove_line = lines->getline(Lines::downmove);
    
    if (!data_line || !downmove_line) return;
    
    for (int i = start; i < end; ++i) {
        if (i > 0) {
            double current_low = (*data_line)[i];
            double prev_low = (*data_line)[i-1];
            double downmove = std::max(prev_low - current_low, 0.0);
            downmove_line->set(i, downmove);
        } else {
            downmove_line->set(i, 0.0);
        }
    }
}

} // namespace indicators
} // namespace backtrader