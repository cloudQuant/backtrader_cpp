#include "feed.h"
#include "linebuffer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ctime>

namespace backtrader {

// AbstractDataBase implementation
AbstractDataBase::AbstractDataBase() : OHLCDateTime() {
    _dataname = params.dataname;
    _name = params.name;
    _compression = params.compression;
    _timeframe = params.timeframe;
}

bool AbstractDataBase::start() {
    if (_started) {
        return true;
    }
    
    _start();
    _started = true;
    _status = DataStatus::CONNECTED;
    
    return true;
}

void AbstractDataBase::stop() {
    if (!_started) {
        return;
    }
    
    _stop();
    _started = false;
    _status = DataStatus::DISCONNECTED;
}

bool AbstractDataBase::preload() {
    if (!_started) {
        if (!start()) {
            return false;
        }
    }
    
    // Load all data at once
    while (load()) {
        // Continue loading
    }
    
    return true;
}

bool AbstractDataBase::load() {
    if (!_started) {
        if (!start()) {
            return false;
        }
    }
    
    return _load();
}

bool AbstractDataBase::next() {
    return load();
}

void AbstractDataBase::rewind() {
    // Reset to beginning - would need to be implemented by subclasses
}

void AbstractDataBase::addfilter(std::function<bool(std::shared_ptr<AbstractDataBase>)> filter) {
    _filters.push_back(filter);
}

bool AbstractDataBase::_barlen() const {
    return !_barstack.empty();
}

bool AbstractDataBase::_barisover() const {
    // Implementation would depend on specific data format
    return true;
}

void AbstractDataBase::_bar2stack() {
    // Move current bar to stack for filter processing
    // This would be implemented based on specific data structure
}

void AbstractDataBase::_stack2bar() {
    // Move bar from stack back to current position
    // This would be implemented based on specific data structure
}

void AbstractDataBase::_updatebar(const std::vector<double>& values) {
    // Update current bar with new values
    if (values.size() >= 7) {
        if (lines->size() > DateTime) lines->getline(DateTime)->set(0, values[0]);
        if (lines->size() > Open) lines->getline(Open)->set(0, values[1]);
        if (lines->size() > High) lines->getline(High)->set(0, values[2]);
        if (lines->size() > Low) lines->getline(Low)->set(0, values[3]);
        if (lines->size() > Close) lines->getline(Close)->set(0, values[4]);
        if (lines->size() > Volume) lines->getline(Volume)->set(0, values[5]);
        if (lines->size() > OpenInterest) lines->getline(OpenInterest)->set(0, values[6]);
    }
}

bool AbstractDataBase::_applyfilters() {
    // Apply all filters to current data
    for (auto& filter : _filters) {
        if (!filter(std::dynamic_pointer_cast<AbstractDataBase>(shared_from_this()))) {
            return false;
        }
    }
    return true;
}

void AbstractDataBase::_processbars() {
    // Process bars through filters
    // This would implement the complex filter chain logic
}

std::shared_ptr<AbstractDataBase> AbstractDataBase::clone() const {
    // This would need to be implemented by derived classes
    return nullptr;
}

// DataBase implementation
DataBase::DataBase() : AbstractDataBase() {
}

bool DataBase::load() {
    if (!_loadstarted) {
        _loadstarted = true;
    }
    
    bool result = _load();
    if (result) {
        _loadcount++;
        
        // Apply filters
        if (!_applyfilters()) {
            // Filter rejected this bar, try next
            return load();
        }
    }
    
    return result;
}

bool DataBase::_load() {
    // Base implementation - should be overridden by derived classes
    return false;
}

std::chrono::system_clock::time_point DataBase::date2num(std::chrono::system_clock::time_point dt) {
    return dt;
}

std::chrono::system_clock::time_point DataBase::num2date(double num) {
    auto duration = std::chrono::duration<double>(num);
    return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
}

// FeedBase implementation
FeedBase::FeedBase() {
}

bool FeedBase::start() {
    if (_started) {
        return true;
    }
    
    bool all_started = true;
    for (auto& data : datas) {
        if (!data->start()) {
            all_started = false;
        }
    }
    
    _started = all_started;
    _status = all_started ? DataStatus::CONNECTED : DataStatus::DISCONNECTED;
    
    return all_started;
}

void FeedBase::stop() {
    if (!_started) {
        return;
    }
    
    for (auto& data : datas) {
        data->stop();
    }
    
    _started = false;
    _status = DataStatus::DISCONNECTED;
}

void FeedBase::adddata(std::shared_ptr<AbstractDataBase> data) {
    datas.push_back(data);
    data->_feed = std::dynamic_pointer_cast<FeedBase>(shared_from_this());
}

bool FeedBase::next() {
    if (!_started) {
        if (!start()) {
            return false;
        }
    }
    
    bool any_loaded = false;
    for (auto& data : datas) {
        if (data->next()) {
            any_loaded = true;
        }
    }
    
    return any_loaded;
}

bool FeedBase::load() {
    return next();
}

bool FeedBase::islive() const {
    for (const auto& data : datas) {
        if (data->islive()) {
            return true;
        }
    }
    return false;
}

DataStatus FeedBase::getstatus() const {
    return _status;
}

// CSVDataBase implementation
CSVDataBase::CSVDataBase() : DataBase() {
}

bool CSVDataBase::start() {
    if (_started) {
        return true;
    }
    
    // Open file
    if (!params.dataname.empty()) {
        file_ = new std::ifstream(params.dataname);
        if (!file_->is_open()) {
            std::cerr << "Could not open file: " << params.dataname << std::endl;
            return false;
        }
        file_opened_ = true;
        
        // Skip header rows if specified
        for (int i = 0; i < csv_params.skiprows; ++i) {
            if (!std::getline(*file_, current_line_)) {
                break;
            }
        }
    }
    
    return DataBase::start();
}

void CSVDataBase::stop() {
    if (file_opened_ && file_) {
        file_->close();
        delete file_;
        file_ = nullptr;
        file_opened_ = false;
    }
    
    DataBase::stop();
}

bool CSVDataBase::_load() {
    if (!file_ || !file_opened_) {
        return false;
    }
    
    if (!std::getline(*file_, current_line_)) {
        return false; // End of file
    }
    
    // Parse CSV line
    auto tokens = parse_csv_line(current_line_);
    
    // Call derived class implementation
    return _loadline(tokens);
}

std::vector<std::string> CSVDataBase::parse_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, csv_params.separator)) {
        // Remove quotes if present
        if (!token.empty() && token[0] == csv_params.quotechar && 
            token[token.length()-1] == csv_params.quotechar) {
            token = token.substr(1, token.length()-2);
        }
        
        // Remove initial space if configured
        if (csv_params.skipinitialspace) {
            token.erase(0, token.find_first_not_of(" \t"));
        }
        
        tokens.push_back(token);
    }
    
    return tokens;
}

