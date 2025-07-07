#include "indicators/percentchange.h"
#include <limits>

namespace backtrader {
namespace indicators {

// PercentChange implementation
PercentChange::PercentChange() : Indicator(), data_source_(nullptr), current_index_(0) {
    setup_lines();
    _minperiod(params.period + 1); // Need period + 1 for comparison
}

PercentChange::PercentChange(std::shared_ptr<LineSeries> data_source, int period) 
    : Indicator(), data_source_(data_source), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
}

PercentChange::PercentChange(std::shared_ptr<LineRoot> data_source, int period) 
    : Indicator(), data_source_(nullptr), current_index_(0) {
    params.period = period;
    setup_lines();
    _minperiod(params.period + 1);
    
    // Convert LineRoot to LineSeries if possible, or add to datas
    if (data_source) {
        auto data_series = std::dynamic_pointer_cast<LineSeries>(data_source);
        if (data_series) {
            datas.push_back(data_series);
            data_source_ = data_series;
        }
    }
}

double PercentChange::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(pctchange);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*line)[ago];
}

int PercentChange::getMinPeriod() const {
    return params.period + 1;
}

void PercentChange::calculate() {
    if (data_source_ && current_index_ < data_source_->size()) {
        // Implementation for LineSeries-based calculation
        current_index_++;
    } else {
        // Use existing next() method for traditional calculation
        next();
    }
}

void PercentChange::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PercentChange::prenext() {
    Indicator::prenext();
}

void PercentChange::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto pctchange_line = lines->getline(pctchange);
    
    if (data_line && pctchange_line) {
        double current_value = (*data_line)[0];
        double period_ago_value = (*data_line)[-params.period];
        
        if (period_ago_value != 0.0) {
            // Formula: (current / period_ago) - 1.0
            double pct_change = (current_value / period_ago_value) - 1.0;
            pctchange_line->set(0, pct_change);
        } else {
            pctchange_line->set(0, 0.0);
        }
    }
}

void PercentChange::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_line = datas[0]->lines->getline(0);
    auto pctchange_line = lines->getline(pctchange);
    
    if (!data_line || !pctchange_line) return;
    
    for (int i = start; i < end; ++i) {
        if (i >= params.period) {
            double current_value = (*data_line)[i];
            double period_ago_value = (*data_line)[i - params.period];
            
            if (period_ago_value != 0.0) {
                double pct_change = (current_value / period_ago_value) - 1.0;
                pctchange_line->set(i, pct_change);
            } else {
                pctchange_line->set(i, 0.0);
            }
        }
    }
}

} // namespace indicators
} // namespace backtrader