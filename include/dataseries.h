#pragma once

#include "lineseries.h"
#include "timeframe.h"
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <limits>

namespace backtrader {

class DataSeries : public LineSeries {
public:
    // Standard line indices (matching the actual order in _get_line_names())
    static constexpr int DateTime = 0;
    static constexpr int Open = 1;
    static constexpr int High = 2;
    static constexpr int Low = 3;
    static constexpr int Close = 4;
    static constexpr int Volume = 5;
    static constexpr int OpenInterest = 6;
    
    static const std::vector<int> LineOrder;
    
    DataSeries();
    virtual ~DataSeries() = default;
    
    // Data properties
    std::string _name = "";
    std::string name = "";
    int _compression = 1;
    TimeFrame _timeframe = TimeFrame::Days;
    
    // Plot configuration
    struct PlotInfo {
        bool plot = true;
        bool plotind = true;
        bool plotylimited = true;
    } plotinfo;
    
    // OHLCV accessor methods (override LineSeries virtual methods)
    double datetime(int ago = 0) const override;
    double open(int ago = 0) const override;
    double high(int ago = 0) const override;
    double low(int ago = 0) const override;
    double close(int ago = 0) const override;
    double volume(int ago = 0) const override;
    double openinterest(int ago = 0) const override;
    
    // Writer support
    std::vector<std::string> getwriterheaders();
    std::vector<std::string> getwritervalues();
    std::map<std::string, std::string> getwriterinfo();
    
protected:
    std::vector<std::string> _get_line_names() const override;
};

class OHLC : public DataSeries {
public:
    OHLC();
    virtual ~OHLC() = default;
    
protected:
    std::vector<std::string> _get_line_names() const override;
};

class OHLCDateTime : public OHLC {
public:
    OHLCDateTime();
    virtual ~OHLCDateTime() = default;
    
protected:
    std::vector<std::string> _get_line_names() const override;
};

// Filter wrapper for data filtering
class SimpleFilterWrapper {
public:
    using FilterFunc = std::function<bool(std::shared_ptr<DataSeries>)>;
    
    SimpleFilterWrapper(std::shared_ptr<DataSeries> data, FilterFunc filter);
    virtual ~SimpleFilterWrapper() = default;
    
    bool operator()(std::shared_ptr<DataSeries> data);
    
private:
    FilterFunc filter_;
};

// Bar class for OHLC data aggregation
class Bar {
public:
    static constexpr double MAXDATE = 2958465.0; // Simplified max date
    
    Bar(bool maxdate = false);
    virtual ~Bar() = default;
    
    // Bar data
    double close = std::numeric_limits<double>::quiet_NaN();
    double low = std::numeric_limits<double>::infinity();
    double high = -std::numeric_limits<double>::infinity();
    double open = std::numeric_limits<double>::quiet_NaN();
    double volume = 0.0;
    double openinterest = 0.0;
    double datetime = 0.0;
    
    // Replay flag
    bool replaying = false;
    
    // Bar operations
    void bstart(bool maxdate = false);
    bool isopen() const;
    bool bupdate(std::shared_ptr<DataSeries> data, bool reopen = false);
    
    // Access operators for compatibility
    double& operator[](const std::string& key);
    const double& operator[](const std::string& key) const;
    
private:
    std::map<std::string, double*> field_map_;
    void _init_field_map();
};

} // namespace backtrader