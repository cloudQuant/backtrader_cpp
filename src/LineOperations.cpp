/**
 * @file LineOperations.cpp
 * @brief Implementation of LineOperations system
 */

#include "LineOperations.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace backtrader {
namespace line {

// ========== Helper Functions ==========

double getValue(const Operand& operand, int ago) {
    return std::visit([ago](const auto& arg) -> double {
        using T = std::decay_t<decltype(arg)>;
        
        if constexpr (std::is_same_v<T, double>) {
            return arg;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<LineBuffer>>) {
            if (arg && arg->isValidIndex(ago)) {
                return (*arg)[ago];
            }
            return NAN_VALUE;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<LineSeries>>) {
            if (arg && !arg->empty()) {
                // For LineSeries, use the first line (typically close price)
                auto lines = arg->getLines();
                if (!lines.empty() && lines[0] && lines[0]->isValidIndex(ago)) {
                    return (*lines[0])[ago];
                }
            }
            return NAN_VALUE;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<LineOperation>>) {
            if (arg) {
                return (*arg)[ago];
            }
            return NAN_VALUE;
        }
        else {
            return NAN_VALUE;
        }
    }, operand);
}

// ========== LineOperation Implementation ==========

LineOperation::LineOperation(OperationType type, std::vector<Operand> operands)
    : operation_type_(type)
    , operands_(std::move(operands))
    , enable_caching_(true) {
}

double LineOperation::operator[](int ago) const {
    // Check cache first
    if (enable_caching_) {
        auto it = value_cache_.find(ago);
        if (it != value_cache_.end()) {
            return it->second;
        }
    }
    
    double result = NAN_VALUE;
    
    // Evaluate based on operation type
    switch (operation_type_) {
        case OperationType::Add:
        case OperationType::Subtract:
        case OperationType::Multiply:
        case OperationType::Divide:
        case OperationType::Modulo:
        case OperationType::Power:
            result = evaluateArithmetic(ago);
            break;
            
        case OperationType::Equal:
        case OperationType::NotEqual:
        case OperationType::LessThan:
        case OperationType::LessEqual:
        case OperationType::GreaterThan:
        case OperationType::GreaterEqual:
            result = evaluateComparison(ago);
            break;
            
        case OperationType::And:
        case OperationType::Or:
        case OperationType::Not:
            result = evaluateLogical(ago);
            break;
            
        case OperationType::Positive:
        case OperationType::Negative:
        case OperationType::Absolute:
            result = evaluateUnary(ago);
            break;
            
        case OperationType::Sin:
        case OperationType::Cos:
        case OperationType::Tan:
        case OperationType::Log:
        case OperationType::Log10:
        case OperationType::Exp:
        case OperationType::Sqrt:
        case OperationType::Floor:
        case OperationType::Ceil:
        case OperationType::Round:
            result = evaluateMathFunction(ago);
            break;
    }
    
    // Cache result
    if (enable_caching_) {
        value_cache_[ago] = result;
    }
    
    return result;
}

void LineOperation::forward(double value, int size) {
    // Forward operation doesn't apply to lazy operations
    // Just invalidate cache
    invalidateCache();
}

void LineOperation::backwards(int size, bool force) {
    // Backwards operation doesn't apply to lazy operations
    // Just invalidate cache
    invalidateCache();
}

void LineOperation::reset() {
    LineBuffer::reset();
    invalidateCache();
}

void LineOperation::invalidateCache() {
    value_cache_.clear();
}

// ========== Evaluation Methods ==========

double LineOperation::evaluateArithmetic(int ago) const {
    if (operands_.size() < 2) {
        return NAN_VALUE;
    }
    
    double left = getValue(operands_[0], ago);
    double right = getValue(operands_[1], ago);
    
    if (std::isnan(left) || std::isnan(right)) {
        return NAN_VALUE;
    }
    
    switch (operation_type_) {
        case OperationType::Add:
            return left + right;
        case OperationType::Subtract:
            return left - right;
        case OperationType::Multiply:
            return left * right;
        case OperationType::Divide:
            return (right != 0.0) ? left / right : NAN_VALUE;
        case OperationType::Modulo:
            return (right != 0.0) ? std::fmod(left, right) : NAN_VALUE;
        case OperationType::Power:
            return std::pow(left, right);
        default:
            return NAN_VALUE;
    }
}

double LineOperation::evaluateComparison(int ago) const {
    if (operands_.size() < 2) {
        return NAN_VALUE;
    }
    
    double left = getValue(operands_[0], ago);
    double right = getValue(operands_[1], ago);
    
    if (std::isnan(left) || std::isnan(right)) {
        return NAN_VALUE;
    }
    
    bool result = false;
    constexpr double EPSILON = 1e-9;
    
    switch (operation_type_) {
        case OperationType::Equal:
            result = std::abs(left - right) < EPSILON;
            break;
        case OperationType::NotEqual:
            result = std::abs(left - right) >= EPSILON;
            break;
        case OperationType::LessThan:
            result = left < right;
            break;
        case OperationType::LessEqual:
            result = left <= right || std::abs(left - right) < EPSILON;
            break;
        case OperationType::GreaterThan:
            result = left > right;
            break;
        case OperationType::GreaterEqual:
            result = left >= right || std::abs(left - right) < EPSILON;
            break;
        default:
            return NAN_VALUE;
    }
    
    return result ? 1.0 : 0.0;
}

double LineOperation::evaluateLogical(int ago) const {
    switch (operation_type_) {
        case OperationType::Not:
            if (operands_.size() >= 1) {
                double value = getValue(operands_[0], ago);
                if (!std::isnan(value)) {
                    return (value == 0.0) ? 1.0 : 0.0;
                }
            }
            return NAN_VALUE;
            
        case OperationType::And:
        case OperationType::Or:
            if (operands_.size() >= 2) {
                double left = getValue(operands_[0], ago);
                double right = getValue(operands_[1], ago);
                
                if (std::isnan(left) || std::isnan(right)) {
                    return NAN_VALUE;
                }
                
                bool left_bool = (left != 0.0);
                bool right_bool = (right != 0.0);
                
                if (operation_type_ == OperationType::And) {
                    return (left_bool && right_bool) ? 1.0 : 0.0;
                } else {
                    return (left_bool || right_bool) ? 1.0 : 0.0;
                }
            }
            return NAN_VALUE;
            
        default:
            return NAN_VALUE;
    }
}

double LineOperation::evaluateUnary(int ago) const {
    if (operands_.empty()) {
        return NAN_VALUE;
    }
    
    double value = getValue(operands_[0], ago);
    if (std::isnan(value)) {
        return NAN_VALUE;
    }
    
    switch (operation_type_) {
        case OperationType::Positive:
            return value;
        case OperationType::Negative:
            return -value;
        case OperationType::Absolute:
            return std::abs(value);
        default:
            return NAN_VALUE;
    }
}

double LineOperation::evaluateMathFunction(int ago) const {
    if (operands_.empty()) {
        return NAN_VALUE;
    }
    
    double value = getValue(operands_[0], ago);
    if (std::isnan(value)) {
        return NAN_VALUE;
    }
    
    switch (operation_type_) {
        case OperationType::Sin:
            return std::sin(value);
        case OperationType::Cos:
            return std::cos(value);
        case OperationType::Tan:
            return std::tan(value);
        case OperationType::Log:
            return (value > 0.0) ? std::log(value) : NAN_VALUE;
        case OperationType::Log10:
            return (value > 0.0) ? std::log10(value) : NAN_VALUE;
        case OperationType::Exp:
            return std::exp(value);
        case OperationType::Sqrt:
            return (value >= 0.0) ? std::sqrt(value) : NAN_VALUE;
        case OperationType::Floor:
            return std::floor(value);
        case OperationType::Ceil:
            return std::ceil(value);
        case OperationType::Round:
            return std::round(value);
        default:
            return NAN_VALUE;
    }
}

// ========== Arithmetic Operators ==========

LineOperation LineOperation::operator+(const Operand& other) const {
    return LineOperation(OperationType::Add, {
        std::make_shared<LineOperation>(*this), other
    });
}

LineOperation LineOperation::operator-(const Operand& other) const {
    return LineOperation(OperationType::Subtract, {
        std::make_shared<LineOperation>(*this), other
    });
}

LineOperation LineOperation::operator*(const Operand& other) const {
    return LineOperation(OperationType::Multiply, {
        std::make_shared<LineOperation>(*this), other
    });
}

LineOperation LineOperation::operator/(const Operand& other) const {
    return LineOperation(OperationType::Divide, {
        std::make_shared<LineOperation>(*this), other
    });
}

// ========== Comparison Operators ==========

LineOperation LineOperation::operator==(const Operand& other) const {
    return LineOperation(OperationType::Equal, {
        std::make_shared<LineOperation>(*this), other
    });
}

LineOperation LineOperation::operator<(const Operand& other) const {
    return LineOperation(OperationType::LessThan, {
        std::make_shared<LineOperation>(*this), other
    });
}

LineOperation LineOperation::operator>(const Operand& other) const {
    return LineOperation(OperationType::GreaterThan, {
        std::make_shared<LineOperation>(*this), other
    });
}

// ========== Mathematical Functions ==========

LineOperation LineOperation::abs() const {
    return LineOperation(OperationType::Absolute, {
        std::make_shared<LineOperation>(*this)
    });
}

LineOperation LineOperation::sin() const {
    return LineOperation(OperationType::Sin, {
        std::make_shared<LineOperation>(*this)
    });
}

LineOperation LineOperation::cos() const {
    return LineOperation(OperationType::Cos, {
        std::make_shared<LineOperation>(*this)
    });
}

LineOperation LineOperation::sqrt() const {
    return LineOperation(OperationType::Sqrt, {
        std::make_shared<LineOperation>(*this)
    });
}

// ========== Utility Methods ==========

void LineOperation::precompute(int periods) {
    for (int i = 0; i < periods; ++i) {
        (*this)[-i]; // This will cache the value
    }
}

std::string LineOperation::getExpressionString() const {
    std::ostringstream oss;
    
    switch (operation_type_) {
        case OperationType::Add:
            oss << "(+ ";
            break;
        case OperationType::Subtract:
            oss << "(- ";
            break;
        case OperationType::Multiply:
            oss << "(* ";
            break;
        case OperationType::Divide:
            oss << "(/ ";
            break;
        case OperationType::Sin:
            oss << "(sin ";
            break;
        case OperationType::Cos:
            oss << "(cos ";
            break;
        default:
            oss << "(op" << static_cast<int>(operation_type_) << " ";
            break;
    }
    
    for (size_t i = 0; i < operands_.size(); ++i) {
        if (i > 0) oss << " ";
        
        std::visit([&oss](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, double>) {
                oss << std::fixed << std::setprecision(4) << arg;
            } else if constexpr (std::is_same_v<T, std::shared_ptr<LineOperation>>) {
                if (arg) {
                    oss << arg->getExpressionString();
                } else {
                    oss << "null_op";
                }
            } else {
                oss << "line";
            }
        }, operands_[i]);
    }
    
