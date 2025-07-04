#include "indicators/dma.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// ZeroLagIndicator implementation
ZeroLagIndicator::ZeroLagIndicator() : Indicator(), ec_(0.0), ec1_(0.0), ec2_(0.0) {
    setup_lines();
    _minperiod(params.period);
    
    // Calculate alpha
    alpha_ = 2.0 / (params.period + 1.0);
    alpha1_ = 1.0 - alpha_;
}

void ZeroLagIndicator::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void ZeroLagIndicator::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto zerolag_line = lines->getline(Lines::zerolag);
    
    if (!data_line || !zerolag_line) return;
    
    double price = (*data_line)[0];
    
    // Calculate EMA
    double ema = alpha_ * price + alpha1_ * ec1_;
    
    // Calculate error correction
    double least_error = 1000000.0;
    double best_ec = ec_;
    
    for (double gain = -params.gainlimit; gain <= params.gainlimit; gain += 1.0) {
        double test_ec = alpha_ * (ema + gain * (price - ec2_)) + alpha1_ * ec1_;
        double error = std::abs(price - test_ec);
        
        if (error < least_error) {
            least_error = error;
            best_ec = test_ec;
        }
    }
    
    // Update values
    ec2_ = ec1_;
    ec1_ = ec_;
    ec_ = best_ec;
    
    zerolag_line->set(0, ec_);
}

void ZeroLagIndicator::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto zerolag_line = lines->getline(Lines::zerolag);
    
    if (!data_line || !zerolag_line) return;
    
    // Reset state
    ec_ = 0.0;
    ec1_ = 0.0;
    ec2_ = 0.0;
    
    for (int i = start; i < end; ++i) {
        double price = (*data_line)[i];
        
        // Calculate EMA
        double ema = alpha_ * price + alpha1_ * ec1_;
        
        // Calculate error correction
        double least_error = 1000000.0;
        double best_ec = ec_;
        
        for (double gain = -params.gainlimit; gain <= params.gainlimit; gain += 1.0) {
            double test_ec = alpha_ * (ema + gain * (price - ec2_)) + alpha1_ * ec1_;
            double error = std::abs(price - test_ec);
            
            if (error < least_error) {
                least_error = error;
                best_ec = test_ec;
            }
        }
        
        // Update values
        ec2_ = ec1_;
        ec1_ = ec_;
        ec_ = best_ec;
        
        zerolag_line->set(i, ec_);
    }
}

// DicksonMovingAverage implementation
DicksonMovingAverage::DicksonMovingAverage() : Indicator() {
    setup_lines();
    _minperiod(std::max(params.period, params.hperiod));
    
    // Create component indicators
    zerolag_ = std::make_shared<ZeroLagIndicator>();
    zerolag_->params.period = params.period;
    zerolag_->params.gainlimit = params.gainlimit;
    
    hma_ = std::make_shared<HMA>();
    hma_->params.period = params.hperiod;
}

void DicksonMovingAverage::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DicksonMovingAverage::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto dma_line = lines->getline(Lines::dma);
    if (!dma_line) return;
    
    // Set data for component indicators
    if (zerolag_ && hma_) {
        zerolag_->datas = datas;
        hma_->datas = datas;
        
        // Calculate components
        zerolag_->next();
        hma_->next();
        
        // Get values from components
        double zerolag_value = 0.0;
        double hma_value = 0.0;
        
        if (zerolag_->lines && zerolag_->lines->getline(0)) {
            zerolag_value = (*zerolag_->lines->getline(0))[0];
        }
        
        if (hma_->lines && hma_->lines->getline(0)) {
            hma_value = (*hma_->lines->getline(0))[0];
        }
        
        // Calculate DMA as average of components
        dma_line->set(0, (zerolag_value + hma_value) / 2.0);
    }
}

void DicksonMovingAverage::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto dma_line = lines->getline(Lines::dma);
    if (!dma_line) return;
    
    // Set data for component indicators
    if (zerolag_ && hma_) {
        zerolag_->datas = datas;
        hma_->datas = datas;
        
        // Calculate components
        zerolag_->once(start, end);
        hma_->once(start, end);
        
        // Combine results
        for (int i = start; i < end; ++i) {
            double zerolag_value = 0.0;
            double hma_value = 0.0;
            
            if (zerolag_->lines && zerolag_->lines->getline(0)) {
                zerolag_value = (*zerolag_->lines->getline(0))[i];
            }
            
            if (hma_->lines && hma_->lines->getline(0)) {
                hma_value = (*hma_->lines->getline(0))[i];
            }
            
            // Calculate DMA as average of components
            dma_line->set(i, (zerolag_value + hma_value) / 2.0);
        }
    }
}

} // namespace backtrader