#include "indicators/lrsi.h"

namespace backtrader {

// LaguerreRSI implementation
LaguerreRSI::LaguerreRSI() : Indicator(), l0_(0.0), l1_(0.0), l2_(0.0), l3_(0.0) {
    setup_lines();
    _minperiod(params.period);
}

LaguerreRSI::LaguerreRSI(std::shared_ptr<LineRoot> data, double gamma) 
    : Indicator(), l0_(0.0), l1_(0.0), l2_(0.0), l3_(0.0) {
    params.gamma = gamma;
    setup_lines();
    _minperiod(params.period);
}

void LaguerreRSI::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void LaguerreRSI::prenext() {
    Indicator::prenext();
}

void LaguerreRSI::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto lrsi_line = lines->getline(lrsi);
    auto data_line = datas[0]->lines->getline(0);
    
    if (!lrsi_line || !data_line) return;
    
    // Cache previous intermediate values
    double l0_1 = l0_;
    double l1_1 = l1_;
    double l2_1 = l2_;
    
    double g = params.gamma;
    double current_data = (*data_line)[0];
    
    // Calculate Laguerre filter values
    l0_ = (1.0 - g) * current_data + g * l0_1;
    l1_ = -g * l0_ + l0_1 + g * l1_1;
    l2_ = -g * l1_ + l1_1 + g * l2_1;
    l3_ = -g * l2_ + l2_1 + g * l3_;
    
    // Calculate cumulative up and down movements
    double cu = 0.0;
    double cd = 0.0;
    
    if (l0_ >= l1_) {
        cu += l0_ - l1_;
    } else {
        cd += l1_ - l0_;
    }
    
    if (l1_ >= l2_) {
        cu += l1_ - l2_;
    } else {
        cd += l2_ - l1_;
    }
    
    if (l2_ >= l3_) {
        cu += l2_ - l3_;
    } else {
        cd += l3_ - l2_;
    }
    
    // Calculate LRSI
    double den = cu + cd;
    lrsi_line->set(0, (den == 0.0) ? 1.0 : cu / den);
}

void LaguerreRSI::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto lrsi_line = lines->getline(lrsi);
    auto data_line = datas[0]->lines->getline(0);
    
    if (!lrsi_line || !data_line) return;
    
    // Initialize filter values
    double l0 = 0.0, l1 = 0.0, l2 = 0.0, l3 = 0.0;
    
    for (int i = start; i < end; ++i) {
        // Cache previous values
        double l0_1 = l0;
        double l1_1 = l1;
        double l2_1 = l2;
        
        double g = params.gamma;
        double current_data = (*data_line)[i];
        
        // Calculate Laguerre filter values
        l0 = (1.0 - g) * current_data + g * l0_1;
        l1 = -g * l0 + l0_1 + g * l1_1;
        l2 = -g * l1 + l1_1 + g * l2_1;
        l3 = -g * l2 + l2_1 + g * l3;
        
        // Calculate cumulative up and down movements
        double cu = 0.0;
        double cd = 0.0;
        
        if (l0 >= l1) {
            cu += l0 - l1;
        } else {
            cd += l1 - l0;
        }
        
        if (l1 >= l2) {
            cu += l1 - l2;
        } else {
            cd += l2 - l1;
        }
        
        if (l2 >= l3) {
            cu += l2 - l3;
        } else {
            cd += l3 - l2;
        }
        
        // Calculate LRSI
        double den = cu + cd;
        lrsi_line->set(i, (den == 0.0) ? 1.0 : cu / den);
    }
}

// LaguerreFilter implementation
LaguerreFilter::LaguerreFilter() : Indicator(), l0_(0.0), l1_(0.0), l2_(0.0), l3_(0.0) {
    setup_lines();
    _minperiod(1);
}

void LaguerreFilter::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void LaguerreFilter::prenext() {
    Indicator::prenext();
}

void LaguerreFilter::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto lfilter_line = lines->getline(lfilter);
    auto data_line = datas[0]->lines->getline(0);
    
    if (!lfilter_line || !data_line) return;
    
    // Cache previous intermediate values
    double l0_1 = l0_;
    double l1_1 = l1_;
    double l2_1 = l2_;
    
    double g = params.gamma;
    double current_data = (*data_line)[0];
    
    // Calculate Laguerre filter values
    l0_ = (1.0 - g) * current_data + g * l0_1;
    l1_ = -g * l0_ + l0_1 + g * l1_1;
    l2_ = -g * l1_ + l1_1 + g * l2_1;
    l3_ = -g * l2_ + l2_1 + g * l3_;
    
    // Calculate filtered value
    lfilter_line->set(0, (l0_ + (2.0 * l1_) + (2.0 * l2_) + l3_) / 6.0);
}

void LaguerreFilter::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto lfilter_line = lines->getline(lfilter);
    auto data_line = datas[0]->lines->getline(0);
    
    if (!lfilter_line || !data_line) return;
    
    // Initialize filter values
    double l0 = 0.0, l1 = 0.0, l2 = 0.0, l3 = 0.0;
    
    for (int i = start; i < end; ++i) {
        // Cache previous values
        double l0_1 = l0;
        double l1_1 = l1;
        double l2_1 = l2;
        
        double g = params.gamma;
        double current_data = (*data_line)[i];
        
        // Calculate Laguerre filter values
        l0 = (1.0 - g) * current_data + g * l0_1;
        l1 = -g * l0 + l0_1 + g * l1_1;
        l2 = -g * l1 + l1_1 + g * l2_1;
        l3 = -g * l2 + l2_1 + g * l3;
        
        // Calculate filtered value
        lfilter_line->set(i, (l0 + (2.0 * l1) + (2.0 * l2) + l3) / 6.0);
    }
}

double LaguerreRSI::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return 0.0;
    }
    
    auto lrsi_line = lines->getline(Lines::lrsi);
    if (!lrsi_line || lrsi_line->size() == 0) {
        return 0.0;
    }
    
    return (*lrsi_line)[ago];
}

int LaguerreRSI::getMinPeriod() const {
    return params.period;
}

void LaguerreRSI::calculate() {
    next();
}

} // namespace backtrader