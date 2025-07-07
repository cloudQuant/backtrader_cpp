#pragma once

#include "../writer.h"
#include <sstream>
#include <vector>
#include <string>

namespace backtrader {
namespace writers {

// WriterStringIO - 写入到字符串缓冲区的Writer
class WriterStringIO : public WriterBase {
public:
    WriterStringIO() : csv_format_(false) {}
    virtual ~WriterStringIO() = default;
    
    // 设置CSV格式
    void setCSVFormat(bool csv) { csv_format_ = csv; }
    bool isCSVFormat() const { return csv_format_; }
    
    // 获取输出内容
    const std::vector<std::string>& getOutput() const { return output_lines_; }
    
    // 写入一行
    void writeLine(const std::string& line) {
        output_lines_.push_back(line);
    }
    
    // 其他配置选项
    void setIncludeTimestamp(bool include) { include_timestamp_ = include; }
    bool getIncludeTimestamp() const { return include_timestamp_; }
    
    void setIncludeIndicators(bool include) { include_indicators_ = include; }
    bool getIncludeIndicators() const { return include_indicators_; }
    
    // 清空输出
    void clear() { output_lines_.clear(); }
    
private:
    bool csv_format_;
    bool include_timestamp_ = false;
    bool include_indicators_ = false;
    std::vector<std::string> output_lines_;
};

} // namespace writers
} // namespace backtrader