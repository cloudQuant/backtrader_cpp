#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <deque>

namespace backtrader {
namespace indicators {

class SMA : public Indicator {
public:
    // Constructor with period only (original)
    explicit SMA(int period = 30);
    // Constructor with data source and period (Python-style API)
    SMA(std::shared_ptr<LineSeries> data_source, int period = 30);
    virtual ~SMA() = default;
    
    void next() override;
    void once(int start, int end) override;
    
    // Parameters
    int period;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const { return period; }
    void calculate();
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    std::deque<double> values_;
    double sum_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

} // namespace indicators
} // namespace backtrader