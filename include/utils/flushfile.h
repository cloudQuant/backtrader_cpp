#pragma once

#include <fstream>
#include <string>
#include <memory>

namespace backtrader {
namespace utils {

/**
 * FlushFile - File wrapper that ensures data is flushed after each write
 * 
 * Provides automatic flushing functionality for file output operations.
 * Useful for logging and real-time data output where immediate persistence is required.
 */
class FlushFile {
public:
    // Constructor with filename
    explicit FlushFile(const std::string& filename, 
                      std::ios::openmode mode = std::ios::out);
    
    // Constructor with existing stream
    explicit FlushFile(std::unique_ptr<std::ostream> stream);
    
    // Destructor
    ~FlushFile();
    
    // Delete copy constructor and assignment operator
    FlushFile(const FlushFile&) = delete;
    FlushFile& operator=(const FlushFile&) = delete;
    
    // Move constructor and assignment operator
    FlushFile(FlushFile&& other) noexcept;
    FlushFile& operator=(FlushFile&& other) noexcept;
    
    // Write operations
    FlushFile& write(const std::string& data);
    FlushFile& write(const char* data);
    FlushFile& writeln(const std::string& data);
    
    // Stream operator
    template<typename T>
    FlushFile& operator<<(const T& data) {
        if (stream_) {
            *stream_ << data;
            stream_->flush();
        }
        return *this;
    }
    
    // File operations
    void flush();
    void close();
    bool is_open() const;
    
    // Get underlying stream
    std::ostream* get_stream() const;

private:
    std::unique_ptr<std::ostream> stream_;
    std::unique_ptr<std::ofstream> file_stream_;  // Only used when we own the file
    bool owns_stream_;
    
    void ensure_flushed();
};

/**
 * FlushLogger - Simple logging utility with automatic flushing
 * 
 * Provides thread-safe logging with automatic flushing and timestamps.
 */
class FlushLogger {
public:
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };
    
    explicit FlushLogger(const std::string& filename);
    explicit FlushLogger(std::unique_ptr<std::ostream> stream);
    
    ~FlushLogger() = default;
    
    // Logging methods
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    // Generic log method
    void log(LogLevel level, const std::string& message);
    
    // Set minimum log level
    void set_log_level(LogLevel level);
    
    // Enable/disable timestamps
    void set_timestamps(bool enabled);

private:
    FlushFile file_;
    LogLevel min_level_;
    bool timestamps_enabled_;
    std::mutex mutex_;  // For thread safety
    
    std::string get_timestamp() const;
    std::string level_to_string(LogLevel level) const;
    void write_log_entry(LogLevel level, const std::string& message);
};

} // namespace utils
} // namespace backtrader