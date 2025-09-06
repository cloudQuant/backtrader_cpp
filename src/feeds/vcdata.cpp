#include "../../include/feeds/vcdata.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace backtrader {
namespace feeds {

// Static member definitions
const std::map<std::string, std::vector<std::string>> VCData::MARKET_TIMEZONES = {
    {"Europe/London", {"011", "024", "027", "036", "049", "092", "114", "033", "034", "035", "043", "054", "096", "300"}},
    {"Europe/Berlin", {"005", "006", "008", "012", "013", "014", "015", "017", "019", "025", "029", "030", "037", "038", "052", "053", "060", "061", "072", "073", "074", "075", "080", "093", "094", "097", "111", "112", "113"}},
    {"Asia/Tokyo", {"031"}},
    {"Australia/Melbourne", {"032"}},
    {"America/Argentina/Buenos_Aires", {"044"}},
    {"America/Sao_Paulo", {"045"}},
    {"America/Mexico_City", {"046"}},
    {"America/Santiago", {"047"}},
    {"US/Eastern", {"003", "004", "009", "010", "028", "040", "041", "055", "090", "095", "099"}},
    {"US/Central", {"001", "002", "020", "021", "022", "023", "056"}}
};

const std::map<std::string, std::string> VCData::TIMEZONE_OUTPUTS = {
    {"096.FTSE", "Europe/London"},
    {"096.FTEU3", "Europe/London"},
    {"096.MIB30", "Europe/Berlin"},
    {"096.SSMI", "Europe/Berlin"},
    {"096.HSI", "Asia/Hong_Kong"},
    {"096.BVSP", "America/Sao_Paulo"},
    {"096.MERVAL", "America/Argentina/Buenos_Aires"},
    {"096.DJI", "US/Eastern"},
    {"096.IXIC", "US/Eastern"},
    {"096.NDX", "US/Eastern"}
};

const std::vector<std::string> VCData::EXTRA_TIMEOFFSET_MARKETS = {"096"};

const std::chrono::system_clock::time_point VCData::NULL_DATE = 
    std::chrono::system_clock::from_time_t(std::mktime(&(std::tm{0, 0, 0, 30, 11, -1, 0, 0, 0})));

// VCData implementation

VCData::VCData(const VCParams& params) : params_(params) {
    current_status_ = DataStatus::DISCONNECTED;
    last_status_ = DataStatus::DISCONNECTED;
    feeding_started_ = false;
    bar_index_ = 1;
}

VCData::VCData(const std::string& symbol, const VCParams& params) 
    : params_(params), dataname_(symbol) {
    
    current_status_ = DataStatus::DISCONNECTED;
    last_status_ = DataStatus::DISCONNECTED;
    feeding_started_ = false;
    bar_index_ = 1;
    
    set_symbol(symbol);
}

void VCData::start() {
    AbstractDataBase::start();
    
    statistics_.session_start = std::chrono::system_clock::now();
    
    try {
        // Initialize store connection
        if (!store_) {
            store_ = std::make_shared<stores::VCStore>();
        }
        
        // Initialize symbol information
        initialize_symbol_info();
        
        // Setup timezone and time offsets
        setup_market_timezone();
        calculate_time_offsets();
        
        // Connect to VisualChart
        if (!store_->connect()) {
            update_status(DataStatus::ERROR);
            throw std::runtime_error("Failed to connect to VisualChart server");
        }
        
        update_status(DataStatus::CONNECTED);
        
        // Request symbol information
        if (!store_->subscribe_symbol(dataname_)) {
            update_status(DataStatus::NOTFOUND);
            throw std::runtime_error("Symbol not found: " + dataname_);
        }
        
        // Request historical data if needed
        if (params_.backfill && !params_.historical) {
            request_historical_data();
        }
        
        // Start data feed
        feeding_started_ = true;
        update_status(DataStatus::DELAYED);
        
        std::cout << "VCData started for symbol: " << dataname_ << std::endl;
        
    } catch (const std::exception& e) {
        log_error(-1, "Error starting VCData: " + std::string(e.what()), "start()");
        update_status(DataStatus::ERROR);
        throw;
    }
}

void VCData::stop() {
    AbstractDataBase::stop();
    
    should_stop_ = true;
    feeding_started_ = false;
    
    if (store_) {
        store_->unsubscribe_symbol(dataname_);
        store_->disconnect();
    }
    
    update_status(DataStatus::DISCONNECTED);
    
    auto end_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - statistics_.session_start);
    