// CSVFeedBase implementation
CSVFeedBase::CSVFeedBase() : FeedBase() {
}

std::shared_ptr<CSVDataBase> CSVFeedBase::create_data() {
    return std::make_shared<CSVDataBase>();
}

// DataReplay implementation
DataReplay::DataReplay(std::shared_ptr<AbstractDataBase> source) 
    : AbstractDataBase(), source_data_(source) {
    if (source_data_) {
        params.name = source_data_->params.name + "_replay";
        params.dataname = source_data_->params.dataname;
    }
}

void DataReplay::replay(TimeFrame timeframe, int compression) {
    replay_timeframe_ = timeframe;
    replay_compression_ = compression;
    params.timeframe = timeframe;
    params.compression = compression;
}

bool DataReplay::start() {
    if (!source_data_) {
        std::cerr << "DataReplay::start() - no source data!" << std::endl;
        return false;
    }
    
    std::cerr << "DataReplay::start() - called, _started=" << _started << std::endl;
    
    // Check if already started
    if (_started) {
        return true;
    }
    
    bar_open_ = false;
    source_exhausted_ = false;
    last_dt_ = 0.0;
    bar_delivered_ = false;
    current_bar_.bstart();
    
    // Start source data first
    bool source_started = source_data_->start();
    std::cerr << "DataReplay::start() - source data start result: " << source_started << std::endl;
    
    bool result = source_started && AbstractDataBase::start();
    
    std::cerr << "DataReplay::start() - result = " << result << std::endl;
    
    if (result) {
        // Load and aggregate all data upfront
        std::cerr << "DataReplay::start() - loading and aggregating all data" << std::endl;
        
        while (!source_exhausted_) {
            if (!_load_aggregate()) {
                break;
            }
        }
        
        // Handle any remaining open bar
        if (bar_open_) {
            std::cerr << "DataReplay::start() - appending final bar: dt=" << current_bar_.datetime 
                      << ", close=" << current_bar_.close << std::endl;
            // Add data to lines using append
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(DateTime))) 
                line->append(current_bar_.datetime);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Open))) 
                line->append(current_bar_.open);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(High))) 
                line->append(current_bar_.high);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Low))) 
                line->append(current_bar_.low);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Close))) 
                line->append(current_bar_.close);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Volume))) 
                line->append(current_bar_.volume);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(OpenInterest))) 
                line->append(current_bar_.openinterest);
            
            bar_open_ = false;
        }
        
        if (lines) {
            std::cerr << "DataReplay::start() - lines exists, size=" << lines->size() << std::endl;
            if (lines->size() > 0 && lines->getline(0)) {
                std::cerr << "DataReplay::start() - loaded " << lines->getline(0)->size() << " bars" << std::endl;
            } else {
                std::cerr << "DataReplay::start() - ERROR: no line 0!" << std::endl;
            }
        } else {
            std::cerr << "DataReplay::start() - ERROR: lines is null!" << std::endl;
        }
        
        // After loading all data, reset indices to 0 so data is immediately accessible
        for (size_t i = 0; i < lines->size(); ++i) {
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(i))) {
                std::cerr << "DataReplay::start() - line[" << i << "] before reset: idx=" 
                          << line->get_idx() << ", buflen=" << line->buflen() << std::endl;
                line->set_idx(-1);  // Start before first data point
                std::cerr << "DataReplay::start() - line[" << i << "] after reset: idx=" 
                          << line->get_idx() << ", buflen=" << line->buflen() << std::endl;
            }
        }
    }
    
    return result;
}

