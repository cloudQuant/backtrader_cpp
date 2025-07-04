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
        collect_csv_headers();
        write_csv_header();
    }
}

void WriterFile::stop() {
    close_output();
}

void WriterFile::next() {
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
    
    // Add counter if enabled
    if (params.csv_counter) {
        headers_.push_back("Counter");
    }
    
    // Add data headers (if csv attribute is true)
    for (const auto& data : datas_) {
        if (should_include_in_csv("data")) {
            if (data->lines) {
                headers_.push_back("Data_Open");
                headers_.push_back("Data_High");
                headers_.push_back("Data_Low");
                headers_.push_back("Data_Close");
                headers_.push_back("Data_Volume");
            }
        }
    }
    
    // Add observer headers (default to include)
    for (size_t i = 0; i < observers_.size(); ++i) {
        if (should_include_in_csv("observer")) {
            headers_.push_back("Observer_" + std::to_string(i));
        }
    }
    
    // Add indicator headers (default to exclude, unless explicitly set)
    for (size_t i = 0; i < indicators_.size(); ++i) {
        if (should_include_in_csv("indicator")) {
            headers_.push_back("Indicator_" + std::to_string(i));
        }
    }
}

void WriterFile::collect_csv_values() {
    std::vector<std::string> row;
    
    // Add counter if enabled
    if (params.csv_counter) {
        row.push_back(std::to_string(line_counter_));
    }
    
    // Add data values
    for (const auto& data : datas_) {
        if (should_include_in_csv("data") && data->lines) {
            if (auto open_line = data->lines->getline(DataSeries::Open)) {
                row.push_back(format_value((*open_line)[0]));
            }
            if (auto high_line = data->lines->getline(DataSeries::High)) {
                row.push_back(format_value((*high_line)[0]));
            }
            if (auto low_line = data->lines->getline(DataSeries::Low)) {
                row.push_back(format_value((*low_line)[0]));
            }
            if (auto close_line = data->lines->getline(DataSeries::Close)) {
                row.push_back(format_value((*close_line)[0]));
            }
            if (auto volume_line = data->lines->getline(DataSeries::Volume)) {
                row.push_back(format_value((*volume_line)[0]));
            }
        }
    }
    
    // Add observer values
    for (const auto& observer : observers_) {
        if (should_include_in_csv("observer")) {
            // This would need to be implemented based on observer structure
            row.push_back("0.0"); // Placeholder
        }
    }
    
    // Add indicator values
    for (const auto& indicator : indicators_) {
        if (should_include_in_csv("indicator")) {
            // This would need to be implemented based on indicator structure
            row.push_back("0.0"); // Placeholder
        }
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