    std::cout << "VCData stopped. Session duration: " << duration.count() << " seconds" << std::endl;
    std::cout << "Bars received: " << statistics_.bars_received << std::endl;
    std::cout << "Errors: " << statistics_.errors_count << std::endl;
}

bool VCData::next() {
    if (should_stop_ || !feeding_started_) {
        return false;
    }
    
    return process_market_data();
}

void VCData::preload() {
    // VisualChart data is live/streaming, so preloading is not applicable
    if (params_.historical) {
        request_historical_data();
    }
}

bool VCData::has_live_data() const {
    return current_status_ == DataStatus::LIVE && !data_queue_.empty();
}

void VCData::set_symbol(const std::string& symbol) {
    dataname_ = normalize_symbol_name(symbol);
    market_code_ = extract_market_code(dataname_);
    
    // Set tradename if not explicitly provided
    if (params_.tradename.empty()) {
        tradename_ = dataname_;
    } else {
        tradename_ = params_.tradename;
    }
}

std::chrono::system_clock::time_point VCData::get_time_offset() const {
    return time_offset_;
}

std::string VCData::get_market_timezone() const {
    for (const auto& tz_pair : MARKET_TIMEZONES) {
        const auto& market_codes = tz_pair.second;
        if (std::find(market_codes.begin(), market_codes.end(), market_code_) != market_codes.end()) {
            return tz_pair.first;
        }
    }
    return "UTC";
}

std::string VCData::get_status_description() const {
    switch (current_status_) {
        case DataStatus::DISCONNECTED: return "Disconnected";
        case DataStatus::CONNECTING: return "Connecting";
        case DataStatus::CONNECTED: return "Connected";
        case DataStatus::DELAYED: return "Delayed";
        case DataStatus::LIVE: return "Live";
        case DataStatus::HISTORICAL: return "Historical";
        case DataStatus::ERROR: return "Error";
        case DataStatus::NOTFOUND: return "Symbol Not Found";
        default: return "Unknown";
    }
}

bool VCData::is_connected() const {
    return store_ && store_->is_connected() && current_status_ != DataStatus::DISCONNECTED;
}

VCData::SessionInfo VCData::get_session_info() const {
    SessionInfo info;
    
    // Simplified session info - in practice, this would be more comprehensive
    info.session_start = std::chrono::system_clock::now();
    info.session_end = info.session_start + std::chrono::hours(8);
    info.is_trading_session = is_market_open();
    info.session_name = "Regular Trading";
    
    return info;
}

bool VCData::is_in_trading_session() const {
    return is_market_open();
}

void VCData::set_timezone(const std::string& tz) {
    params_.timezone = tz;
    setup_market_timezone();
}

std::chrono::system_clock::time_point VCData::convert_market_time(
    const std::chrono::system_clock::time_point& vc_time) const {
    
    // Apply market-specific time offsets
    auto adjusted_time = vc_time - market_offset_;
    
    // Apply extra offset for special markets
    if (std::find(EXTRA_TIMEOFFSET_MARKETS.begin(), EXTRA_TIMEOFFSET_MARKETS.end(), market_code_) != 
        EXTRA_TIMEOFFSET_MARKETS.end()) {
        adjusted_time -= std::chrono::hours(1);
    }
    
    // Add millisecond correction if enabled
    if (params_.millisecond) {
        adjusted_time += std::chrono::milliseconds(1);
    }
    
    return adjusted_time;
}

void VCData::initialize_symbol_info() {
    // Normalize symbol name and extract components
    dataname_ = normalize_symbol_name(dataname_);
    market_code_ = extract_market_code(dataname_);
    
    // Validate symbol format
    if (market_code_.empty() || market_code_.length() != 3) {
        throw std::invalid_argument("Invalid VisualChart symbol format: " + dataname_);
    }
    
    std::cout << "Initialized symbol: " << dataname_ << " (market: " << market_code_ << ")" << std::endl;
}

