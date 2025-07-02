#pragma once

#include "LineRoot.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>

namespace backtrader {
namespace data {

/**
 * @brief 简化的数据源，用于策略测试
 */
class SimpleDataFeed {
public:
    struct BarData {
        std::string date;
        double open;
        double high;
        double low;
        double close;
        double volume;
        double openinterest;
    };
    
    // 兼容别名
    using OHLCVData = BarData;
    
private:
    std::shared_ptr<LineRoot> open_;
    std::shared_ptr<LineRoot> high_;
    std::shared_ptr<LineRoot> low_;
    std::shared_ptr<LineRoot> close_;
    std::shared_ptr<LineRoot> volume_;
    std::shared_ptr<LineRoot> datetime_;
    std::string name_;
    std::vector<BarData> raw_data_;  // 原始数据
    size_t current_position_;  // 当前数据位置
    
public:
    
    SimpleDataFeed(const std::string& name = "data") : name_(name), current_position_(0) {
        size_t capacity = 1000;  // 默认容量
        open_ = std::make_shared<LineRoot>(capacity, "open");
        high_ = std::make_shared<LineRoot>(capacity, "high");
        low_ = std::make_shared<LineRoot>(capacity, "low");
        close_ = std::make_shared<LineRoot>(capacity, "close");
        volume_ = std::make_shared<LineRoot>(capacity, "volume");
        datetime_ = std::make_shared<LineRoot>(capacity, "datetime");
    }
    
    // 数据访问接口
    double open(int ago = 0) const { return open_->get(ago); }
    double high(int ago = 0) const { return high_->get(ago); }
    double low(int ago = 0) const { return low_->get(ago); }
    double close(int ago = 0) const { return close_->get(ago); }
    double volume(int ago = 0) const { return volume_->get(ago); }
    double datetime(int ago = 0) const { return datetime_->get(ago); }
    
    // 获取Line对象（用于指标输入）
    std::shared_ptr<LineRoot> getOpen() const { return open_; }
    std::shared_ptr<LineRoot> getHigh() const { return high_; }
    std::shared_ptr<LineRoot> getLow() const { return low_; }
    std::shared_ptr<LineRoot> getClose() const { return close_; }
    std::shared_ptr<LineRoot> getVolume() const { return volume_; }
    std::shared_ptr<LineRoot> getDatetime() const { return datetime_; }
    
    // 简化的close访问（兼容策略代码）
    std::shared_ptr<LineRoot> closeData() const { return close_; }
    
    // 按名称获取数据线（用于CrossOver等指标）
    std::shared_ptr<LineRoot> getLine(const std::string& name) const {
        if (name == "open") return open_;
        if (name == "high") return high_;
        if (name == "low") return low_;
        if (name == "close") return close_;
        if (name == "volume") return volume_;
        if (name == "datetime") return datetime_;
        return close_;  // 默认返回收盘价
    }
    
    // 添加数据（存储到原始数据）
    void addData(const OHLCVData& bar) {
        raw_data_.push_back(bar);
    }
    
    // 批量加载数据
    void loadData(const std::vector<OHLCVData>& data) {
        raw_data_ = data;
    }
    
    // 获取数据长度
    size_t len() const { return raw_data_.size(); }
    
    // 重置到起始位置
    void reset() {
        current_position_ = 0;
        // 重置缓冲区（但不调用home，以避免清空size_）
    }
    
    // 前进一步（用于数据迭代）
    void forward() {
        if (current_position_ < raw_data_.size()) {
            const auto& bar = raw_data_[current_position_];
            
            // 将当前数据加入缓冲区
            open_->forward(bar.open);
            high_->forward(bar.high);
            low_->forward(bar.low);
            close_->forward(bar.close);
            volume_->forward(bar.volume);
            
            // 简化的日期时间处理
            double dt = 20060102.0 + current_position_;  // 简单的日期递增
            datetime_->forward(dt);
            
            current_position_++;
            
            // Debug output (commented out)
            // std::cout << "Forward: position=" << current_position_ << ", close=" << bar.close << ", buffer_size=" << close_->len() << std::endl;
        }
    }
    
    // 获取当前位置
    size_t getCurrentPosition() const { return current_position_; }
    
    // 获取名称
    const std::string& getName() const { return name_; }
};

} // namespace data
} // namespace backtrader