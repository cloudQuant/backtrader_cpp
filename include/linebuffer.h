#pragma once

#include "lineroot.h"
#include <vector>
#include <deque>
#include <memory>
#include <limits>
#include <cmath>
#include <string>

namespace backtrader {

constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

class LineBuffer : public LineSingle {
public:
    enum Mode { UnBounded = 0, QBuffer = 1 };
    
    LineBuffer();
    virtual ~LineBuffer() = default;
    
    // Array-like access (index 0 is current value)
    double operator[](int index) const override;
    void set(int index, double value) override;
    
    // Size and capacity
    size_t size() const override;
    bool empty() const override;
    size_t buflen() const override;
    
    // Position management
    void forward(size_t size = 1) override;
    void backward(size_t size = 1) override;
    void rewind(size_t size = 1) override;
    void extend(size_t size = 1) override;
    void reset() override;
    void clear();  // Clear without adding initial NaN
    void home() override;
    void advance(size_t size = 1) override;
    
    // Index management
    int get_idx() const;
    void set_idx(int idx, bool force = false);
    
    // Buffer mode
    void qbuffer(size_t savemem = 0);
    void minbuffer(size_t size);
    
    // Binding operations
    void addbinding(std::shared_ptr<LineSingle> binding) override;
    void oncebinding() override;
    void bind2line(std::shared_ptr<LineSingle> binding) override;
    
    // Value operations
    double get(int ago = 0) const override;
    void append(double value);
    
    // Iterator-like operations
    std::vector<double> getrange(int start, int end) const;
    
    // Batch access for performance optimization
    const double* data_ptr() const { return array_.data(); }
    size_t data_size() const { return array_.size(); }
    void reserve(size_t capacity) { array_.reserve(capacity); }
    void batch_append(const std::vector<double>& values) {
        array_.insert(array_.end(), values.begin(), values.end());
    }
    
    // Length operations
    size_t len() const { return size(); }
    
    // Array operations
    std::vector<double> array() const;
    
    // Timezone support
    void set_tz(const std::string& tz) { _tz = tz; }
    std::string get_tz() const { return _tz; }
    
private:
    std::vector<double> array_;
    std::vector<std::shared_ptr<LineSingle>> bindings_;
    
    int _idx;
    int lenmark_;
    Mode mode_;
    std::string _tz;
    
    void _makebinding(double value);
    void _checkbounds(int index) const;
};

class LineActions : public LineBuffer, public std::enable_shared_from_this<LineActions> {
public:
    LineActions();
    virtual ~LineActions() = default;
    
    // Action operations
    virtual void next() {}
    virtual void once(int start, int end) {}
    virtual void preonce(int start, int end) {}
    virtual void oncestart(int start, int end) {}
    
    // Arithmetic operations
    std::shared_ptr<LineActions> operator+(const LineActions& other) const;
    std::shared_ptr<LineActions> operator-(const LineActions& other) const;
    std::shared_ptr<LineActions> operator*(const LineActions& other) const;
    std::shared_ptr<LineActions> operator/(const LineActions& other) const;
    
    // Comparison operations
    std::shared_ptr<LineActions> operator<(const LineActions& other) const;
    std::shared_ptr<LineActions> operator<=(const LineActions& other) const;
    std::shared_ptr<LineActions> operator>(const LineActions& other) const;
    std::shared_ptr<LineActions> operator>=(const LineActions& other) const;
    std::shared_ptr<LineActions> operator==(const LineActions& other) const;
    std::shared_ptr<LineActions> operator!=(const LineActions& other) const;
    
    // Logical operations
    std::shared_ptr<LineActions> operator&&(const LineActions& other) const;
    std::shared_ptr<LineActions> operator||(const LineActions& other) const;
    
    // Math functions (note: these are not overrides but new functions)
    std::shared_ptr<LineActions> abs_action() const;
    std::shared_ptr<LineActions> pow_action(double exponent) const;
    
    // Delay operations
    std::shared_ptr<LineActions> delay(int period) const;
    std::shared_ptr<LineActions> operator()(int period) const;
};

class LineNum : public LineActions {
public:
    explicit LineNum(double value);
    virtual ~LineNum() = default;
    
    double operator[](int index) const override;
    void set(int index, double value) override;
    
    size_t size() const override;
    bool empty() const override;
    
private:
    double value_;
};

class LineDelay : public LineActions {
public:
    LineDelay(std::shared_ptr<LineActions> line, int period);
    virtual ~LineDelay() = default;
    
    double operator[](int index) const override;
    void set(int index, double value) override;
    
    size_t size() const override;
    bool empty() const override;
    
private:
    std::shared_ptr<LineActions> line_;
    int period_;
};

class LinesOperation : public LineActions {
public:
    enum OpType {
        Add, Sub, Mul, Div,
        Lt, Le, Gt, Ge, Eq, Ne,
        And, Or,
        Neg, Not,
        Abs, Pow
    };
    
    LinesOperation(std::shared_ptr<LineActions> lhs, std::shared_ptr<LineActions> rhs, OpType op);
    explicit LinesOperation(std::shared_ptr<LineActions> operand, OpType op);
    virtual ~LinesOperation() = default;
    
    double operator[](int index) const override;
    void set(int index, double value) override;
    
    size_t size() const override;
    bool empty() const override;
    
private:
    std::shared_ptr<LineActions> lhs_;
    std::shared_ptr<LineActions> rhs_;
    OpType op_;
    double scalar_;
    bool use_scalar_;
    
    double compute(double a, double b) const;
    double compute_unary(double a) const;
};

} // namespace backtrader