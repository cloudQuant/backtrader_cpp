#pragma once

#include "lineseries.h"
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <numeric>
#include <limits>

namespace backtrader {

class DataSeries;

class LineIterator : public LineSeries {
public:
    // Line types
    static constexpr int IndType = 0;
    static constexpr int StratType = 1;
    static constexpr int ObsType = 2;
    
    LineIterator();
    virtual ~LineIterator() = default;
    
    // Execution hooks
    virtual void prenext() {}
    virtual void nextstart() { next(); }
    virtual void next() {}
    virtual void preonce(int start, int end) {}
    virtual void oncestart(int start, int end) { once(start, end); }
    virtual void once(int start, int end) {}
    
    // Internal execution
    virtual void _next();
    virtual void _once();
    
    // Clock and data management
    std::shared_ptr<LineSeries> _clock;
    std::vector<std::shared_ptr<LineSeries>> datas;
    std::map<std::shared_ptr<LineSeries>, bool> ddatas;
    
    // Data access convenience
    std::shared_ptr<LineSeries> data; // Primary data (datas[0])
    
    // Global bar management
    static void increment_global_bar();
    static size_t get_global_bar_count();
    static void reset_global_bar_count();  // Reset for new test runs
    bool should_execute_for_current_bar();
    
    // Line iterators management
    std::map<int, std::vector<std::shared_ptr<LineIterator>>> _lineiterators;
    
    // Minimum period management
    size_t _minperiod() const { return minperiod_; }
    void _minperiod(size_t period) { minperiod_ = period; }
    void _periodrecalc() override;
    
    // Lifecycle
    void _stage1() override;
    void _stage2() override;
    
    // Data access helpers
    std::vector<std::shared_ptr<LineIterator>> getindicators();
    std::vector<std::shared_ptr<LineIterator>> getindicators_lines();
    std::vector<std::shared_ptr<LineIterator>> getobservers();
    
    // Indicator management
    void addindicator(std::shared_ptr<LineIterator> indicator);
    
    // Line binding
    LineIterator& bindlines(const std::vector<int>& owner = {}, 
                           const std::vector<int>& own = {});
    LineIterator& bind2lines(const std::vector<int>& owner = {}, 
                            const std::vector<int>& own = {});
    LineIterator& bind2line(const std::vector<int>& owner = {}, 
                           const std::vector<int>& own = {});
    
    // Clock update
    void _clk_update() override;
    
    // Notification system
    virtual void _addnotification(const std::string& type, const std::string& msg) {}
    virtual void _notify() override {}
    
    // Plotting
    virtual void _plotinit() {}
    
    // Buffer management
    void qbuffer(size_t savemem = 0);
    
    // Minimum period status
    virtual int _getminperstatus();
    
    // Force next mode flag
    bool _nextforce = false;
    
    // Minimum data count required
    size_t _mindatas = 1;
    
    // Plot information
    struct PlotInfo {
        bool plot = true;
        bool subplot = true;
        std::string plotname = "";
        bool plotskip = false;
        bool plotabove = false;
        bool plotlinelabels = false;
        bool plotlinevalues = true;
        bool plotvaluetags = true;
        double plotymargin = 0.0;
        std::vector<double> plotyhlines;
        std::vector<double> plotyticks;
        std::vector<double> plothlines;
        bool plotforce = false;
        std::shared_ptr<LineIterator> plotmaster = nullptr;
    } plotinfo;
    
protected:
    // minperiod_ is inherited from LineRoot, don't declare it again
    
    // Global bar tracking to prevent multiple executions per bar
    static size_t global_bar_count_;
    size_t last_executed_bar_ = SIZE_MAX;  // Track the last bar this indicator was executed
    
private:
    void _setup_data_aliases();
};

// Data accessor with price constants
class DataAccessor : public LineIterator {
public:
    DataAccessor();
    virtual ~DataAccessor() = default;
    
    // Price access constants (matching DataSeries indices)
    static constexpr int PriceClose = 3;
    static constexpr int PriceLow = 2;
    static constexpr int PriceHigh = 1;
    static constexpr int PriceOpen = 0;
    static constexpr int PriceVolume = 4;
    static constexpr int PriceOpenInterest = 5;
    static constexpr int PriceDateTime = 6;
};

// Base classes for different iterator types
class IndicatorBase : public DataAccessor {
public:
    IndicatorBase();
    virtual ~IndicatorBase() = default;
    
    // Utility methods expected by tests
    virtual size_t size() const;
    virtual std::shared_ptr<LineSingle> getLine(size_t idx = 0) const;
    virtual double get(int ago = 0) const;
    virtual int getMinPeriod() const;
    virtual void calculate() {}
};

class ObserverBase : public DataAccessor {
public:
    ObserverBase();
    virtual ~ObserverBase() = default;
};

class StrategyBase : public DataAccessor {
public:
    StrategyBase();
    virtual ~StrategyBase() = default;
};

// Coupler classes for different line lengths
class SingleCoupler : public LineActions {
public:
    SingleCoupler(std::shared_ptr<LineSeries> cdata, 
                  std::shared_ptr<LineSeries> clock = nullptr);
    virtual ~SingleCoupler() = default;
    
    void next() override;
    
private:
    std::shared_ptr<LineSeries> cdata_;
    std::shared_ptr<LineSeries> _clock;
    size_t dlen_;
    double val_;
};

class MultiCoupler : public LineIterator {
public:
    MultiCoupler();
    virtual ~MultiCoupler() = default;
    
    void next() override;
    
private:
    size_t dlen_;
    size_t dsize_;
    std::vector<double> dvals_;
};

// Factory function for creating couplers
std::shared_ptr<LineIterator> LinesCoupler(std::shared_ptr<LineSeries> cdata,
                                          std::shared_ptr<LineSeries> clock = nullptr);

// Alias
using LineCoupler = decltype(LinesCoupler);

} // namespace backtrader