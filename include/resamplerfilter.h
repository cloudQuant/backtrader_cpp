#pragma once

#include "metabase.h"
#include "dataseries.h"
#include <memory>
#include <chrono>
#include <cmath>

namespace backtrader {

// Forward declarations
class DataSeries;

// TimeFrame enum matching Python version
enum class TimeFrame : int {
    Ticks = 1,
    MicroSeconds = 2,
    Seconds = 3,
    Minutes = 4,
    Days = 5,
    Weeks = 6,
    Months = 7,
    Years = 8
};

// DTFaker class - simulates datetime for real-time feeds
class DTFaker {
public:
    DTFaker(std::shared_ptr<DataSeries> data, std::shared_ptr<DataSeries> forcedata = nullptr);
    virtual ~DTFaker() = default;
    
    // Length
    size_t size() const;
    
    // DateTime access
    std::chrono::system_clock::time_point datetime(int idx = 0) const;
    std::chrono::system_clock::time_point date(int idx = 0) const;
    std::chrono::system_clock::time_point time(int idx = 0) const;
    
    // Indexing
    double operator[](int idx) const;
    
    // Date conversion
    double date2num(const std::chrono::system_clock::time_point& dt) const;
    std::chrono::system_clock::time_point num2date(double num) const;
    
    // Session end
    std::chrono::system_clock::time_point get_next_eos() const;
    
private:
    std::shared_ptr<DataSeries> data_;
    double dt_;
    std::chrono::system_clock::time_point dtime_;
    std::chrono::system_clock::time_point sessionend_;
};

// Bar data holder
struct Bar {
    double open = 0.0;
    double high = -std::numeric_limits<double>::infinity();
    double low = std::numeric_limits<double>::infinity();
    double close = 0.0;
    double volume = 0.0;
    double openinterest = 0.0;
    std::chrono::system_clock::time_point datetime;
    
    Bar(bool maxdate = false);
    void reset();
    void update(double o, double h, double l, double c, double v, double oi = 0.0);
    void bupdate(std::shared_ptr<DataSeries> data);
    std::vector<double> lvalues() const;
};

// Base resampler class
class BaseResampler {
public:
    struct Params {
        bool bar2edge = true;
        bool adjbartime = true;
        bool rightedge = true;
        int boundoff = 0;
        TimeFrame timeframe = TimeFrame::Days;
        int compression = 1;
        bool takelate = true;
        bool sessionend = true;
    } params;
    
    BaseResampler(std::shared_ptr<DataSeries> data);
    virtual ~BaseResampler() = default;
    
    // Main processing method
    virtual bool apply(std::shared_ptr<DataSeries> data) = 0;
    
    // Check if bar is over
    bool check_bar_over(std::shared_ptr<DataSeries> data, bool fromcheck = false, 
                       std::shared_ptr<DataSeries> forcedata = nullptr);
    
protected:
    bool replaying = false;
    bool subdays;
    bool subweeks;
    bool componly;
    Bar bar;
    int compcount;
    bool first_bar_;
    bool doadjusttime;
    std::chrono::system_clock::time_point next_eos_;
    std::shared_ptr<DataSeries> data_;
    
    // Helper methods
    bool late_data(std::shared_ptr<DataSeries> data) const;
    virtual bool bar_over(const DTFaker& data) = 0;
    std::chrono::system_clock::time_point bar_start(const std::chrono::system_clock::time_point& dt) const;
    void adjust_time(Bar& bar, const std::chrono::system_clock::time_point& dt) const;
};

// Resampler class
class Resampler : public BaseResampler {
public:
    Resampler(std::shared_ptr<DataSeries> data);
    virtual ~Resampler() = default;
    
    bool apply(std::shared_ptr<DataSeries> data) override;

protected:
    bool bar_over(const DTFaker& data) override;
    
private:
    std::chrono::system_clock::time_point last_dt_;
    std::chrono::system_clock::time_point bar_start_dt_;
};

// Replayer class
class Replayer : public BaseResampler {
public:
    Replayer(std::shared_ptr<DataSeries> data);
    virtual ~Replayer() = default;
    
    bool apply(std::shared_ptr<DataSeries> data) override;

protected:
    bool bar_over(const DTFaker& data) override;
    
private:
    std::chrono::system_clock::time_point last_dt_;
    std::chrono::system_clock::time_point current_bar_dt_;
    std::vector<Bar> bar_history_;
    size_t current_bar_idx_;
};

// Specific resampler types
class ResamplerTicks : public Resampler {
public:
    ResamplerTicks(std::shared_ptr<DataSeries> data);
};

class ResamplerSeconds : public Resampler {
public:
    ResamplerSeconds(std::shared_ptr<DataSeries> data);
};

class ResamplerMinutes : public Resampler {
public:
    ResamplerMinutes(std::shared_ptr<DataSeries> data);
};

class ResamplerDaily : public Resampler {
public:
    ResamplerDaily(std::shared_ptr<DataSeries> data);
};

class ResamplerWeekly : public Resampler {
public:
    ResamplerWeekly(std::shared_ptr<DataSeries> data);
};

class ResamplerMonthly : public Resampler {
public:
    ResamplerMonthly(std::shared_ptr<DataSeries> data);
};

class ResamplerYearly : public Resampler {
public:
    ResamplerYearly(std::shared_ptr<DataSeries> data);
};

// Specific replayer types
class ReplayerTicks : public Replayer {
public:
    ReplayerTicks(std::shared_ptr<DataSeries> data);
};

class ReplayerSeconds : public Replayer {
public:
    ReplayerSeconds(std::shared_ptr<DataSeries> data);
};

class ReplayerMinutes : public Replayer {
public:
    ReplayerMinutes(std::shared_ptr<DataSeries> data);
};

class ReplayerDaily : public Replayer {
public:
    ReplayerDaily(std::shared_ptr<DataSeries> data);
};

class ReplayerWeekly : public Replayer {
public:
    ReplayerWeekly(std::shared_ptr<DataSeries> data);
};

class ReplayerMonthly : public Replayer {
public:
    ReplayerMonthly(std::shared_ptr<DataSeries> data);
};

// Factory functions
std::shared_ptr<Resampler> create_resampler(TimeFrame timeframe, std::shared_ptr<DataSeries> data);
std::shared_ptr<Replayer> create_replayer(TimeFrame timeframe, std::shared_ptr<DataSeries> data);

// Helper functions
std::string timeframe_to_string(TimeFrame tf);
std::chrono::seconds timeframe_to_seconds(TimeFrame tf);
std::chrono::system_clock::time_point round_to_timeframe(const std::chrono::system_clock::time_point& dt, TimeFrame tf);

} // namespace backtrader