void VCData::calculate_time_offsets() {
    // Initialize time offsets (simplified implementation)
    market_offset_ = std::chrono::milliseconds(0);
    market_offset1_ = market_offset_;
    
    // Apply special offset for certain markets
    if (std::find(EXTRA_TIMEOFFSET_MARKETS.begin(), EXTRA_TIMEOFFSET_MARKETS.end(), market_code_) != 
        EXTRA_TIMEOFFSET_MARKETS.end()) {
        market_offset_ -= std::chrono::hours(1);
    }
    
    offset_diff_ = market_offset_ - market_offset1_;
}

void VCData::setup_market_timezone() {
    // Set timezone based on market code
    std::string market_tz = get_market_timezone();
    
    // Check for specific timezone outputs
    auto it = TIMEZONE_OUTPUTS.find(dataname_);
    if (it != TIMEZONE_OUTPUTS.end()) {
        market_tz = it->second;
    }
    
    if (params_.usetimezones && market_tz != "UTC") {
        std::cout << "Using timezone: " << market_tz << " for market: " << market_code_ << std::endl;
    }
}

bool VCData::process_market_data() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (data_queue_.empty()) {
        return false;
    }
    
    auto market_data = data_queue_.front();
    data_queue_.pop();
    
    // Convert VisualChart time to market time
    auto market_time = convert_market_time(market_data->timestamp);
    
    // Validate data
    if (!validate_bar_data(*market_data)) {
        statistics_.errors_count++;
        return next(); // Try next data point
    }
    
    // Set the data in our lines
    set_datetime(market_time);
    set_open(market_data->open);
    set_high(market_data->high);
    set_low(market_data->low);
    set_close(market_data->close);
    set_volume(market_data->volume);
    set_openinterest(market_data->openinterest);
    
    // Update statistics
    statistics_.bars_received++;
    statistics_.last_update = std::chrono::system_clock::now();
    
    // Update data quality metrics
    update_data_quality_metrics(*market_data);
    
    return true;
}

void VCData::handle_bar_data(const std::shared_ptr<MarketData>& bar) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    data_queue_.push(bar);
    
    // Update status based on data timeliness
    auto now = std::chrono::system_clock::now();
    auto data_age = std::chrono::duration_cast<std::chrono::seconds>(now - bar->timestamp);
    
    if (data_age.count() < 60) { // Data less than 1 minute old
        update_status(DataStatus::LIVE);
    } else {
        update_status(DataStatus::DELAYED);
    }
}

void VCData::handle_tick_data(const std::shared_ptr<MarketData>& tick) {
    statistics_.ticks_received++;
    
    // For tick data, we might aggregate to bars or pass through directly
    handle_bar_data(tick);
}

void VCData::update_status(DataStatus new_status) {
    if (current_status_ != new_status) {
        last_status_ = current_status_;
        current_status_ = new_status;
        
        std::cout << "VCData status changed: " << get_status_description() << std::endl;
    }
}

void VCData::handle_connection_event(bool connected) {
    if (connected) {
        update_status(DataStatus::CONNECTED);
    } else {
        update_status(DataStatus::DISCONNECTED);
    }
}

void VCData::handle_data_quality_event(const std::string& quality) {
    statistics_.data_quality = quality;
}

std::chrono::system_clock::time_point VCData::parse_vc_datetime(double vc_date) const {
    // Convert VisualChart date (days since NULL_DATE) to system time
    auto days = static_cast<int64_t>(vc_date);
    return NULL_DATE + std::chrono::hours(24 * days);
}

double VCData::convert_to_vc_date(const std::chrono::system_clock::time_point& dt) const {
    auto duration = dt - NULL_DATE;
    auto days = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    return static_cast<double>(days);
}

bool VCData::validate_market_time(const std::chrono::system_clock::time_point& dt) const {
    // Basic validation - ensure time is reasonable
    auto now = std::chrono::system_clock::now();
    auto future_limit = now + std::chrono::hours(24);
    auto past_limit = now - std::chrono::hours(24 * 365); // 1 year ago
    
    return dt >= past_limit && dt <= future_limit;
}

