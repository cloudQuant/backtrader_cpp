#pragma once

#include "../feed.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace backtrader {

// Backtrader CSV Data Feed
class BacktraderCSVData : public CSVDataBase {
public:
    struct Params {
        std::string dataname;           // CSV file path
        std::string separator = ",";    // CSV separator
        bool headers = true;            // Has header line
        std::string datetime_format = "%Y-%m-%d";  // Date format
        std::string time_format = "%H:%M:%S";      // Time format
        
        // Column indices (0-based)
        int datetime_col = 0;
        int time_col = 1;      // -1 if no time column
        int open_col = 2;
        int high_col = 3;
        int low_col = 4;
        int close_col = 5;
        int volume_col = 6;
        int openinterest_col = 7;
        
        // Session time (used when no time column)
        std::string sessionend = "17:00:00";
    } params;
    
    BacktraderCSVData();
    virtual ~BacktraderCSVData();
    
    // Override base methods
    bool start() override;
    bool stop() override;
    bool _load() override;
    
private:
    std::ifstream file_;
    std::vector<std::string> current_line_tokens_;
    
    // Helper methods
    bool load_line(const std::vector<std::string>& line_tokens);
    std::vector<std::string> split_line(const std::string& line);
    double parse_datetime(const std::string& date_str, const std::string& time_str = "");
    bool parse_date_time(const std::string& date_str, const std::string& time_str,
                        int& year, int& month, int& day, int& hour, int& minute, int& second);
};

} // namespace backtrader