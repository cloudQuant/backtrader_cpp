#include "feeds/CSVDataFeed.h"
#include "Common.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace backtrader {

CSVDataFeed::CSVDataFeed(const std::string& filename, const CSVConfig& config)
    : filename_(filename),
      config_(config),
      current_index_(0),
      loaded_(false) {
    
    initializeLines();
}

CSVDataFeed::CSVDataFeed(const std::string& filename)
    : filename_(filename),
      config_({}),
      current_index_(0),
      loaded_(false) {
    
    initializeLines();
}

bool CSVDataFeed::load() {
    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename_ << std::endl;
        return false;
    }
    
    data_.clear();
    std::string line;
    size_t line_number = 0;
    
    // Skip header if present
    if (config_.has_header && std::getline(file, line)) {
        line_number++;
    }
    
    // Read data lines
    while (std::getline(file, line)) {
        line_number++;
        if (!line.empty() && line[0] != '#') {  // Skip comments
            parseCSVLine(line, line_number);
        }
    }
    
    file.close();
    
    // Reverse order if requested (CSV might be newest first)
    if (config_.reverse_order) {
        std::reverse(data_.begin(), data_.end());
    }
    
    loaded_ = !data_.empty();
    current_index_ = 0;
    
    if (loaded_) {
        updateLines();
        std::cout << "Loaded " << data_.size() << " data points from " << filename_ << std::endl;
    } else {
        std::cerr << "Error: No valid data found in " << filename_ << std::endl;
    }
    
    return loaded_;
}

bool CSVDataFeed::next() {
    if (!loaded_ || current_index_ >= data_.size() - 1) {
        return false;
    }
    
    current_index_++;
    updateLines();
    return true;
}

void CSVDataFeed::reset() {
    current_index_ = 0;
    if (loaded_) {
        updateLines();
    }
}

const CSVDataFeed::DataBar& CSVDataFeed::getCurrentBar() const {
    if (!loaded_ || current_index_ >= data_.size()) {
        static DataBar empty_bar;
        return empty_bar;
    }
    return data_[current_index_];
}

const CSVDataFeed::DataBar& CSVDataFeed::getBar(size_t index) const {
    if (!loaded_ || index >= data_.size()) {
        static DataBar empty_bar;
        return empty_bar;
    }
    return data_[index];
}

CSVDataFeed::DataStats CSVDataFeed::getStats() const {
    DataStats stats;
    
    if (!loaded_ || data_.empty()) {
        return stats;
    }
    
    stats.total_rows = data_.size();
    stats.valid_rows = 0;
    stats.min_price = std::numeric_limits<double>::max();
    stats.max_price = std::numeric_limits<double>::lowest();
    double total_volume = 0.0;
    
    for (const auto& bar : data_) {
        if (bar.isValid()) {
            stats.valid_rows++;
            stats.min_price = std::min(stats.min_price, bar.low);
            stats.max_price = std::max(stats.max_price, bar.high);
            if (!isNaN(bar.volume)) {
                total_volume += bar.volume;
            }
        }
    }
    
    stats.invalid_rows = stats.total_rows - stats.valid_rows;
    stats.avg_volume = (stats.valid_rows > 0) ? total_volume / stats.valid_rows : 0.0;
    
    if (!data_.empty()) {
        stats.start_date = data_.front().datetime;
        stats.end_date = data_.back().datetime;
    }
    
    return stats;
}

