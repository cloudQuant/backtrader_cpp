#pragma once

#include "../feed.h"
#include <memory>
#include <vector>
#include <deque>

namespace backtrader {
namespace filters {

/**
 * Renko - Renko chart filter
 * 
 * Transforms regular OHLC bars into Renko bricks based on price movements.
 * Renko charts ignore time and focus only on price movements of a fixed size.
 * 
 * Params:
 *   - size: Size of each Renko brick (price movement required)
 *   - dynamic: If true, use ATR-based dynamic sizing
 *   - atr_period: Period for ATR calculation when dynamic=true
 */
class Renko : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        double size = 1.0;          // Fixed brick size
        bool dynamic = false;       // Use dynamic sizing based on ATR
        int atr_period = 14;        // ATR period for dynamic sizing
        double atr_multiplier = 1.0; // ATR multiplier for dynamic sizing
    };

    Renko(std::shared_ptr<AbstractDataBase> dataname, const Params& params = Params{});
    virtual ~Renko() = default;

    // AbstractDataBase interface
    void start() override;
    void preload() override;
    bool next() override;

private:
    // Renko brick structure
    struct RenkoBrick {
        double open;
        double high;
        double low;
        double close;
        double volume;
        double openinterest;
        std::chrono::system_clock::time_point datetime;
        
        RenkoBrick(double o, double h, double l, double c, double v, double oi,
                   const std::chrono::system_clock::time_point& dt)
            : open(o), high(h), low(l), close(c), volume(v), openinterest(oi), datetime(dt) {}
    };

    // Parameters
    Params params_;
    
    // Underlying data source
    std::shared_ptr<AbstractDataBase> dataname_;
    
    // Renko state
    std::deque<RenkoBrick> renko_bricks_;
    double current_brick_base_ = 0.0;
    bool first_brick_ = true;
    
    // ATR calculation for dynamic sizing
    std::vector<double> atr_values_;
    double current_atr_ = 0.0;
    
    // Volume and OI accumulation
    double accumulated_volume_ = 0.0;
    double accumulated_oi_ = 0.0;
    
    // Internal methods
    double get_brick_size() const;
    void calculate_atr();
    void process_price_movement(double price, double volume, double oi,
                               const std::chrono::system_clock::time_point& datetime);
    
    void create_brick(double open, double close, double volume, double oi,
                     const std::chrono::system_clock::time_point& datetime);
    
    bool load_next_brick();
    
    // ATR calculation helper
    double calculate_true_range(double high, double low, double prev_close) const;
};

} // namespace filters
} // namespace backtrader