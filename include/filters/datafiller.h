#pragma once

#include "../feed.h"
#include "../timeframe.h"
#include <deque>
#include <chrono>
#include <cmath>

namespace backtrader {
namespace filters {

/**
 * DataFiller - Fills gaps in source data using timing information
 * 
 * This class fills missing bars in the data source using timeframe and compression
 * information. Missing bars are filled with the last closing price.
 * 
 * Params:
 *   - fill_price: Price to use for filling (nullptr = use last close)
 *   - fill_vol: Volume to use for filling (NaN by default)
 *   - fill_oi: Open interest to use for filling (NaN by default)
 */
class DataFiller : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::shared_ptr<double> fill_price = nullptr;  // nullptr = use last close
        double fill_vol = std::numeric_limits<double>::quiet_NaN();
        double fill_oi = std::numeric_limits<double>::quiet_NaN();
    };

    DataFiller(std::shared_ptr<AbstractDataBase> dataname, const Params& params = Params{});
    virtual ~DataFiller() = default;

    // AbstractDataBase interface
    void start() override;
    void preload() override;
    bool next() override;
    
    // DataFiller specific methods
    void set_fill_price(double price);
    void set_fill_vol(double vol);
    void set_fill_oi(double oi);

private:
    // Fill bar structure
    struct FillBar {
        std::chrono::system_clock::time_point datetime;
        double price;
        
        FillBar(const std::chrono::system_clock::time_point& dt, double p)
            : datetime(dt), price(p) {}
    };

    // Parameters
    Params params_;
    
    // Underlying data source
    std::shared_ptr<AbstractDataBase> dataname_;
    
    // Fill bars queue
    std::deque<FillBar> fillbars_;
    
    // Current bar state
    bool has_data_bar_ = false;
    
    // Time delta unit for current timeframe
    std::chrono::milliseconds time_delta_unit_;
    
    // Time delta mapping for different timeframes
    static const std::map<TimeFrame, std::chrono::milliseconds> time_deltas_;
    
    // Internal methods
    bool copy_from_data();
    bool fill_from_bars();
    void calculate_time_delta();
    void fill_missing_bars(const std::chrono::system_clock::time_point& prev_time,
                          const std::chrono::system_clock::time_point& curr_time,
                          double last_price);
    
    // Time utility methods
    std::chrono::system_clock::time_point get_session_end(
        const std::chrono::system_clock::time_point& ref_time) const;
    std::chrono::system_clock::time_point get_session_start(
        const std::chrono::system_clock::time_point& ref_time) const;
    
    // Initialize time delta mapping
    static std::map<TimeFrame, std::chrono::milliseconds> init_time_deltas();
};

} // namespace filters
} // namespace backtrader