void VCData::log_error(int code, const std::string& message, const std::string& context) {
    ErrorInfo error;
    error.error_code = code;
    error.error_message = message;
    error.timestamp = std::chrono::system_clock::now();
    error.context = context;
    
    recent_errors_.push_back(error);
    
    // Keep only recent errors (last 100)
    if (recent_errors_.size() > 100) {
        recent_errors_.erase(recent_errors_.begin());
    }
    
    statistics_.errors_count++;
    
    std::cerr << "VCData Error [" << code << "]: " << message;
    if (!context.empty()) {
        std::cerr << " (Context: " << context << ")";
    }
    std::cerr << std::endl;
}

void VCData::cleanup_old_errors() {
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24);
    
    recent_errors_.erase(
        std::remove_if(recent_errors_.begin(), recent_errors_.end(),
            [cutoff](const ErrorInfo& error) {
                return error.timestamp < cutoff;
            }),
        recent_errors_.end()
    );
}

bool VCData::is_market_open() const {
    // Simplified market hours check
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* tm = std::localtime(&time_t);
    
    if (!tm) return false;
    
    // Basic market hours (9:30 AM to 4:00 PM EST)
    int hour = tm->tm_hour;
    int minute = tm->tm_min;
    int time_minutes = hour * 60 + minute;
    
    return time_minutes >= (9 * 60 + 30) && time_minutes <= (16 * 60);
}

std::chrono::system_clock::time_point VCData::get_next_session_start() const {
    // Simplified - return next 9:30 AM
    auto now = std::chrono::system_clock::now();
    return now + std::chrono::hours(24); // Placeholder
}

std::chrono::system_clock::time_point VCData::get_session_end() const {
    // Simplified - return 4:00 PM today
    auto now = std::chrono::system_clock::now();
    return now + std::chrono::hours(8); // Placeholder
}

bool VCData::validate_bar_data(const MarketData& data) const {
    // Basic OHLC validation
    if (data.high < data.low) return false;
    if (data.high < data.open || data.high < data.close) return false;
    if (data.low > data.open || data.low > data.close) return false;
    
    // Volume validation
    if (data.volume < 0) return false;
    
    // Price validation
    if (data.open <= 0 || data.high <= 0 || data.low <= 0 || data.close <= 0) return false;
    
    // Outlier detection
    if (params_.filter_outliers && is_data_outlier(data.close)) return false;
    
    return true;
}

bool VCData::is_data_outlier(double price) const {
    if (statistics_.bars_received == 0) return false;
    
    // Simple outlier detection based on deviation from last close
    double deviation = std::abs(price - statistics_.last_update.time_since_epoch().count()) / price;
    return deviation > params_.outlier_threshold;
}

void VCData::update_data_quality_metrics(const MarketData& data) {
    // Update latency calculation
    auto now = std::chrono::system_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(now - data.timestamp);
    
    // Update running average
    if (statistics_.bars_received > 0) {
        statistics_.average_latency_ms = (statistics_.average_latency_ms * (statistics_.bars_received - 1) + 
                                        latency.count()) / statistics_.bars_received;
    } else {
        statistics_.average_latency_ms = latency.count();
    }
    
    // Update data quality assessment
    if (statistics_.average_latency_ms < 100) {
        statistics_.data_quality = "Excellent";
    } else if (statistics_.average_latency_ms < 500) {
        statistics_.data_quality = "Good";
    } else if (statistics_.average_latency_ms < 1000) {
        statistics_.data_quality = "Fair";
    } else {
        statistics_.data_quality = "Poor";
    }
}

void VCData::request_historical_data() {
    if (!store_) return;
    
    // Request historical data based on parameters
    auto end_date = params_.todate;
    if (end_date == std::chrono::system_clock::time_point{}) {
        end_date = std::chrono::system_clock::now();
    }
    
    auto start_date = params_.fromdate;
    if (start_date == std::chrono::system_clock::time_point{}) {
        // Default to 30 days back
        start_date = end_date - std::chrono::hours(24 * 30);
    }
    
    store_->request_historical_data(dataname_, start_date, end_date, params_.max_bars);
    update_status(DataStatus::HISTORICAL);
}

