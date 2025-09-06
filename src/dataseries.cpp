#include "dataseries.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace backtrader {

// TimeFrame utility function
std::string timeframe_name(TimeFrame tf) {
    return timeframe_to_string(tf);
}

// DataSeries implementation
const std::vector<int> DataSeries::LineOrder = {
    DateTime, Open, High, Low, Close, Volume, OpenInterest
};

DataSeries::DataSeries() : LineSeries() {
    // Manually initialize lines with the correct line names
    // Cannot use _init_lines() in constructor due to virtual function dispatch
    std::vector<std::string> line_names = {"datetime", "open", "high", "low", "close", "volume", "openinterest"};
    lines = Lines::derive("DataSeries", line_names);
    
    // Update LineMultiple's lines_ vector
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
    
    name = _name;
}

std::vector<std::string> DataSeries::getwriterheaders() {
    std::vector<std::string> headers = {_name, "len"};
    
    for (int lo : LineOrder) {
        if (lo < static_cast<int>(lines->size())) {
            headers.push_back(getlinealias(lo));
        }
    }
    
    // Add remaining line aliases
    auto aliases = lines->get_aliases();
    if (aliases.size() > LineOrder.size()) {
        for (size_t i = LineOrder.size(); i < aliases.size(); ++i) {
            headers.push_back(aliases[i]);
        }
    }
    
    return headers;
}

std::vector<std::string> DataSeries::getwritervalues() {
    size_t l = size();
    std::vector<std::string> values = {_name, std::to_string(l)};
    
    if (l > 0) {
        // Add datetime
        if (DateTime < static_cast<int>(lines->size())) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << (*lines->getline(DateTime))[0];
            values.push_back(oss.str());
        }
        
        // Add OHLCV values
        for (size_t i = 1; i < LineOrder.size(); ++i) {
            int line_idx = LineOrder[i];
            if (line_idx < static_cast<int>(lines->size())) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << (*lines->getline(line_idx))[0];
                values.push_back(oss.str());
            }
        }
        
        // Add remaining lines
        for (size_t i = LineOrder.size(); i < lines->size(); ++i) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << (*lines->getline(i))[0];
            values.push_back(oss.str());
        }
    } else {
        // No values yet - add empty strings
        for (size_t i = 0; i < lines->size(); ++i) {
            values.push_back("");
        }
    }
    
    return values;
}

std::map<std::string, std::string> DataSeries::getwriterinfo() {
    std::map<std::string, std::string> info;
    info["Name"] = _name;
    info["Timeframe"] = timeframe_name(_timeframe);
    info["Compression"] = std::to_string(_compression);
    return info;
}

std::vector<std::string> DataSeries::_get_line_names() const {
    return {"datetime", "open", "high", "low", "close", "volume", "openinterest"};
}

// OHLCV accessor methods implementation
double DataSeries::datetime(int ago) const {
    if (DateTime < static_cast<int>(lines->size())) {
        auto line = lines->getline(DateTime);
        if (line) return (*line)[-ago];
    }
    return 0.0;
}

double DataSeries::open(int ago) const {
    if (Open < static_cast<int>(lines->size())) {
        auto line = lines->getline(Open);
        if (line) return (*line)[-ago];
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double DataSeries::high(int ago) const {
    if (High < static_cast<int>(lines->size())) {
        auto line = lines->getline(High);
        if (line) return (*line)[-ago];
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double DataSeries::low(int ago) const {
    if (Low < static_cast<int>(lines->size())) {
        auto line = lines->getline(Low);
        if (line) return (*line)[-ago];
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double DataSeries::close(int ago) const {
    if (Close < static_cast<int>(lines->size())) {
        auto line = lines->getline(Close);
        if (line) {
            return (*line)[-ago];
        }
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double DataSeries::volume(int ago) const {
    if (Volume < static_cast<int>(lines->size())) {
        auto line = lines->getline(Volume);
        if (line) return (*line)[-ago];
    }
    return 0.0;
}

double DataSeries::openinterest(int ago) const {
    if (OpenInterest < static_cast<int>(lines->size())) {
        auto line = lines->getline(OpenInterest);
        if (line) return (*line)[-ago];
    }
    return 0.0;
}

// OHLC implementation
OHLC::OHLC() : DataSeries() {
}

std::vector<std::string> OHLC::_get_line_names() const {
    return {"close", "low", "high", "open", "volume", "openinterest"};
}

// OHLCDateTime implementation
OHLCDateTime::OHLCDateTime() : OHLC() {
}

std::vector<std::string> OHLCDateTime::_get_line_names() const {
    return {"datetime"};
}

// SimpleFilterWrapper implementation
SimpleFilterWrapper::SimpleFilterWrapper(std::shared_ptr<DataSeries> data, FilterFunc filter)
    : filter_(filter) {
}

bool SimpleFilterWrapper::operator()(std::shared_ptr<DataSeries> data) {
    if (filter_(data)) {
        data->backward();
        return true;
    }
    return false;
}

// Bar implementation
Bar::Bar(bool maxdate) {
    _init_field_map();
    bstart(maxdate);
}

void Bar::bstart(bool maxdate) {
    close = std::numeric_limits<double>::quiet_NaN();
    low = std::numeric_limits<double>::infinity();
    high = -std::numeric_limits<double>::infinity();
    open = std::numeric_limits<double>::quiet_NaN();
    volume = 0.0;
    openinterest = 0.0;
    datetime = maxdate ? MAXDATE : 0.0;
}

bool Bar::isopen() const {
    // NaN is the value which is not equal to itself
    return open == open; // False if NaN, True otherwise
}

bool Bar::bupdate(std::shared_ptr<DataSeries> data, bool reopen) {
    if (reopen) {
        bstart();
    }
    
    if (data->lines->size() > DataSeries::DateTime) {
        datetime = (*data->lines->getline(DataSeries::DateTime))[0];
    }
    
    if (data->lines->size() > DataSeries::High) {
        high = std::max(high, (*data->lines->getline(DataSeries::High))[0]);
    }
    
    if (data->lines->size() > DataSeries::Low) {
        low = std::min(low, (*data->lines->getline(DataSeries::Low))[0]);
    }
    
    if (data->lines->size() > DataSeries::Close) {
        close = (*data->lines->getline(DataSeries::Close))[0];
    }
    
    if (data->lines->size() > DataSeries::Volume) {
        volume += (*data->lines->getline(DataSeries::Volume))[0];
    }
    
    if (data->lines->size() > DataSeries::OpenInterest) {
        openinterest = (*data->lines->getline(DataSeries::OpenInterest))[0];
    }
    
    double o = open;
    if (reopen || !(o == o)) { // Check if open is NaN
        if (data->lines->size() > DataSeries::Open) {
            open = (*data->lines->getline(DataSeries::Open))[0];
        }
        return true; // just opened the bar
    }
    
    return false;
}

double& Bar::operator[](const std::string& key) {
    auto it = field_map_.find(key);
    if (it != field_map_.end()) {
        return *it->second;
    }
    throw std::runtime_error("Invalid field name: " + key);
}

const double& Bar::operator[](const std::string& key) const {
    auto it = field_map_.find(key);
    if (it != field_map_.end()) {
        return *it->second;
    }
    throw std::runtime_error("Invalid field name: " + key);
}

void Bar::_init_field_map() {
    field_map_["close"] = &close;
    field_map_["low"] = &low;
    field_map_["high"] = &high;
    field_map_["open"] = &open;
    field_map_["volume"] = &volume;
    field_map_["openinterest"] = &openinterest;
    field_map_["datetime"] = &datetime;
}

} // namespace backtrader