#pragma once

#include "../indicator.h"
#include <vector>
#include <algorithm>
#include <limits>

namespace backtrader {
namespace utils {

/**
 * Fractal - Fractal indicator utility
 * 
 * Identifies fractal patterns in price data.
 * 
 * A bearish fractal occurs when there is a pattern with the highest high 
 * in the middle and two lower highs on each side.
 * 
 * A bullish fractal occurs when there is a pattern with the lowest low 
 * in the middle and two higher lows on each side.
 * 
 * References:
 *   [1] http://www.investopedia.com/articles/trading/06/fractals.asp
 * 
 * Params:
 *   - period: Number of bars to look at (default: 5)
 *   - bardist: Distance to max/min in absolute percentage (default: 0.015)
 *   - shift_to_potential_fractal: Shift to potential fractal (default: 2)
 */
class Fractal : public PeriodN {
public:
    // Parameters structure
    struct Params {
        int period = 5;
        double bardist = 0.015;  // 1.5%
        int shift_to_potential_fractal = 2;
    };

    // Lines
    enum Lines {
        FRACTAL_BEARISH = 0,
        FRACTAL_BULLISH = 1
    };

    Fractal(std::shared_ptr<DataSeries> data, const Params& params = Params{});
    virtual ~Fractal() = default;

    // Indicator interface
    void next() override;

    // Fractal detection
    bool is_bearish_fractal(int lookback = 0) const;
    bool is_bullish_fractal(int lookback = 0) const;
    
    double get_bearish_fractal_value(int lookback = 0) const;
    double get_bullish_fractal_value(int lookback = 0) const;

private:
    // Parameters
    Params params_;
    
    // Internal methods
    std::vector<double> get_recent_highs() const;
    std::vector<double> get_recent_lows() const;
    
    bool detect_bearish_fractal();
    bool detect_bullish_fractal();
    
    // Find max/min index in vector
    size_t find_max_index(const std::vector<double>& values) const;
    size_t find_min_index(const std::vector<double>& values) const;
};

} // namespace utils
} // namespace backtrader