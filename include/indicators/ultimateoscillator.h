#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Ultimate Oscillator
class UltimateOscillator : public Indicator {
public:
    struct Params {
        int p1 = 7;               // First period
        int p2 = 14;              // Second period 
        int p3 = 28;              // Third period
        double upperband = 70.0;  // Upper band level
        double lowerband = 30.0;  // Lower band level
    } params;
    
    // Lines
    enum Lines { 
        uo = 0  // Ultimate Oscillator line
    };
    
    UltimateOscillator();
    UltimateOscillator(std::shared_ptr<LineSeries> data_source, int p1 = 7, int p2 = 14, int p3 = 28);
    // DataSeries constructors for disambiguation
    UltimateOscillator(std::shared_ptr<DataSeries> data_source);
    UltimateOscillator(std::shared_ptr<DataSeries> data_source, int p1, int p2 = 14, int p3 = 28);
    virtual ~UltimateOscillator() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double calculate_buying_pressure(double high, double low, double close, double prev_close);
    double calculate_true_range(double high, double low, double prev_close);
    double get_sum_bp(int period);
    double get_sum_tr(int period);
    
    // Circular buffers for sum calculations
    std::vector<double> bp_values_;   // Buying Pressure values
    std::vector<double> tr_values_;   // True Range values
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    bool batch_calculated_ = false;
};

// Aliases  
using UO = UltimateOscillator;

} // namespace indicators
} // namespace backtrader