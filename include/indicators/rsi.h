#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <deque>

namespace backtrader {
namespace indicators {

class RSI : public Indicator {
public:
    // Constructor with period only (original)
    explicit RSI(int period = 14);
    // Constructor with data source and period (Python-style API)
    RSI(std::shared_ptr<LineSeries> data_source, int period = 14);
    // Constructor for test framework compatibility
    RSI(std::shared_ptr<LineRoot> data);
    RSI(std::shared_ptr<LineRoot> data, int period);
    virtual ~RSI() = default;
    
    void next() override;
    void once(int start, int end) override;
    
    // Parameters
    int period;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const { return _minperiod(); }
    void calculate() override;
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    std::deque<double> gains_;
    std::deque<double> losses_;
    double avg_gain_;
    double avg_loss_;
    double prev_value_;
    bool first_calculation_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

} // namespace indicators
} // namespace backtrader