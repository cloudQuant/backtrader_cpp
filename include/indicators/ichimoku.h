#pragma once

#include "../indicator.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Ichimoku Cloud indicator
class Ichimoku : public Indicator {
public:
    struct Params {
        int tenkan = 9;         // Tenkan-sen period
        int kijun = 26;         // Kijun-sen period  
        int senkou = 52;        // Senkou span B period
        int senkou_lead = 26;   // Forward push for senkou spans
        int chikou = 26;        // Backward push for chikou span
    } params;
    
    // Line indices
    enum LineIndex { 
        tenkan_sen = 0,     // Tenkan-sen (Conversion Line)
        kijun_sen = 1,      // Kijun-sen (Base Line)
        senkou_span_a = 2,  // Senkou Span A (Leading Span A)
        senkou_span_b = 3,  // Senkou Span B (Leading Span B)
        chikou_span = 4     // Chikou Span (Lagging Span)
    };
    
    Ichimoku();
    Ichimoku(std::shared_ptr<LineSeries> data_source);  // For test framework
    Ichimoku(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low, std::shared_ptr<LineSeries> close,
             int tenkan = 9, int kijun = 26, int senkou = 52);
    // DataSeries constructors for disambiguation
    Ichimoku(std::shared_ptr<DataSeries> data_source);
    Ichimoku(std::shared_ptr<DataSeries> data_source, int tenkan, int kijun = 26, int senkou = 52);
    virtual ~Ichimoku() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    size_t size() const override;
    void calculate() override;
    
    // Ichimoku specific methods
    double getTenkanSen(int ago = 0) const;
    double getKijunSen(int ago = 0) const;
    double getSenkouSpanA(int ago = 0) const;
    double getSenkouSpanB(int ago = 0) const;
    double getChikouSpan(int ago = 0) const;
    
    // Shorter aliases expected by tests
    double getSenkouA(int ago = 0) const { return getSenkouSpanA(ago); }
    double getSenkouB(int ago = 0) const { return getSenkouSpanB(ago); }
    double getChikou(int ago = 0) const { return getChikouSpan(ago); }
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double get_highest(int period, int offset = 0);
    double get_lowest(int period, int offset = 0);
    double calculate_tenkan_sen(int offset = 0);
    double calculate_kijun_sen(int offset = 0);
    double calculate_senkou_span_a(int offset = 0);
    double calculate_senkou_span_b(int offset = 0);
    
    // Storage for forward/backward shifted values
    std::vector<double> senkou_span_a_values_;
    std::vector<double> senkou_span_b_values_;
    std::vector<double> chikou_span_values_;
    
    // Data sources

    size_t current_index_;
};

} // namespace indicators
} // namespace backtrader