void DataReplay::stop() {
    if (source_data_) source_data_->stop();
    AbstractDataBase::stop();
}

size_t DataReplay::size() const {
    // Return actual line size
    if (lines && lines->size() > 0) {
        auto line = lines->getline(0);
        if (line) {
            if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(line)) {
                size_t sz = linebuf->buflen();  // Use buflen() instead of size()
                std::cerr << "DataReplay::size() - returning buflen=" << sz << std::endl;
                return sz;
            }
            size_t sz = line->size();
            std::cerr << "DataReplay::size() - returning size=" << sz << std::endl;
            return sz;
        }
    }
    
    std::cerr << "DataReplay::size() - returning 0 (no lines)" << std::endl;
    return 0;
}

void DataReplay::forward(size_t size) {
    // Don't override forward behavior - just use the base class implementation
    LineSeries::forward(size);
}

bool DataReplay::_load() {
    // This is called during normal operation to get next bar
    // Since we pre-loaded all data in start(), just return false
    return false;
}

bool DataReplay::_load_aggregate() {
    // This is called during start() to aggregate all bars
    if (source_exhausted_) {
        return false;
    }
    
    // Try to complete a bar
    while (!source_exhausted_) {
        // Check if source has more data
        if (!source_data_->load()) {
            std::cerr << "DataReplay::_load_aggregate() - source data exhausted" << std::endl;
            source_exhausted_ = true;
            break;
        }
        
        // Update current bar with source data
        _updatebar();
        
        // Check if current bar is complete
        double current_dt = source_data_->datetime(0);
        if (bar_open_ && _checkbarover(current_dt)) {
            std::cerr << "DataReplay::_load_aggregate() - bar is complete!" << std::endl;
            // Bar is complete, deliver it
            
            // Add data to lines using append
            std::cerr << "DataReplay::_load_aggregate() - appending bar: dt=" << current_bar_.datetime 
                      << ", close=" << current_bar_.close << std::endl;
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(DateTime))) 
                line->append(current_bar_.datetime);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Open))) 
                line->append(current_bar_.open);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(High))) 
                line->append(current_bar_.high);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Low))) 
                line->append(current_bar_.low);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Close))) 
                line->append(current_bar_.close);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Volume))) 
                line->append(current_bar_.volume);
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(OpenInterest))) 
                line->append(current_bar_.openinterest);
            
            // Reset for next bar
            bar_open_ = false;
            current_bar_.bstart();
            
            return true;
        }
    }
    
    // Return false when no more complete bars
    return false;
}

