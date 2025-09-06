#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <deque>
#include <type_traits>
#include <iostream>

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
    // Constructor for test framework compatibility with DataSeries
    SMA(std::shared_ptr<DataSeries> data_source);
    SMA(std::shared_ptr<DataSeries> data_source, int period);
    
    // Tag types for constructor disambiguation
    struct IndicatorSourceTag {};
    static constexpr IndicatorSourceTag from_indicator{};
    
    // Constructor with indicator as data source (for nested indicators)
    SMA(IndicatorSourceTag, std::shared_ptr<IndicatorBase> indicator_source, int period);
    
    // Convenience factory method for creating SMA from another indicator
    template<typename IndicatorType>
    static std::shared_ptr<SMA> fromIndicator(std::shared_ptr<IndicatorType> indicator_source, int period) {
        static_assert(std::is_base_of_v<IndicatorBase, IndicatorType>, "IndicatorType must inherit from IndicatorBase");
        return std::make_shared<SMA>(from_indicator, std::static_pointer_cast<IndicatorBase>(indicator_source), period);
    }
    virtual ~SMA() = default;
    
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    void _once() override;
    
    // Parameters
    int period;
    
    // Utility methods for tests
    double get(int ago = 0) const override;
    int getMinPeriod() const override { 
        // For nested indicators, the minimum period was set in constructor
        // Use the internal minperiod_ value directly
        int min_period = static_cast<int>(minperiod_);
        return std::max(1, min_period); 
    }
    Params getParams() const { return Params(period); }
    void calculate() override;
    
    // Override size to return the size of our SMA line
    size_t size() const override;
    
protected:
    std::vector<std::string> _get_line_names() const override;
    
private:
    std::deque<double> values_;
    double sum_;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;

    // IndicatorBase support for nested indicators
    std::shared_ptr<IndicatorBase> indicator_source_;
};

} // namespace indicators
} // namespace backtrader