#include "feeds/pandafeed.h"
#include <algorithm>
#include <stdexcept>
#include <cctype>

namespace backtrader {

// PandasDirectData implementation
PandasDirectData::PandasDirectData() : DataBase() {
    // Initialize with default parameters
}

void PandasDirectData::start() {
    DataBase::start();
    
    if (!params.dataname || params.dataname->empty()) {
        throw std::runtime_error("PandasDirectData: dataname DataFrame is empty or null");
    }
    
    // Reset iterator on each start
    row_iterator_ = std::make_unique<DataFrame::RowIterator>(params.dataname->get_iterator());
}

bool PandasDirectData::load() {
    if (!row_iterator_ || !row_iterator_->has_next()) {
        return false; // No more data
    }
    
    try {
        // Get next row
        const auto& row = row_iterator_->next();
        
        // Parse datetime
        double datetime_num = parse_datetime_value(row, params.datetime_idx);
        
        // Parse OHLCV data with validation for negative indices (missing columns)
        double open = get_column_value(row, params.open_idx);
        double high = get_column_value(row, params.high_idx);
        double low = get_column_value(row, params.low_idx);
        double close = get_column_value(row, params.close_idx);
        double volume = get_column_value(row, params.volume_idx);
        double openinterest = get_column_value(row, params.openinterest_idx);
        
        // Set current bar data
        set_current_bar(datetime_num, open, high, low, close, volume, openinterest);
        
        return true;
        
    } catch (const std::exception&) {
        return false; // Parsing failed
    }
}

double PandasDirectData::get_column_value(const std::vector<double>& row, int column_index) {
    if (column_index < 0) {
        return 0.0; // Column not present
    }
    
    if (static_cast<size_t>(column_index) >= row.size()) {
        return 0.0; // Index out of bounds
    }
    
    return row[column_index];
}

double PandasDirectData::parse_datetime_value(const std::vector<double>& row, int datetime_index) {
    if (datetime_index < 0 || static_cast<size_t>(datetime_index) >= row.size()) {
        throw std::runtime_error("Invalid datetime column index");
    }
    
    // In a real implementation, this would convert pandas timestamp to backtrader date number
    // For now, assume the datetime is already in numeric format
    return row[datetime_index];
}

// PandasData implementation
PandasData::PandasData() : DataBase(), datetime_idx_(-1), open_idx_(-1), high_idx_(-1),
                          low_idx_(-1), close_idx_(-1), volume_idx_(-1), openinterest_idx_(-1) {
    // Initialize with default parameters
}

void PandasData::start() {
    DataBase::start();
    
    if (!params.dataname || params.dataname->empty()) {
        throw std::runtime_error("PandasData: dataname DataFrame is empty or null");
    }
    
    // Map column names to indices
    map_columns();
    
    // Reset iterator on each start
    row_iterator_ = std::make_unique<DataFrame::RowIterator>(params.dataname->get_iterator());
}

bool PandasData::load() {
    if (!row_iterator_ || !row_iterator_->has_next()) {
        return false; // No more data
    }
    
    try {
        // Get next row
        const auto& row = row_iterator_->next();
        
        // Parse datetime
        if (datetime_idx_ < 0) {
            throw std::runtime_error("Datetime column not found");
        }
        double datetime_num = row[datetime_idx_];
        
        // Parse OHLCV data
        double open = (open_idx_ >= 0) ? row[open_idx_] : 0.0;
        double high = (high_idx_ >= 0) ? row[high_idx_] : 0.0;
        double low = (low_idx_ >= 0) ? row[low_idx_] : 0.0;
        double close = (close_idx_ >= 0) ? row[close_idx_] : 0.0;
        double volume = (volume_idx_ >= 0) ? row[volume_idx_] : 0.0;
        double openinterest = (openinterest_idx_ >= 0) ? row[openinterest_idx_] : 0.0;
        
        // Set current bar data
        set_current_bar(datetime_num, open, high, low, close, volume, openinterest);
        
        return true;
        
    } catch (const std::exception&) {
        return false; // Parsing failed
    }
}

void PandasData::map_columns() {
    // Map datetime column
    if (params.datetime_col.empty() && params.auto_detect) {
        datetime_idx_ = auto_detect_datetime_column();
    } else {
        datetime_idx_ = find_column_index(params.datetime_col);
    }
    
    // Map OHLCV columns
    open_idx_ = find_column_index(params.open_col);
    high_idx_ = find_column_index(params.high_col);
    low_idx_ = find_column_index(params.low_col);
    close_idx_ = find_column_index(params.close_col);
    volume_idx_ = find_column_index(params.volume_col);
    openinterest_idx_ = find_column_index(params.openinterest_col);
    
    // Auto-detect missing columns if enabled
    if (params.auto_detect) {
        if (open_idx_ < 0) {
            open_idx_ = auto_detect_column({"open", "o"});
        }
        if (high_idx_ < 0) {
            high_idx_ = auto_detect_column({"high", "h"});
        }
        if (low_idx_ < 0) {
            low_idx_ = auto_detect_column({"low", "l"});
        }
        if (close_idx_ < 0) {
            close_idx_ = auto_detect_column({"close", "c", "adj_close", "adjclose"});
        }
        if (volume_idx_ < 0) {
            volume_idx_ = auto_detect_column({"volume", "vol", "v"});
        }
        if (openinterest_idx_ < 0) {
            openinterest_idx_ = auto_detect_column({"openinterest", "oi", "open_interest"});
        }
    }
}

int PandasData::find_column_index(const std::string& column_name) {
    if (column_name.empty()) {
        return -1;
    }
    
    for (size_t i = 0; i < params.dataname->columns.size(); ++i) {
        if (column_matches(params.dataname->columns[i], column_name)) {
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

int PandasData::auto_detect_datetime_column() {
    // Look for common datetime column names
    std::vector<std::string> datetime_names = {
        "datetime", "date", "time", "timestamp", "dt", "index"
    };
    
    return auto_detect_column(datetime_names);
}

int PandasData::auto_detect_column(const std::vector<std::string>& possible_names) {
    for (const std::string& name : possible_names) {
        int idx = find_column_index(name);
        if (idx >= 0) {
            return idx;
        }
    }
    return -1;
}

std::string PandasData::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool PandasData::column_matches(const std::string& actual, const std::string& expected) {
    if (params.case_insensitive) {
        return to_lower(actual) == to_lower(expected);
    } else {
        return actual == expected;
    }
}

} // namespace backtrader