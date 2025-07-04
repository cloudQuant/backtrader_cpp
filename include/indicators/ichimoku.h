#pragma once

#include "../indicator.h"
#include <memory>

namespace backtrader {

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
    
    // Lines
    enum Lines { 
        tenkan_sen = 0,     // Tenkan-sen (Conversion Line)
        kijun_sen = 1,      // Kijun-sen (Base Line)
        senkou_span_a = 2,  // Senkou Span A (Leading Span A)
        senkou_span_b = 3,  // Senkou Span B (Leading Span B)
        chikou_span = 4     // Chikou Span (Lagging Span)
    };
    
    Ichimoku();
    virtual ~Ichimoku() = default;
    
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
};

} // namespace backtrader