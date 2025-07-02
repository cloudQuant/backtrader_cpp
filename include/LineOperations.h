/**
 * @file LineOperations.h
 * @brief Operator Overloading System - Lazy evaluation chains for mathematical and comparison operations
 * 
 * This file implements the operator overloading system from backtrader's Python codebase,
 * providing lazy evaluation chains for mathematical operations (+, -, *, /), comparison
 * operations (<, >, ==, etc.), and logical operations. This enables natural mathematical
 * expressions like: data.close > data.open * 1.01
 */

#pragma once

#include "LineBuffer.h"
#include "LineSeries.h"
#include <functional>
#include <memory>
#include <variant>
#include <concepts>
#include <type_traits>
#include <cmath>
#include <algorithm>

namespace backtrader {
namespace line {

// Forward declarations
class LineOperation;
class ArithmeticOperation;
class ComparisonOperation;
class LogicalOperation;

/**
 * @brief Operation types for lazy evaluation
 */
enum class OperationType {
    // Arithmetic operations
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    
    // Comparison operations
    Equal,
    NotEqual,
    LessThan,
    LessEqual,
    GreaterThan,
    GreaterEqual,
    
    // Logical operations
    And,
    Or,
    Not,
    
    // Unary operations
    Positive,
    Negative,
    Absolute,
    
    // Mathematical functions
    Sin,
    Cos,
    Tan,
    Log,
    Log10,
    Exp,
    Sqrt,
    Floor,
    Ceil,
    Round
};

/**
 * @brief Operand types for operations
 */
using Operand = std::variant<
    double,                                    // Scalar value
    std::shared_ptr<LineBuffer>,              // Line buffer
    std::shared_ptr<LineSeries>,              // Line series  
    std::shared_ptr<LineOperation>            // Nested operation
>;

// LineOperand concept removed for compatibility

/**
 * @brief Helper function to get value from any operand type
 */
double getValue(const Operand& operand, int ago = 0);

/**
 * @brief Base class for all line operations
 */
class LineOperation : public LineBuffer {
private:
    OperationType operation_type_;
    std::vector<Operand> operands_;
    mutable std::unordered_map<int, double> value_cache_;
    bool enable_caching_;
    
    // Evaluation methods
    double evaluateArithmetic(int ago) const;
    double evaluateComparison(int ago) const;
    double evaluateLogical(int ago) const;
    double evaluateUnary(int ago) const;
    double evaluateMathFunction(int ago) const;
    
protected:
    void invalidateCache();
    
public:
    /**
     * @brief Constructor
     * @param type Operation type
     * @param operands Vector of operands
     */
    LineOperation(OperationType type, std::vector<Operand> operands);
    
    /**
     * @brief Get operation type
     */
    OperationType getOperationType() const { return operation_type_; }
    
    /**
     * @brief Get operands
     */
    const std::vector<Operand>& getOperands() const { return operands_; }
    
    /**
     * @brief Enable/disable result caching
     */
    void setCaching(bool enable) { enable_caching_ = enable; }
    
    // Override LineBuffer methods for lazy evaluation
    double operator[](int ago) const override;
    void forward(double value = NAN_VALUE, int size = 1) override;
    void backwards(int size = 1, bool force = false) override;
    void reset() override;
    
    // ========== Arithmetic Operators ==========
    
    LineOperation operator+(const Operand& other) const;
    LineOperation operator-(const Operand& other) const;
    LineOperation operator*(const Operand& other) const;
    LineOperation operator/(const Operand& other) const;
    LineOperation operator%(const Operand& other) const;
    
    // Compound assignment operators
    LineOperation& operator+=(const Operand& other);
    LineOperation& operator-=(const Operand& other);
    LineOperation& operator*=(const Operand& other);
    LineOperation& operator/=(const Operand& other);
    
    // ========== Comparison Operators ==========
    
    LineOperation operator==(const Operand& other) const;
    LineOperation operator!=(const Operand& other) const;
    LineOperation operator<(const Operand& other) const;
    LineOperation operator<=(const Operand& other) const;
    LineOperation operator>(const Operand& other) const;
    LineOperation operator>=(const Operand& other) const;
    
    // ========== Logical Operators ==========
    
    LineOperation operator&&(const Operand& other) const;
    LineOperation operator||(const Operand& other) const;
    LineOperation operator!() const;
    
    // ========== Unary Operators ==========
    
    LineOperation operator+() const;  // Unary plus
    LineOperation operator-() const;  // Unary minus
    
    // ========== Mathematical Functions ==========
    
    LineOperation abs() const;
    LineOperation sin() const;
    LineOperation cos() const;
    LineOperation tan() const;
    LineOperation log() const;
    LineOperation log10() const;
    LineOperation exp() const;
    LineOperation sqrt() const;
    LineOperation floor() const;
    LineOperation ceil() const;
    LineOperation round() const;
    LineOperation pow(const Operand& exponent) const;
    
    // ========== Utility Methods ==========
    
    /**
     * @brief Force evaluation and caching of result
     * @param periods Number of periods to pre-calculate
     */
    void precompute(int periods = 100);
    
    /**
     * @brief Get operation tree as string for debugging
     */
    std::string getExpressionString() const;
    
    /**
     * @brief Check if operation tree is valid
     */
    bool validate() const;
    
    /**
     * @brief Optimize operation tree
     * @return Optimized version of this operation
     */
    std::shared_ptr<LineOperation> optimize() const;
};

/**
 * @brief Specialized arithmetic operation class
 */
class ArithmeticOperation : public LineOperation {
public:
    ArithmeticOperation(OperationType type, Operand left, Operand right)
        : LineOperation(type, {std::move(left), std::move(right)}) {}
    
