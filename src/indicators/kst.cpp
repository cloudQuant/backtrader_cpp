#include "indicators/kst.h"
#include <limits>

namespace backtrader {

// KnowSureThing implementation
KnowSureThing::KnowSureThing() : Indicator() {
    setup_lines();
    
    // Calculate minimum period needed
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    _minperiod(max_roc_period + max_ma_period);
    
    // Create ROC indicators
    roc1_ = std::make_shared<indicators::RateOfChange100>();
    roc1_->params.period = params.rp1;
    
    roc2_ = std::make_shared<indicators::RateOfChange100>();
    roc2_->params.period = params.rp2;
    
    roc3_ = std::make_shared<indicators::RateOfChange100>();
    roc3_->params.period = params.rp3;
    
    roc4_ = std::make_shared<indicators::RateOfChange100>();
    roc4_->params.period = params.rp4;
    
    // Create SMAs for ROCs
    rcma1_ = std::make_shared<indicators::SMA>();
    rcma1_->params.period = params.rma1;
    
    rcma2_ = std::make_shared<indicators::SMA>();
    rcma2_->params.period = params.rma2;
    
    rcma3_ = std::make_shared<indicators::SMA>();
    rcma3_->params.period = params.rma3;
    
    rcma4_ = std::make_shared<indicators::SMA>();
    rcma4_->params.period = params.rma4;
    
    // Create SMA for signal line
    signal_sma_ = std::make_shared<indicators::SMA>();
    signal_sma_->params.period = params.rsignal;
}

KnowSureThing::KnowSureThing(std::shared_ptr<LineRoot> data) : Indicator() {
    setup_lines();
    
    // Calculate minimum period needed
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    _minperiod(max_roc_period + max_ma_period + params.rsignal);
    
    // Create ROC indicators with data
    roc1_ = std::make_shared<indicators::RateOfChange100>();
    roc1_->params.period = params.rp1;
    
    roc2_ = std::make_shared<indicators::RateOfChange100>();
    roc2_->params.period = params.rp2;
    
    roc3_ = std::make_shared<indicators::RateOfChange100>();
    roc3_->params.period = params.rp3;
    
    roc4_ = std::make_shared<indicators::RateOfChange100>();
    roc4_->params.period = params.rp4;
    
    // Create SMAs for ROCs
    rcma1_ = std::make_shared<indicators::SMA>();
    rcma1_->params.period = params.rma1;
    
    rcma2_ = std::make_shared<indicators::SMA>();
    rcma2_->params.period = params.rma2;
    
    rcma3_ = std::make_shared<indicators::SMA>();
    rcma3_->params.period = params.rma3;
    
    rcma4_ = std::make_shared<indicators::SMA>();
    rcma4_->params.period = params.rma4;
    
    // Create SMA for signal line
    signal_sma_ = std::make_shared<indicators::SMA>();
    signal_sma_->params.period = params.rsignal;
}

double KnowSureThing::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(Lines::kst);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int KnowSureThing::getMinPeriod() const {
    int max_roc_period = std::max({params.rp1, params.rp2, params.rp3, params.rp4});
    int max_ma_period = std::max({params.rma1, params.rma2, params.rma3, params.rma4});
    return max_roc_period + max_ma_period + params.rsignal;
}

void KnowSureThing::calculate() {
    next();
}

std::shared_ptr<LineBuffer> KnowSureThing::getLine(int index) const {
    if (!lines || static_cast<size_t>(index) >= lines->size()) {
        return nullptr;
    }
    return lines->getline(index);
}

void KnowSureThing::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void KnowSureThing::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto kst_line = lines->getline(kst);
    auto signal_line = lines->getline(signal);
    
    if (!kst_line || !signal_line) return;
    
    // Set data for all ROC indicators
    roc1_->datas = datas;
    roc2_->datas = datas;
    roc3_->datas = datas;
    roc4_->datas = datas;
    
    // Calculate ROCs
    roc1_->next();
    roc2_->next();
    roc3_->next();
    roc4_->next();
    
    // Get ROC values
    double roc1_val = 0.0, roc2_val = 0.0, roc3_val = 0.0, roc4_val = 0.0;
    
    if (roc1_->lines && roc1_->lines->getline(0))
        roc1_val = (*roc1_->lines->getline(0))[0];
    if (roc2_->lines && roc2_->lines->getline(0))
        roc2_val = (*roc2_->lines->getline(0))[0];
    if (roc3_->lines && roc3_->lines->getline(0))
        roc3_val = (*roc3_->lines->getline(0))[0];
    if (roc4_->lines && roc4_->lines->getline(0))
        roc4_val = (*roc4_->lines->getline(0))[0];
    
    // Store ROC values for SMA calculations
    static std::vector<double> roc1_values, roc2_values, roc3_values, roc4_values;
    roc1_values.push_back(roc1_val);
    roc2_values.push_back(roc2_val);
    roc3_values.push_back(roc3_val);
    roc4_values.push_back(roc4_val);
    
