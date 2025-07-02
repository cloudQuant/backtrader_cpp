#pragma once

#include "DataFeed.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace backtrader {
namespace data {

/**
 * @brief CSV数据源
 * 
 * 从CSV文件读取OHLCV数据
 */
class CSVDataFeed : public DataFeed {
private:
    std::string filename_;
    std::ifstream file_;
    std::vector<OHLCVData> data_;
    size_t data_index_;
    
    // CSV格式配置
    std::string separator_;
    bool has_header_;
    std::vector<std::string> headers_;
    
    // 列索引映射
    struct ColumnMapping {
        int datetime_col = 0;
        int open_col = 1;
        int high_col = 2;
        int low_col = 3;
        int close_col = 4;
        int volume_col = 5;
        std::string datetime_format = "%Y-%m-%d %H:%M:%S";
    } column_mapping_;
    
public:
    /**
     * @brief 构造函数
     * @param filename CSV文件路径
     * @param name 数据源名称
     * @param separator 分隔符，默认逗号
     * @param has_header 是否有表头，默认true
     */
    explicit CSVDataFeed(const std::string& filename,
                        const std::string& name = "",
                        const std::string& separator = ",",
                        bool has_header = true)
        : DataFeed(name.empty() ? filename : name),
          filename_(filename),
          data_index_(0),
          separator_(separator),
          has_header_(has_header) {
        
        setStatus(DataStatus::HISTORICAL);
        
        // 设置参数
        setParam("filename", filename);
        setParam("separator", separator);
        setParam("has_header", has_header ? "true" : "false");
    }
    
    /**
     * @brief 析构函数
     */
    ~CSVDataFeed() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    /**
     * @brief 设置列映射
     * @param datetime_col 时间列索引
     * @param open_col 开盘价列索引
     * @param high_col 最高价列索引
     * @param low_col 最低价列索引
     * @param close_col 收盘价列索引
     * @param volume_col 成交量列索引
     */
    void setColumnMapping(int datetime_col, int open_col, int high_col,
                         int low_col, int close_col, int volume_col = -1) {
        column_mapping_.datetime_col = datetime_col;
        column_mapping_.open_col = open_col;
        column_mapping_.high_col = high_col;
        column_mapping_.low_col = low_col;
        column_mapping_.close_col = close_col;
        column_mapping_.volume_col = volume_col;
    }
    
    /**
     * @brief 设置时间格式
     * @param format 时间格式字符串
     */
    void setDateTimeFormat(const std::string& format) {
        column_mapping_.datetime_format = format;
    }
    
    /**
     * @brief 加载CSV文件
     * @return true if successful
     */
    bool load() {
        file_.open(filename_);
        if (!file_.is_open()) {
            return false;
        }
        
        data_.clear();
        std::string line;
        
        // 读取表头
        if (has_header_ && std::getline(file_, line)) {
            parseHeader(line);
        }
        
        // 读取数据行
        while (std::getline(file_, line)) {
            if (line.empty() || line[0] == '#') {
                continue; // 跳过空行和注释行
            }
            
            OHLCVData ohlcv;
            if (parseLine(line, ohlcv)) {
                data_.push_back(ohlcv);
            }
        }
        
        file_.close();
        data_index_ = 0;
        
        return !data_.empty();
    }
    
    /**
     * @brief 检查是否有更多数据
     * @return true if has more data
     */
    bool hasNext() const override {
        return data_index_ < data_.size();
    }
    
    /**
     * @brief 获取下一个数据点
     * @return true if successful
     */
    bool next() override {
        if (!hasNext()) {
            return false;
        }
        
        addData(data_[data_index_]);
        data_index_++;
        
        return true;
    }
    
    /**
     * @brief 重置到开始位置
     */
    void reset() override {
        DataFeed::reset();
        data_index_ = 0;
    }
    
    /**
     * @brief 获取总数据量
     * @return 数据量
     */
    size_t getTotalDataCount() const {
        return data_.size();
    }
    
