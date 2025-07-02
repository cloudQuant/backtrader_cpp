#pragma once

#include "LineRoot.h"
#include "Common.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace backtrader {
namespace data {

/**
 * @brief OHLCV数据结构
 */
struct OHLCVData {
    std::chrono::system_clock::time_point datetime;
    double open;
    double high;
    double low;
    double close;
    double volume;
    
    OHLCVData() : open(NaN), high(NaN), low(NaN), close(NaN), volume(NaN) {}
    
    OHLCVData(const std::chrono::system_clock::time_point& dt,
              double o, double h, double l, double c, double v = NaN)
        : datetime(dt), open(o), high(h), low(l), close(c), volume(v) {}
    
    bool isValid() const {
        return isFinite(open) && isFinite(high) && isFinite(low) && isFinite(close) &&
               high >= low && high >= open && high >= close && 
               low <= open && low <= close;
    }
};

/**
 * @brief 数据源状态枚举
 */
enum class DataStatus {
    LIVE,        // 实时数据
    DELAYED,     // 延迟数据
    HISTORICAL,  // 历史数据
    DISCONNECTED // 断开连接
};

/**
 * @brief 数据源基类
 * 
 * 提供统一的数据接口，支持历史数据和实时数据
 */
class DataFeed {
protected:
    std::string name_;
    DataStatus status_;
    size_t current_index_;
    
    // OHLCV数据线
    std::shared_ptr<LineRoot> datetime_line_;
    std::shared_ptr<LineRoot> open_line_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
    std::shared_ptr<LineRoot> volume_line_;
    
    // 参数
    std::unordered_map<std::string, std::string> params_;
    
public:
    /**
     * @brief 构造函数
     * @param name 数据源名称
     */
    explicit DataFeed(const std::string& name = "DataFeed")
        : name_(name), status_(DataStatus::DISCONNECTED), current_index_(0) {
        
        // 初始化数据线
        datetime_line_ = std::make_shared<LineRoot>(10000, name + "_datetime");
        open_line_ = std::make_shared<LineRoot>(10000, name + "_open");
        high_line_ = std::make_shared<LineRoot>(10000, name + "_high");
        low_line_ = std::make_shared<LineRoot>(10000, name + "_low");
        close_line_ = std::make_shared<LineRoot>(10000, name + "_close");
        volume_line_ = std::make_shared<LineRoot>(10000, name + "_volume");
    }
    
    virtual ~DataFeed() = default;
    
    // 基础接口
    
    /**
     * @brief 获取数据源名称
     * @return 名称
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief 获取数据源状态
     * @return 状态
     */
    DataStatus getStatus() const { return status_; }
    
    /**
     * @brief 获取当前索引
     * @return 索引
     */
    size_t getCurrentIndex() const { return current_index_; }
    
    /**
     * @brief 获取数据长度
     * @return 数据长度
     */
    virtual size_t len() const {
        return close_line_ ? close_line_->len() : 0;
    }
    
    /**
     * @brief 检查是否有更多数据
     * @return true if has more data
     */
    virtual bool hasNext() const = 0;
    
    /**
     * @brief 获取下一个数据点
     * @return true if successful
     */
    virtual bool next() = 0;
    
    /**
     * @brief 重置到开始位置
     */
    virtual void reset() {
        current_index_ = 0;
        if (datetime_line_) datetime_line_->home();
        if (open_line_) open_line_->home();
        if (high_line_) high_line_->home();
        if (low_line_) low_line_->home();
        if (close_line_) close_line_->home();
        if (volume_line_) volume_line_->home();
    }
    
    // 数据线访问
    
    /**
     * @brief 获取时间线
     * @return 时间数据线
     */
    std::shared_ptr<LineRoot> datetime() const { return datetime_line_; }
    
    /**
     * @brief 获取开盘价线
     * @return 开盘价数据线
     */
    std::shared_ptr<LineRoot> open() const { return open_line_; }
    
    /**
     * @brief 获取最高价线
     * @return 最高价数据线
     */
    std::shared_ptr<LineRoot> high() const { return high_line_; }
    
    /**
     * @brief 获取最低价线
     * @return 最低价数据线
     */
    std::shared_ptr<LineRoot> low() const { return low_line_; }
    
    /**
     * @brief 获取收盘价线
     * @return 收盘价数据线
     */
    std::shared_ptr<LineRoot> close() const { return close_line_; }
    
    /**
     * @brief 获取成交量线
     * @return 成交量数据线
     */
    std::shared_ptr<LineRoot> volume() const { return volume_line_; }
    
    // 便利访问方法
    
    /**
     * @brief 获取当前OHLCV数据
     * @return OHLCV数据
     */
    OHLCVData getCurrentOHLCV() const {
        if (!close_line_ || close_line_->empty()) {
            return OHLCVData();
        }
        
        // 时间戳（简化处理，使用索引转换）
        auto now = std::chrono::system_clock::now();
        
        return OHLCVData(
            now,
            open_line_->get(0),
            high_line_->get(0),
            low_line_->get(0),
            close_line_->get(0),
            volume_line_->get(0)
        );
    }
    
    /**
     * @brief 获取指定偏移的OHLCV数据
     * @param ago 偏移量
     * @return OHLCV数据
     */
    OHLCVData getOHLCV(int ago) const {
        if (!close_line_ || close_line_->empty()) {
            return OHLCVData();
        }
        
        try {
            auto now = std::chrono::system_clock::now();
            
            return OHLCVData(
                now,
                open_line_->get(ago),
                high_line_->get(ago),
                low_line_->get(ago),
                close_line_->get(ago),
                volume_line_->get(ago)
            );
        } catch (...) {
            return OHLCVData();
        }
    }
    
