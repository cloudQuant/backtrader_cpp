#pragma once

#include "../feed.h"
#include <memory>
#include <vector>

namespace backtrader {
namespace filters {

/**
 * BSplitter - Bar splitter filter
 * 
 * Splits bars into smaller timeframes by dividing each bar into multiple sub-bars.
 * Useful for creating higher frequency data from lower frequency bars.
 * 
 * Params:
 *   - splits: Number of sub-bars to create from each original bar
 *   - method: Split method ('equal', 'random', 'volume_weighted')
 */
class BSplitter : public AbstractDataBase {
public:
    // Split methods
    enum class SplitMethod {
        EQUAL,           // Equal price distribution
        RANDOM,          // Random price walk
        VOLUME_WEIGHTED  // Volume-weighted distribution
    };

    // Parameters structure
    struct Params {
        int splits = 2;
        SplitMethod method = SplitMethod::EQUAL;
        unsigned int seed = 0;  // Random seed for random method
    };

    BSplitter(std::shared_ptr<AbstractDataBase> dataname, const Params& params = Params{});
    virtual ~BSplitter() = default;

    // AbstractDataBase interface
    void start() override;
    void preload() override;
    bool next() override;

private:
    // Sub-bar structure
    struct SubBar {
        double open, high, low, close;
        double volume, openinterest;
        std::chrono::system_clock::time_point datetime;
        
        SubBar(double o, double h, double l, double c, double v, double oi,
               const std::chrono::system_clock::time_point& dt)
            : open(o), high(h), low(l), close(c), volume(v), openinterest(oi), datetime(dt) {}
    };

    // Parameters
    Params params_;
    
    // Underlying data source
    std::shared_ptr<AbstractDataBase> dataname_;
    
    // Sub-bars queue
    std::vector<SubBar> sub_bars_;
    size_t current_sub_bar_ = 0;
    
    // Random number generator
    std::mt19937 rng_;
    
    // Internal methods
    void split_bar();
    void split_equal();
    void split_random();
    void split_volume_weighted();
    
    // Time distribution
    std::chrono::system_clock::time_point calculate_sub_bar_time(
        const std::chrono::system_clock::time_point& start_time,
        const std::chrono::system_clock::time_point& end_time,
        int sub_bar_index) const;
};

} // namespace filters
} // namespace backtrader