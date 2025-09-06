#pragma once

#include "dataseries.h"
#include "metabase.h"
#include <string>
#include <vector>
#include <memory>
#include <deque>
#include <functional>
#include <chrono>

namespace backtrader {

// Forward declarations
class FeedBase;
class Resampler;
class Replayer;

// Data status enumeration
enum class DataStatus {
    LIVE = 1,
    DELAYED = 2,
    CONNECTED = 4,
    DISCONNECTED = 8,
    NOTSUBSCRIBED = 16,
    UNKNOWN = 32
};

// Abstract data base class
class AbstractDataBase : public OHLCDateTime, public std::enable_shared_from_this<AbstractDataBase> {
public:
    // Parameters
    struct Params {
        std::string dataname = "";
        std::string name = "";
        int compression = 1;
        TimeFrame timeframe = TimeFrame::Days;
        std::chrono::system_clock::time_point fromdate;
        std::chrono::system_clock::time_point todate;
        std::chrono::system_clock::time_point sessionstart;
        std::chrono::system_clock::time_point sessionend;
        std::vector<std::function<bool(std::shared_ptr<AbstractDataBase>)>> filters;
        std::string tz = "";
        std::string tzinput = "";
        double qcheck = 0.0; // timeout in seconds to check for events
        std::string calendar = "";
    } params;
    
    AbstractDataBase();
    virtual ~AbstractDataBase() = default;
    
    // Data identification
    std::string _dataname = "";
    std::string _name = "";
    int _compression = 1;
    TimeFrame _timeframe = TimeFrame::Days;
    
    // Feed reference
    std::shared_ptr<FeedBase> _feed = nullptr;
    
    // Notifications queue
    std::deque<std::string> notifs;
    
    // Filter support
    std::deque<std::shared_ptr<void>> _barstack;
    std::deque<std::shared_ptr<void>> _barstash;
    std::vector<std::function<bool(std::shared_ptr<AbstractDataBase>)>> _filters;
    std::vector<std::function<bool(std::shared_ptr<AbstractDataBase>)>> _ffilters;
    
    // Status tracking
    DataStatus _status = DataStatus::UNKNOWN;
    bool _started = false;
    bool _disconnected = false;
    
    // Lifecycle methods
    virtual bool start();
    virtual void stop();
    virtual bool preload();
    virtual bool load();
    virtual bool next();
    virtual void rewind();
    
    // Status methods
    virtual bool islive() const { return false; }
    virtual DataStatus getstatus() const { return _status; }
    virtual void setstatus(DataStatus status) { _status = status; }
    
    // Data operations
    virtual bool haslivedata() const { return false; }
    virtual void do_extend(bool value) {}
    virtual bool do_backfill_at(std::chrono::system_clock::time_point datetime, int size) { return false; }
    virtual bool do_backfill(int size) { return false; }
    
    // Filter operations
    void addfilter(std::function<bool(std::shared_ptr<AbstractDataBase>)> filter);
    bool _barlen() const;
    bool _barisover() const;
    void _bar2stack();
    void _stack2bar();
    void _updatebar(const std::vector<double>& values);
    
    // Resample/Replay support
    std::shared_ptr<Resampler> _resample = nullptr;
    std::shared_ptr<Replayer> _replay = nullptr;
    
    // Clone operation
    virtual std::shared_ptr<AbstractDataBase> clone() const;
    
protected:
    // Internal load method
    virtual bool _load() = 0;
    virtual void _start() {}
    virtual void _stop() {}
    
    // Filter processing
    bool _applyfilters();
    void _processbars();
};

// Data base class
class DataBase : public AbstractDataBase {
public:
    DataBase();
    virtual ~DataBase() = default;
    
    // Extended parameters
    struct ExtendedParams {
        bool reverse = false;
        bool adjclose = true;
        bool roundvolume = false;
        double volume_fill_price = 0.0;
        bool nocase = true;
    } ext_params;
    
    // Data loading
    bool load() override;
    bool _load() override;
    
    // Time operations
    virtual std::chrono::system_clock::time_point date2num(std::chrono::system_clock::time_point dt);
    virtual std::chrono::system_clock::time_point num2date(double num);
    
protected:
    // Internal state
    bool _loadstarted = false;
    size_t _loadcount = 0;
};

// Feed base class
class FeedBase : public std::enable_shared_from_this<FeedBase> {
public:
    FeedBase();
    virtual ~FeedBase() = default;
    
    // Data management
    std::vector<std::shared_ptr<AbstractDataBase>> datas;
    
    // Lifecycle
    virtual bool start();
    virtual void stop();
    
    // Data operations
    virtual void adddata(std::shared_ptr<AbstractDataBase> data);
    virtual bool next();
    virtual bool load();
    