    // Static factory methods
    static std::shared_ptr<ArithmeticOperation> add(Operand left, Operand right);
    static std::shared_ptr<ArithmeticOperation> subtract(Operand left, Operand right);
    static std::shared_ptr<ArithmeticOperation> multiply(Operand left, Operand right);
    static std::shared_ptr<ArithmeticOperation> divide(Operand left, Operand right);
};

/**
 * @brief Specialized comparison operation class
 */
class ComparisonOperation : public LineOperation {
public:
    ComparisonOperation(OperationType type, Operand left, Operand right)
        : LineOperation(type, {std::move(left), std::move(right)}) {}
    
    // Static factory methods
    static std::shared_ptr<ComparisonOperation> equal(Operand left, Operand right);
    static std::shared_ptr<ComparisonOperation> lessThan(Operand left, Operand right);
    static std::shared_ptr<ComparisonOperation> greaterThan(Operand left, Operand right);
};

/**
 * @brief Function operation class for mathematical functions
 */
class FunctionOperation : public LineOperation {
public:
    FunctionOperation(OperationType type, Operand operand)
        : LineOperation(type, {std::move(operand)}) {}
    
    // Static factory methods
    static std::shared_ptr<FunctionOperation> sin(Operand operand);
    static std::shared_ptr<FunctionOperation> cos(Operand operand);
    static std::shared_ptr<FunctionOperation> log(Operand operand);
    static std::shared_ptr<FunctionOperation> sqrt(Operand operand);
};

// ========== Global Operator Overloads ==========

// Enable operators for LineBuffer
LineOperation operator+(const LineBuffer& left, const Operand& right);
LineOperation operator-(const LineBuffer& left, const Operand& right);
LineOperation operator*(const LineBuffer& left, const Operand& right);
LineOperation operator/(const LineBuffer& left, const Operand& right);

LineOperation operator==(const LineBuffer& left, const Operand& right);
LineOperation operator!=(const LineBuffer& left, const Operand& right);
LineOperation operator<(const LineBuffer& left, const Operand& right);
LineOperation operator>(const LineBuffer& left, const Operand& right);

// Enable operators for LineSeries  
LineOperation operator+(const LineSeries& left, const Operand& right);
LineOperation operator-(const LineSeries& left, const Operand& right);
LineOperation operator*(const LineSeries& left, const Operand& right);
LineOperation operator/(const LineSeries& left, const Operand& right);

// Enable operators for scalars with line types
LineOperation operator+(double left, const LineBuffer& right);
LineOperation operator-(double left, const LineBuffer& right);
LineOperation operator*(double left, const LineBuffer& right);
LineOperation operator/(double left, const LineBuffer& right);

// ========== Mathematical Function Wrappers ==========

// Global mathematical functions that work with line types
LineOperation sin(const Operand& operand);
LineOperation cos(const Operand& operand);
LineOperation tan(const Operand& operand);
LineOperation log(const Operand& operand);
LineOperation log10(const Operand& operand);
LineOperation exp(const Operand& operand);
LineOperation sqrt(const Operand& operand);
LineOperation abs(const Operand& operand);
LineOperation floor(const Operand& operand);
LineOperation ceil(const Operand& operand);
LineOperation round(const Operand& operand);
LineOperation pow(const Operand& base, const Operand& exponent);

// ========== Utility Functions ==========

/**
 * @brief Check if operand is a constant value
 */
bool isConstant(const Operand& operand);

/**
 * @brief Get constant value (if operand is constant)
 */
double getConstantValue(const Operand& operand);

/**
 * @brief Simplify operation tree by combining constants
 */
std::shared_ptr<LineOperation> simplify(std::shared_ptr<LineOperation> operation);

/**
 * @brief Convert operation tree to string representation
 */
std::string operationToString(const LineOperation& operation);

// ========== C++20 Concepts and Features ==========

/**
 * @brief Template function for creating operations with any compatible type
 */
template<typename T, typename U>
auto createOperation(OperationType type, T&& left, U&& right) {
    return LineOperation(type, {Operand{std::forward<T>(left)}, Operand{std::forward<U>(right)}});
}

/**
 * @brief Range-based operation application (C++17 compatible)
 */
template<typename R>
auto applyOperation(OperationType type, R&& range) {
    std::vector<Operand> operands;
    operands.reserve(range.size());
    
    for (auto&& item : range) {
        operands.emplace_back(item);
    }
    
    return LineOperation(type, std::move(operands));
}

// ========== Expression Templates (C++20) ==========

/**
 * @brief Expression template for compile-time optimization
 */
template<typename Expr>
class LineExpression {
private:
    Expr expr_;
    
public:
    explicit LineExpression(Expr expr) : expr_(std::move(expr)) {}
    
    double operator[](int ago) const {
        return expr_.evaluate(ago);
    }
    
    // Convert to LineOperation for runtime evaluation
    LineOperation materialize() const {
        return expr_.materialize();
    }
};

/**
 * @brief Binary expression template
 */
template<typename Left, typename Right, OperationType Op>
class BinaryExpression {
private:
    Left left_;
    Right right_;
    
public:
    BinaryExpression(Left left, Right right) : left_(std::move(left)), right_(std::move(right)) {}
    
    double evaluate(int ago) const {
        double l_val = getValue(left_, ago);
        double r_val = getValue(right_, ago);
        
        switch (Op) {
            case OperationType::Add: return l_val + r_val;
            case OperationType::Subtract: return l_val - r_val;
            case OperationType::Multiply: return l_val * r_val;
            case OperationType::Divide: return r_val != 0.0 ? l_val / r_val : NAN_VALUE;
            default: return NAN_VALUE;
        }
    }
    
    LineOperation materialize() const {
        return LineOperation(Op, {Operand{left_}, Operand{right_}});
    }
};

} // namespace line
} // namespace backtrader