    // 参数管理
    
    /**
     * @brief 设置参数
     * @param key 参数名
     * @param value 参数值
     */
    void setParam(const std::string& key, const std::string& value) {
        params_[key] = value;
    }
    
    /**
     * @brief 获取参数
     * @param key 参数名
     * @param default_value 默认值
     * @return 参数值
     */
    std::string getParam(const std::string& key, const std::string& default_value = "") const {
        auto it = params_.find(key);
        return (it != params_.end()) ? it->second : default_value;
    }
    
    /**
     * @brief 检查参数是否存在
     * @param key 参数名
     * @return true if exists
     */
    bool hasParam(const std::string& key) const {
        return params_.find(key) != params_.end();
    }
    
    // 数据验证
    
    /**
     * @brief 验证数据完整性
     * @return 验证结果
     */
    virtual bool validate() const {
        if (!close_line_ || close_line_->empty()) {
            return false;
        }
        
        size_t length = close_line_->len();
        
        // 检查各数据线长度一致性
        if (open_line_->len() != length ||
            high_line_->len() != length ||
            low_line_->len() != length ||
            volume_line_->len() != length) {
            return false;
        }
        
        // 检查数据有效性
        for (size_t i = 0; i < std::min(length, size_t(100)); ++i) {
            int ago = -static_cast<int>(i);
            OHLCVData data = getOHLCV(ago);
            if (!data.isValid()) {
                return false;
            }
        }
        
        return true;
    }
    
protected:
    /**
     * @brief 添加数据点
     * @param data OHLCV数据
     */
    void addData(const OHLCVData& data) {
        if (!data.isValid()) {
            return;
        }
        
        // 时间戳转换为double（Unix时间戳）
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            data.datetime.time_since_epoch()).count();
        
        datetime_line_->forward(static_cast<double>(timestamp));
        open_line_->forward(data.open);
        high_line_->forward(data.high);
        low_line_->forward(data.low);
        close_line_->forward(data.close);
        volume_line_->forward(data.volume);
        
        current_index_++;
    }
    
    /**
     * @brief 设置状态
     * @param status 新状态
     */
    void setStatus(DataStatus status) {
        status_ = status;
    }
};

/**
 * @brief 静态数据源
 * 
 * 从预加载的数据中提供数据
 */
class StaticDataFeed : public DataFeed {
private:
    std::vector<OHLCVData> data_;
    size_t data_index_;
    
public:
    /**
     * @brief 构造函数
     * @param data 数据向量
     * @param name 数据源名称
     */
    explicit StaticDataFeed(const std::vector<OHLCVData>& data, 
                           const std::string& name = "StaticDataFeed")
        : DataFeed(name), data_(data), data_index_(0) {
        
        setStatus(DataStatus::HISTORICAL);
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
     * @brief 获取剩余数据量
     * @return 剩余数据量
     */
    size_t getRemainingDataCount() const {
        return (data_index_ < data_.size()) ? data_.size() - data_index_ : 0;
    }
    
    /**
     * @brief 跳过指定数量的数据点
     * @param count 跳过数量
     * @return 实际跳过的数量
     */
    size_t skip(size_t count) {
        size_t available = getRemainingDataCount();
        size_t to_skip = std::min(count, available);
        
        for (size_t i = 0; i < to_skip; ++i) {
            next();
        }
        
        return to_skip;
    }
    
    /**
     * @brief 批量加载数据
     * @param count 加载数量，0表示全部
     * @return 实际加载的数量
     */
    size_t loadBatch(size_t count = 0) {
        if (count == 0) {
            count = getRemainingDataCount();
        }
        
        size_t loaded = 0;
        while (loaded < count && hasNext()) {
            if (next()) {
                loaded++;
            } else {
                break;
            }
        }
        
        return loaded;
    }
};

/**
 * @brief 数据源工厂
 */
class DataFeedFactory {
public:
    /**
     * @brief 创建静态数据源
     * @param data 数据向量
     * @param name 名称
     * @return 数据源智能指针
     */
    static std::unique_ptr<DataFeed> createStatic(const std::vector<OHLCVData>& data,
                                                 const std::string& name = "StaticData") {
        return std::make_unique<StaticDataFeed>(data, name);
    }
    
    /**
     * @brief 创建随机数据源（用于测试）
     * @param count 数据数量
     * @param initial_price 初始价格
     * @param volatility 波动率
     * @param name 名称
     * @return 数据源智能指针
     */
    static std::unique_ptr<DataFeed> createRandom(size_t count,
                                                 double initial_price = 100.0,
                                                 double volatility = 0.02,
                                                 const std::string& name = "RandomData");
    
    /**
     * @brief 创建正弦波数据源（用于测试）
     * @param count 数据数量
     * @param amplitude 振幅
     * @param frequency 频率
     * @param base_price 基础价格
     * @param name 名称
     * @return 数据源智能指针
     */
    static std::unique_ptr<DataFeed> createSineWave(size_t count,
                                                   double amplitude = 10.0,
                                                   double frequency = 0.1,
                                                   double base_price = 100.0,
                                                   const std::string& name = "SineWave");
};

} // namespace data
} // namespace backtrader