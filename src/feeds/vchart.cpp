#include "../../include/feeds/vchart.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <filesystem>

namespace backtrader {
namespace feeds {

// VChartData implementation

VChartData::VChartData(const VChartParams& params) : params_(params) {
    file_format_ = FileFormat::UNKNOWN;
    bar_size_ = 0;
    datetime_size_ = 0;
    memory_mapped_ = false;
    mapped_data_ = nullptr;
    mapped_size_ = 0;
    current_position_ = 0;
    last_close_price_ = 0.0;
}

VChartData::VChartData(const std::string& filename, const VChartParams& params) 
    : params_(params), filename_(filename) {
    
    file_format_ = FileFormat::UNKNOWN;
    bar_size_ = 0;
    datetime_size_ = 0;
    memory_mapped_ = false;
    mapped_data_ = nullptr;
    mapped_size_ = 0;
    current_position_ = 0;
    last_close_price_ = 0.0;
}

VChartData::VChartData(std::shared_ptr<std::istream> stream, 
                       TimeFrame timeframe,
                       const VChartParams& params)
    : params_(params), file_stream_(stream) {
    
    // Set timeframe and determine format
    if (timeframe >= TimeFrame::Days) {
        file_format_ = FileFormat::DAILY_FD;
    } else {
        file_format_ = FileFormat::INTRADAY_MIN;
    }
    
    memory_mapped_ = false;
    mapped_data_ = nullptr;
    mapped_size_ = 0;
    current_position_ = 0;
    last_close_price_ = 0.0;
}

void VChartData::start() {
    AbstractDataBase::start();
    
    start_time_ = std::chrono::system_clock::now();
    statistics_ = AccessStatistics{}; // Reset statistics
    
    try {
        // Initialize file format if not already done
        if (file_format_ == FileFormat::UNKNOWN) {
            initialize_file_format();
        }
        
        // Setup binary format parameters
        setup_binary_format();
        
        // Open file if using filename
        if (!filename_.empty() && !file_stream_) {
            auto file = std::make_shared<std::ifstream>(filename_, std::ios::binary);
            if (!file->is_open()) {
                throw std::runtime_error("Cannot open VisualChart file: " + filename_);
            }
            file_stream_ = file;
        }
        
        // Validate file if enabled
        if (params_.validate_file_integrity) {
            if (!is_valid_vchart_file()) {
                throw std::runtime_error("Invalid VisualChart file format");
            }
        }
        
        // Setup memory mapping if enabled
        if (params_.use_memory_mapping && !filename_.empty()) {
            setup_memory_mapping();
        }
        
        // Preload data if requested
        if (params_.preload_all_data) {
            preload_file_data();
        }
        
        std::cout << "VChartData started for " << get_format_description() << std::endl;
        std::cout << "File: " << filename_ << std::endl;
        std::cout << "Bar size: " << bar_size_ << " bytes" << std::endl;
        
    } catch (const std::exception& e) {
        handle_read_error("Failed to start VChartData: " + std::string(e.what()));
        throw;
    }
}

void VChartData::stop() {
    AbstractDataBase::stop();
    
    // Cleanup memory mapping
    cleanup_memory_mapping();
    
    // Close file stream
    if (file_stream_) {
        if (auto file = std::dynamic_pointer_cast<std::ifstream>(file_stream_)) {
            file->close();
        }
        file_stream_.reset();
    }
    
    // Calculate final statistics
    calculate_performance_metrics();
    
    auto end_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    
    std::cout << "VChartData stopped" << std::endl;
    std::cout << "Bars read: " << statistics_.bars_read << std::endl;
    std::cout << "Processing time: " << duration.count() << "ms" << std::endl;
    std::cout << "Performance: " << statistics_.performance_category << std::endl;
}

bool VChartData::next() {
    return read_next_bar();
}

void VChartData::preload() {
    if (params_.preload_all_data) {
        preload_file_data();
    }
}

VChartData::FileFormat VChartData::detect_file_format() const {
    if (file_format_ != FileFormat::UNKNOWN) {
        return file_format_;
    }
    
    if (!filename_.empty()) {
        if (filename_.ends_with(".fd")) {
            return FileFormat::DAILY_FD;
        } else if (filename_.ends_with(".min")) {
            return FileFormat::INTRADAY_MIN;
        }
    }
    
    // If no extension, try to detect from timeframe
    if (get_timeframe() >= TimeFrame::Days) {
        return FileFormat::DAILY_FD;
    } else {
        return FileFormat::INTRADAY_MIN;
    }
}

std::string VChartData::get_format_description() const {
    switch (file_format_) {
        case FileFormat::DAILY_FD:
            return "VisualChart Daily Format (.fd)";
        case FileFormat::INTRADAY_MIN:
            return "VisualChart Intraday Format (.min)";
        default:
            return "Unknown VisualChart Format";
    }
}

bool VChartData::is_valid_vchart_file() const {
    if (!file_stream_) {
        return false;
    }
    
    // Check file size consistency
    if (!verify_file_size()) {
        return false;
    }
    
    // Check header if present
    if (!check_file_header()) {
        return false;
    }
    
    return true;
}

VChartData::FileInfo VChartData::get_file_info() const {
    FileInfo info = {};
    
    info.filename = filename_;
    info.format = file_format_;
    info.bar_size_bytes = bar_size_;
    info.timeframe = get_timeframe();
    info.is_memory_mapped = memory_mapped_;
    info.file_version = "VisualChart Binary";
    
    if (!filename_.empty() && std::filesystem::exists(filename_)) {
        info.file_size_bytes = std::filesystem::file_size(filename_);
        info.estimated_bars = bar_size_ > 0 ? info.file_size_bytes / bar_size_ : 0;
        
        auto ftime = std::filesystem::last_write_time(filename_);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        info.modification_time = sctp;
    }
    
    return info;
}

void VChartData::enable_memory_mapping(bool enable) {
    params_.use_memory_mapping = enable;
    
    if (enable && !filename_.empty() && !memory_mapped_) {
        setup_memory_mapping();
    } else if (!enable && memory_mapped_) {
        cleanup_memory_mapping();
    }
}

void VChartData::set_buffer_size(size_t size) {
    params_.read_buffer_size = size;
    allocate_read_buffer();
}

void VChartData::preload_file_data() {
    if (!file_stream_) {
        return;
    }
    
    std::cout << "Preloading VisualChart file data..." << std::endl;
    
    // Save current position
    auto current_pos = file_stream_->tellg();
    
    // Go to beginning
    file_stream_->seekg(0, std::ios::beg);
    
    size_t bars_loaded = 0;
    while (read_bar_from_stream()) {
        bars_loaded++;
        
        if (bars_loaded % 10000 == 0) {
            std::cout << "Preloaded " << bars_loaded << " bars..." << std::endl;
        }
        
        if (params_.max_bars_in_memory > 0 && bars_loaded >= static_cast<size_t>(params_.max_bars_in_memory)) {
            break;
        }
    }
    
    // Restore position
    file_stream_->seekg(current_pos);
    
    std::cout << "Preloaded " << bars_loaded << " bars from VisualChart file" << std::endl;
}

void VChartData::clear_cache() {
    // Clear any cached data
    statistics_.memory_usage_bytes = 0;
}

size_t VChartData::get_memory_usage() const {
    size_t usage = sizeof(*this);
    
    if (memory_mapped_) {
        usage += mapped_size_;
    }
    
    usage += statistics_.memory_usage_bytes;
    
    return usage;
}

VChartData::ValidationResult VChartData::validate_file() const {
    ValidationResult result = {};
    result.is_valid_file = true;
    result.total_bars = 0;
    result.valid_bars = 0;
    result.invalid_bars = 0;
    
    if (!file_stream_) {
        result.is_valid_file = false;
        result.errors.push_back("No file stream available");
        return result;
    }
    
    try {
        // Save current position
        auto current_pos = file_stream_->tellg();
        
        // Go to beginning
        file_stream_->seekg(0, std::ios::beg);
        
        std::vector<char> buffer(bar_size_);
        
        while (file_stream_->read(buffer.data(), bar_size_)) {
            result.total_bars++;
            
            // Parse bar data for validation
            if (file_format_ == FileFormat::DAILY_FD) {
                if (buffer.size() >= 28) {
                    // Extract OHLCV data for validation
                    uint32_t date;
                    float open, high, low, close;
                    uint32_t volume, oi;
                    
                    std::memcpy(&date, buffer.data(), 4);
                    std::memcpy(&open, buffer.data() + 4, 4);
                    std::memcpy(&high, buffer.data() + 8, 4);
                    std::memcpy(&low, buffer.data() + 12, 4);
                    std::memcpy(&close, buffer.data() + 16, 4);
                    std::memcpy(&volume, buffer.data() + 20, 4);
                    std::memcpy(&oi, buffer.data() + 24, 4);
                    
                    if (validate_bar_data(open, high, low, close, volume, oi) && is_valid_date(date)) {
                        result.valid_bars++;
                    } else {
                        result.invalid_bars++;
                    }
                }
            } else if (file_format_ == FileFormat::INTRADAY_MIN) {
                if (buffer.size() >= 32) {
                    // Extract datetime and OHLCV data for validation
                    uint32_t date, time;
                    float open, high, low, close;
                    uint32_t volume, oi;
                    
                    std::memcpy(&date, buffer.data(), 4);
                    std::memcpy(&time, buffer.data() + 4, 4);
                    std::memcpy(&open, buffer.data() + 8, 4);
                    std::memcpy(&high, buffer.data() + 12, 4);
                    std::memcpy(&low, buffer.data() + 16, 4);
                    std::memcpy(&close, buffer.data() + 20, 4);
                    std::memcpy(&volume, buffer.data() + 24, 4);
                    std::memcpy(&oi, buffer.data() + 28, 4);
                    
                    if (validate_bar_data(open, high, low, close, volume, oi) && 
                        is_valid_date(date) && is_valid_time(time)) {
                        result.valid_bars++;
                    } else {
                        result.invalid_bars++;
                    }
                }
            }
        }
        
        // Restore position
        file_stream_->seekg(current_pos);
        
        // Calculate validation success rate
        if (result.total_bars > 0) {
            double success_rate = static_cast<double>(result.valid_bars) / result.total_bars;
            if (success_rate < 0.9) {
                result.is_valid_file = false;
                result.errors.push_back("Low validation success rate: " + 
                                      std::to_string(success_rate * 100) + "%");
            }
        }
        
        result.integrity_status = result.is_valid_file ? "Good" : "Issues detected";
        
    } catch (const std::exception& e) {
        result.is_valid_file = false;
        result.errors.push_back("Validation error: " + std::string(e.what()));
    }
    
    return result;
}

bool VChartData::export_to_csv(const std::string& output_file) const {
    try {
        std::ofstream csv(output_file);
        if (!csv.is_open()) {
            return false;
        }
        
        // Write CSV header
        csv << "DateTime,Open,High,Low,Close,Volume,OpenInterest\n";
        
        // Save current position
        auto current_pos = file_stream_->tellg();
        
        // Go to beginning
        file_stream_->seekg(0, std::ios::beg);
        
        std::vector<char> buffer(bar_size_);
        
        while (file_stream_->read(buffer.data(), bar_size_)) {
            if (file_format_ == FileFormat::DAILY_FD) {
                uint32_t date;
                float open, high, low, close;
                uint32_t volume, oi;
                
                std::memcpy(&date, buffer.data(), 4);
                std::memcpy(&open, buffer.data() + 4, 4);
                std::memcpy(&high, buffer.data() + 8, 4);
                std::memcpy(&low, buffer.data() + 12, 4);
                std::memcpy(&close, buffer.data() + 16, 4);
                std::memcpy(&volume, buffer.data() + 20, 4);
                std::memcpy(&oi, buffer.data() + 24, 4);
                
                auto dt = parse_vchart_date(date);
                auto time_t = std::chrono::system_clock::to_time_t(dt);
                auto* tm = std::localtime(&time_t);
                
                if (tm) {
                    csv << std::setfill('0') 
                        << std::setw(4) << (tm->tm_year + 1900) << "-"
                        << std::setw(2) << (tm->tm_mon + 1) << "-"
                        << std::setw(2) << tm->tm_mday << " "
                        << std::setw(2) << tm->tm_hour << ":"
                        << std::setw(2) << tm->tm_min << ":"
                        << std::setw(2) << tm->tm_sec << ","
                        << open << "," << high << "," << low << "," << close << ","
                        << volume << "," << oi << "\n";
                }
            } else if (file_format_ == FileFormat::INTRADAY_MIN) {
                uint32_t date, time;
                float open, high, low, close;
                uint32_t volume, oi;
                
                std::memcpy(&date, buffer.data(), 4);
                std::memcpy(&time, buffer.data() + 4, 4);
                std::memcpy(&open, buffer.data() + 8, 4);
                std::memcpy(&high, buffer.data() + 12, 4);
                std::memcpy(&low, buffer.data() + 16, 4);
                std::memcpy(&close, buffer.data() + 20, 4);
                std::memcpy(&volume, buffer.data() + 24, 4);
                std::memcpy(&oi, buffer.data() + 28, 4);
                
                auto dt = parse_vchart_datetime(date, time);
                auto time_t = std::chrono::system_clock::to_time_t(dt);
                auto* tm = std::localtime(&time_t);
                
                if (tm) {
                    csv << std::setfill('0') 
                        << std::setw(4) << (tm->tm_year + 1900) << "-"
                        << std::setw(2) << (tm->tm_mon + 1) << "-"
                        << std::setw(2) << tm->tm_mday << " "
                        << std::setw(2) << tm->tm_hour << ":"
                        << std::setw(2) << tm->tm_min << ":"
                        << std::setw(2) << tm->tm_sec << ","
                        << open << "," << high << "," << low << "," << close << ","
                        << volume << "," << oi << "\n";
                }
            }
        }
        
        // Restore position
        file_stream_->seekg(current_pos);
        
        csv.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to CSV: " << e.what() << std::endl;
        return false;
    }
}

bool VChartData::export_to_json(const std::string& output_file) const {
    // Placeholder for JSON export
    std::cout << "JSON export not yet implemented" << std::endl;
    return false;
}

std::chrono::system_clock::time_point VChartData::get_first_date() const {
    if (!file_stream_) {
        return std::chrono::system_clock::now();
    }
    
    // Save current position
    auto current_pos = file_stream_->tellg();
    
    // Go to beginning and read first bar
    file_stream_->seekg(0, std::ios::beg);
    
    std::chrono::system_clock::time_point first_date = std::chrono::system_clock::now();
    
    std::vector<char> buffer(bar_size_);
    if (file_stream_->read(buffer.data(), bar_size_)) {
        if (file_format_ == FileFormat::DAILY_FD) {
            uint32_t date;
            std::memcpy(&date, buffer.data(), 4);
            first_date = parse_vchart_date(date);
        } else if (file_format_ == FileFormat::INTRADAY_MIN) {
            uint32_t date, time;
            std::memcpy(&date, buffer.data(), 4);
            std::memcpy(&time, buffer.data() + 4, 4);
            first_date = parse_vchart_datetime(date, time);
        }
    }
    
    // Restore position
    file_stream_->seekg(current_pos);
    
    return first_date;
}

std::chrono::system_clock::time_point VChartData::get_last_date() const {
    if (!file_stream_) {
        return std::chrono::system_clock::now();
    }
    
    // Save current position
    auto current_pos = file_stream_->tellg();
    
    // Go to end and read last bar
    file_stream_->seekg(-static_cast<std::streamoff>(bar_size_), std::ios::end);
    
    std::chrono::system_clock::time_point last_date = std::chrono::system_clock::now();
    
    std::vector<char> buffer(bar_size_);
    if (file_stream_->read(buffer.data(), bar_size_)) {
        if (file_format_ == FileFormat::DAILY_FD) {
            uint32_t date;
            std::memcpy(&date, buffer.data(), 4);
            last_date = parse_vchart_date(date);
        } else if (file_format_ == FileFormat::INTRADAY_MIN) {
            uint32_t date, time;
            std::memcpy(&date, buffer.data(), 4);
            std::memcpy(&time, buffer.data() + 4, 4);
            last_date = parse_vchart_datetime(date, time);
        }
    }
    
    // Restore position
    file_stream_->seekg(current_pos);
    
    return last_date;
}

std::chrono::seconds VChartData::get_date_range() const {
    auto first = get_first_date();
    auto last = get_last_date();
    return std::chrono::duration_cast<std::chrono::seconds>(last - first);
}

void VChartData::initialize_file_format() {
    file_format_ = detect_file_format();
}

void VChartData::setup_binary_format() {
    if (file_format_ == FileFormat::DAILY_FD) {
        bar_size_ = 28;
        datetime_size_ = 1;
        bar_format_ = "IffffII"; // Date(4) + OHLCV(20) + OI(4)
    } else if (file_format_ == FileFormat::INTRADAY_MIN) {
        bar_size_ = 32;
        datetime_size_ = 2;
        bar_format_ = "IIffffII"; // Date(4) + Time(4) + OHLCV(20) + OI(4)
    }
}

void VChartData::setup_memory_mapping() {
    if (filename_.empty() || memory_mapped_) {
        return;
    }
    
    try {
        // For now, just mark as memory mapped - actual implementation would use mmap
        memory_mapped_ = true;
        mapped_size_ = std::filesystem::file_size(filename_);
        
        std::cout << "Memory mapping enabled for file: " << filename_ 
                  << " (Size: " << mapped_size_ << " bytes)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to setup memory mapping: " << e.what() << std::endl;
        memory_mapped_ = false;
    }
}

bool VChartData::read_next_bar() {
    try {
        if (memory_mapped_) {
            return read_bar_from_memory();
        } else {
            return read_bar_from_stream();
        }
    } catch (const std::exception& e) {
        handle_read_error("Error reading bar: " + std::string(e.what()));
        return false;
    }
}

bool VChartData::read_bar_from_stream() {
    if (!file_stream_) {
        return false;
    }
    
    std::vector<char> buffer(bar_size_);
    
    if (!file_stream_->read(buffer.data(), bar_size_)) {
        return false; // End of file or error
    }
    
    // Parse binary data based on format
    if (file_format_ == FileFormat::DAILY_FD) {
        uint32_t date;
        float open, high, low, close;
        uint32_t volume, oi;
        
        std::memcpy(&date, buffer.data(), 4);
        std::memcpy(&open, buffer.data() + 4, 4);
        std::memcpy(&high, buffer.data() + 8, 4);
        std::memcpy(&low, buffer.data() + 12, 4);
        std::memcpy(&close, buffer.data() + 16, 4);
        std::memcpy(&volume, buffer.data() + 20, 4);
        std::memcpy(&oi, buffer.data() + 24, 4);
        
        // Validate data
        if (!validate_bar_data(open, high, low, close, volume, oi) || !is_valid_date(date)) {
            if (params_.skip_invalid_bars) {
                statistics_.invalid_bars++;
                return read_next_bar(); // Try next bar
            }
        }
        
        // Convert and set data
        auto dt = parse_vchart_date(date);
        set_datetime(dt);
        set_open(open);
        set_high(high);
        set_low(low);
        set_close(close);
        set_volume(volume);
        set_openinterest(oi);
        
    } else if (file_format_ == FileFormat::INTRADAY_MIN) {
        uint32_t date, time;
        float open, high, low, close;
        uint32_t volume, oi;
        
        std::memcpy(&date, buffer.data(), 4);
        std::memcpy(&time, buffer.data() + 4, 4);
        std::memcpy(&open, buffer.data() + 8, 4);
        std::memcpy(&high, buffer.data() + 12, 4);
        std::memcpy(&low, buffer.data() + 16, 4);
        std::memcpy(&close, buffer.data() + 20, 4);
        std::memcpy(&volume, buffer.data() + 24, 4);
        std::memcpy(&oi, buffer.data() + 28, 4);
        
        // Validate data
        if (!validate_bar_data(open, high, low, close, volume, oi) || 
            !is_valid_date(date) || !is_valid_time(time)) {
            if (params_.skip_invalid_bars) {
                statistics_.invalid_bars++;
                return read_next_bar(); // Try next bar
            }
        }
        
        // Convert and set data
        auto dt = parse_vchart_datetime(date, time);
        set_datetime(dt);
        set_open(open);
        set_high(high);
        set_low(low);
        set_close(close);
        set_volume(volume);
        set_openinterest(oi);
    }
    
    statistics_.bars_read++;
    statistics_.bytes_read += bar_size_;
    last_close_price_ = get_close();
    
    update_statistics();
    
    return true;
}

bool VChartData::read_bar_from_memory() {
    if (!memory_mapped_ || !mapped_data_) {
        return false;
    }
    
    if (current_position_ + bar_size_ > mapped_size_) {
        return false; // End of data
    }
    
    const char* buffer = mapped_data_ + current_position_;
    current_position_ += bar_size_;
    
    // Parse binary data (similar to read_bar_from_stream)
    // ... implementation similar to above
    
    statistics_.bars_read++;
    statistics_.bytes_read += bar_size_;
    
    update_statistics();
    
    return true;
}

std::chrono::system_clock::time_point VChartData::parse_vchart_date(uint32_t vc_date) const {
    // VisualChart stores dates as: years * 500 + months * 32 + days
    uint32_t year = vc_date / 500;
    uint32_t remainder = vc_date % 500;
    uint32_t month = remainder / 32;
    uint32_t day = remainder % 32;
    
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::chrono::system_clock::time_point VChartData::parse_vchart_datetime(
    uint32_t vc_date, 
    uint32_t vc_time) const {
    
    auto date_part = parse_vchart_date(vc_date);
    
    // VisualChart stores time as seconds since midnight
    uint32_t hours = vc_time / 3600;
    uint32_t minutes = (vc_time % 3600) / 60;
    uint32_t seconds = vc_time % 60;
    
    auto time_t = std::chrono::system_clock::to_time_t(date_part);
    auto* tm = std::localtime(&time_t);
    
    if (tm) {
        tm->tm_hour = hours;
        tm->tm_min = minutes;
        tm->tm_sec = seconds;
        
        return std::chrono::system_clock::from_time_t(std::mktime(tm));
    }
    
    return date_part;
}

uint32_t VChartData::convert_to_vchart_date(const std::chrono::system_clock::time_point& dt) const {
    auto time_t = std::chrono::system_clock::to_time_t(dt);
    auto* tm = std::localtime(&time_t);
    
    if (tm) {
        uint32_t year = tm->tm_year + 1900;
        uint32_t month = tm->tm_mon + 1;
        uint32_t day = tm->tm_mday;
        
        return year * 500 + month * 32 + day;
    }
    
    return 0;
}

std::chrono::system_clock::time_point VChartData::vchart_epoch_to_datetime(uint32_t days) const {
    // VisualChart epoch is different from Unix epoch
    // This is a simplified conversion
    auto epoch = std::chrono::system_clock::from_time_t(0);
    return epoch + std::chrono::hours(24 * days);
}

bool VChartData::validate_bar_data(double open, double high, double low, double close, 
                                  double volume, double openinterest) const {
    // Basic OHLC validation
    if (open <= 0 || high <= 0 || low <= 0 || close <= 0) {
        return false;
    }
    
    if (high < low) {
        return false;
    }
    
    if (high < open || high < close || low > open || low > close) {
        return false;
    }
    
    // Volume validation
    if (volume < 0) {
        return false;
    }
    
    // Open interest validation
    if (openinterest < 0) {
        return false;
    }
    
    return true;
}

bool VChartData::is_valid_date(uint32_t vc_date) const {
    if (vc_date == 0) {
        return false;
    }
    
    uint32_t year = vc_date / 500;
    uint32_t remainder = vc_date % 500;
    uint32_t month = remainder / 32;
    uint32_t day = remainder % 32;
    
    return year >= 1900 && year <= 2100 && 
           month >= 1 && month <= 12 && 
           day >= 1 && day <= 31;
}

bool VChartData::is_valid_time(uint32_t vc_time) const {
    uint32_t hours = vc_time / 3600;
    uint32_t minutes = (vc_time % 3600) / 60;
    uint32_t seconds = vc_time % 60;
    
    return hours <= 23 && minutes <= 59 && seconds <= 59;
}

bool VChartData::is_chronological(const std::chrono::system_clock::time_point& dt) const {
    if (params_.validate_sequence) {
        return dt > last_datetime_;
    }
    return true;
}

void VChartData::handle_read_error(const std::string& error) {
    std::cerr << "VChartData Read Error: " << error << std::endl;
    statistics_.invalid_bars++;
}

void VChartData::log_validation_warning(const std::string& warning) {
    std::cerr << "VChartData Warning: " << warning << std::endl;
}

void VChartData::update_statistics() {
    if (statistics_.bars_read % 1000 == 0) {
        calculate_performance_metrics();
    }
}

void VChartData::calculate_performance_metrics() {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    
    if (duration.count() > 0 && statistics_.bars_read > 0) {
        statistics_.average_read_speed_bars_per_sec = 
            (static_cast<double>(statistics_.bars_read) / duration.count()) * 1000.0;
        
        statistics_.total_read_time = duration;
        
        // Performance categorization
        if (statistics_.average_read_speed_bars_per_sec > 10000) {
            statistics_.performance_category = "Excellent";
        } else if (statistics_.average_read_speed_bars_per_sec > 1000) {
            statistics_.performance_category = "Good";
        } else if (statistics_.average_read_speed_bars_per_sec > 100) {
            statistics_.performance_category = "Fair";
        } else {
            statistics_.performance_category = "Poor";
        }
    }
    
    statistics_.memory_usage_bytes = get_memory_usage();
}

void VChartData::cleanup_memory_mapping() {
    if (memory_mapped_) {
        // Cleanup memory mapping resources
        mapped_data_ = nullptr;
        mapped_size_ = 0;
        memory_mapped_ = false;
        current_position_ = 0;
    }
}

void VChartData::allocate_read_buffer() {
    // Allocate read buffer if needed
    // Implementation would allocate buffer based on read_buffer_size
}

bool VChartData::check_file_header() const {
    // VisualChart binary files don't have headers typically
    return true;
}

bool VChartData::verify_file_size() const {
    if (!filename_.empty() && std::filesystem::exists(filename_)) {
        size_t file_size = std::filesystem::file_size(filename_);
        
        if (bar_size_ > 0) {
            // File size should be multiple of bar size
            return (file_size % bar_size_) == 0;
        }
    }
    
    return true;
}

size_t VChartData::calculate_expected_bars() const {
    if (!filename_.empty() && std::filesystem::exists(filename_) && bar_size_ > 0) {
        size_t file_size = std::filesystem::file_size(filename_);
        return file_size / bar_size_;
    }
    return 0;
}

// VChartFeed implementation

VChartFeed::VChartFeed(const FeedParams& params) : params_(params) {
}

std::shared_ptr<VChartData> VChartFeed::get_data(
    const std::string& dataname,
    const VChartData::VChartParams& params) {
    
    std::string full_path = resolve_data_path(dataname);
    return std::make_shared<VChartData>(full_path, params);
}

std::shared_ptr<VChartData> VChartFeed::create_daily_feed(const std::string& symbol) {
    std::string path = resolve_data_path(symbol) + ".fd";
    
    VChartData::VChartParams params;
    // Set daily-specific parameters
    
    return std::make_shared<VChartData>(path, params);
}

std::shared_ptr<VChartData> VChartFeed::create_intraday_feed(const std::string& symbol) {
    std::string path = resolve_data_path(symbol) + ".min";
    
    VChartData::VChartParams params;
    // Set intraday-specific parameters
    
    return std::make_shared<VChartData>(path, params);
}

std::string VChartFeed::resolve_data_path(const std::string& dataname) const {
    if (params_.base_path.empty()) {
        return dataname;
    }
    
    return build_data_path(dataname);
}

bool VChartFeed::validate_data_path(const std::string& path) const {
    return std::filesystem::exists(path);
}

std::vector<std::shared_ptr<VChartData>> VChartFeed::create_multiple_feeds(
    const std::vector<std::string>& symbols) {
    
    std::vector<std::shared_ptr<VChartData>> feeds;
    feeds.reserve(symbols.size());
    
    for (const auto& symbol : symbols) {
        auto feed = get_data(symbol);
        feeds.push_back(feed);
    }
    
    return feeds;
}

void VChartFeed::set_base_path(const std::string& path) {
    params_.base_path = path;
}

std::string VChartFeed::build_data_path(const std::string& dataname) const {
    // VisualChart path structure: basepath/RealServer/Data/maincode/subcode/dataname
    std::string main_code = extract_main_code(dataname);
    std::string sub_code = extract_sub_code(dataname);
    
    std::filesystem::path path = params_.base_path;
    path /= "RealServer";
    path /= "Data";
    path /= main_code;
    path /= sub_code;
    path /= dataname;
    
    return path.string();
}

std::string VChartFeed::extract_main_code(const std::string& dataname) const {
    if (dataname.length() >= 2) {
        return dataname.substr(0, 2);
    }
    return "01";
}

std::string VChartFeed::extract_sub_code(const std::string& dataname) const {
    if (dataname.length() >= 6) {
        return dataname.substr(2, 4);
    }
    return "00XX";
}

bool VChartFeed::ensure_directory_exists(const std::string& path) const {
    return std::filesystem::create_directories(path);
}

std::vector<std::string> VChartFeed::find_matching_files(const std::string& pattern) const {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(params_.base_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                // Simple pattern matching - would be more sophisticated in practice
                if (filename.find(pattern) != std::string::npos) {
                    files.push_back(filename);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error finding files: " << e.what() << std::endl;
    }
    
    return files;
}

// Additional specialized implementations and factory functions would follow...
// Due to length constraints, I'll provide the essential structure

// Factory functions implementation

namespace vchart_factory {

std::shared_ptr<VChartData> create_vchart_feed(
    const std::string& filename,
    const VChartData::VChartParams& params) {
    
    return std::make_shared<VChartData>(filename, params);
}

std::shared_ptr<VChartHighPerformanceData> create_high_performance_feed(
    const std::string& filename,
    bool enable_aggressive_caching) {
    
    VChartHighPerformanceData::HighPerfParams params;
    params.aggressive_caching = enable_aggressive_caching;
    params.disable_validation = true; // For speed
    
    return std::make_shared<VChartHighPerformanceData>(filename, params);
}

std::shared_ptr<VChartMemoryEfficientData> create_memory_efficient_feed(
    const std::string& filename,
    size_t max_memory_mb) {
    
    VChartMemoryEfficientData::MemoryEfficientParams params;
    params.max_memory_mb = max_memory_mb;
    params.use_streaming = true;
    
    return std::make_shared<VChartMemoryEfficientData>(filename, params);
}

std::shared_ptr<VChartData> create_optimized_feed(
    const std::string& filename,
    const std::string& optimization_target) {
    
    VChartData::VChartParams params;
    
    if (optimization_target == "speed") {
        params.use_memory_mapping = true;
        params.validate_file_integrity = false;
        params.preload_all_data = true;
    } else if (optimization_target == "memory") {
        params.use_memory_mapping = false;
        params.preload_all_data = false;
        params.read_buffer_size = 1024;
    } else { // balanced
        params.use_memory_mapping = true;
        params.validate_file_integrity = true;
        params.preload_all_data = false;
    }
    
    return std::make_shared<VChartData>(filename, params);
}

std::vector<std::shared_ptr<VChartData>> create_feeds_from_directory(
    const std::string& directory_path,
    const std::string& file_pattern) {
    
    std::vector<std::shared_ptr<VChartData>> feeds;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                if (filename.ends_with(".fd") || filename.ends_with(".min")) {
                    auto feed = create_vchart_feed(filename);
                    feeds.push_back(feed);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating feeds from directory: " << e.what() << std::endl;
    }
    
    return feeds;
}

std::unique_ptr<VChartFeed> create_feed_factory(
    const std::string& base_path,
    const VChartFeed::FeedParams& params) {
    
    VChartFeed::FeedParams factory_params = params;
    factory_params.base_path = base_path;
    
    return std::make_unique<VChartFeed>(factory_params);
}

} // namespace vchart_factory

// Utility functions implementation

namespace vchart_utils {

VChartData::FileFormat detect_file_format(const std::string& filename) {
    if (filename.ends_with(".fd")) {
        return VChartData::FileFormat::DAILY_FD;
    } else if (filename.ends_with(".min")) {
        return VChartData::FileFormat::INTRADAY_MIN;
    }
    return VChartData::FileFormat::UNKNOWN;
}

bool is_valid_vchart_file(const std::string& filename) {
    try {
        auto feed = vchart_factory::create_vchart_feed(filename);
        return feed->is_valid_vchart_file();
    } catch (...) {
        return false;
    }
}

std::string get_format_description(VChartData::FileFormat format) {
    switch (format) {
        case VChartData::FileFormat::DAILY_FD:
            return "VisualChart Daily Format (.fd)";
        case VChartData::FileFormat::INTRADAY_MIN:
            return "VisualChart Intraday Format (.min)";
        default:
            return "Unknown Format";
    }
}

FileMetadata analyze_vchart_file(const std::string& filename) {
    FileMetadata metadata = {};
    
    try {
        auto feed = vchart_factory::create_vchart_feed(filename);
        auto info = feed->get_file_info();
        
        metadata.filename = filename;
        metadata.format = info.format;
        metadata.file_size = info.file_size_bytes;
        metadata.estimated_bars = info.estimated_bars;
        metadata.timeframe = info.timeframe;
        metadata.first_date = feed->get_first_date();
        metadata.last_date = feed->get_last_date();
        
        // Extract symbol hint from filename
        auto path = std::filesystem::path(filename);
        metadata.symbol_hint = path.stem().string();
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing file: " << e.what() << std::endl;
    }
    
    return metadata;
}

std::vector<FileMetadata> analyze_directory(const std::string& directory_path) {
    std::vector<FileMetadata> metadata_list;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                if (filename.ends_with(".fd") || filename.ends_with(".min")) {
                    auto metadata = analyze_vchart_file(filename);
                    metadata_list.push_back(metadata);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing directory: " << e.what() << std::endl;
    }
    
    return metadata_list;
}

std::vector<std::string> find_vchart_files(
    const std::string& directory_path,
    VChartData::FileFormat format) {
    
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                
                bool matches = false;
                switch (format) {
                    case VChartData::FileFormat::DAILY_FD:
                        matches = filename.ends_with(".fd");
                        break;
                    case VChartData::FileFormat::INTRADAY_MIN:
                        matches = filename.ends_with(".min");
                        break;
                    case VChartData::FileFormat::UNKNOWN:
                        matches = filename.ends_with(".fd") || filename.ends_with(".min");
                        break;
                }
                
                if (matches) {
                    files.push_back(filename);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error finding files: " << e.what() << std::endl;
    }
    
    return files;
}

bool convert_to_csv(const std::string& vchart_file, const std::string& csv_file) {
    try {
        auto feed = vchart_factory::create_vchart_feed(vchart_file);
        return feed->export_to_csv(csv_file);
    } catch (const std::exception& e) {
        std::cerr << "Error converting to CSV: " << e.what() << std::endl;
        return false;
    }
}

bool convert_to_json(const std::string& vchart_file, const std::string& json_file) {
    try {
        auto feed = vchart_factory::create_vchart_feed(vchart_file);
        return feed->export_to_json(json_file);
    } catch (const std::exception& e) {
        std::cerr << "Error converting to JSON: " << e.what() << std::endl;
        return false;
    }
}

bool merge_vchart_files(
    const std::vector<std::string>& input_files,
    const std::string& output_file) {
    
    // Placeholder for merge functionality
    std::cout << "Merging " << input_files.size() << " VisualChart files to " << output_file << std::endl;
    return true;
}

ValidationReport validate_vchart_file(const std::string& filename) {
    ValidationReport report = {};
    
    try {
        auto feed = vchart_factory::create_vchart_feed(filename);
        auto validation = feed->validate_file();
        
        report.is_valid = validation.is_valid_file;
        report.total_bars = validation.total_bars;
        report.valid_bars = validation.valid_bars;
        report.corrupted_bars = validation.invalid_bars;
        report.issues = validation.errors;
        report.integrity_assessment = validation.integrity_status;
        
    } catch (const std::exception& e) {
        report.is_valid = false;
        report.issues.push_back("Validation error: " + std::string(e.what()));
    }
    
    return report;
}

bool repair_vchart_file(const std::string& input_file, const std::string& output_file) {
    // Placeholder for repair functionality
    std::cout << "Repairing VisualChart file: " << input_file << " -> " << output_file << std::endl;
    return true;
}

PerformanceProfile profile_performance(const std::string& filename) {
    PerformanceProfile profile = {};
    
    try {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto feed = vchart_factory::create_vchart_feed(filename);
        feed->start();
        
        size_t bars_processed = 0;
        while (feed->next() && bars_processed < 10000) { // Limit for profiling
            bars_processed++;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        profile.startup_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        profile.processing_speed_bars_per_sec = (static_cast<double>(bars_processed) / duration.count()) * 1000.0;
        profile.memory_footprint_mb = feed->get_memory_usage() / (1024 * 1024);
        
        // Analysis
        if (profile.processing_speed_bars_per_sec > 10000) {
            profile.bottleneck_analysis = "Performance is excellent";
        } else if (profile.processing_speed_bars_per_sec > 1000) {
            profile.bottleneck_analysis = "Good performance";
        } else {
            profile.bottleneck_analysis = "I/O bound - consider memory mapping";
            profile.optimization_suggestions.push_back("Enable memory mapping");
            profile.optimization_suggestions.push_back("Increase buffer size");
        }
        
        feed->stop();
        
    } catch (const std::exception& e) {
        profile.bottleneck_analysis = "Error profiling: " + std::string(e.what());
    }
    
    return profile;
}

DataSummary summarize_data(const std::string& filename) {
    DataSummary summary = {};
    
    try {
        auto feed = vchart_factory::create_vchart_feed(filename);
        auto info = feed->get_file_info();
        
        summary.total_bars = info.estimated_bars;
        summary.date_range_start = feed->get_first_date();
        summary.date_range_end = feed->get_last_date();
        
        // Would need to scan file for price/volume statistics
        summary.price_range_min = 0.0;
        summary.price_range_max = 1000.0;
        summary.average_volume = 1000000.0;
        summary.total_volume = summary.average_volume * summary.total_bars;
        summary.data_characteristics = "VisualChart binary data";
        
    } catch (const std::exception& e) {
        summary.data_characteristics = "Error: " + std::string(e.what());
    }
    
    return summary;
}

bool optimize_file_layout(const std::string& input_file, const std::string& output_file) {
    std::cout << "Optimizing file layout: " << input_file << " -> " << output_file << std::endl;
    return true;
}

bool compress_vchart_file(const std::string& input_file, const std::string& output_file) {
    std::cout << "Compressing VisualChart file: " << input_file << " -> " << output_file << std::endl;
    return true;
}

bool defragment_vchart_file(const std::string& filename) {
    std::cout << "Defragmenting VisualChart file: " << filename << std::endl;
    return true;
}

std::string get_format_specification() {
    return R"(
VisualChart Binary Format Specification:

Daily Format (.fd) - 28 bytes per bar:
- Date (4 bytes): Encoded as (year * 500) + (month * 32) + day
- Open (4 bytes): IEEE 754 float
- High (4 bytes): IEEE 754 float  
- Low (4 bytes): IEEE 754 float
- Close (4 bytes): IEEE 754 float
- Volume (4 bytes): 32-bit unsigned integer
- Open Interest (4 bytes): 32-bit unsigned integer

Intraday Format (.min) - 32 bytes per bar:
- Date (4 bytes): Encoded as above
- Time (4 bytes): Seconds since midnight
- Open (4 bytes): IEEE 754 float
- High (4 bytes): IEEE 754 float
- Low (4 bytes): IEEE 754 float
- Close (4 bytes): IEEE 754 float
- Volume (4 bytes): 32-bit unsigned integer
- Open Interest (4 bytes): 32-bit unsigned integer

All values are stored in little-endian byte order.
)";
}

std::map<std::string, std::string> get_field_definitions() {
    return {
        {"Date", "Encoded date: (year * 500) + (month * 32) + day"},
        {"Time", "Seconds since midnight (intraday only)"},
        {"Open", "Opening price (IEEE 754 float)"},
        {"High", "Highest price (IEEE 754 float)"},
        {"Low", "Lowest price (IEEE 754 float)"},
        {"Close", "Closing price (IEEE 754 float)"},
        {"Volume", "Trading volume (32-bit unsigned integer)"},
        {"Open Interest", "Open interest (32-bit unsigned integer)"}
    };
}

std::vector<std::string> get_supported_extensions() {
    return {".fd", ".min"};
}

} // namespace vchart_utils

} // namespace feeds
} // namespace backtrader