    /**
     * @brief 获取文件名
     * @return 文件名
     */
    const std::string& getFileName() const {
        return filename_;
    }
    
    /**
     * @brief 获取表头
     * @return 表头向量
     */
    const std::vector<std::string>& getHeaders() const {
        return headers_;
    }
    
    /**
     * @brief 自动检测列映射（基于表头）
     * @return true if successful
     */
    bool autoDetectColumns() {
        if (!has_header_ || headers_.empty()) {
            return false;
        }
        
        // 常见的列名映射
        std::unordered_map<std::string, std::string> name_mapping = {
            {"date", "datetime"}, {"time", "datetime"}, {"datetime", "datetime"},
            {"timestamp", "datetime"}, {"dt", "datetime"},
            {"open", "open"}, {"o", "open"},
            {"high", "high"}, {"h", "high"}, {"max", "high"},
            {"low", "low"}, {"l", "low"}, {"min", "low"},
            {"close", "close"}, {"c", "close"}, {"price", "close"},
            {"volume", "volume"}, {"vol", "volume"}, {"v", "volume"}
        };
        
        // 重置列映射
        column_mapping_.datetime_col = -1;
        column_mapping_.open_col = -1;
        column_mapping_.high_col = -1;
        column_mapping_.low_col = -1;
        column_mapping_.close_col = -1;
        column_mapping_.volume_col = -1;
        
        // 查找列映射
        for (size_t i = 0; i < headers_.size(); ++i) {
            std::string header = toLower(trim(headers_[i]));
            
            auto it = name_mapping.find(header);
            if (it != name_mapping.end()) {
                const std::string& target = it->second;
                
                if (target == "datetime" && column_mapping_.datetime_col == -1) {
                    column_mapping_.datetime_col = static_cast<int>(i);
                } else if (target == "open" && column_mapping_.open_col == -1) {
                    column_mapping_.open_col = static_cast<int>(i);
                } else if (target == "high" && column_mapping_.high_col == -1) {
                    column_mapping_.high_col = static_cast<int>(i);
                } else if (target == "low" && column_mapping_.low_col == -1) {
                    column_mapping_.low_col = static_cast<int>(i);
                } else if (target == "close" && column_mapping_.close_col == -1) {
                    column_mapping_.close_col = static_cast<int>(i);
                } else if (target == "volume" && column_mapping_.volume_col == -1) {
                    column_mapping_.volume_col = static_cast<int>(i);
                }
            }
        }
        
        // 检查必需列是否找到
        return column_mapping_.datetime_col >= 0 && 
               column_mapping_.open_col >= 0 &&
               column_mapping_.high_col >= 0 &&
               column_mapping_.low_col >= 0 &&
               column_mapping_.close_col >= 0;
    }
    
    /**
     * @brief 验证CSV数据
     * @return 验证结果
     */
    bool validateCSV() const {
        if (data_.empty()) {
            return false;
        }
        
        // 检查数据完整性
        for (const auto& ohlcv : data_) {
            if (!ohlcv.isValid()) {
                return false;
            }
        }
        
        // 检查时间序列单调性
        for (size_t i = 1; i < data_.size(); ++i) {
            if (data_[i].datetime <= data_[i-1].datetime) {
                return false; // 时间不是单调递增的
            }
        }
        
        return true;
    }
    
    /**
     * @brief 获取数据统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const {
        if (data_.empty()) {
            return "No data available";
        }
        
        double min_price = data_[0].low;
        double max_price = data_[0].high;
        double total_volume = 0.0;
        
        for (const auto& ohlcv : data_) {
            min_price = std::min(min_price, ohlcv.low);
            max_price = std::max(max_price, ohlcv.high);
            if (isFinite(ohlcv.volume)) {
                total_volume += ohlcv.volume;
            }
        }
        
        std::ostringstream oss;
        oss << "Data points: " << data_.size() << "\n";
        oss << "Price range: " << min_price << " - " << max_price << "\n";
        oss << "Total volume: " << total_volume << "\n";
        oss << "Start time: " << formatDateTime(data_.front().datetime) << "\n";
        oss << "End time: " << formatDateTime(data_.back().datetime);
        
        return oss.str();
    }
    
private:
    /**
     * @brief 解析表头行
     * @param line 表头行
     */
    void parseHeader(const std::string& line) {
        headers_ = split(line, separator_);
        
        // 清理表头（去除空格和引号）
        for (auto& header : headers_) {
            header = trim(header);
            if (header.front() == '"' && header.back() == '"') {
                header = header.substr(1, header.length() - 2);
            }
        }
    }
    