void VCData::process_historical_response() {
    // Process historical data response from store
    // This would be called by the store when historical data is received
}

std::string VCData::normalize_symbol_name(const std::string& symbol) const {
    std::string normalized = symbol;
    
    // Remove spaces from copy-paste errors
    if (normalized.length() > 3 && std::isspace(normalized[3])) {
        normalized = normalized.substr(0, 3) + normalized.substr(4);
    }
    
    // Ensure it starts with "010" if not present
    if (normalized.length() >= 3 && normalized.substr(0, 3) != "010") {
        normalized = "010" + normalized;
    }
    
    return normalized;
}

bool VCData::is_continuous_future(const std::string& symbol) const {
    // Simplified check for continuous futures
    return symbol.find("001") != std::string::npos;
}

std::string VCData::extract_market_code(const std::string& symbol) const {
    if (symbol.length() >= 3) {
        return symbol.substr(0, 3);
    }
    return "";
}

// VCLiveData implementation

VCLiveData::VCLiveData(const LiveParams& params) : VCData(params), live_params_(params) {
    tick_count_ = 0;
    last_tick_time_ = std::chrono::system_clock::now();
}

VCLiveData::VCLiveData(const std::string& symbol, const LiveParams& params) 
    : VCData(symbol, params), live_params_(params) {
    tick_count_ = 0;
    last_tick_time_ = std::chrono::system_clock::now();
}

void VCLiveData::enable_tick_data(bool enable) {
    live_params_.tick_data = enable;
    if (store_) {
        if (enable) {
            store_->subscribe_tick_data(dataname_);
        } else {
            store_->unsubscribe_tick_data(dataname_);
        }
    }
}

void VCLiveData::set_latency_monitoring(bool enable, double threshold_ms) {
    live_params_.prioritize_speed = enable;
    live_params_.latency_threshold_ms = threshold_ms;
}

double VCLiveData::get_current_latency_ms() const {
    return statistics_.average_latency_ms;
}

size_t VCLiveData::get_tick_rate_per_second() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - statistics_.session_start);
    
    if (duration.count() > 0) {
        return tick_count_ / duration.count();
    }
    return 0;
}

void VCLiveData::handle_live_tick(const std::shared_ptr<MarketData>& tick) {
    tick_count_++;
    last_tick_time_ = std::chrono::system_clock::now();
    
    monitor_latency(tick);
    handle_tick_data(tick);
}

void VCLiveData::monitor_latency(const std::shared_ptr<MarketData>& data) {
    auto now = std::chrono::system_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(now - data->timestamp);
    
    if (latency.count() > live_params_.latency_threshold_ms) {
        log_error(-1, "High latency detected: " + std::to_string(latency.count()) + "ms", "monitor_latency");
    }
}

void VCLiveData::attempt_reconnection() {
    if (live_params_.auto_reconnect && !is_connected()) {
        // Implement reconnection logic
        std::cout << "Attempting to reconnect..." << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(live_params_.reconnect_delay_ms));
        
        try {
            start();
        } catch (const std::exception& e) {
            log_error(-1, "Reconnection failed: " + std::string(e.what()), "attempt_reconnection");
        }
    }
}

// VCHistoricalData implementation

VCHistoricalData::VCHistoricalData(const HistoricalParams& params) 
    : VCData(params), historical_params_(params) {
    cache_loaded_ = false;
}

VCHistoricalData::VCHistoricalData(const std::string& symbol,
                                  const std::chrono::system_clock::time_point& start_date,
                                  const std::chrono::system_clock::time_point& end_date,
                                  const HistoricalParams& params)
    : VCData(symbol, params), historical_params_(params) {
    
    params_.fromdate = start_date;
    params_.todate = end_date;
    params_.historical = true;
    cache_loaded_ = false;
}

void VCHistoricalData::set_date_range(const std::chrono::system_clock::time_point& start,
                                     const std::chrono::system_clock::time_point& end) {
    params_.fromdate = start;
    params_.todate = end;
}

