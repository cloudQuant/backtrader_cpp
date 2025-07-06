#include "indicators/crossover.h"

namespace backtrader {

// NonZeroDifference implementation
NonZeroDifference::NonZeroDifference() : Indicator(), last_nzd_(0.0) {
    setup_lines();
    _minperiod(2); // Needs at least 2 periods to compare
}

void NonZeroDifference::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void NonZeroDifference::add_data(std::shared_ptr<LineActions> data) {
    if (!data0_) {
        data0_ = data;
    } else if (!data1_) {
        data1_ = data;
    }
    // Don't add to datas vector since type mismatch (LineActions vs LineSeries)
}

void NonZeroDifference::prenext() {
    Indicator::prenext();
}

void NonZeroDifference::nextstart() {
    if (!data0_ || !data1_) return;
    
    auto nzd_line = lines->getline(nzd);
    if (nzd_line) {
        double diff = (*data0_)[0] - (*data1_)[0];
        nzd_line->set(0, diff);
        last_nzd_ = diff;
    }
}

void NonZeroDifference::next() {
    if (!data0_ || !data1_) return;
    
    auto nzd_line = lines->getline(nzd);
    if (nzd_line) {
        double diff = (*data0_)[0] - (*data1_)[0];
        if (diff != 0.0) {
            nzd_line->set(0, diff);
            last_nzd_ = diff;
        } else {
            nzd_line->set(0, last_nzd_);
        }
    }
}

void NonZeroDifference::oncestart(int start, int end) {
    if (!data0_ || !data1_ || start >= end) return;
    
    auto nzd_line = lines->getline(nzd);
    if (nzd_line) {
        double diff = (*data0_)[start] - (*data1_)[start];
        nzd_line->set(start, diff);
    }
}

void NonZeroDifference::once(int start, int end) {
    if (!data0_ || !data1_) return;
    
    auto nzd_line = lines->getline(nzd);
    if (!nzd_line) return;
    
    double prev = (start > 0) ? (*nzd_line)[start - 1] : 0.0;
    
    for (int i = start; i < end; ++i) {
        double diff = (*data0_)[i] - (*data1_)[i];
        if (diff != 0.0) {
            prev = diff;
        }
        nzd_line->set(i, prev);
    }
}

// CrossBase implementation
CrossBase::CrossBase(bool crossup) : Indicator(), crossup_(crossup) {
    setup_lines();
    _minperiod(2); // Needs previous value to detect crossover
}

void CrossBase::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void CrossBase::add_data(std::shared_ptr<LineActions> data) {
    if (!data0_) {
        data0_ = data;
    } else if (!data1_) {
        data1_ = data;
        // Create NZD indicator once we have both data sources
        nzd_ = std::make_shared<NonZeroDifference>();
        nzd_->add_data(data0_);
        nzd_->add_data(data1_);
    }
    // Base class add_data for datas vector
    // Don't add to datas vector since type mismatch (LineActions vs LineSeries)
}

void CrossBase::prenext() {
    if (nzd_) nzd_->prenext();
    Indicator::prenext();
}

void CrossBase::next() {
    if (!data0_ || !data1_ || !nzd_) return;
    
    // Update NZD first
    nzd_->next();
    
    auto cross_line = lines->getline(cross);
    auto nzd_line = nzd_->lines->getline(NonZeroDifference::nzd);
    
    if (cross_line && nzd_line) {
        double current_data0 = (*data0_)[0];
        double current_data1 = (*data1_)[0];
        double prev_nzd = (*nzd_line)[-1];
        
        bool cross_detected = false;
        
        if (crossup_) {
            // CrossUp: previous diff was negative and current data0 > data1
            cross_detected = (prev_nzd < 0.0) && (current_data0 > current_data1);
        } else {
            // CrossDown: previous diff was positive and current data0 < data1
            cross_detected = (prev_nzd > 0.0) && (current_data0 < current_data1);
        }
        
        cross_line->set(0, cross_detected ? 1.0 : 0.0);
    }
}

void CrossBase::once(int start, int end) {
    if (!data0_ || !data1_ || !nzd_) return;
    
    // Calculate NZD first
    nzd_->once(start, end);
    
    auto cross_line = lines->getline(cross);
    auto nzd_line = nzd_->lines->getline(NonZeroDifference::nzd);
    
    if (!cross_line || !nzd_line) return;
    
    for (int i = start; i < end; ++i) {
        double current_data0 = (*data0_)[i];
        double current_data1 = (*data1_)[i];
        double prev_nzd = (i > 0) ? (*nzd_line)[i - 1] : 0.0;
        
        bool cross_detected = false;
        
        if (crossup_) {
            cross_detected = (prev_nzd < 0.0) && (current_data0 > current_data1);
        } else {
            cross_detected = (prev_nzd > 0.0) && (current_data0 < current_data1);
        }
        
        cross_line->set(i, cross_detected ? 1.0 : 0.0);
    }
}

// CrossUp implementation
CrossUp::CrossUp() : CrossBase(true) {
}

// CrossDown implementation
CrossDown::CrossDown() : CrossBase(false) {
}

// CrossOver implementation
CrossOver::CrossOver() : Indicator() {
    setup_lines();
    _minperiod(2); // Needs previous value to detect crossover
}

CrossOver::CrossOver(std::shared_ptr<LineActions> data0, std::shared_ptr<LineActions> data1) : Indicator() {
    setup_lines();
    _minperiod(2); // Needs previous value to detect crossover
    data0_ = data0;
    data1_ = data1;
}

void CrossOver::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void CrossOver::add_data(std::shared_ptr<LineActions> data) {
    if (!data0_) {
        data0_ = data;
    } else if (!data1_) {
        data1_ = data;
        // Create CrossUp and CrossDown indicators
        upcross_ = std::make_shared<CrossUp>();
        upcross_->add_data(data0_);
        upcross_->add_data(data1_);
        
        downcross_ = std::make_shared<CrossDown>();
        downcross_->add_data(data0_);
        downcross_->add_data(data1_);
    }
    // Base class add_data for datas vector
    // Don't add to datas vector since type mismatch (LineActions vs LineSeries)
}

void CrossOver::prenext() {
    if (upcross_) upcross_->prenext();
    if (downcross_) downcross_->prenext();
    Indicator::prenext();
}

void CrossOver::next() {
    if (!upcross_ || !downcross_) return;
    
    // Update sub-indicators
    upcross_->next();
    downcross_->next();
    
    auto crossover_line = lines->getline(crossover);
    auto upcross_line = upcross_->lines->getline(CrossUp::cross);
    auto downcross_line = downcross_->lines->getline(CrossDown::cross);
    
    if (crossover_line && upcross_line && downcross_line) {
        double up_signal = (*upcross_line)[0];
        double down_signal = (*downcross_line)[0];
        
        // +1 for up cross, -1 for down cross, 0 for no cross
        crossover_line->set(0, up_signal - down_signal);
    }
}

void CrossOver::once(int start, int end) {
    if (!upcross_ || !downcross_) return;
    
    // Calculate sub-indicators
    upcross_->once(start, end);
    downcross_->once(start, end);
    
    auto crossover_line = lines->getline(crossover);
    auto upcross_line = upcross_->lines->getline(CrossUp::cross);
    auto downcross_line = downcross_->lines->getline(CrossDown::cross);
    
    if (!crossover_line || !upcross_line || !downcross_line) return;
    
    for (int i = start; i < end; ++i) {
        double up_signal = (*upcross_line)[i];
        double down_signal = (*downcross_line)[i];
        
        crossover_line->set(i, up_signal - down_signal);
    }
}

double CrossOver::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto crossover_line = lines->getline(crossover);
    if (!crossover_line || crossover_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Convert ago to positive index from current position
    int index;
    if (ago <= 0) {
        index = static_cast<int>(crossover_line->size()) - 1 + ago;
    } else {
        index = static_cast<int>(crossover_line->size()) - 1 + ago;
    }
    
    if (index < 0 || index >= static_cast<int>(crossover_line->size())) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*crossover_line)[index];
}

} // namespace backtrader