#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "atr.h"
#include "smma.h"
#include <memory>
#include <vector>

namespace backtrader {
namespace indicators {

// Directional Movement Indicator
class DirectionalMovement : public Indicator {
public:
    struct Params {
        int period = 14;
    } params;
    
    // Line indices - matches Python order
    enum LineIndex {
        adx = 0,      // Average Directional Movement Index
        adxr = 1,     // Average Directional Movement Index Rating
        plusDI = 2,   // Plus Directional Index
        minusDI = 3   // Minus Directional Index
    };
    
    DirectionalMovement();
    DirectionalMovement(std::shared_ptr<DataSeries> data_source);
    DirectionalMovement(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~DirectionalMovement() = default;
    
    // Override methods
    void calculate() override;
    size_t size() const override;
    double get(int ago = 0) const override;
    
    // DM specific methods
    double getDIPlus(int ago = 0) const;
    double getDIMinus(int ago = 0) const;
    double getADX(int ago = 0) const;
    double getADXR(int ago = 0) const;
    int getMinPeriod() const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    void calculate_dm_values();
    
    // Internal indicators
    std::shared_ptr<ATR> atr_;
    std::shared_ptr<SMMA> plusDMav_;
    std::shared_ptr<SMMA> minusDMav_;
    std::shared_ptr<SMMA> adx_smma_;
    
    // Temporary buffers for DM calculations
    std::shared_ptr<LineSeries> plusDM_series_;
    std::shared_ptr<LineSeries> minusDM_series_;
    std::shared_ptr<LineSeries> dx_series_;
};

// Aliases
using DM = DirectionalMovement;
using DirectionalMove = DirectionalMovement;

} // namespace indicators
} // namespace backtrader