void DataReplay::_updatebar() {
    if (!source_data_) return;
    
    double dt = source_data_->datetime(0);
    double open = source_data_->open(0);
    double high = source_data_->high(0);
    double low = source_data_->low(0);
    double close = source_data_->close(0);
    double volume = source_data_->volume(0);
    double openinterest = source_data_->openinterest(0);
    
    if (!bar_open_) {
        // Start new bar
        current_bar_.bstart();
        current_bar_.datetime = dt;
        current_bar_.open = open;
        // Don't overwrite the initial high/low from bstart()
        // Let the max/min logic handle it properly
        current_bar_.high = std::max(current_bar_.high, high);
        current_bar_.low = std::min(current_bar_.low, low);
        current_bar_.close = close;
        current_bar_.volume = volume;
        current_bar_.openinterest = openinterest;
        bar_open_ = true;
        last_dt_ = dt;
    } else {
        // Update existing bar
        current_bar_.high = std::max(current_bar_.high, high);
        current_bar_.low = std::min(current_bar_.low, low);
        current_bar_.close = close;
        current_bar_.volume += volume;
        current_bar_.openinterest = openinterest;
        current_bar_.datetime = dt;
    }
}

// DataResample implementation
DataResample::DataResample(std::shared_ptr<AbstractDataBase> data) 
    : AbstractDataBase(), source_data_(data), resample_timeframe_(TimeFrame::Days), resample_compression_(1) {
    if (source_data_) {
        // Copy relevant parameters from source
        params.dataname = source_data_->params.dataname;
        params.name = source_data_->params.name + "_resample";
    }
    
    // Initialize lines for OHLCV data - CRITICAL for proper functioning
    lines = std::make_shared<Lines>();
    lines->add_line(std::make_shared<LineBuffer>());  // DateTime
    lines->add_line(std::make_shared<LineBuffer>());  // Open
    lines->add_line(std::make_shared<LineBuffer>());  // High
    lines->add_line(std::make_shared<LineBuffer>());  // Low
    lines->add_line(std::make_shared<LineBuffer>());  // Close
    lines->add_line(std::make_shared<LineBuffer>());  // Volume
    lines->add_line(std::make_shared<LineBuffer>());  // OpenInterest
    
    // Set up line aliases
    lines->add_alias("datetime", DateTime);
    lines->add_alias("open", Open);
    lines->add_alias("high", High);
    lines->add_alias("low", Low);
    lines->add_alias("close", Close);
    lines->add_alias("volume", Volume);
    lines->add_alias("openinterest", OpenInterest);
}

