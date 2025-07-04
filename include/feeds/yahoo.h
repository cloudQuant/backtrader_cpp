#pragma once

#include "../feed.h"
#include "csvgeneric.h"
#include <string>
#include <vector>
#include <memory>

namespace backtrader {

// YahooFinanceCSVData - parses pre-downloaded Yahoo CSV data feeds
class YahooFinanceCSVData : public CSVDataBase {
public:
    struct Params {
        std::string dataname;           // Filename to parse or file-like object
        bool reverse = false;           // Whether data is in reverse order
        bool adjclose = true;           // Use dividend/split adjusted close
        bool adjvolume = true;          // Adjust volume if adjclose is true
        bool round = true;              // Round values after adjustment
        bool roundvolume = false;       // Round volume after adjustment
        int decimals = 2;               // Number of decimals to round to
        int roundvolume_decimals = 0;   // Decimals for volume rounding
        bool swapcloses = false;        // Swap close and adjusted close columns
        
        // CSV format parameters
        std::string separator = ",";    // CSV separator
        bool headers = true;            // Has header line
        std::string datetime_format = "%Y-%m-%d";
        
        // Column indices for Yahoo format
        int datetime_idx = 0;           // Date column
        int open_idx = 1;               // Open column
        int high_idx = 2;               // High column
        int low_idx = 3;                // Low column
        int close_idx = 4;              // Close column
        int adjclose_idx = 5;           // Adjusted Close column
        int volume_idx = 6;             // Volume column
        int openinterest_idx = -1;      // Not available in Yahoo format
    } params;
    
    YahooFinanceCSVData();
    virtual ~YahooFinanceCSVData() = default;
    
    // Feed interface
    void start() override;
    bool load() override;
    
    // Get adjusted close line
    const std::vector<double>& get_adjclose_line() const { return adjclose_line_; }
    
private:
    // Additional line for adjusted close
    std::vector<double> adjclose_line_;
    
    // Raw data storage
    std::vector<std::vector<std::string>> raw_data_;
    bool data_loaded_;
    size_t current_row_;
    
    // Helper methods
    bool load_csv_file();
    void reverse_data_if_needed();
    bool parse_row(const std::vector<std::string>& row);
    void apply_adjustments();
    double calculate_adjustment_factor(double close_price, double adj_close_price);
    double round_to_decimals(double value, int decimals);
};

// YahooLegacyCSV - legacy Yahoo CSV format support
class YahooLegacyCSV : public YahooFinanceCSVData {
public:
    YahooLegacyCSV() {
        // Legacy format specific settings
        params.swapcloses = false;
    }
    virtual ~YahooLegacyCSV() = default;
};

// YahooFinanceData - direct Yahoo Finance data download
class YahooFinanceData : public YahooFinanceCSVData {
public:
    struct DownloadParams {
        std::string symbol;             // Stock symbol (e.g., "AAPL")
        std::string fromdate;           // Start date (YYYY-MM-DD)
        std::string todate;             // End date (YYYY-MM-DD)
        std::string period1;            // Unix timestamp start
        std::string period2;            // Unix timestamp end
        std::string interval = "1d";    // Data interval (1d, 1wk, 1mo)
        bool events = false;            // Include dividend/split events
        std::string crumb;              // Authentication crumb
        bool buffered = true;           // Buffer downloaded data
        int retries = 3;                // Download retry attempts
    } download_params;
    
    YahooFinanceData();
    virtual ~YahooFinanceData() = default;
    
    // Feed interface
    void start() override;
    
private:
    // Download functionality
    bool download_data();
    std::string build_download_url();
    bool parse_downloaded_data(const std::string& csv_data);
    
    // Authentication
    std::string get_crumb();
    std::string extract_crumb_from_page(const std::string& page_content);
    
    // Date conversion
    std::string date_to_unix_timestamp(const std::string& date_str);
    
    // HTTP client functionality (simplified)
    std::string http_get(const std::string& url);
};

// YahooFinance - alias for YahooFinanceData
using YahooFinance = YahooFinanceData;

} // namespace backtrader