    oss << ")";
    return oss.str();
}

bool LineOperation::validate() const {
    // Check operand count based on operation type
    size_t expected_operands = 2; // Most operations are binary
    
    switch (operation_type_) {
        case OperationType::Not:
        case OperationType::Positive:
        case OperationType::Negative:
        case OperationType::Absolute:
        case OperationType::Sin:
        case OperationType::Cos:
        case OperationType::Tan:
        case OperationType::Log:
        case OperationType::Log10:
        case OperationType::Exp:
        case OperationType::Sqrt:
        case OperationType::Floor:
        case OperationType::Ceil:
        case OperationType::Round:
            expected_operands = 1;
            break;
        default:
            expected_operands = 2;
            break;
    }
    
    return operands_.size() == expected_operands;
}

// ========== Static Factory Methods ==========

std::shared_ptr<ArithmeticOperation> ArithmeticOperation::add(Operand left, Operand right) {
    return std::make_shared<ArithmeticOperation>(OperationType::Add, std::move(left), std::move(right));
}

std::shared_ptr<ArithmeticOperation> ArithmeticOperation::subtract(Operand left, Operand right) {
    return std::make_shared<ArithmeticOperation>(OperationType::Subtract, std::move(left), std::move(right));
}

std::shared_ptr<ArithmeticOperation> ArithmeticOperation::multiply(Operand left, Operand right) {
    return std::make_shared<ArithmeticOperation>(OperationType::Multiply, std::move(left), std::move(right));
}

