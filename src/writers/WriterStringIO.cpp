#include "writers/WriterStringIO.h"
#include <sstream>
#include <algorithm>

namespace backtrader {
namespace writers {

WriterStringIO::WriterStringIO() : WriterFile() {
    // Set the output stream to our string stream
    set_output_stream(&string_stream_);
    // Also set CSV format by default
    params.csv = true;
}

void WriterStringIO::setCSVFormat(bool csv) {
    params.csv = csv;
}

void WriterStringIO::start() {
    // Clear any previous output
    clear();
    
    // Don't call start_output() which would reset the stream
    // Instead, manually do what parent start() does without calling start_output()
    line_counter_ = 0;
    
    if (params.csv) {
        // Write opening separator
        write_separator(0, '=');
        
        collect_csv_headers();
        write_csv_header();
    }
    
    // Capture initial output (like headers)
    captureOutput();
}

void WriterStringIO::stop() {
    // Call parent stop
    WriterFile::stop();
    
    // Capture any final output
    captureOutput();
}

void WriterStringIO::next() {
    // Call parent next to write data
    WriterFile::next();
    
    // Capture the output
    captureOutput();
}

void WriterStringIO::captureOutput() {
    // Get current content from string stream
    std::string current_content = string_stream_.str();
    
    // Debug
    //std::cerr << "Debug: captureOutput() - string_stream_ content length: " << current_content.length() << std::endl;
    
    // Split into lines and add to output_lines_
    std::istringstream iss(current_content);
    std::string line;
    
    // Clear existing lines and re-parse all content
    output_lines_.clear();
    while (std::getline(iss, line)) {
        output_lines_.push_back(line);
    }
    
    // If there are any incomplete lines at the end, add them too
    if (!current_content.empty() && current_content.back() != '\n') {
        // Get the position of the last newline
        size_t last_newline = current_content.find_last_of('\n');
        if (last_newline != std::string::npos) {
            std::string last_line = current_content.substr(last_newline + 1);
            if (!last_line.empty()) {
                output_lines_.push_back(last_line);
            }
        }
    }
    
    //std::cerr << "Debug: captureOutput() - lines captured: " << output_lines_.size() << std::endl;
}

} // namespace writers
} // namespace backtrader