    // Status
    virtual bool islive() const;
    virtual DataStatus getstatus() const;
    
protected:
    bool _started = false;
    DataStatus _status = DataStatus::UNKNOWN;
};

// CSV Data base class
class CSVDataBase : public DataBase {
public:
    // CSV Parameters
    struct CSVParams {
        char separator = ',';
        bool headers = false;
        bool skipinitialspace = false;
        char quotechar = '"';
        bool doublequote = true;
        char escapechar = '\0';
        int skiprows = 0;
        int skipfooter = 0;
        
        // Date/time columns
        int datetime = 0;
        int time = -1;
        int open = 1;
        int high = 2;
        int low = 3;
        int close = 4;
        int volume = 5;
        int openinterest = 6;
        
        // Date/time format
        std::string dtformat = "%Y-%m-%d";
        std::string tmformat = "%H:%M:%S";
        
        // Null values
        std::string nullvalue = "nan";
    } csv_params;
    
    CSVDataBase();
    virtual ~CSVDataBase() = default;
    
    // CSV specific methods
    bool start() override;
    void stop() override;
    
protected:
    bool _load() override;
    virtual bool _loadline(const std::vector<std::string>& linetokens) { return false; }
    
    // CSV parsing
    std::vector<std::string> parse_csv_line(const std::string& line);
    
    // File handling
    std::ifstream* file_ = nullptr;
    std::string current_line_;
    bool file_opened_ = false;
};

// CSV Feed base class
class CSVFeedBase : public FeedBase {
public:
    // Data class to use
    using DataClass = CSVDataBase;
    
    CSVFeedBase();
    virtual ~CSVFeedBase() = default;
    
    // Factory method
    virtual std::shared_ptr<CSVDataBase> create_data();
};

// Bar structure for aggregating OHLC data
struct _Bar {
    double datetime = 0.0;
    double open = std::numeric_limits<double>::quiet_NaN();
    double high = -std::numeric_limits<double>::infinity();
    double low = std::numeric_limits<double>::infinity();
    double close = std::numeric_limits<double>::quiet_NaN();
    double volume = 0.0;
    double openinterest = 0.0;
    
    void bstart() {
        open = std::numeric_limits<double>::quiet_NaN();
        high = -std::numeric_limits<double>::infinity();
        low = std::numeric_limits<double>::infinity();
        close = std::numeric_limits<double>::quiet_NaN();
        volume = 0.0;
        openinterest = 0.0;
    }
    
    bool isopen() const {
        // NaN != NaN, so this returns true if open is not NaN
        return open == open;
    }
};

// Data Replay class - replays historical data with timing
class DataReplay : public AbstractDataBase {
public:
    DataReplay(std::shared_ptr<AbstractDataBase> source);
    virtual ~DataReplay() = default;
    
    // Replay configuration
    void replay(TimeFrame timeframe, int compression = 1);
    
    // Override data loading
    bool start() override;
    void stop() override;
    
    // Override size to return a large value initially
    size_t size() const override;
    
    // Override forward to load data
    void forward(size_t size = 1) override;
    
protected:
    bool _load() override;
    
private:
    std::shared_ptr<AbstractDataBase> source_data_;
    TimeFrame replay_timeframe_ = TimeFrame::Days;
    int replay_compression_ = 1;
    
    // Replay state
    _Bar current_bar_;
    bool bar_open_ = false;
    bool bar_delivered_ = false;
    bool source_exhausted_ = false;
    double last_dt_ = 0.0;
    
    // Helper methods
    bool _checkbarover(double dt);
    void _updatebar();
    void _deliverbar();
    bool _load_aggregate();
};

// Data Resample class - resamples data to different timeframes
class DataResample : public AbstractDataBase {
public:
    DataResample(std::shared_ptr<AbstractDataBase> data);
    // Compatibility constructor for DataSeries
    DataResample(std::shared_ptr<DataSeries> data);
    virtual ~DataResample() = default;
    
    // Resample configuration
    void resample(TimeFrame timeframe, int compression = 1);
    
    // Data interface
    bool start() override;
    void stop() override;
    
    // Override size to return actual data size
    size_t size() const override;
    
    // Override buflen to return total buffer length
    size_t buflen() const override;
    
    // Override forward to load data
    void forward(size_t size = 1) override;
    
protected:
    bool _load() override;
    
private:
    std::shared_ptr<AbstractDataBase> source_data_;
    TimeFrame resample_timeframe_;
    int resample_compression_;
    
    // Resample state (similar to DataReplay)
    _Bar current_bar_;
    bool bar_open_ = false;
    bool bar_delivered_ = false;
    bool source_exhausted_ = false;
    double last_dt_ = 0.0;
    bool preloaded_ = false;
    
    // Helper methods
    bool _checkbarover(double dt);
    void _updatebar();
    void _updatebar_internal();
    void _append_bar();
    bool _load_aggregate();
};

} // namespace backtrader