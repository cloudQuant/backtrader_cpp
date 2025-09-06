#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
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
    RSI(std::shared_ptr<DataSeries> data_source, int period = 14);
    virtual ~RSI() = default;
    
    void next() override;
    void once(int start, int end) override;
    
    // Parameters
    int period;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    size_t size() const override;
    int getMinPeriod() const { return _minperiod(); }
    void calculate() override;
    
    // Overbought/Oversold status method
    double getOverboughtOversoldStatus() const;
    
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