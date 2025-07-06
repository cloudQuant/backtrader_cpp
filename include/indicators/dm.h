#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Directional Movement Indicator
class DirectionalMovement : public Indicator {
public:
    struct Params {
        int period = 14;
    } params;
    
    // Line indices
    enum LineIndex {
        plusDM = 0,   // Plus Directional Movement
        minusDM = 1,  // Minus Directional Movement
        plusDI = 2,   // Plus Directional Index
        minusDI = 3   // Minus Directional Index
    };
    
    DirectionalMovement();
    DirectionalMovement(std::shared_ptr<LineSeries> data_source, int period = 14);
    DirectionalMovement(std::shared_ptr<LineRoot> high, std::shared_ptr<LineRoot> low, 
                       std::shared_ptr<LineRoot> close, int period = 14);
    // Constructor for test framework compatibility
    DirectionalMovement(std::shared_ptr<LineRoot> data);
    virtual ~DirectionalMovement() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
    // DM specific methods
    double getDIPlus(int ago = 0) const;
    double getDIMinus(int ago = 0) const;
    double getDX(int ago = 0) const;
    double getADX(int ago = 0) const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    
    // LineRoot support (for multi-line constructor)
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
    
    // Previous values for calculation
    double prev_high_;
    double prev_low_;
    double prev_close_;
    bool first_run_;
};

// Aliases
using DM = DirectionalMovement;
using DirectionalMove = DirectionalMovement;

} // namespace indicators
} // namespace backtrader