void VCHistoricalData::enable_data_adjustments(bool splits, bool dividends) {
    historical_params_.adjust_for_splits = splits;
    historical_params_.adjust_for_dividends = dividends;
}

VCHistoricalData::ContinuityReport VCHistoricalData::analyze_continuity() const {
    ContinuityReport report;
    
    if (cached_data_.size() < 2) {
        report.is_continuous = true;
        report.gap_count = 0;
        report.completeness_percentage = 100.0;
        return report;
    }
    
    // Analyze gaps in the data
    report.gap_count = 0;
    report.largest_gap = std::chrono::seconds(0);
    
    for (size_t i = 1; i < cached_data_.size(); ++i) {
        auto gap = cached_data_[i]->timestamp - cached_data_[i-1]->timestamp;
        auto gap_seconds = std::chrono::duration_cast<std::chrono::seconds>(gap);
        
        if (gap_seconds > std::chrono::hours(24)) { // Gap larger than 1 day
            report.gaps.push_back({cached_data_[i-1]->timestamp, gap_seconds});
            report.gap_count++;
            
            if (gap_seconds > report.largest_gap) {
                report.largest_gap = gap_seconds;
            }
        }
    }
    
    report.is_continuous = report.gap_count == 0;
    
    // Calculate completeness percentage
    if (!cached_data_.empty()) {
        auto total_span = cached_data_.back()->timestamp - cached_data_.front()->timestamp;
        auto gap_time = std::accumulate(report.gaps.begin(), report.gaps.end(), 
                                       std::chrono::seconds(0),
                                       [](std::chrono::seconds sum, const auto& gap) {
                                           return sum + gap.second;
                                       });
        
        auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(total_span);
        if (total_seconds.count() > 0) {
            report.completeness_percentage = 100.0 * (1.0 - static_cast<double>(gap_time.count()) / total_seconds.count());
        }
    }
    
    return report;
}

bool VCHistoricalData::fill_data_gaps() {
    if (!historical_params_.fill_gaps) {
        return false;
    }
    
    // Implement gap filling based on the selected method
    auto continuity = analyze_continuity();
    
    for (const auto& gap : continuity.gaps) {
        // Find the gap and fill it
        // This is a simplified implementation
    }
    
    return true;
}

void VCHistoricalData::load_historical_data() {
    if (cache_loaded_ || !historical_params_.cache_data) {
        return;
    }
    
    request_historical_data();
    cache_loaded_ = true;
}

void VCHistoricalData::validate_data_sequence() {
    if (!historical_params_.validate_continuity) {
        return;
    }
    
    // Validate chronological order
    for (size_t i = 1; i < cached_data_.size(); ++i) {
        if (cached_data_[i]->timestamp <= cached_data_[i-1]->timestamp) {
            log_error(-1, "Data sequence violation at index " + std::to_string(i), "validate_data_sequence");
        }
    }
}

void VCHistoricalData::apply_adjustments() {
    if (!historical_params_.adjust_for_splits && !historical_params_.adjust_for_dividends) {
        return;
    }
    
    // Apply stock split and dividend adjustments
    // This would require corporate action data
}

std::shared_ptr<VCHistoricalData::MarketData> VCHistoricalData::interpolate_gap(
    const std::shared_ptr<MarketData>& before,
    const std::shared_ptr<MarketData>& after,
    const std::chrono::system_clock::time_point& target_time) const {
    
    auto gap_data = std::make_shared<MarketData>();
    gap_data->timestamp = target_time;
    
    // Simple linear interpolation
    auto total_duration = after->timestamp - before->timestamp;
    auto target_duration = target_time - before->timestamp;
    double ratio = static_cast<double>(target_duration.count()) / total_duration.count();
    
    gap_data->open = before->close; // Use previous close as open
    gap_data->close = before->close + (after->close - before->close) * ratio;
    gap_data->high = std::max(gap_data->open, gap_data->close);
    gap_data->low = std::min(gap_data->open, gap_data->close);
    gap_data->volume = 0; // No volume for interpolated data
    gap_data->openinterest = before->openinterest; // Keep previous OI
    
    return gap_data;
}