DataResample::DataResample(std::shared_ptr<DataSeries> data) 
    : AbstractDataBase(), source_data_(std::static_pointer_cast<AbstractDataBase>(data)), 
      resample_timeframe_(TimeFrame::Days), resample_compression_(1) {
    // Cast DataSeries to AbstractDataBase for compatibility
    if (source_data_) {
        params.dataname = source_data_->params.dataname;
        params.name = source_data_->params.name + "_resample";
    } else {
        params.name = "data_resample";
    }
    
    // Initialize lines for OHLCV data - CRITICAL for proper functioning
    lines = std::make_shared<Lines>();
    lines->add_line(std::make_shared<LineBuffer>());  // DateTime
    lines->add_line(std::make_shared<LineBuffer>());  // Open
    lines->add_line(std::make_shared<LineBuffer>());  // High
    lines->add_line(std::make_shared<LineBuffer>());  // Low
    lines->add_line(std::make_shared<LineBuffer>());  // Close
    lines->add_line(std::make_shared<LineBuffer>());  // Volume
    lines->add_line(std::make_shared<LineBuffer>());  // OpenInterest
    
    // Set up line aliases
    lines->add_alias("datetime", DateTime);
    lines->add_alias("open", Open);
    lines->add_alias("high", High);
    lines->add_alias("low", Low);
    lines->add_alias("close", Close);
    lines->add_alias("volume", Volume);
    lines->add_alias("openinterest", OpenInterest);
}

void DataResample::resample(TimeFrame timeframe, int compression) {
    resample_timeframe_ = timeframe;
    resample_compression_ = compression;
    params.timeframe = timeframe;
    params.compression = compression;
}

bool DataResample::start() {
    if (!source_data_) {
        std::cerr << "DataResample::start() - no source data!" << std::endl;
        return false;
    }
    
    std::cerr << "DataResample::start() - called" << std::endl;
    
    bar_open_ = false;
    source_exhausted_ = false;
    last_dt_ = 0.0;
    bar_delivered_ = false;
    current_bar_.bstart();
    
    // Start source data first
    bool source_started = source_data_->start();
    std::cerr << "DataResample::start() - source data start result: " << source_started << std::endl;
    
    bool result = source_started && AbstractDataBase::start();
    
    std::cerr << "DataResample::start() - result = " << result << std::endl;
    
    if (result) {
        // Process all source data and generate resampled bars during start()
        std::cerr << "DataResample::start() - processing source data for resampling" << std::endl;
        
        size_t bar_count = 0;
        while (!source_exhausted_) {
            if (!_load_aggregate()) {
                break;
            }
            bar_count++;
        }
        
        // Handle any remaining open bar
        if (bar_open_) {
            std::cerr << "DataResample::start() - appending final bar: dt=" << current_bar_.datetime 
                      << ", close=" << current_bar_.close << std::endl;
            _append_bar();
            bar_open_ = false;
            bar_count++;
        }
        
        std::cerr << "DataResample::start() - generated " << bar_count << " resampled bars" << std::endl;
        
        if (lines) {
            std::cerr << "DataResample::start() - lines exists, size=" << lines->size() << std::endl;
            if (lines->size() > 0 && lines->getline(0)) {
                auto sz = lines->getline(0)->size();
                std::cerr << "DataResample::start() - final size: " << sz << std::endl;
            } else {
                std::cerr << "DataResample::start() - ERROR: no line 0!" << std::endl;
            }
        } else {
            std::cerr << "DataResample::start() - ERROR: lines is null!" << std::endl;
        }
        
        // Reset indices to -1 so first forward() call will advance to index 0
        for (size_t i = 0; i < lines->size(); ++i) {
            if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(i))) {
                std::cerr << "DataResample::start() - line[" << i << "] before reset: idx=" 
                          << line->get_idx() << ", buflen=" << line->buflen() << std::endl;
                line->set_idx(-1);  // Start before first data point
                std::cerr << "DataResample::start() - line[" << i << "] after reset: idx=" 
                          << line->get_idx() << ", buflen=" << line->buflen() << std::endl;
            }
        }
        
        preloaded_ = true;
        std::cerr << "DataResample::start() - preloaded_ set to true" << std::endl;
    }
    
    return result;
}

