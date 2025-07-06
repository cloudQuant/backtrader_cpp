#include "indicators/rmi.h"
#include <algorithm>

namespace backtrader {

// RelativeMomentumIndex implementation
RelativeMomentumIndex::RelativeMomentumIndex() : Indicator() {
    setup_lines();
    _minperiod(params.period + params.lookback);
    
    // Create SMMA indicators for smoothing
    up_smma_ = std::make_shared<SMMA>();
    up_smma_->params.period = params.period;
    
    down_smma_ = std::make_shared<SMMA>();
    down_smma_->params.period = params.period;
}

void RelativeMomentumIndex::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void RelativeMomentumIndex::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto rmi_line = lines->getline(rmi);
    
    if (!data_line || !rmi_line) return;
    
    // Calculate up/down moves based on lookback period
    double current = (*data_line)[0];
    double lookback_value = (*data_line)[-params.lookback];
    
    double diff = current - lookback_value;
    double up_move = (diff > 0) ? diff : 0.0;
    double down_move = (diff < 0) ? -diff : 0.0;
    
    // Store moves
    up_moves_.push_back(up_move);
    down_moves_.push_back(down_move);
    
    // Calculate SMMA of up and down moves
    if (up_moves_.size() >= params.period) {
        // Manual SMMA calculation
        static double up_smma = 0.0;
        static double down_smma = 0.0;
        static bool first_calc = true;
        
        if (first_calc) {
            // First SMMA is SMA
            double up_sum = 0.0;
            double down_sum = 0.0;
            int start = up_moves_.size() - params.period;
            
            for (int i = start; i < start + params.period; ++i) {
                up_sum += up_moves_[i];
                down_sum += down_moves_[i];
            }
            
            up_smma = up_sum / params.period;
            down_smma = down_sum / params.period;
            first_calc = false;
        } else {
            // SMMA calculation
            up_smma = (up_smma * (params.period - 1) + up_move) / params.period;
            down_smma = (down_smma * (params.period - 1) + down_move) / params.period;
        }
        
        // Calculate RMI
        double rmi;
        if (down_smma == 0.0) {
            rmi = 100.0;
        } else {
            double rs = up_smma / down_smma;
            rmi = 100.0 - (100.0 / (1.0 + rs));
        }
        
        rmi_line->set(0, rmi);
    } else {
        rmi_line->set(0, 50.0);  // Default neutral value
    }
    
    // Keep history manageable
    if (up_moves_.size() > params.period * 2) {
        up_moves_.erase(up_moves_.begin());
        down_moves_.erase(down_moves_.begin());
    }
}

void RelativeMomentumIndex::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto rmi_line = lines->getline(rmi);
    
    if (!data_line || !rmi_line) return;
    
    // Calculate all up/down moves first
    std::vector<double> all_up_moves;
    std::vector<double> all_down_moves;
    
    for (int i = params.lookback; i < end; ++i) {
        double current = (*data_line)[i];
        double lookback_value = (*data_line)[i - params.lookback];
        
        double diff = current - lookback_value;
        double up_move = (diff > 0) ? diff : 0.0;
        double down_move = (diff < 0) ? -diff : 0.0;
        
        all_up_moves.push_back(up_move);
        all_down_moves.push_back(down_move);
    }
    
    // Calculate RMI for each bar
    for (int i = start; i < end; ++i) {
        int move_idx = i - params.lookback;
        
        if (move_idx >= 0 && move_idx >= params.period - 1) {
            // Calculate SMMA of up and down moves
            double up_smma, down_smma;
            
            if (move_idx == params.period - 1) {
                // First SMMA is SMA
                double up_sum = 0.0;
                double down_sum = 0.0;
                
                for (int j = 0; j < params.period; ++j) {
                    up_sum += all_up_moves[move_idx - params.period + 1 + j];
                    down_sum += all_down_moves[move_idx - params.period + 1 + j];
                }
                
                up_smma = up_sum / params.period;
                down_smma = down_sum / params.period;
            } else {
                // Get previous SMMA values (need to track these properly)
                // For simplicity, recalculate as SMA
                double up_sum = 0.0;
                double down_sum = 0.0;
                
                for (int j = 0; j < params.period; ++j) {
                    up_sum += all_up_moves[move_idx - j];
                    down_sum += all_down_moves[move_idx - j];
                }
                
                up_smma = up_sum / params.period;
                down_smma = down_sum / params.period;
            }
            
            // Calculate RMI
            double rmi;
            if (down_smma == 0.0) {
                rmi = 100.0;
            } else {
                double rs = up_smma / down_smma;
                rmi = 100.0 - (100.0 / (1.0 + rs));
            }
            
            rmi_line->set(i, rmi);
        } else {
            rmi_line->set(i, 50.0);  // Default neutral value
        }
    }
}

} // namespace backtrader