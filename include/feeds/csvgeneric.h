#pragma once

#include "../feed.h"
#include <string>
#include <vector>
#include <fstream>

namespace backtrader {

// Generic CSV Data Feed with configurable column mapping
class GenericCSVData : public CSVDataBase {
public:
    struct Params {
        std::string dataname;           // CSV file path
        std::string separator = ",";    // CSV separator
        bool headers = true;            // Has header line
        
        // Column indices (-1 means field not present)
        int datetime_idx = 0;
        int time_idx = -1;              // Separate time column
        int open_idx = 1;
        int high_idx = 2;
        int low_idx = 3;
        int close_idx = 4;
        int volume_idx = 5;
        int openinterest_idx = 6;
        
        // Date/time format
        std::string dtformat = "%Y-%m-%d";  // strptime format
        std::string tmformat = "%H:%M:%S";  // time format if separate
        
        // Unix timestamp mode (1=int seconds, 2=float seconds)
        int unix_timestamp = 0;
        
        // Missing value replacement
        double nullvalue = 0.0;
        
        // Reverse chronological order
        bool reverse = false;
        
        // Field names for flexible mapping
        std::string datetime_name = "datetime";
        std::string open_name = "open";
        std::string high_name = "high";
        std::string low_name = "low";
        std::string close_name = "close";
        std::string volume_name = "volume";
        std::string openinterest_name = "openinterest";
    } params;
    
    GenericCSVData();
    virtual ~GenericCSVData();
    
    // Override base methods
    bool start() override;
    bool stop() override;
    bool _load() override;
    
private:
    std::ifstream file_;
    std::vector<std::vector<std::string>> all_lines_;  // For reverse mode
    size_t current_line_index_;
    bool use_reverse_mode_;
    
    // Helper methods
    bool load_line(const std::vector<std::string>& line_tokens);
    std::vector<std::string> split_line(const std::string& line);
    double parse_datetime(const std::string& date_str, const std::string& time_str = "");
    double parse_unix_timestamp(const std::string& timestamp_str);
    bool parse_field(const std::vector<std::string>& tokens, int field_idx, double& value);
    void map_headers_to_indices(const std::vector<std::string>& headers);
};

} // namespace backtrader