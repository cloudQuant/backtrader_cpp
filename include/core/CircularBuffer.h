#pragma once

#include "Common.h"
#include <vector>
#include <stdexcept>

namespace backtrader {

/**
 * @brief 高性能环形缓冲区，支持负索引访问
 * 
 * 核心数据结构，模拟Python版本的LineBuffer行为
 * 支持负索引 (ago)，其中 -1 表示最新值，-2 表示前一个值，依此类推
 */
template<typename T>
class CircularBuffer {
private:
    std::vector<T> data_;
    size_t capacity_;
    size_t size_;
    size_t head_;  // 指向最新数据的位置
    
public:
    /**
     * @brief 构造函数
     * @param capacity 缓冲区容量，默认1000
     */
    explicit CircularBuffer(size_t capacity = 1000)
        : data_(capacity), capacity_(capacity), size_(0), head_(0) {
        if (capacity == 0) {
            throw std::invalid_argument("CircularBuffer capacity must be greater than 0");
        }
    }
    
    /**
     * @brief 获取缓冲区大小
     * @return 当前存储的元素数量
     */
    size_t len() const { return size_; }
    
    /**
     * @brief 检查缓冲区是否为空
     * @return true if empty
     */
    bool empty() const { return size_ == 0; }
    
    /**
     * @brief 获取缓冲区容量
     * @return 最大容量
     */
    size_t capacity() const { return capacity_; }
    
    /**
     * @brief 负索引访问操作符
     * @param ago 负偏移量，-1表示最新值，-2表示前一个值
     * @return 元素引用
     */
    T& operator[](int ago) {
        if (size_ == 0) {
            throw std::out_of_range("CircularBuffer is empty");
        }
        
        if (ago > 0) {
            throw std::invalid_argument("CircularBuffer only supports negative indexing (ago <= 0)");
        }
        
        size_t abs_ago = static_cast<size_t>(-ago);
        if (abs_ago > size_) {
            throw std::out_of_range("Index out of range");
        }
        
        // 计算实际位置
        size_t pos;
        if (abs_ago == 0) {
            pos = head_;
        } else {
            pos = (head_ + capacity_ - abs_ago + 1) % capacity_;
        }
        
        return data_[pos];
    }
    
    /**
     * @brief 常量版本的负索引访问
     */
    const T& operator[](int ago) const {
        return const_cast<CircularBuffer<T>*>(this)->operator[](ago);
    }
    
    /**
     * @brief 便利方法，等同于 operator[]
     */
    T& get(int ago = 0) {
        return (*this)[ago];
    }
    
    const T& get(int ago = 0) const {
        return (*this)[ago];
    }
    
    /**
     * @brief 向前移动并添加新值
     * @param value 新值
     * @param size 移动步数，默认1
     */
    void forward(const T& value = T{}, size_t size = 1) {
        for (size_t i = 0; i < size; ++i) {
            if (i == size - 1) {
                // 最后一次设置实际值
                head_ = (head_ + 1) % capacity_;
                data_[head_] = value;
            } else {
                // 中间步骤设置默认值或NaN
                head_ = (head_ + 1) % capacity_;
                if constexpr (std::is_floating_point_v<T>) {
                    data_[head_] = std::numeric_limits<T>::quiet_NaN();
                } else {
                    data_[head_] = T{};
                }
            }
            
            if (size_ < capacity_) {
                ++size_;
            }
        }
    }
    
    /**
     * @brief 向后移动
     * @param size 移动步数，默认1
     */
    void backward(size_t size = 1) {
        if (size > size_) {
            throw std::out_of_range("Cannot move backward more than current size");
        }
        
        head_ = (head_ + capacity_ - size) % capacity_;
        size_ -= size;
    }
    
    /**
     * @brief 回到起始位置
     */
    void home() {
        head_ = 0;
        size_ = 0;
    }
    
    /**
     * @brief 获取原始数据指针（用于性能敏感操作）
     * @return 数据指针
     */
    const T* data() const {
        return data_.data();
    }
    
    /**
     * @brief 获取连续数据视图（仅在数据连续时有效）
     * @param start_ago 起始偏移
     * @param count 数量
     * @return 指向连续数据的指针，如果数据不连续则返回nullptr
     */
    const T* getContinuousView(int start_ago, size_t count) const {
        if (size_ == 0 || count == 0) return nullptr;
        
        size_t abs_start = static_cast<size_t>(-start_ago);
        if (abs_start + count > size_) return nullptr;
        
        size_t start_pos = (head_ + capacity_ - abs_start + 1) % capacity_;
        
        // 检查数据是否连续存储
        if (start_pos + count <= capacity_) {
            return &data_[start_pos];
        }
        
        return nullptr;  // 数据跨越了缓冲区边界，不连续
    }
    
    /**
     * @brief 批量获取数据（处理非连续情况）
     * @param start_ago 起始偏移
     * @param count 数量
     * @param output 输出缓冲区
     */
    void getBatch(int start_ago, size_t count, T* output) const {
        if (size_ == 0 || count == 0) return;
        
        size_t abs_start = static_cast<size_t>(-start_ago);
        if (abs_start + count > size_) {
            throw std::out_of_range("Batch request out of range");
        }
        
        for (size_t i = 0; i < count; ++i) {
            output[i] = (*this)[start_ago - static_cast<int>(i)];
        }
    }
};

} // namespace backtrader