// Factory functions implementation

namespace vc_factory {

std::shared_ptr<VCData> create_vc_feed(
    const std::string& symbol,
    const VCData::VCParams& params) {
    
    return std::make_shared<VCData>(symbol, params);
}

std::shared_ptr<VCLiveData> create_live_vc_feed(
    const std::string& symbol,
    bool enable_ticks) {
    
    VCLiveData::LiveParams params;
    params.tick_data = enable_ticks;
    params.prioritize_speed = true;
    
    return std::make_shared<VCLiveData>(symbol, params);
}

std::shared_ptr<VCHistoricalData> create_historical_vc_feed(
    const std::string& symbol,
    const std::chrono::system_clock::time_point& start_date,
    const std::chrono::system_clock::time_point& end_date) {
    
    VCHistoricalData::HistoricalParams params;
    params.historical = true;
    params.cache_data = true;
    
    return std::make_shared<VCHistoricalData>(symbol, start_date, end_date, params);
}

std::vector<std::shared_ptr<VCData>> create_portfolio_feeds(
    const std::vector<std::string>& symbols,
    const VCData::VCParams& params) {
    
    std::vector<std::shared_ptr<VCData>> feeds;
    feeds.reserve(symbols.size());
    
    for (const auto& symbol : symbols) {
        feeds.push_back(std::make_shared<VCData>(symbol, params));
    }
    
    return feeds;
}

std::shared_ptr<VCData> create_auto_vc_feed(
    const std::string& partial_symbol,
    const VCData::VCParams& params) {
    
    // Auto-complete symbol if needed
    std::string full_symbol = partial_symbol;
    if (full_symbol.length() < 6) {
        full_symbol = "010" + partial_symbol;
    }
    
    return std::make_shared<VCData>(full_symbol, params);
}

} // namespace vc_factory

// Utility functions implementation

