#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

class EMA : public Indicator {
public:
    // Constructor with period only (original)
    explicit EMA(int period = 30);
    // Constructor for test framework compatibility
    explicit EMA(std::shared_ptr<LineRoot> data);
    // Constructor with data source and period (Python-style API)
    EMA(std::shared_ptr<LineSeries> data_source, int period = 30);
    EMA(std::shared_ptr<LineRoot> data_source, int period);
    virtual ~EMA() = default;
    
    void next() override;
    void once(int start, int end) override;
    
    // Parameters
    int period;
    double alpha;
    
    // Utility methods for tests
    double get(int ago = 0) const override;
    int getMinPeriod() const override { return period; }
    void calculate() override;
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    bool first_value_;
    double ema_value_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // LineRoot support
    std::shared_ptr<LineRoot> lineroot_source_;
};

} // namespace indicators
} // namespace backtrader