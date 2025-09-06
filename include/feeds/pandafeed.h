#pragma once

#include "../feed.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace backtrader {

// Simplified DataFrame structure for C++ (since we don't have pandas)
// In practice, this could interface with libraries like Arrow or custom data structures
struct DataFrame {
    std::vector<std::string> columns;
    std::vector<std::vector<double>> data;  // Row-major data storage
    std::vector<std::string> index;         // Row indices (could be dates)
    
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    
    // Access row by index
    const std::vector<double>& operator[](size_t row) const { return data[row]; }
    std::vector<double>& operator[](size_t row) { return data[row]; }
    
    // Get column index by name
    int get_column_index(const std::string& column_name) const {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i] == column_name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    // Iterator-like access to rows
    struct RowIterator {
        const DataFrame* df;
        size_t current_row;
        
        RowIterator(const DataFrame* dataframe, size_t row) : df(dataframe), current_row(row) {}
        
        bool has_next() const { return current_row < df->size(); }
        const std::vector<double>& next() { return (*df)[current_row++]; }
    };
    
    RowIterator get_iterator() const { return RowIterator(this, 0); }
};

// PandasDirectData - uses a DataFrame as feed source, iterating directly over tuples
class PandasDirectData : public DataBase {
public:
    struct Params {
        std::shared_ptr<DataFrame> dataname;  // Pandas DataFrame equivalent
        int datetime_idx = 0;                 // Datetime column index
        int open_idx = 1;                     // Open column index  
        int high_idx = 2;                     // High column index
        int low_idx = 3;                      // Low column index
        int close_idx = 4;                    // Close column index
        int volume_idx = 5;                   // Volume column index
        int openinterest_idx = 6;             // Open interest column index
    } params;
    
    PandasDirectData();
    virtual ~PandasDirectData() = default;
    
    // Feed interface
    void start() override;
    bool load() override;
    
private:
    // Data iteration
    std::unique_ptr<DataFrame::RowIterator> row_iterator_;
    
    // Helper methods
    double get_column_value(const std::vector<double>& row, int column_index);
    double parse_datetime_value(const std::vector<double>& row, int datetime_index);
};

// PandasData - flexible column mapping with auto-detection
class PandasData : public DataBase {
public:
    struct Params {
        std::shared_ptr<DataFrame> dataname;     // DataFrame source
        
        // Column name mapping (empty string means auto-detect)
        std::string datetime_col = "";           // Datetime column name
        std::string open_col = "open";           // Open column name
        std::string high_col = "high";           // High column name  
        std::string low_col = "low";             // Low column name
        std::string close_col = "close";         // Close column name
        std::string volume_col = "volume";       // Volume column name
        std::string openinterest_col = "openinterest";  // Open interest column name
        
        // Auto-detection settings
        bool auto_detect = true;                 // Auto-detect column names
        bool case_insensitive = true;            // Case-insensitive matching
    } params;
    
    PandasData();
    virtual ~PandasData() = default;
    
    // Feed interface  
    void start() override;
    bool load() override;
    
private:
    // Column indices after mapping
    int datetime_idx_;
    int open_idx_;
    int high_idx_;
    int low_idx_;
    int close_idx_;
    int volume_idx_;
    int openinterest_idx_;
    
    // Data iteration
    std::unique_ptr<DataFrame::RowIterator> row_iterator_;
    
    // Helper methods
    void map_columns();
    int find_column_index(const std::string& column_name);
    int auto_detect_datetime_column();
    int auto_detect_column(const std::vector<std::string>& possible_names);
    std::string to_lower(const std::string& str);
    bool column_matches(const std::string& actual, const std::string& expected);
};

} // namespace backtrader