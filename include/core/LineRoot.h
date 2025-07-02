#pragma once

#include "CircularBuffer.h"
#include "Common.h"
#include <memory>
#include <vector>
#include <functional>

namespace backtrader {

/**
 * @brief 数据线基类，模拟Python版本的LineRoot
 * 
 * 提供了数据存储、访问和操作的核心功能
 * 支持运算符重载和延迟计算
 */
class LineRoot {
protected:
    CircularBuffer<double> buffer_;
    size_t min_period_;
    std::string name_;
    
public:
    /**
     * @brief 构造函数
     * @param capacity 缓冲区容量
     * @param name 数据线名称
     */
    explicit LineRoot(size_t capacity = 1000, const std::string& name = "")
        : buffer_(capacity), min_period_(1), name_(name) {}
    
    virtual ~LineRoot() = default;
    
    // 基础访问方法
    /**
     * @brief 获取数据线长度
     * @return 当前数据数量
     */
    size_t len() const { return buffer_.len(); }
    
    /**
     * @brief 获取缓冲区容量
     * @return 缓冲区大小
     */
    size_t buflen() const { return buffer_.capacity(); }
    
    /**
     * @brief 检查是否为空
     * @return true if empty
     */
    bool empty() const { return buffer_.empty(); }
    
    /**
     * @brief 负索引访问
     * @param ago 偏移量，0表示最新值，-1表示前一个值
     * @return 数据值
     */
    virtual double operator[](int ago) const {
        return buffer_[ago];
    }
    
    /**
     * @brief 函数调用访问，等同于operator[]
     * @param ago 偏移量
     * @return 数据值
     */
    virtual double operator()(int ago) const {
        return buffer_[ago];
    }
    
    /**
     * @brief 便利方法，等同于operator[]
     * @param ago 偏移量，默认0
     * @return 数据值
     */
    virtual double get(int ago = 0) const {
        return buffer_[ago];
    }
    
    /**
     * @brief 向前移动并设置新值
     * @param value 新值
     * @param size 移动步数
     */
    virtual void forward(double value = NaN, size_t size = 1) {
        buffer_.forward(value, size);
    }
    
    /**
     * @brief 向后移动
     * @param size 移动步数
     */
    void backward(size_t size = 1) {
        buffer_.backward(size);
    }
    
    /**
     * @brief 回到起始位置
     */
    void home() {
        buffer_.home();
    }
    
    /**
     * @brief 获取最小周期
     * @return 最小周期
     */
    size_t getMinPeriod() const { return min_period_; }
    
    /**
     * @brief 设置最小周期
     * @param period 新的最小周期
     */
    void setMinPeriod(size_t period) { min_period_ = period; }
    
    /**
     * @brief 获取数据线名称
     * @return 名称
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief 设置数据线名称
     * @param name 新名称
     */
    void setName(const std::string& name) { name_ = name; }
    
    /**
     * @brief 获取底层缓冲区的引用
     * @return CircularBuffer引用
     */
    const CircularBuffer<double>& getBuffer() const { return buffer_; }
    
    /**
     * @brief 获取原始数据指针
     * @return 数据指针
     */
    const double* getRawData() const { return buffer_.data(); }
    
    // 运算符重载 - 支持数据线间运算
    
    /**
     * @brief 加法运算符重载
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator+(const T& other) const;
    
    /**
     * @brief 减法运算符重载
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator-(const T& other) const;
    
    /**
     * @brief 乘法运算符重载
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator*(const T& other) const;
    
    /**
     * @brief 除法运算符重载
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator/(const T& other) const;
    
    /**
     * @brief 大于比较运算符
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator>(const T& other) const;
    
    /**
     * @brief 小于比较运算符
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator<(const T& other) const;
    
    /**
     * @brief 大于等于比较运算符
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator>=(const T& other) const;
    
    /**
     * @brief 小于等于比较运算符
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator<=(const T& other) const;
    
    /**
     * @brief 等于比较运算符
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator==(const T& other) const;
    
    /**
     * @brief 不等于比较运算符
     */
    template<typename T>
    std::shared_ptr<LineRoot> operator!=(const T& other) const;
    
protected:
    /**
     * @brief 执行二元操作
     * @param other 另一个操作数
     * @param op 操作函数
     * @return 结果数据线
     */
    template<typename T, typename Operation>
    std::shared_ptr<LineRoot> binaryOperation(const T& other, Operation op) const;
};

/**
 * @brief 运算结果数据线
 * 
 * 用于存储运算结果的特殊数据线
 */
class OperationLineRoot : public LineRoot {
private:
    std::function<double(int)> calculation_func_;
    
public:
    /**
     * @brief 构造函数
     * @param calc_func 计算函数
     * @param capacity 缓冲区容量
     * @param name 名称
     */
    explicit OperationLineRoot(
        std::function<double(int)> calc_func,
        size_t capacity = 1000,
        const std::string& name = "operation"
    ) : LineRoot(capacity, name), calculation_func_(calc_func) {}
    
