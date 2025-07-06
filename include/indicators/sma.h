#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <deque>

namespace backtrader {
namespace indicators {

class SMA : public Indicator {
public:
    // Parameter structure for getParams() compatibility
    struct Params {
        int period;
        Params(int p) : period(p) {}
    };
    
    // Constructor with period only (original)
    explicit SMA(int period = 30);
    // Constructor with data source and period (Python-style API)
    SMA(std::shared_ptr<LineSeries> data_source, int period = 30);
    SMA(std::shared_ptr<LineRoot> data_source, int period = 30);
    virtual ~SMA() = default;
    
    void next() override;
    void once(int start, int end) override;
    
    // Parameters
    int period;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const { return period; }
    Params getParams() const { return Params(period); }
    void calculate() override;
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    std::deque<double> values_;
    double sum_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // LineRoot support
    std::shared_ptr<LineRoot> lineroot_source_;
};

} // namespace indicators
} // namespace backtrader