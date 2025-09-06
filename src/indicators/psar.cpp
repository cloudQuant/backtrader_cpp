#include "indicators/psar.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace backtrader {

// ParabolicSAR implementation
ParabolicSAR::ParabolicSAR() : Indicator(), initialized_(false) {
    setup_lines();
    _minperiod(params.period);
    
    // Initialize status arrays
    status_[0] = SarStatus();
    status_[1] = SarStatus();
}

void ParabolicSAR::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void ParabolicSAR::prenext() {
    Indicator::prenext();
    
    if (len() == 1) {
        // First bar, not enough data
        return;
    } else if (len() == 2) {
        // Second bar, kick-start calculation
        nextstart();
    } else {
        // Regular calculation
        next();
    }
    
    // Set NaN for prenext phase
    auto psar_line = lines->getline(psar);
    if (psar_line) {
        psar_line->set(0, std::numeric_limits<double>::quiet_NaN());
    }
}

void ParabolicSAR::nextstart() {
    if (initialized_) {
        next();
        return;
    }
    
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);   // High line
    auto low_line = datas[0]->lines->getline(2);    // Low line
    auto close_line = datas[0]->lines->getline(4);  // Close line
    
    if (!high_line || !low_line || !close_line) return;
    
    // Initialize status for the previous length index
    int plen_idx = (len() - 1) % 2;
    SarStatus& status = status_[plen_idx];
    
    // Calculate initial SAR as average of high and low
    status.sar = ((*high_line)[0] + (*low_line)[0]) / 2.0;
    status.af = params.af;
    
    // Determine initial trend based on close price comparison
    if ((*close_line)[0] >= (*close_line)[-1]) {
        // Uptrend detected, but set up as if coming from downtrend
        status.tr = false;  // Will be reversed in next()
        status.ep = (*low_line)[-1];
    } else {
        // Downtrend detected, but set up as if coming from uptrend
        status.tr = true;   // Will be reversed in next()
        status.ep = (*high_line)[-1];
    }
    
    initialized_ = true;
    next();  // Proceed with calculation
}

void ParabolicSAR::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto psar_line = lines->getline(psar);
    
    if (!high_line || !low_line || !psar_line) return;
    
    double hi = (*high_line)[0];
    double lo = (*low_line)[0];
    
    int plen_idx = (len() - 1) % 2;  // Previous length index
    SarStatus& prev_status = status_[plen_idx];
    
    bool tr = prev_status.tr;
    double sar = prev_status.sar;
    double ep = prev_status.ep;
    double af = prev_status.af;
    
    // Check if SAR penetrated the price to switch trend
    if ((tr && sar >= lo) || (!tr && sar <= hi)) {
        // Reverse the trend
        tr = !tr;
        sar = prev_status.ep;  // New SAR is previous extreme price
        ep = tr ? hi : lo;     // Select new extreme price
        af = params.af;        // Reset acceleration factor
    }
    
    // Update current SAR value
    psar_line->set(0, sar);
    
    // Update extreme price and acceleration factor if needed
    if (tr) {  // Uptrend
        if (hi > ep) {
            ep = hi;
            af = std::min(af + params.af, params.afmax);
        }
    } else {  // Downtrend
        if (lo < ep) {
            ep = lo;
            af = std::min(af + params.af, params.afmax);
        }
    
    // Calculate SAR for next period
    sar = sar + af * (ep - sar);
    
    // Ensure SAR doesn't penetrate the last two periods' highs/lows
    if (tr) {  // Uptrend - SAR should not go above the last two lows
        double lo1 = (*low_line)[-1];
        if (sar > lo || sar > lo1) {
            sar = std::min(lo, lo1);
        }
    } else {  // Downtrend - SAR should not go below the last two highs
        double hi1 = (*high_line)[-1];
        if (sar < hi || sar < hi1) {
            sar = std::max(hi, hi1);
        }
    
    // Store the new status for next iteration
    int new_idx = !plen_idx;  // Switch to the other status slot
    status_[new_idx] = SarStatus(sar, tr, af, ep);
}

void ParabolicSAR::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto high_line = datas[0]->lines->getline(1);
    auto low_line = datas[0]->lines->getline(2);
    auto close_line = datas[0]->lines->getline(4);
    auto psar_line = lines->getline(psar);
    
    if (!high_line || !low_line || !close_line || !psar_line) return;
    
    // Initialize status for the first calculation
    SarStatus current_status;
    
    // Handle initial values (first two bars)
    if (start >= 1) {
        current_status.sar = ((*high_line)[start] + (*low_line)[start]) / 2.0;
        current_status.af = params.af;
        
        if ((*close_line)[start] >= (*close_line)[start - 1]) {
            current_status.tr = false;  // Will be reversed
            current_status.ep = (*low_line)[start - 1];
        } else {
            current_status.tr = true;   // Will be reversed
            current_status.ep = (*high_line)[start - 1];
        }
    
    for (int i = start; i < end; ++i) {
        if (i < params.period) {
            psar_line->set(i, std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        double hi = (*high_line)[i];
        double lo = (*low_line)[i];
        
        bool tr = current_status.tr;
        double sar = current_status.sar;
        double ep = current_status.ep;
        double af = current_status.af;
        
        // Check for trend reversal
        if ((tr && sar >= lo) || (!tr && sar <= hi)) {
            tr = !tr;
            sar = current_status.ep;
            ep = tr ? hi : lo;
            af = params.af;
        }
        
        // Set SAR value
        psar_line->set(i, sar);
        
        // Update extreme price and acceleration factor
        if (tr) {
            if (hi > ep) {
                ep = hi;
                af = std::min(af + params.af, params.afmax);
            }
        } else {
            if (lo < ep) {
                ep = lo;
                af = std::min(af + params.af, params.afmax);
            }
        
        // Calculate next SAR
        sar = sar + af * (ep - sar);
        
        // Ensure SAR doesn't penetrate highs/lows
        if (tr) {
            double lo_curr = (*low_line)[i];
            double lo_prev = (i > 0) ? (*low_line)[i - 1] : lo_curr;
            if (sar > lo_curr || sar > lo_prev) {
                sar = std::min(lo_curr, lo_prev);
            }
        } else {
            double hi_curr = (*high_line)[i];
            double hi_prev = (i > 0) ? (*high_line)[i - 1] : hi_curr;
            if (sar < hi_curr || sar < hi_prev) {
                sar = std::max(hi_curr, hi_prev);
            }
        
        // Update status for next iteration
        current_status = SarStatus(sar, tr, af, ep);
    }
}

size_t ParabolicSAR::size() const {
    if (!lines || lines->size() == 0) return 0;
    auto psar_line = lines->getline(psar);
    return psar_line ? psar_line->size() : 0;
}

}
}
}
}
}
} // namespace backtrader