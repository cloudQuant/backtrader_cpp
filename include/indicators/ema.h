#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

class EMA : public Indicator {
public:
    // Constructor with period only (original)
    explicit EMA(int period = 30);
    // Constructor with data source and period (Python-style API)
    EMA(std::shared_ptr<LineSeries> data_source, int period = 30);
    virtual ~EMA() = default;
    
    void next() override;
    void once(int start, int end) override;
    
    // Parameters
    int period;
    double alpha;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const { return period; }
    void calculate();
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    bool first_value_;
    double ema_value_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

} // namespace indicators
} // namespace backtrader