void DataResample::stop() {
    if (source_data_) source_data_->stop();
    AbstractDataBase::stop();
}

bool DataResample::_load() {
    // For preloaded data, check if we have more bars to process
    if (!preloaded_ || !lines || lines->size() == 0) {
        return false;
    }
    
    // Check if we have more data to provide
    auto line = lines->getline(0);
    if (!line) {
        return false;
    }
    
    if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(line)) {
        int current_idx = linebuf->get_idx();
        size_t total_bars = linebuf->buflen();
        
        // If we haven't reached the last bar yet, there's more data
        bool has_more = (current_idx + 1) < static_cast<int>(total_bars);
        
        std::cerr << "DataResample::_load() - current_idx=" << current_idx 
                  << ", total_bars=" << total_bars << ", has_more=" << has_more << std::endl;
        
        return has_more;
    }
    
    return false;
}

size_t DataResample::size() const {
    // For DataResample, the size() method should return the buffer length in runonce mode
    // In runonce mode, strategies need to know the full buffer size for batch processing
    if (lines && lines->size() > 0) {
        auto line = lines->getline(0);
        if (line) {
            if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(line)) {
                int idx = linebuf->get_idx();
                size_t buflen_val = linebuf->buflen();
                
                // For preloaded data (runonce mode), always return full buffer size
                if (preloaded_ && buflen_val > 0) {
                    std::cerr << "DataResample::size() - preloaded runonce mode, idx=" << idx << ", returning buflen=" << buflen_val << std::endl;
                    return buflen_val;
                }
                
                // For non-runonce mode, return current position + 1
                if (idx >= 0 && static_cast<size_t>(idx) < buflen_val) {
                    size_t current_size = idx + 1;
                    std::cerr << "DataResample::size() - non-runonce mode, returning current_size=" << current_size 
                              << " (idx=" << idx << ", buflen=" << buflen_val << ")" << std::endl;
                    return current_size;
                }
                
                // Fallback for edge cases
                if (buflen_val > 0) {
                    std::cerr << "DataResample::size() - fallback, returning buflen=" << buflen_val << std::endl;
                    return buflen_val;
                }
            }
        }
    }
    
    return 0;
}

size_t DataResample::buflen() const {
    // For buflen(), always return the total buffer length
    if (lines && lines->size() > 0) {
        auto line = lines->getline(0);
        if (line) {
            if (auto linebuf = std::dynamic_pointer_cast<LineBuffer>(line)) {
                size_t buflen_val = linebuf->buflen();
                std::cerr << "DataResample::buflen() - returning " << buflen_val << std::endl;
                return buflen_val;
            }
        }
    }
    
    return 0;
}

void DataResample::forward(size_t size) {
    // Use the base class implementation
    LineSeries::forward(size);
}

bool DataResample::_load_aggregate() {
    // Try to complete a bar
    while (!source_exhausted_) {
        // Check if source has more data
        if (!source_data_->load()) {
            source_exhausted_ = true;
            
            // Handle any remaining open bar as the final bar
            if (bar_open_) {
                std::cerr << "DataResample::_load_aggregate() - final bar: dt=" << current_bar_.datetime 
                          << ", close=" << current_bar_.close << std::endl;
                return true;
            }
            break;
        }
        
        // Update current bar with source data
        _updatebar();
        
        // Check if current bar is complete
        double current_dt = source_data_->datetime(0);
        if (bar_open_ && _checkbarover(current_dt)) {
            // Bar is complete, append it and prepare for next
            std::cerr << "DataResample::_load_aggregate() - completed bar: dt=" << current_bar_.datetime 
                      << ", close=" << current_bar_.close << std::endl;
            
            _append_bar();
            
            // Reset for next bar
            bar_open_ = false;
            current_bar_.bstart();
            return true;
        }
    }
    
    // Return false when no more complete bars
    return false;
}

