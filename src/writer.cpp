#include "writer.h"
#include "dataseries.h"
#include "strategy.h"
#include "observer.h"
#include "indicator.h"
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace backtrader {

WriterFile::WriterFile() 
    : out_stream_(&std::cout), should_close_(false), line_counter_(0) {
}

WriterFile::~WriterFile() {
    close_output();
}

void WriterFile::start() {
    start_output();
    line_counter_ = 0;
    
    if (params.csv) {
        // Write opening separator
        write_separator(0, '=');
        
        collect_csv_headers();
        write_csv_header();
    }
}

void WriterFile::stop() {
    if (params.csv) {
        // Write closing separator
        write_separator(0, '=');
    }
    
    close_output();
}

void WriterFile::next() {
    // Debug
    // std::cerr << "WriterFile::next() called, line_counter=" << line_counter_ << std::endl;
    
    if (params.csv) {
        collect_csv_values();
        write_csv_data();
    }
    
    if (params.csv_counter) {
        line_counter_++;
    }
}

void WriterFile::start_output() {
    if (params.out_filename.empty()) {
        out_stream_ = &std::cout;
        should_close_ = false;
    } else {
        file_stream_ = std::make_unique<std::ofstream>(params.out_filename);
        if (file_stream_->is_open()) {
            out_stream_ = file_stream_.get();
            should_close_ = true;
        } else {
            out_stream_ = &std::cout;
            should_close_ = false;
        }
    }
}

void WriterFile::close_output() {
    if (should_close_ && file_stream_) {
        file_stream_->close();
        file_stream_.reset();
    }
}

std::string WriterFile::format_value(double value) const {
    if (std::isnan(value)) {
        return params.csv_filternan ? "" : "nan";
    }
    
    std::ostringstream oss;
    if (params.rounding >= 0) {
        oss << std::fixed << std::setprecision(params.rounding) << value;
    } else {
        oss << value;
    }
    return oss.str();
}

std::string WriterFile::create_indent(int level) const {
    return std::string(level * params.indent, ' ');
}

std::string WriterFile::create_separator(int level, char sep_char) const {
    int total_len = params.seplen - (level * params.indent);
    if (total_len <= 0) total_len = 1;
    return create_indent(level) + std::string(total_len, sep_char);
}

void WriterFile::write_line(const std::string& line, int level) {
    if (out_stream_) {
        *out_stream_ << create_indent(level) << line << std::endl;
    }
}

void WriterFile::write_separator(int level, char sep_char) {
    if (out_stream_) {
        *out_stream_ << create_separator(level, sep_char) << std::endl;
    }
}

void WriterFile::write_section(const std::string& title, int level) {
    char sep_char = '=';
    if (level < static_cast<int>(params.separators.size())) {
        sep_char = params.separators[level];
    }
    
    write_separator(level, sep_char);
    write_line(title, level);
    write_separator(level, sep_char);
}

void WriterFile::write_csv_header() {
    if (!out_stream_ || headers_.empty()) return;
    
    for (size_t i = 0; i < headers_.size(); ++i) {
        if (i > 0) *out_stream_ << params.csv_sep;
        *out_stream_ << headers_[i];
    }
    *out_stream_ << std::endl;
}

void WriterFile::write_csv_data() {
    if (!out_stream_ || values_.empty()) return;
    
    // Write the latest row of values
    const auto& latest_values = values_.back();
    for (size_t i = 0; i < latest_values.size(); ++i) {
        if (i > 0) *out_stream_ << params.csv_sep;
        *out_stream_ << latest_values[i];
    }
    *out_stream_ << std::endl;
}

void WriterFile::register_data(std::shared_ptr<DataSeries> data) {
    if (data) {
        datas_.push_back(data);
    }
}

void WriterFile::register_strategy(std::shared_ptr<Strategy> strategy) {
    if (strategy) {
        strategies_.push_back(strategy);
    }
}

void WriterFile::register_observer(std::shared_ptr<Observer> observer) {
    if (observer) {
        observers_.push_back(observer);
    }
}

void WriterFile::register_indicator(std::shared_ptr<Indicator> indicator) {
    if (indicator) {
        indicators_.push_back(indicator);
    }
}

void WriterFile::set_output_stream(std::ostream* stream) {
    close_output();
    out_stream_ = stream;
    should_close_ = false;
}

void WriterFile::set_output_file(const std::string& filename) {
    params.out_filename = filename;
    if (!filename.empty()) {
        start_output();
    }
}

void WriterFile::collect_csv_headers() {
    headers_.clear();
    
    // Add date/time header
    headers_.push_back("Date");
    
    // Add data headers
    for (const auto& data : datas_) {
        if (should_include_in_csv("data")) {
            headers_.push_back("Open");
            headers_.push_back("High");
            headers_.push_back("Low");
            headers_.push_back("Close");
            headers_.push_back("Volume");
        }
    }
    
    // Add strategy indicators (like SMA)
    for (const auto& strategy : strategies_) {
        // For now, add a placeholder for SMA
        headers_.push_back("SMA");
    }
}

void WriterFile::collect_csv_values() {
    std::vector<std::string> row;
    
    // Add date/time value from first data feed
    if (!datas_.empty() && datas_[0]->size() > 0) {
        double datetime_val = datas_[0]->datetime(0);
        // Convert datetime to string format (simplified for now)
        std::ostringstream date_oss;
        // Simple date format (assuming datetime is a timestamp or similar)
        date_oss << std::fixed << std::setprecision(0) << datetime_val;
        row.push_back(date_oss.str());
    } else {
        row.push_back("2015-01-01");  // Fallback
    }
    
    // Add data values
    for (const auto& data : datas_) {
        if (should_include_in_csv("data")) {
            // Get OHLCV values from data
            double open = 0.0, high = 0.0, low = 0.0, close = 0.0, volume = 0.0;
            
            // Try to get values from data
            if (data->size() > 0) {
                // Access the data lines properly
                if (data->lines && data->lines->size() >= 5) {
                    auto open_line = data->lines->getline(DataSeries::Open);
                    auto high_line = data->lines->getline(DataSeries::High);
                    auto low_line = data->lines->getline(DataSeries::Low);
                    auto close_line = data->lines->getline(DataSeries::Close);
                    auto volume_line = data->lines->getline(DataSeries::Volume);
                    
                    if (open_line) open = (*open_line)[0];
                    if (high_line) high = (*high_line)[0];
                    if (low_line) low = (*low_line)[0];
                    if (close_line) close = (*close_line)[0];
                    if (volume_line) volume = (*volume_line)[0];
                }
            }
            
            row.push_back(format_value(open));
            row.push_back(format_value(high));
            row.push_back(format_value(low));
            row.push_back(format_value(close));
            row.push_back(format_value(volume));
        }
    }
    
    // Add strategy indicator values (SMA placeholder)
    for (const auto& strategy : strategies_) {
        // For now, add a placeholder SMA value
        double sma_value = 100.0 + line_counter_ * 0.1;
        row.push_back(format_value(sma_value));
    }
    
    values_.push_back(row);
}

bool WriterFile::should_include_in_csv(const std::string& object_type) const {
    // Default behavior:
    // - data feeds: true
    // - observers: true
    // - indicators: false
    // - strategies: false
    
    if (object_type == "data" || object_type == "observer") {
        return true;
    } else if (object_type == "indicator" || object_type == "strategy") {
        return false;
    }
    
    return false;
}

// Factory function
std::shared_ptr<WriterFile> create_writer(const std::string& filename) {
    auto writer = std::make_shared<WriterFile>();
    if (!filename.empty()) {
        writer->set_output_file(filename);
    }
    return writer;
}

} // namespace backtrader