bool CSVDataFeed::exportToCSV(const std::string& output_filename) const {
    if (!loaded_) {
        return false;
    }
    
    std::ofstream file(output_filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "Date,Open,High,Low,Close,Volume";
    if (config_.adj_close_col >= 0) {
        file << ",Adj Close";
    }
    file << "\n";
    
    // Write data
    for (const auto& bar : data_) {
        if (bar.isValid()) {
            file << bar.datetime << ","
                 << bar.open << ","
                 << bar.high << ","
                 << bar.low << ","
                 << bar.close << ","
                 << (isNaN(bar.volume) ? 0.0 : bar.volume);
            
            if (config_.adj_close_col >= 0) {
                file << "," << (isNaN(bar.adj_close) ? bar.close : bar.adj_close);
            }
            file << "\n";
        }
    }
    
    file.close();
    return true;
}

bool CSVDataFeed::parseCSVLine(const std::string& line, size_t line_number) {
    std::vector<std::string> fields = splitLine(line, config_.separator);
    
    // Check minimum required columns
    size_t max_col = std::max({config_.datetime_col, config_.open_col, 
                              config_.high_col, config_.low_col, config_.close_col});
    if (config_.volume_col >= 0) {
        max_col = std::max(max_col, static_cast<size_t>(config_.volume_col));
    }
    if (config_.adj_close_col >= 0) {
        max_col = std::max(max_col, static_cast<size_t>(config_.adj_close_col));
    }
    
    if (fields.size() <= max_col) {
        if (!config_.skip_invalid_rows) {
            std::cerr << "Warning: Line " << line_number << " has insufficient columns" << std::endl;
        }
        return false;
    }
    
    DataBar bar;
    
    try {
        // Parse required fields
        bar.datetime = fields[config_.datetime_col];
        bar.open = parseDouble(fields[config_.open_col]);
        bar.high = parseDouble(fields[config_.high_col]);
        bar.low = parseDouble(fields[config_.low_col]);
        bar.close = parseDouble(fields[config_.close_col]);
        
        // Parse optional fields
        if (config_.volume_col >= 0 && config_.volume_col < static_cast<int>(fields.size())) {
            bar.volume = parseDouble(fields[config_.volume_col]);
        }
        
        if (config_.adj_close_col >= 0 && config_.adj_close_col < static_cast<int>(fields.size())) {
            bar.adj_close = parseDouble(fields[config_.adj_close_col]);
        }
        
        // Validate data
        if (validateBar(bar)) {
            data_.push_back(bar);
            return true;
        } else {
            if (!config_.skip_invalid_rows) {
                std::cerr << "Warning: Line " << line_number << " contains invalid data" << std::endl;
            }
            return false;
        }
        
    } catch (const std::exception& e) {
        if (!config_.skip_invalid_rows) {
            std::cerr << "Error parsing line " << line_number << ": " << e.what() << std::endl;
        }
        return false;
    }
}

std::vector<std::string> CSVDataFeed::splitLine(const std::string& line, const std::string& separator) const {
    std::vector<std::string> fields;
    size_t start = 0;
    size_t end = 0;
    
    while ((end = line.find(separator, start)) != std::string::npos) {
        fields.push_back(line.substr(start, end - start));
        start = end + separator.length();
    }
    fields.push_back(line.substr(start));
    
    // Trim whitespace from fields
    for (auto& field : fields) {
        field.erase(0, field.find_first_not_of(" \t\r\n"));
        field.erase(field.find_last_not_of(" \t\r\n") + 1);
    }
    
    return fields;
}

double CSVDataFeed::parseDouble(const std::string& str) const {
    if (str.empty() || str == "null" || str == "NULL" || str == "nan" || str == "NaN") {
        return NaN;
    }
    
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return NaN;
    }
}

bool CSVDataFeed::validateBar(const DataBar& bar) const {
    // Check for valid OHLC data
    if (isNaN(bar.open) || isNaN(bar.high) || isNaN(bar.low) || isNaN(bar.close)) {
        return false;
    }
    
    // Check price ranges
    if (bar.open < config_.min_price || bar.open > config_.max_price ||
        bar.high < config_.min_price || bar.high > config_.max_price ||
        bar.low < config_.min_price || bar.low > config_.max_price ||
        bar.close < config_.min_price || bar.close > config_.max_price) {
        return false;
    }
    
    // Check OHLC relationships
    if (bar.high < bar.low || 
        bar.high < bar.open || bar.high < bar.close ||
        bar.low > bar.open || bar.low > bar.close) {
        return false;
    }
    
    // Check volume if present
    if (!isNaN(bar.volume) && bar.volume < 0.0) {
        return false;
    }
    
    return true;
}

void CSVDataFeed::initializeLines() {
    size_t capacity = 10000;  // Default capacity
    
    open_line_ = std::make_shared<LineRoot>(capacity, "open");
    high_line_ = std::make_shared<LineRoot>(capacity, "high");
    low_line_ = std::make_shared<LineRoot>(capacity, "low");
    close_line_ = std::make_shared<LineRoot>(capacity, "close");
    volume_line_ = std::make_shared<LineRoot>(capacity, "volume");
    adj_close_line_ = std::make_shared<LineRoot>(capacity, "adj_close");
}

void CSVDataFeed::updateLines() {
    if (!loaded_ || current_index_ >= data_.size()) {
        return;
    }
    
    const DataBar& current_bar = data_[current_index_];
    
    // Update all line values with current bar data
    open_line_->forward(current_bar.open);
    high_line_->forward(current_bar.high);
    low_line_->forward(current_bar.low);
    close_line_->forward(current_bar.close);
    volume_line_->forward(current_bar.volume);
    adj_close_line_->forward(isNaN(current_bar.adj_close) ? current_bar.close : current_bar.adj_close);
}

} // namespace backtrader