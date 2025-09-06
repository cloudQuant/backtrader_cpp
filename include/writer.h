#pragma once

#include "metabase.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>

namespace backtrader {

// Forward declarations
class DataSeries;
class Strategy;
class Observer;
class Indicator;

// Base writer class
class WriterBase {
public:
    virtual ~WriterBase() = default;
    
    // Virtual methods that derived classes can override
    virtual void start() {}
    virtual void stop() {}
    virtual void next() {}
};

// File writer class for output and logging
class WriterFile : public WriterBase {
public:
    struct Params {
        std::string out_filename = "";          // Output filename (empty means stdout)
        bool close_out = false;                 // Whether to close output stream
        bool csv = false;                       // Whether to output CSV format
        std::string csv_sep = ",";              // CSV separator
        bool csv_filternan = true;              // Filter NaN values in CSV
        bool csv_counter = true;                // Keep line counter
        int indent = 2;                         // Indentation spaces
        std::vector<char> separators = {'=', '-', '+', '*', '.', '~', '"', '^', '#'};
        int seplen = 79;                        // Separator line length
        int rounding = -1;                      // Decimal places (-1 means no rounding)
    } params;
    
    WriterFile();
    virtual ~WriterFile();
    
    // Main interface
    void start() override;
    void stop() override;
    void next() override;
    
    // CSV output
    void write_csv_header();
    void write_csv_data();
    
    // Text output
    void write_line(const std::string& line, int level = 0);
    void write_separator(int level = 0, char sep_char = '=');
    void write_section(const std::string& title, int level = 0);
    
    // Data registration
    void register_data(std::shared_ptr<DataSeries> data);
    void register_strategy(std::shared_ptr<Strategy> strategy);
    void register_observer(std::shared_ptr<Observer> observer);
    void register_indicator(std::shared_ptr<Indicator> indicator);
    
    // Output stream management
    void set_output_stream(std::ostream* stream);
    void set_output_file(const std::string& filename);
    
protected:
    // Output stream
    std::ostream* out_stream_;
    std::unique_ptr<std::ofstream> file_stream_;
    bool should_close_;
    
    // Counters
    size_t line_counter_;
    
    // Headers and values for CSV
    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> values_;
    
    // Registered objects
    std::vector<std::shared_ptr<DataSeries>> datas_;
    std::vector<std::shared_ptr<Strategy>> strategies_;
    std::vector<std::shared_ptr<Observer>> observers_;
    std::vector<std::shared_ptr<Indicator>> indicators_;
    
    // Helper methods
    void start_output();
    void close_output();
    std::string format_value(double value) const;
    std::string create_separator(int level, char sep_char) const;
    std::string create_indent(int level) const;
    
    // CSV helpers
    void collect_csv_headers();
    void collect_csv_values();
    bool should_include_in_csv(const std::string& object_type) const;
};

// Factory function
std::shared_ptr<WriterFile> create_writer(const std::string& filename = "");

} // namespace backtrader