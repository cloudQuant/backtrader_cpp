#pragma once

#include "../indicator.h"
#include "../dataseries.h"

namespace backtrader {
namespace indicators {

// Simple indicator that extracts and passes through the close line from a DataSeries
// This is used to provide compatibility with Python's data.close Line object
class CloseLine : public Indicator {
public:
    enum Lines { 
        close = 0
    };
    
    CloseLine();
    explicit CloseLine(std::shared_ptr<DataSeries> data_source);
    virtual ~CloseLine() = default;
    
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    void _once() override;
    
    double get(int ago = 0) const override;
    size_t size() const override;
    
private:
    std::shared_ptr<DataSeries> data_source_;
    void setup_lines();
};

} // namespace indicators
} // namespace backtrader