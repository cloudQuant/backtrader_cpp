#pragma once

#include "LineRoot.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace backtrader {

/**
 * @brief CSV Data Feed for loading historical price data
 * 
 * Loads OHLCV data from CSV files with configurable column mapping.
 * Supports various date formats and handles missing data.
 */
class CSVDataFeed {
public:
    /**
     * @brief Data structure for a single bar
     */
    struct DataBar {
        std::string datetime;
        double open = NaN;
        double high = NaN;
        double low = NaN;
        double close = NaN;
        double volume = NaN;
        double adj_close = NaN;
        
        bool isValid() const {
            return !isNaN(open) && !isNaN(high) && !isNaN(low) && !isNaN(close);
        }
    };
    
    /**
     * @brief Configuration for CSV parsing
     */
    struct CSVConfig {
        std::string separator = ",";
        bool has_header = true;
        int datetime_col = 0;
        int open_col = 1;
        int high_col = 2;
        int low_col = 3;
        int close_col = 4;
        int volume_col = 5;
        int adj_close_col = -1;  // -1 means not present
        
        // Date format settings
        std::string date_format = "%Y-%m-%d";
        bool reverse_order = false;  // If true, reverse data order (oldest first)
        
        // Data validation
        bool skip_invalid_rows = true;
        double min_price = 0.001;
        double max_price = 1000000.0;
    };

private:
    std::string filename_;
    CSVConfig config_;
    std::vector<DataBar> data_;
    size_t current_index_;
    bool loaded_;
    
    // LineRoot objects for each data series
    std::shared_ptr<LineRoot> datetime_line_;
    std::shared_ptr<LineRoot> open_line_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
    std::shared_ptr<LineRoot> volume_line_;
    std::shared_ptr<LineRoot> adj_close_line_;

public:
    explicit CSVDataFeed(const std::string& filename);
    explicit CSVDataFeed(const std::string& filename, const CSVConfig& config);
    
    /**
     * @brief Load data from CSV file
     */
    bool load();
    
    /**
     * @brief Get number of data points
     */
    size_t size() const { return data_.size(); }
    
    /**
     * @brief Check if data is loaded
     */
    bool isLoaded() const { return loaded_; }
    
    /**
     * @brief Get current data index
     */
    size_t getCurrentIndex() const { return current_index_; }
    
    /**
     * @brief Move to next data point
     */
    bool next();
    
    /**
     * @brief Reset to beginning
     */
    void reset();
    
    /**
     * @brief Get current data bar
     */
    const DataBar& getCurrentBar() const;
    
    /**
     * @brief Get data bar at specific index
     */
    const DataBar& getBar(size_t index) const;
    
    /**
     * @brief Get all data
     */
    const std::vector<DataBar>& getAllData() const { return data_; }
    
    // Line accessors
    std::shared_ptr<LineRoot> getOpenLine() const { return open_line_; }
    std::shared_ptr<LineRoot> getHighLine() const { return high_line_; }
    std::shared_ptr<LineRoot> getLowLine() const { return low_line_; }
    std::shared_ptr<LineRoot> getCloseLine() const { return close_line_; }
    std::shared_ptr<LineRoot> getVolumeLine() const { return volume_line_; }
    std::shared_ptr<LineRoot> getAdjCloseLine() const { return adj_close_line_; }
    
    /**
     * @brief Get data statistics
     */
    struct DataStats {
        size_t total_rows = 0;
        size_t valid_rows = 0;
        size_t invalid_rows = 0;
        double min_price = 0.0;
        double max_price = 0.0;
        double avg_volume = 0.0;
        std::string start_date;
        std::string end_date;
    };
    
    DataStats getStats() const;
    
    /**
     * @brief Export data to CSV (useful for cleaned data)
     */
    bool exportToCSV(const std::string& output_filename) const;

private:
    bool parseCSVLine(const std::string& line, size_t line_number);
    std::vector<std::string> splitLine(const std::string& line, const std::string& separator) const;
    double parseDouble(const std::string& str) const;
    bool validateBar(const DataBar& bar) const;
    void initializeLines();
    void updateLines();
};

} // namespace backtrader