std::shared_ptr<ArithmeticOperation> ArithmeticOperation::divide(Operand left, Operand right) {
    return std::make_shared<ArithmeticOperation>(OperationType::Divide, std::move(left), std::move(right));
}

// ========== Global Operator Overloads ==========

LineOperation operator+(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::Add, {
        std::make_shared<LineBuffer>(left), right
    });
}

LineOperation operator-(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::Subtract, {
        std::make_shared<LineBuffer>(left), right
    });
}

LineOperation operator*(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::Multiply, {
        std::make_shared<LineBuffer>(left), right
    });
}

LineOperation operator/(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::Divide, {
        std::make_shared<LineBuffer>(left), right
    });
}

LineOperation operator==(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::Equal, {
        std::make_shared<LineBuffer>(left), right
    });
}

LineOperation operator<(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::LessThan, {
        std::make_shared<LineBuffer>(left), right
    });
}

LineOperation operator>(const LineBuffer& left, const Operand& right) {
    return LineOperation(OperationType::GreaterThan, {
        std::make_shared<LineBuffer>(left), right
    });
}

// Scalar operators
LineOperation operator+(double left, const LineBuffer& right) {
    return LineOperation(OperationType::Add, {
        left, std::make_shared<LineBuffer>(right)
    });
}

LineOperation operator*(double left, const LineBuffer& right) {
    return LineOperation(OperationType::Multiply, {
        left, std::make_shared<LineBuffer>(right)
    });
}

// ========== Mathematical Function Wrappers ==========

LineOperation sin(const Operand& operand) {
    return LineOperation(OperationType::Sin, {operand});
}

LineOperation cos(const Operand& operand) {
    return LineOperation(OperationType::Cos, {operand});
}

LineOperation sqrt(const Operand& operand) {
    return LineOperation(OperationType::Sqrt, {operand});
}

LineOperation abs(const Operand& operand) {
    return LineOperation(OperationType::Absolute, {operand});
}

LineOperation log(const Operand& operand) {
    return LineOperation(OperationType::Log, {operand});
}

LineOperation pow(const Operand& base, const Operand& exponent) {
    return LineOperation(OperationType::Power, {base, exponent});
}

// ========== Utility Functions ==========

bool isConstant(const Operand& operand) {
    return std::holds_alternative<double>(operand);
}

double getConstantValue(const Operand& operand) {
    if (auto* value = std::get_if<double>(&operand)) {
        return *value;
    }
    return NAN_VALUE;
}

std::string operationToString(const LineOperation& operation) {
    return operation.getExpressionString();
}

} // namespace line
} // namespace backtrader