#include "indicators/directionalmove.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// UpMove implementation
UpMove::UpMove() : Indicator() {
    setup_lines();
    _minperiod(2); // Need previous value
}

void UpMove::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void UpMove::prenext() {
    Indicator::prenext();
}

void UpMove::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto upmove_line = lines->getline(upmove);
    
    if (data_line && upmove_line) {
        double current = (*data_line)[0];
        double previous = (*data_line)[-1];
        upmove_line->set(0, current - previous);
    }
}

void UpMove::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto upmove_line = lines->getline(upmove);
    
    if (!data_line || !upmove_line) return;
    
    for (int i = start; i < end; ++i) {
        if (i > 0) {
            double current = (*data_line)[i];
            double previous = (*data_line)[i - 1];
            upmove_line->set(i, current - previous);
        }

// DownMove implementation
DownMove::DownMove() : Indicator() {
    setup_lines();
    _minperiod(2); // Need previous value
}

void DownMove::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DownMove::prenext() {
    Indicator::prenext();
}

void DownMove::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto downmove_line = lines->getline(downmove);
    
    if (data_line && downmove_line) {
        double current = (*data_line)[0];
        double previous = (*data_line)[-1];
        downmove_line->set(0, previous - current);
    }
}

void DownMove::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto downmove_line = lines->getline(downmove);
    
    if (!data_line || !downmove_line) return;
    
    for (int i = start; i < end; ++i) {
        if (i > 0) {
            double current = (*data_line)[i];
            double previous = (*data_line)[i - 1];
            downmove_line->set(i, previous - current);
        }

// DirectionalIndicatorBase implementation
DirectionalIndicatorBase::DirectionalIndicatorBase(bool calc_plus, bool calc_minus)
    : Indicator(), calc_plus_(calc_plus), calc_minus_(calc_minus), di_plus_(0.0), di_minus_(0.0) {
    
    // Create ATR indicator
    atr_ = std::make_shared<AverageTrueRange>();
    atr_->params.period = params.period;
    
    // Create SMMA for +DM and -DM if needed
    if (calc_plus_) {
        plus_dm_smma_ = std::make_shared<SmoothedMovingAverage>();
        plus_dm_smma_->params.period = params.period;
    }
    
    if (calc_minus_) {
        minus_dm_smma_ = std::make_shared<SmoothedMovingAverage>();
        minus_dm_smma_->params.period = params.period;
    }
    
    _minperiod(params.period + 1); // Need previous values for movement calculation
}

void DirectionalIndicatorBase::prenext() {
    if (atr_) atr_->prenext();
    if (plus_dm_smma_) plus_dm_smma_->prenext();
    if (minus_dm_smma_) minus_dm_smma_->prenext();
    Indicator::prenext();
}

void DirectionalIndicatorBase::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to ATR if not already done
    if (atr_->datas.empty() && !datas.empty()) {
        atr_->datas = datas;
    }
    
    // Update ATR
    atr_->next();
    
    // Calculate +DM and -DM
    double plus_dm = calculate_plus_dm();
    double minus_dm = calculate_minus_dm();
    
    // Store DM values and connect to SMAs
    if (calc_plus_) {
        plus_dm_values_.push_back(plus_dm);
        if (plus_dm_smma_->datas.empty()) {
            auto plus_dm_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
            auto plus_dm_line = plus_dm_lines->getline(0);
            if (plus_dm_line) {
                plus_dm_line->set(0, plus_dm);
            }
    
    if (calc_minus_) {
        minus_dm_values_.push_back(minus_dm);
        if (minus_dm_smma_->datas.empty()) {
            auto minus_dm_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
            auto minus_dm_line = minus_dm_lines->getline(0);
            if (minus_dm_line) {
                minus_dm_line->set(0, minus_dm);
            }
    
    // Calculate DI values
    auto atr_line = atr_->lines->getline(AverageTrueRange::atr);
    if (atr_line) {
        double atr_value = (*atr_line)[0];
        
        if (calc_plus_ && plus_dm_smma_->lines) {
            auto plus_dm_smma_line = plus_dm_smma_->lines->getline(SmoothedMovingAverage::smma);
            if (plus_dm_smma_line && atr_value != 0.0) {
                di_plus_ = 100.0 * (*plus_dm_smma_line)[0] / atr_value;
            }
        
        if (calc_minus_ && minus_dm_smma_->lines) {
            auto minus_dm_smma_line = minus_dm_smma_->lines->getline(SmoothedMovingAverage::smma);
            if (minus_dm_smma_line && atr_value != 0.0) {
                di_minus_ = 100.0 * (*minus_dm_smma_line)[0] / atr_value;
            }
    
    calculate_indicators();
}

void DirectionalIndicatorBase::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to ATR if not already done
    if (atr_->datas.empty() && !datas.empty()) {
        atr_->datas = datas;
    }
    
    // Calculate ATR
    atr_->once(start, end);
    
    // Build DM arrays and calculate SMAs
    // Implementation would be similar to next() but for the entire range
    // For brevity, using simplified approach
    for (int i = start; i < end; ++i) {
        next();
    }
}

double DirectionalIndicatorBase::calculate_plus_dm() {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    
    if (!high_line || !low_line) return 0.0;
    
    double high_current = (*high_line)[0];
    double high_previous = (*high_line)[-1];
    double low_current = (*low_line)[0];
    double low_previous = (*low_line)[-1];
    
    double upmove = high_current - high_previous;
    double downmove = low_previous - low_current;
    
    // +DM = upmove if upmove > downmove and upmove > 0, else 0
    if (upmove > downmove && upmove > 0.0) {
        return upmove;
    }
    return 0.0;
}

double DirectionalIndicatorBase::calculate_minus_dm() {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    auto high_line = datas[0]->lines->getline(1); // High line
    auto low_line = datas[0]->lines->getline(2);  // Low line
    
    if (!high_line || !low_line) return 0.0;
    
    double high_current = (*high_line)[0];
    double high_previous = (*high_line)[-1];
    double low_current = (*low_line)[0];
    double low_previous = (*low_line)[-1];
    
    double upmove = high_current - high_previous;
    double downmove = low_previous - low_current;
    
    // -DM = downmove if downmove > upmove and downmove > 0, else 0
    if (downmove > upmove && downmove > 0.0) {
        return downmove;
    }
    return 0.0;
}

// PlusDirectionalIndicator implementation
PlusDirectionalIndicator::PlusDirectionalIndicator() : DirectionalIndicatorBase(true, false) {
    setup_lines();
}

void PlusDirectionalIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PlusDirectionalIndicator::calculate_indicators() {
    auto plus_di_line = lines->getline(plusDI);
    if (plus_di_line) {
        plus_di_line->set(0, di_plus_);
    }
}

// MinusDirectionalIndicator implementation
MinusDirectionalIndicator::MinusDirectionalIndicator() : DirectionalIndicatorBase(false, true) {
    setup_lines();
}

void MinusDirectionalIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void MinusDirectionalIndicator::calculate_indicators() {
    auto minus_di_line = lines->getline(minusDI);
    if (minus_di_line) {
        minus_di_line->set(0, di_minus_);
    }
}

// DirectionalIndicator implementation
DirectionalIndicator::DirectionalIndicator() : DirectionalIndicatorBase(true, true) {
    setup_lines();
}

void DirectionalIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DirectionalIndicator::calculate_indicators() {
    auto plus_di_line = lines->getline(plusDI);
    auto minus_di_line = lines->getline(minusDI);
    
    if (plus_di_line) {
        plus_di_line->set(0, di_plus_);
    }
    if (minus_di_line) {
        minus_di_line->set(0, di_minus_);
    }
}

// AverageDirectionalMovementIndex implementation
AverageDirectionalMovementIndex::AverageDirectionalMovementIndex() : DirectionalIndicatorBase(true, true) {
    setup_lines();
    
    // Create SMMA for DX
    dx_smma_ = std::make_shared<SmoothedMovingAverage>();
    dx_smma_->params.period = params.period;
    
    _minperiod(2 * params.period);
}

void AverageDirectionalMovementIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AverageDirectionalMovementIndex::prenext() {
    DirectionalIndicatorBase::prenext();
    if (dx_smma_) dx_smma_->prenext();
}

void AverageDirectionalMovementIndex::next() {
    DirectionalIndicatorBase::next();
    
    // Calculate DX
    double di_sum = di_plus_ + di_minus_;
    double dx = 0.0;
    if (di_sum != 0.0) {
        dx = 100.0 * std::abs(di_plus_ - di_minus_) / di_sum;
    }
    
    dx_values_.push_back(dx);
    
    // Connect DX to SMMA if not already done
    if (dx_smma_->datas.empty()) {
        auto dx_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
        auto dx_line = dx_lines->getline(0);
        if (dx_line) {
            dx_line->set(0, dx);
        }
    
    dx_smma_->next();
    
    calculate_indicators();
}

void AverageDirectionalMovementIndex::once(int start, int end) {
    DirectionalIndicatorBase::once(start, end);
    
    // Calculate DX and ADX for the range
    // Implementation similar to next() but for entire range
}

void AverageDirectionalMovementIndex::calculate_indicators() {
    auto adx_line = lines->getline(adx);
    if (adx_line && dx_smma_->lines) {
        auto dx_smma_line = dx_smma_->lines->getline(SmoothedMovingAverage::smma);
        if (dx_smma_line) {
            adx_line->set(0, (*dx_smma_line)[0]);
        }

// AverageDirectionalMovementIndexRating implementation
AverageDirectionalMovementIndexRating::AverageDirectionalMovementIndexRating() : AverageDirectionalMovementIndex() {
    setup_lines();
    _minperiod(3 * params.period); // Need period more for ADXR calculation
}

void AverageDirectionalMovementIndexRating::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void AverageDirectionalMovementIndexRating::next() {
    AverageDirectionalMovementIndex::next();
    
    auto adx_line = lines->getline(adx);
    auto adxr_line = lines->getline(adxr);
    
    if (adx_line && adxr_line) {
        double current_adx = (*adx_line)[0];
        double period_ago_adx = (*adx_line)[-params.period];
        adxr_line->set(0, (current_adx + period_ago_adx) / 2.0);
    }
}

void AverageDirectionalMovementIndexRating::once(int start, int end) {
    AverageDirectionalMovementIndex::once(start, end);
    
    auto adx_line = lines->getline(adx);
    auto adxr_line = lines->getline(adxr);
    
    if (adx_line && adxr_line) {
        for (int i = start; i < end; ++i) {
            if (i >= params.period) {
                double current_adx = (*adx_line)[i];
                double period_ago_adx = (*adx_line)[i - params.period];
                adxr_line->set(i, (current_adx + period_ago_adx) / 2.0);
            }

// DirectionalMovementIndex implementation
DirectionalMovementIndex::DirectionalMovementIndex() : AverageDirectionalMovementIndex() {
    setup_lines();
}

void DirectionalMovementIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DirectionalMovementIndex::calculate_indicators() {
    AverageDirectionalMovementIndex::calculate_indicators();
    
    auto plus_di_line = lines->getline(plusDI);
    auto minus_di_line = lines->getline(minusDI);
    
    if (plus_di_line) {
        plus_di_line->set(0, di_plus_);
    }
    if (minus_di_line) {
        minus_di_line->set(0, di_minus_);
    }
}

// DirectionalMovement implementation
DirectionalMovement::DirectionalMovement() : AverageDirectionalMovementIndexRating() {
    setup_lines();
}

void DirectionalMovement::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DirectionalMovement::calculate_indicators() {
    auto plus_di_line = lines->getline(plusDI);
    auto minus_di_line = lines->getline(minusDI);
    
    if (plus_di_line) {
        plus_di_line->set(0, di_plus_);
    }
    if (minus_di_line) {
        minus_di_line->set(0, di_minus_);
    }
}

}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
} // namespace backtrader