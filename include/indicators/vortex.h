#pragma once

#include "../indicator.h"
#include <memory>

namespace backtrader {

// Vortex Indicator
class Vortex : public Indicator {
public:
    struct Params {
        int period = 14;  // Period for Vortex calculation
    } params;
    
    // Lines
    enum Lines { 
        vi_plus = 0,   // +VI (Positive Vortex Indicator)
        vi_minus = 1   // -VI (Negative Vortex Indicator)
    };
    
    Vortex();
    virtual ~Vortex() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper methods
    double get_sum_vm_plus(int period);
    double get_sum_vm_minus(int period);
    double get_sum_tr(int period);
    
    // Circular buffers for sum calculations
    std::vector<double> vm_plus_values_;   // |High[0] - Low[-1]| values
    std::vector<double> vm_minus_values_;  // |Low[0] - High[-1]| values
    std::vector<double> tr_values_;        // True Range values
};

// Aliases
using VI = Vortex;

} // namespace backtrader