namespace vc_utils {

SymbolInfo parse_vc_symbol(const std::string& symbol) {
    SymbolInfo info;
    info.full_symbol = symbol;
    
    if (symbol.length() >= 3) {
        info.market_code = symbol.substr(0, 3);
    }
    
    if (symbol.length() > 3) {
        info.base_symbol = symbol.substr(3);
    }
    
    info.is_continuous = symbol.find("001") != std::string::npos;
    info.description = "VisualChart Symbol: " + symbol;
    info.currency = "USD"; // Default
    info.tick_size = 0.01; // Default
    info.point_value = 1.0; // Default
    
    return info;
}

std::string build_vc_symbol(const std::string& market_code, 
                           const std::string& base_symbol,
                           const std::string& contract) {
    
    std::string symbol = market_code + base_symbol;
    if (!contract.empty()) {
        symbol += contract;
    }
    return symbol;
}

MarketInfo get_market_info(const std::string& market_code) {
    MarketInfo info;
    info.market_code = market_code;
    info.market_name = "Market " + market_code;
    info.timezone = "UTC";
    info.supports_tick_data = true;
    info.available_timeframes = {TimeFrame::Minutes, TimeFrame::Days};
    
    return info;
}

std::vector<std::string> get_available_markets() {
    return {"001", "002", "003", "010", "096"};
}

bool is_market_open(const std::string& market_code) {
    // Simplified market hours check
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* tm = std::localtime(&time_t);
    
    if (!tm) return false;
    
    int hour = tm->tm_hour;
    return hour >= 9 && hour <= 16; // Simple 9 AM to 4 PM
}

QualityMetrics assess_data_quality(const VCData& feed) {
    QualityMetrics metrics;
    auto stats = feed.get_statistics();
    
    metrics.completeness_score = 0.95; // Placeholder
    metrics.timeliness_score = stats.average_latency_ms < 100 ? 1.0 : 0.8;
    metrics.accuracy_score = 0.98; // Placeholder
    metrics.outlier_count = 0;
    metrics.gap_count = 0;
    metrics.average_latency = std::chrono::milliseconds(static_cast<int>(stats.average_latency_ms));
    
    double overall = (metrics.completeness_score + metrics.timeliness_score + metrics.accuracy_score) / 3.0;
    
    if (overall >= 0.95) metrics.quality_grade = "A";
    else if (overall >= 0.85) metrics.quality_grade = "B";
    else if (overall >= 0.75) metrics.quality_grade = "C";
    else if (overall >= 0.65) metrics.quality_grade = "D";
    else metrics.quality_grade = "F";
    
    return metrics;
}

PerformanceMetrics measure_performance(const VCData& feed) {
    PerformanceMetrics metrics;
    auto stats = feed.get_statistics();
    
    auto session_duration = std::chrono::duration_cast<std::chrono::seconds>(
        stats.last_update - stats.session_start);
    
    if (session_duration.count() > 0) {
        metrics.throughput_bars_per_second = static_cast<double>(stats.bars_received) / session_duration.count();
        metrics.throughput_ticks_per_second = static_cast<double>(stats.ticks_received) / session_duration.count();
    }
    
    metrics.average_processing_time = std::chrono::milliseconds(10); // Placeholder
    metrics.memory_usage_mb = 50; // Placeholder
    metrics.cpu_utilization_percent = 5.0; // Placeholder
    
    if (metrics.throughput_bars_per_second > 1000) {
        metrics.performance_category = "Excellent";
    } else if (metrics.throughput_bars_per_second > 100) {
        metrics.performance_category = "Good";
    } else {
        metrics.performance_category = "Fair";
    }
    
    return metrics;
}

DiagnosticReport run_diagnostics(const VCData& feed) {
    DiagnosticReport report;
    
    report.connection_stable = feed.is_connected();
    report.ping_time = std::chrono::milliseconds(50); // Placeholder
    report.packet_loss_percent = 0.1; // Placeholder
    report.reconnection_count = 0; // Placeholder
    
    if (report.connection_stable) {
        report.issues.push_back("No issues detected");
        report.recommendations.push_back("Connection is stable");
    } else {
        report.issues.push_back("Connection unstable");
        report.recommendations.push_back("Check network connectivity");
    }
    
    return report;
}

VCData::VCParams create_optimal_config(const std::string& use_case, const std::string& market_code) {
    VCData::VCParams params;
    
    if (use_case == "live_trading") {
        params.qcheck = 0.1;
        params.historical = false;
        params.usetimezones = true;
        params.timeout_ms = 1000;
    } else if (use_case == "backtesting") {
        params.historical = true;
        params.backfill = true;
        params.validate_ohlc = true;
        params.max_bars = 100000;
    } else if (use_case == "research") {
        params.historical = true;
        params.validate_ohlc = true;
        params.filter_outliers = true;
        params.include_volume = true;
        params.include_openinterest = true;
    }
    
    return params;
}

std::map<std::string, std::string> get_recommended_settings(const std::string& symbol) {
    std::map<std::string, std::string> settings;
    
    auto symbol_info = parse_vc_symbol(symbol);
    
    settings["timeout_ms"] = "5000";
    settings["qcheck"] = "0.5";
    settings["historical"] = "false";
    settings["usetimezones"] = "true";
    
    return settings;
}

bool export_to_csv(const VCData& feed, const std::string& filename, const std::string& format) {
    // Placeholder implementation
    std::cout << "Exporting VCData to CSV: " << filename << std::endl;
    return true;
}

bool import_from_vc_file(const std::string& vc_filename, VCData& feed) {
    // Placeholder implementation
    std::cout << "Importing from VC file: " << vc_filename << std::endl;
    return true;
}

ErrorAnalysis analyze_errors(const VCData& feed) {
    ErrorAnalysis analysis;
    auto errors = feed.get_recent_errors();
    
    analysis.error_rate_percent = 0.1; // Placeholder
    
    for (const auto& error : errors) {
        analysis.error_frequency[error.error_code]++;
    }
    
    if (analysis.error_frequency.empty()) {
        analysis.common_issues.push_back("No errors detected");
        analysis.solutions.push_back("System operating normally");
    } else {
        analysis.common_issues.push_back("Connection errors");
        analysis.solutions.push_back("Check network connectivity");
    }
    
    return analysis;
}

} // namespace vc_utils

} // namespace feeds
} // namespace backtrader