void DataResample::_append_bar() {
    // Add current bar data to lines using append
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(DateTime))) 
        line->append(current_bar_.datetime);
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Open))) 
        line->append(current_bar_.open);
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(High))) 
        line->append(current_bar_.high);
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Low))) 
        line->append(current_bar_.low);
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Close))) 
        line->append(current_bar_.close);
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Volume))) 
        line->append(current_bar_.volume);
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(OpenInterest))) 
        line->append(current_bar_.openinterest);
}

void DataResample::_updatebar_internal() {
    // Set the current bar data in the lines for access via datetime(0), close(0), etc.
    if (!lines) return;
    
    // Ensure we have at least the basic OHLC lines
    while (lines->size() <= static_cast<size_t>(Close)) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    // Set the current bar data
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(DateTime))) {
        if (line->buflen() == 0) line->append(current_bar_.datetime);
        else line->set(0, current_bar_.datetime);
    }
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Open))) {
        if (line->buflen() == 0) line->append(current_bar_.open);
        else line->set(0, current_bar_.open);
    }
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(High))) {
        if (line->buflen() == 0) line->append(current_bar_.high);
        else line->set(0, current_bar_.high);
    }
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Low))) {
        if (line->buflen() == 0) line->append(current_bar_.low);
        else line->set(0, current_bar_.low);
    }
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Close))) {
        if (line->buflen() == 0) line->append(current_bar_.close);
        else line->set(0, current_bar_.close);
    }
    
    // Add volume and open interest if needed
    if (lines->size() <= static_cast<size_t>(Volume)) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(Volume))) {
        if (line->buflen() == 0) line->append(current_bar_.volume);
        else line->set(0, current_bar_.volume);
    }
    
    if (lines->size() <= static_cast<size_t>(OpenInterest)) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    if (auto line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(OpenInterest))) {
        if (line->buflen() == 0) line->append(current_bar_.openinterest);
        else line->set(0, current_bar_.openinterest);
    }
    
    std::cerr << "DataResample::_updatebar_internal() - set current bar: dt=" << current_bar_.datetime 
              << ", close=" << current_bar_.close << std::endl;
}

void DataResample::_updatebar() {
    if (!source_data_) return;
    
    double dt = source_data_->datetime(0);
    double open = source_data_->open(0);
    double high = source_data_->high(0);
    double low = source_data_->low(0);
    double close = source_data_->close(0);
    double volume = source_data_->volume(0);
    double openinterest = source_data_->openinterest(0);
    
    if (!bar_open_) {
        // Start new bar
        current_bar_.bstart();
        current_bar_.datetime = dt;
        current_bar_.open = open;
        // Don't overwrite the initial high/low from bstart()
        // Let the max/min logic handle it properly
        current_bar_.high = std::max(current_bar_.high, high);
        current_bar_.low = std::min(current_bar_.low, low);
        current_bar_.close = close;
        current_bar_.volume = volume;
        current_bar_.openinterest = openinterest;
        bar_open_ = true;
        last_dt_ = dt;
    } else {
        // Update existing bar
        current_bar_.high = std::max(current_bar_.high, high);
        current_bar_.low = std::min(current_bar_.low, low);
        current_bar_.close = close;
        current_bar_.volume += volume;
        current_bar_.openinterest = openinterest;
        current_bar_.datetime = dt;
    }
}

