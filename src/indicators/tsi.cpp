#include "indicators/tsi.h"
#include <cmath>

namespace backtrader {

// TrueStrengthIndicator implementation
TrueStrengthIndicator::TrueStrengthIndicator() : Indicator() {
    setup_lines();
    
    // Create EMA indicators for double smoothing
    sm1_ = std::make_shared<EMA>();
    sm1_->params.period = params.period1;
    
    sm12_ = std::make_shared<EMA>();
    sm12_->params.period = params.period2;
    
    sm2_ = std::make_shared<EMA>();
    sm2_->params.period = params.period1;
    
    sm22_ = std::make_shared<EMA>();
    sm22_->params.period = params.period2;
    
    // TSI needs pchange + period1 + period2 - 1 for full calculation
    _minperiod(params.pchange + params.period1 + params.period2 - 1);
}

void TrueStrengthIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void TrueStrengthIndicator::prenext() {
    if (sm1_) sm1_->prenext();
    if (sm12_) sm12_->prenext();
    if (sm2_) sm2_->prenext();
    if (sm22_) sm22_->prenext();
    Indicator::prenext();
}

void TrueStrengthIndicator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    // Calculate price change
    double current_price = (*data_line)[0];
    double previous_price = (*data_line)[-params.pchange];
    double price_change = current_price - previous_price;
    double abs_price_change = std::abs(price_change);
    
    // Store price changes for the smoothing EMAs
    price_changes_.push_back(price_change);
    abs_price_changes_.push_back(abs_price_change);
    
    // Connect price changes to first EMAs if not already done
    if (sm1_->datas.empty()) {
        // Create LineActions from price_changes vector
        auto pc_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
        auto pc_line = pc_lines->getline(0);
        if (pc_line && !price_changes_.empty()) {
            pc_line->set(0, price_changes_.back());
        }
        sm1_->add_data(pc_lines);
    }
    
    if (sm2_->datas.empty()) {
        // Create LineActions from abs_price_changes vector
        auto apc_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
        auto apc_line = apc_lines->getline(0);
        if (apc_line && !abs_price_changes_.empty()) {
            apc_line->set(0, abs_price_changes_.back());
        }
        sm2_->add_data(apc_lines);
    }
    
    // Update first level EMAs
    sm1_->next();
    sm2_->next();
    
    // Connect first EMAs to second EMAs if not already done
    if (sm12_->datas.empty() && sm1_->lines) {
        sm12_->add_data(sm1_->lines);
    }
    if (sm22_->datas.empty() && sm2_->lines) {
        sm22_->add_data(sm2_->lines);
    }
    
    // Update second level EMAs
    sm12_->next();
    sm22_->next();
    
    // Calculate TSI
    auto tsi_line = lines->getline(tsi);
    auto sm12_line = sm12_->lines->getline(EMA::ema);
    auto sm22_line = sm22_->lines->getline(EMA::ema);
    
    if (tsi_line && sm12_line && sm22_line) {
        double sm12_value = (*sm12_line)[0];
        double sm22_value = (*sm22_line)[0];
        
        if (sm22_value != 0.0) {
            tsi_line->set(0, 100.0 * (sm12_value / sm22_value));
        } else {
            tsi_line->set(0, 0.0);
        }
    }
}

void TrueStrengthIndicator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    if (!data_line) return;
    
    // Build price change vectors
    std::vector<double> all_price_changes;
    std::vector<double> all_abs_price_changes;
    
    for (int i = start - params.pchange; i < end; ++i) {
        if (i >= params.pchange) {
            double current_price = (*data_line)[i];
            double previous_price = (*data_line)[i - params.pchange];
            double price_change = current_price - previous_price;
            double abs_price_change = std::abs(price_change);
            
            all_price_changes.push_back(price_change);
            all_abs_price_changes.push_back(abs_price_change);
        }
    }
    
    // Create line objects for price changes
    auto pc_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
    auto apc_if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
    
    auto pc_line = pc_lines->getline(0);
    auto apc_line = apc_lines->getline(0);
    
    if (pc_line && apc_line) {
        // Fill with price change data
        for (size_t i = 0; i < all_price_changes.size(); ++i) {
            pc_line->set(i, all_price_changes[i]);
            apc_line->set(i, all_abs_price_changes[i]);
        }
        
        // Connect to first level EMAs
        sm1_->add_data(pc_lines);
        sm2_->add_data(apc_lines);
        
        // Calculate first level EMAs
        sm1_->once(0, all_price_changes.size());
        sm2_->once(0, all_abs_price_changes.size());
        
        // Connect to second level EMAs
        sm12_->add_data(sm1_->lines);
        sm22_->add_data(sm2_->lines);
        
        // Calculate second level EMAs
        sm12_->once(params.period1 - 1, all_price_changes.size());
        sm22_->once(params.period1 - 1, all_abs_price_changes.size());
        
        // Calculate TSI values
        auto tsi_line = lines->getline(tsi);
        auto sm12_line = sm12_->lines->getline(EMA::ema);
        auto sm22_line = sm22_->lines->getline(EMA::ema);
        
        if (tsi_line && sm12_line && sm22_line) {
            for (int i = start; i < end; ++i) {
                int ema_idx = i - start + params.period1 + params.period2 - 2;
                if (ema_idx >= 0 && ema_idx < all_price_changes.size()) {
                    double sm12_value = (*sm12_line)[ema_idx];
                    double sm22_value = (*sm22_line)[ema_idx];
                    
                    if (sm22_value != 0.0) {
                        tsi_line->set(i, 100.0 * (sm12_value / sm22_value));
                    } else {
                        tsi_line->set(i, 0.0);
                    }
                }
            }
        }
    }
}

} // namespace backtrader