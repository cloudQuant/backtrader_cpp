#include "../../include/utils/flushfile.h"
#include <iostream>
#include <stdexcept>

namespace backtrader {
namespace utils {

FlushFile::FlushFile(const std::string& filename, std::ios_base::openmode mode, bool auto_flush)
    : filename_(filename), auto_flush_(auto_flush), is_open_(false) {
    open(filename, mode);
}

FlushFile::FlushFile(std::ostream& stream, bool auto_flush)
    : stream_(&stream), auto_flush_(auto_flush), is_open_(true), owns_stream_(false) {}

FlushFile::~FlushFile() {
    close();
}

void FlushFile::open(const std::string& filename, std::ios_base::openmode mode) {
    close(); // Close any existing file
    
    filename_ = filename;
    file_stream_ = std::make_unique<std::ofstream>(filename, mode);
    
    if (!file_stream_->is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    stream_ = file_stream_.get();
    is_open_ = true;
    owns_stream_ = true;
}

void FlushFile::close() {
    if (is_open_ && owns_stream_ && file_stream_) {
        flush();
        file_stream_->close();
        file_stream_.reset();
    }
    
    is_open_ = false;
    stream_ = nullptr;
}

bool FlushFile::is_open() const {
    return is_open_ && stream_ != nullptr;
}

void FlushFile::flush() {
    if (is_open() && stream_) {
        stream_->flush();
    }
}

void FlushFile::write(const std::string& data) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << data;
    
    if (auto_flush_) {
        flush();
    }
}

void FlushFile::write(const char* data) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << data;
    
    if (auto_flush_) {
        flush();
    }
}

void FlushFile::writeline(const std::string& line) {
    write(line + "\n");
}

void FlushFile::writelines(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        writeline(line);
    }
}

FlushFile& FlushFile::operator<<(const std::string& data) {
    write(data);
    return *this;
}

FlushFile& FlushFile::operator<<(const char* data) {
    write(data);
    return *this;
}

FlushFile& FlushFile::operator<<(int value) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << value;
    
    if (auto_flush_) {
        flush();
    }
    
    return *this;
}

FlushFile& FlushFile::operator<<(double value) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << value;
    
    if (auto_flush_) {
        flush();
    }
    
    return *this;
}

FlushFile& FlushFile::operator<<(long long value) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << value;
    
    if (auto_flush_) {
        flush();
    }
    
    return *this;
}

FlushFile& FlushFile::operator<<(bool value) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << (value ? "true" : "false");
    
    if (auto_flush_) {
        flush();
    }
    
    return *this;
}

FlushFile& FlushFile::operator<<(std::ostream& (*manip)(std::ostream&)) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open for writing");
    }
    
    *stream_ << manip;
    
    if (auto_flush_) {
        flush();
    }
    
    return *this;
}

void FlushFile::set_auto_flush(bool auto_flush) {
    auto_flush_ = auto_flush;
}

bool FlushFile::get_auto_flush() const {
    return auto_flush_;
}

std::string FlushFile::get_filename() const {
    return filename_;
}

std::streamsize FlushFile::tellp() {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open");
    }
    
    if (auto* ofstream = dynamic_cast<std::ofstream*>(stream_)) {
        return ofstream->tellp();
    }
    
    return -1;
}

void FlushFile::seekp(std::streampos pos) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open");
    }
    
    if (auto* ofstream = dynamic_cast<std::ofstream*>(stream_)) {
        ofstream->seekp(pos);
    }
}

void FlushFile::seekp(std::streamoff off, std::ios_base::seekdir dir) {
    if (!is_open() || !stream_) {
        throw std::runtime_error("File is not open");
    }
    
    if (auto* ofstream = dynamic_cast<std::ofstream*>(stream_)) {
        ofstream->seekp(off, dir);
    }
}

bool FlushFile::good() const {
    return is_open() && stream_ && stream_->good();
}

bool FlushFile::eof() const {
    return !is_open() || !stream_ || stream_->eof();
}

bool FlushFile::fail() const {
    return !is_open() || !stream_ || stream_->fail();
}

bool FlushFile::bad() const {
    return !is_open() || !stream_ || stream_->bad();
}

void FlushFile::clear() {
    if (is_open() && stream_) {
        stream_->clear();
    }
}

// Static factory methods for common use cases
std::unique_ptr<FlushFile> FlushFile::create_stdout_wrapper(bool auto_flush) {
    return std::make_unique<FlushFile>(std::cout, auto_flush);
}

std::unique_ptr<FlushFile> FlushFile::create_stderr_wrapper(bool auto_flush) {
    return std::make_unique<FlushFile>(std::cerr, auto_flush);
}

std::unique_ptr<FlushFile> FlushFile::create_file(const std::string& filename, 
                                                 std::ios_base::openmode mode, 
                                                 bool auto_flush) {
    return std::make_unique<FlushFile>(filename, mode, auto_flush);
}

std::unique_ptr<FlushFile> FlushFile::create_append_file(const std::string& filename, 
                                                        bool auto_flush) {
    return create_file(filename, std::ios::out | std::ios::app, auto_flush);
}

std::unique_ptr<FlushFile> FlushFile::create_truncate_file(const std::string& filename, 
                                                          bool auto_flush) {
    return create_file(filename, std::ios::out | std::ios::trunc, auto_flush);
}

// Buffered writer implementation
BufferedWriter::BufferedWriter(std::unique_ptr<FlushFile> file, size_t buffer_size)
    : file_(std::move(file)), buffer_size_(buffer_size), buffer_pos_(0) {
    buffer_.reserve(buffer_size);
}

BufferedWriter::~BufferedWriter() {
    flush();
}

void BufferedWriter::write(const std::string& data) {
    if (buffer_pos_ + data.size() > buffer_size_) {
        flush();
    }
    
    buffer_ += data;
    buffer_pos_ += data.size();
}

void BufferedWriter::writeline(const std::string& line) {
    write(line + "\n");
}

void BufferedWriter::flush() {
    if (buffer_pos_ > 0 && file_) {
        file_->write(buffer_);
        file_->flush();
        buffer_.clear();
        buffer_pos_ = 0;
    }
}

bool BufferedWriter::is_open() const {
    return file_ && file_->is_open();
}

void BufferedWriter::close() {
    flush();
    if (file_) {
        file_->close();
    }
}

size_t BufferedWriter::buffer_size() const {
    return buffer_size_;
}

size_t BufferedWriter::buffered_bytes() const {
    return buffer_pos_;
}

} // namespace utils
} // namespace backtrader