    // Calculate SMAs of ROCs
    double rcma1 = 0.0, rcma2 = 0.0, rcma3 = 0.0, rcma4 = 0.0;
    
    if (roc1_values.size() >= params.rma1) {
        double sum = 0.0;
        int start = roc1_values.size() - params.rma1;
        for (int i = start; i < roc1_values.size(); ++i)
            sum += roc1_values[i];
        rcma1 = sum / params.rma1;
    }
    
    if (roc2_values.size() >= params.rma2) {
        double sum = 0.0;
        int start = roc2_values.size() - params.rma2;
        for (int i = start; i < roc2_values.size(); ++i)
            sum += roc2_values[i];
        rcma2 = sum / params.rma2;
    }
    
    if (roc3_values.size() >= params.rma3) {
        double sum = 0.0;
        int start = roc3_values.size() - params.rma3;
        for (int i = start; i < roc3_values.size(); ++i)
            sum += roc3_values[i];
        rcma3 = sum / params.rma3;
    }
    
    if (roc4_values.size() >= params.rma4) {
        double sum = 0.0;
        int start = roc4_values.size() - params.rma4;
        for (int i = start; i < roc4_values.size(); ++i)
            sum += roc4_values[i];
        rcma4 = sum / params.rma4;
    }
    
    // Calculate KST as weighted sum
    double kst = params.rfactors[0] * rcma1 + 
                 params.rfactors[1] * rcma2 + 
                 params.rfactors[2] * rcma3 + 
                 params.rfactors[3] * rcma4;
    
    kst_line->set(0, kst);
    
    // Store KST values for signal calculation
    static std::vector<double> kst_values;
    kst_values.push_back(kst);
    
    // Calculate signal line (SMA of KST)
    if (kst_values.size() >= params.rsignal) {
        double sum = 0.0;
        int start = kst_values.size() - params.rsignal;
        for (int i = start; i < kst_values.size(); ++i)
            sum += kst_values[i];
        signal_line->set(0, sum / params.rsignal);
    } else {
        signal_line->set(0, kst);
    }
    
    // Keep history manageable
    if (roc1_values.size() > params.rma1 * 2) roc1_values.erase(roc1_values.begin());
    if (roc2_values.size() > params.rma2 * 2) roc2_values.erase(roc2_values.begin());
    if (roc3_values.size() > params.rma3 * 2) roc3_values.erase(roc3_values.begin());
    if (roc4_values.size() > params.rma4 * 2) roc4_values.erase(roc4_values.begin());
    if (kst_values.size() > params.rsignal * 2) kst_values.erase(kst_values.begin());
}

void KnowSureThing::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto kst_line = lines->getline(kst);
    auto signal_line = lines->getline(signal);
    
    if (!kst_line || !signal_line) return;
    
    // Set data for all ROC indicators
    roc1_->datas = datas;
    roc2_->datas = datas;
    roc3_->datas = datas;
    roc4_->datas = datas;
    
    // Calculate all ROCs
    roc1_->once(0, end);
    roc2_->once(0, end);
    roc3_->once(0, end);
    roc4_->once(0, end);
    
    // Get ROC lines
    auto roc1_line = roc1_->lines->getline(0);
    auto roc2_line = roc2_->lines->getline(0);
    auto roc3_line = roc3_->lines->getline(0);
    auto roc4_line = roc4_->lines->getline(0);
    
    // Calculate KST for all bars
    for (int i = start; i < end; ++i) {
        // Calculate SMAs of ROCs
        double rcma1 = 0.0, rcma2 = 0.0, rcma3 = 0.0, rcma4 = 0.0;
        
        if (i >= params.rma1 - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.rma1; ++j)
                sum += (*roc1_line)[i - j];
            rcma1 = sum / params.rma1;
        }
        
        if (i >= params.rma2 - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.rma2; ++j)
                sum += (*roc2_line)[i - j];
            rcma2 = sum / params.rma2;
        }
        
        if (i >= params.rma3 - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.rma3; ++j)
                sum += (*roc3_line)[i - j];
            rcma3 = sum / params.rma3;
        }
        
        if (i >= params.rma4 - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.rma4; ++j)
                sum += (*roc4_line)[i - j];
            rcma4 = sum / params.rma4;
        }
        
        // Calculate KST
        double kst = params.rfactors[0] * rcma1 + 
                     params.rfactors[1] * rcma2 + 
                     params.rfactors[2] * rcma3 + 
                     params.rfactors[3] * rcma4;
        
        kst_line->set(i, kst);
        
        // Calculate signal line
        if (i >= start + params.rsignal - 1) {
            double sum = 0.0;
            for (int j = 0; j < params.rsignal; ++j)
                sum += (*kst_line)[i - j];
            signal_line->set(i, sum / params.rsignal);
        } else {
            signal_line->set(i, kst);
        }
    }
}

} // namespace backtrader