    /**
     * @brief 解析数据行
     * @param line 数据行
     * @param ohlcv 输出的OHLCV数据
     * @return true if successful
     */
    bool parseLine(const std::string& line, OHLCVData& ohlcv) {
        auto fields = split(line, separator_);
        
        // 检查字段数量
        int required_fields = std::max({column_mapping_.datetime_col,
                                       column_mapping_.open_col,
                                       column_mapping_.high_col,
                                       column_mapping_.low_col,
                                       column_mapping_.close_col}) + 1;
        
        if (static_cast<int>(fields.size()) < required_fields) {
            return false;
        }
        
        try {
            // 解析时间
            ohlcv.datetime = parseDateTime(trim(fields[column_mapping_.datetime_col]));
            
            // 解析价格数据
            ohlcv.open = std::stod(trim(fields[column_mapping_.open_col]));
            ohlcv.high = std::stod(trim(fields[column_mapping_.high_col]));
            ohlcv.low = std::stod(trim(fields[column_mapping_.low_col]));
            ohlcv.close = std::stod(trim(fields[column_mapping_.close_col]));
            
            // 解析成交量（可选）
            if (column_mapping_.volume_col >= 0 && 
                column_mapping_.volume_col < static_cast<int>(fields.size())) {
                std::string volume_str = trim(fields[column_mapping_.volume_col]);
                if (!volume_str.empty()) {
                    ohlcv.volume = std::stod(volume_str);
                } else {
                    ohlcv.volume = NaN;
                }
            } else {
                ohlcv.volume = NaN;
            }
            
            return ohlcv.isValid();
            
        } catch (const std::exception&) {
            return false;
        }
    }
    
    /**
     * @brief 解析时间字符串
     * @param datetime_str 时间字符串
     * @return 时间点
     */
    std::chrono::system_clock::time_point parseDateTime(const std::string& datetime_str) {
        // 简化的时间解析，支持常见格式
        std::tm tm = {};
        
        // 尝试常见格式
        std::vector<std::string> formats = {
            "%Y-%m-%d %H:%M:%S",
            "%Y-%m-%d",
            "%m/%d/%Y %H:%M:%S",
            "%m/%d/%Y",
            "%d/%m/%Y %H:%M:%S",
            "%d/%m/%Y"
        };
        
        for (const auto& format : formats) {
            std::istringstream ss(datetime_str);
            ss >> std::get_time(&tm, format.c_str());
            
            if (!ss.fail()) {
                auto time_t = std::mktime(&tm);
                return std::chrono::system_clock::from_time_t(time_t);
            }
        }
        
        // 如果解析失败，使用当前时间
        return std::chrono::system_clock::now();
    }
    
    /**
     * @brief 格式化时间点
     * @param tp 时间点
     * @return 格式化字符串
     */
    std::string formatDateTime(const std::chrono::system_clock::time_point& tp) const {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    
    /**
     * @brief 分割字符串
     * @param str 输入字符串
     * @param delimiter 分隔符
     * @return 分割结果
     */
    std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = str.find(delimiter);
        
        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        
        result.push_back(str.substr(start));
        return result;
    }
    
    /**
     * @brief 去除字符串两端空格
     * @param str 输入字符串
     * @return 处理后字符串
     */
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
    
    /**
     * @brief 转换为小写
     * @param str 输入字符串
     * @return 小写字符串
     */
    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
};

} // namespace data
} // namespace backtrader