    /**
     * @brief 重写访问方法，支持延迟计算
     */
    double operator[](int ago) const override {
        if (calculation_func_) {
            return calculation_func_(ago);
        }
        return LineRoot::operator[](ago);
    }
    
    double operator()(int ago) const override {
        return operator[](ago);
    }
    
    double get(int ago = 0) const override {
        return operator[](ago);
    }
};

// 模板实现

template<typename T, typename Operation>
std::shared_ptr<LineRoot> LineRoot::binaryOperation(const T& other, Operation op) const {
    if constexpr (std::is_arithmetic_v<T>) {
        // 与标量运算
        auto calc_func = [this, other, op](int ago) -> double {
            if (this->len() == 0) return NaN;
            try {
                double left_val = this->get(ago);
                return op(left_val, static_cast<double>(other));
            } catch (...) {
                return NaN;
            }
        };
        
        return std::make_shared<OperationLineRoot>(calc_func, this->buflen(), 
                                                  this->name_ + "_op_scalar");
    } else {
        // 与另一个LineRoot运算
        auto calc_func = [this, &other, op](int ago) -> double {
            if (this->len() == 0 || other.len() == 0) return NaN;
            try {
                double left_val = this->get(ago);
                double right_val = other.get(ago);
                return op(left_val, right_val);
            } catch (...) {
                return NaN;
            }
        };
        
        return std::make_shared<OperationLineRoot>(calc_func, 
                                                  std::min(this->buflen(), other.buflen()),
                                                  this->name_ + "_op_" + other.getName());
    }
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator+(const T& other) const {
    return binaryOperation(other, [](double a, double b) { return a + b; });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator-(const T& other) const {
    return binaryOperation(other, [](double a, double b) { return a - b; });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator*(const T& other) const {
    return binaryOperation(other, [](double a, double b) { return a * b; });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator/(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (b != 0.0) ? a / b : NaN; 
    });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator>(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (a > b) ? 1.0 : 0.0; 
    });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator<(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (a < b) ? 1.0 : 0.0; 
    });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator>=(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (a >= b) ? 1.0 : 0.0; 
    });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator<=(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (a <= b) ? 1.0 : 0.0; 
    });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator==(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (std::abs(a - b) < 1e-9) ? 1.0 : 0.0; 
    });
}

template<typename T>
std::shared_ptr<LineRoot> LineRoot::operator!=(const T& other) const {
    return binaryOperation(other, [](double a, double b) { 
        return (std::abs(a - b) >= 1e-9) ? 1.0 : 0.0; 
    });
}

// 支持标量在左侧的运算符重载
template<typename T>
std::shared_ptr<LineRoot> operator+(const T& scalar, const LineRoot& line) {
    return line + scalar;
}

template<typename T>
std::shared_ptr<LineRoot> operator-(const T& scalar, const LineRoot& line) {
    auto calc_func = [&line, scalar](int ago) -> double {
        if (line.len() == 0) return NaN;
        try {
            double right_val = line.get(ago);
            return static_cast<double>(scalar) - right_val;
        } catch (...) {
            return NaN;
        }
    };
    
    return std::make_shared<OperationLineRoot>(calc_func, line.buflen(), 
                                              "scalar_minus_" + line.getName());
}

template<typename T>
std::shared_ptr<LineRoot> operator*(const T& scalar, const LineRoot& line) {
    return line * scalar;
}

template<typename T>
std::shared_ptr<LineRoot> operator/(const T& scalar, const LineRoot& line) {
    auto calc_func = [&line, scalar](int ago) -> double {
        if (line.len() == 0) return NaN;
        try {
            double right_val = line.get(ago);
            return (right_val != 0.0) ? static_cast<double>(scalar) / right_val : NaN;
        } catch (...) {
            return NaN;
        }
    };
    
    return std::make_shared<OperationLineRoot>(calc_func, line.buflen(), 
                                              "scalar_div_" + line.getName());
}

} // namespace backtrader