bool DataResample::_checkbarover(double dt) {
    if (resample_timeframe_ == TimeFrame::Weeks) {
        // Check if we've moved to a new week
        // Assuming dt values are sequential daily indices (0, 1, 2, ...)
        
        // On first bar, don't check for week change
        if (last_dt_ == 0.0) {
            return false;
        }
        
        // Simple week detection based on integer division
        // Assuming 5 trading days per week
        int current_week = static_cast<int>(dt) / 5;
        int last_week = static_cast<int>(last_dt_) / 5;
        
        // Apply compression if specified
        if (resample_compression_ > 1) {
            current_week = current_week / resample_compression_;
            last_week = last_week / resample_compression_;
        }
        
        // Return true if we've moved to a new week
        return current_week > last_week;
    } else if (resample_timeframe_ == TimeFrame::Months) {
        // Check if we've moved to a new month
        // Assuming dt values are sequential daily indices
        // Approximately 21 trading days per month
        int current_month = static_cast<int>(dt) / 21;
        int last_month = static_cast<int>(last_dt_) / 21;
        
        // Apply compression if specified
        if (resample_compression_ > 1) {
            current_month = current_month / resample_compression_;
            last_month = last_month / resample_compression_;
        }
        
        return current_month > last_month;
    } else if (resample_timeframe_ == TimeFrame::Days) {
        // For daily resampling, each source bar becomes one resampled bar
        // Check if compression is applied
        if (resample_compression_ <= 1) {
            // 1:1 mapping - complete bar on every source bar
            return true;
        } else {
            // Multiple days per bar based on compression
            int current_day_group = static_cast<int>(dt) / resample_compression_;
            int last_day_group = static_cast<int>(last_dt_) / resample_compression_;
            return current_day_group > last_day_group;
        }
    } else if (resample_timeframe_ == TimeFrame::Years) {
        // Check if we've moved to a new year
        // Assuming dt values are sequential daily indices
        // Approximately 252 trading days per year
        int current_year = static_cast<int>(dt) / 252;
        int last_year = static_cast<int>(last_dt_) / 252;
        
        return current_year > last_year;
    }
    
    // For other timeframes, return false for now
    return false;
}

void DataReplay::_deliverbar() {
    // This method is called when a weekly bar is complete
    // But we don't actually deliver it here in replay mode
    // The bar data is already being built in current_bar_
    // We just mark that a bar boundary was crossed
    std::cerr << "DataReplay::_deliverbar() - weekly bar complete" << std::endl;
}

bool DataReplay::_checkbarover(double dt) {
    if (replay_timeframe_ == TimeFrame::Weeks) {
        // Check if we've moved to a new week
        // Assuming dt values are sequential daily indices (0, 1, 2, ...)
        
        // On first bar, don't check for week change
        if (last_dt_ == 0.0) {
            return false;
        }
        
        // Simple week detection based on integer division
        // Assuming 5 trading days per week
        int current_week = static_cast<int>(dt) / 5;
        int last_week = static_cast<int>(last_dt_) / 5;
        
        // Apply compression if specified
        if (replay_compression_ > 1) {
            current_week = current_week / replay_compression_;
            last_week = last_week / replay_compression_;
        }
        
        // Return true if we've moved to a new week
        return current_week > last_week;
    } else if (replay_timeframe_ == TimeFrame::Months) {
        // Check if we've moved to a new month
        // Assuming dt values are sequential daily indices
        // Approximately 21 trading days per month
        int current_month = static_cast<int>(dt) / 21;
        int last_month = static_cast<int>(last_dt_) / 21;
        
        // Apply compression if specified
        if (replay_compression_ > 1) {
            current_month = current_month / replay_compression_;
            last_month = last_month / replay_compression_;
        }
        
        return current_month > last_month;
    } else if (replay_timeframe_ == TimeFrame::Years) {
        // Check if we've moved to a new year
        // Assuming dt values are sequential daily indices
        // Approximately 252 trading days per year
        int current_year = static_cast<int>(dt) / 252;
        int last_year = static_cast<int>(last_dt_) / 252;
        
        return current_year > last_year;
    }
    
    // For other timeframes, return false for now
    return false;
}


} // namespace backtrader