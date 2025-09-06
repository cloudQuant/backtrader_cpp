#pragma once

#include "../writer.h"
#include <sstream>
#include <vector>
#include <string>
#include <memory>

namespace backtrader {
namespace writers {

// WriterStringIO - 写入到字符串缓冲区的Writer
class WriterStringIO : public WriterFile {
public:
    WriterStringIO();
    virtual ~WriterStringIO() = default;
    
    // 设置CSV格式
    void setCSVFormat(bool csv);
    bool isCSVFormat() const { return params.csv; }
    
    // 获取输出内容
    const std::vector<std::string>& getOutput() const { return output_lines_; }
    
    // 其他配置选项
    void setIncludeTimestamp(bool include) { include_timestamp_ = include; }
    bool getIncludeTimestamp() const { return include_timestamp_; }
    
    void setIncludeIndicators(bool include) { include_indicators_ = include; }
    bool getIncludeIndicators() const { return include_indicators_; }
    
    // 清空输出
    void clear() { 
        output_lines_.clear(); 
        string_stream_.str("");
        string_stream_.clear();
    }
    
    // Override start to capture output
    void start() override;
    void stop() override;
    void next() override;
    
private:
    std::ostringstream string_stream_;
    std::vector<std::string> output_lines_;
    bool include_timestamp_ = false;
    bool include_indicators_ = false;
    
    // Capture lines written to the stream
    void captureOutput();
};

} // namespace writers
} // namespace backtrader