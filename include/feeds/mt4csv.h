#pragma once

#include "../feed.h"
#include <fstream>
#include <string>
#include <vector>

namespace backtrader {
namespace feeds {

/**
 * MT4CSVFeed - MetaTrader 4 CSV data feed
 * 
 * Reads CSV files exported from MetaTrader 4 platform.
 * Handles the specific MT4 CSV format and timeframe conventions.
 */
class MT4CSVFeed : public AbstractDataBase {
public:
    struct Params {
        std::string filename;
        char separator = ',';
        bool reverse = false;          // Reverse chronological order
        std::string dtformat = "%Y.%m.%d %H:%M";  // MT4 datetime format
        std::string timeframe;         // M1, M5, M15, M30, H1, H4, D1, etc.
        bool adjust_dst = false;       // Adjust for daylight saving time
        int gmt_offset = 0;           // GMT offset in hours
    };

    MT4CSVFeed(const Params& params);
    virtual ~MT4CSVFeed() = default;

    void start() override;
    bool next() override;
    void preload() override;

private:
    Params params_;
    std::ifstream file_;
    std::vector<std::vector<std::string>> data_;
    size_t current_index_ = 0;
    
    void parse_csv_line(const std::string& line, std::vector<std::string>& fields);
    std::chrono::system_clock::time_point parse_mt4_datetime(const std::string& dt_str);
    void process_mt4_data();
};

} // namespace feeds
} // namespace backtrader