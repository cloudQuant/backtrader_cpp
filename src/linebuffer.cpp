#include "linebuffer.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace backtrader {

LineBuffer::LineBuffer() 
    : LineSingle(), _idx(0), lenmark_(0), mode_(UnBounded) {
    array_.push_back(NAN_VALUE);
}

double LineBuffer::operator[](int index) const {
    _checkbounds(index);
    
    // Match Python indexing convention:
    // index 0 = current value
    // negative indices = past values (-1 = previous, -2 = two bars ago, etc.)
    // positive indices = future values (1 = next, 2 = two bars ahead, etc.)
    
    int actual_idx = _idx + index;  // Add index since negative means past
    
    // Bounds checking
    if (actual_idx < 0 || actual_idx >= static_cast<int>(array_.size())) {
        return NAN_VALUE;
    }
    
    return array_[actual_idx];
}

void LineBuffer::set(int index, double value) {
    // Match Python indexing convention
    int actual_idx = _idx + index;
    
    if (actual_idx < 0) {
        throw std::runtime_error("Cannot set values before buffer start");
    }
    
    if (actual_idx >= static_cast<int>(array_.size())) {
        array_.resize(actual_idx + 1, NAN_VALUE);
    }
    
    array_[actual_idx] = value;
    _makebinding(value);
}

size_t LineBuffer::size() const {
    // If array is empty (after clear()), return 0
    if (array_.empty()) {
        return 0;
    }
    return _idx + 1;
}

bool LineBuffer::empty() const {
    return _idx < 0;
}

size_t LineBuffer::buflen() const {
    return array_.size();
}

void LineBuffer::forward(size_t size) {
    _idx += size;
    if (_idx >= static_cast<int>(array_.size())) {
        array_.resize(_idx + 1, NAN_VALUE);
    }
}

void LineBuffer::backward(size_t size) {
    _idx -= size;
    if (_idx < 0) {
        _idx = 0;
    }
}

void LineBuffer::rewind(size_t size) {
    _idx -= size;
    if (_idx < 0) {
        _idx = 0;
    }
}

void LineBuffer::extend(size_t size) {
    array_.resize(array_.size() + size, NAN_VALUE);
}

void LineBuffer::reset() {
    _idx = 0;
    lenmark_ = 0;
    array_.clear();
    array_.push_back(NAN_VALUE);
}

void LineBuffer::clear() {
    _idx = -1;  // Set to -1 so first append() will set _idx to 0
    lenmark_ = 0;
    array_.clear();
    // Do NOT add initial NaN
}

void LineBuffer::home() {
    _idx = 0;
}

void LineBuffer::advance(size_t size) {
    forward(size);
}

int LineBuffer::get_idx() const {
    return _idx;
}

void LineBuffer::set_idx(int idx, bool force) {
    if (mode_ == QBuffer) {
        if (force || _idx < lenmark_) {
            _idx = idx;
        }
    } else {
        _idx = idx;
    }
}

void LineBuffer::qbuffer(size_t savemem) {
    mode_ = QBuffer;
    if (savemem > 0) {
        lenmark_ = savemem;
    }
}

void LineBuffer::minbuffer(size_t size) {
    if (mode_ == QBuffer && array_.size() > size) {
        // Keep only the last 'size' elements
        std::vector<double> new_array(array_.end() - size, array_.end());
        array_ = std::move(new_array);
        _idx = std::min(_idx, static_cast<int>(size - 1));
    }
}

void LineBuffer::addbinding(std::shared_ptr<LineSingle> binding) {
    bindings_.push_back(binding);
}

void LineBuffer::oncebinding() {
    // Called after once() operations to sync bindings
    for (auto& binding : bindings_) {
        binding->oncebinding();
    }
}

void LineBuffer::bind2line(std::shared_ptr<LineSingle> binding) {
    addbinding(binding);
}

double LineBuffer::get(int ago) const {
    return operator[](ago);
}

void LineBuffer::append(double value) {
    forward(1);
    set(0, value);
}

std::vector<double> LineBuffer::getrange(int start, int end) const {
    std::vector<double> result;
    for (int i = start; i < end; ++i) {
        result.push_back(operator[](i));
    }
    return result;
}

std::vector<double> LineBuffer::array() const {
    return array_;
}

void LineBuffer::_makebinding(double value) {
    for (auto& binding : bindings_) {
        binding->set(0, value);
    }
}

void LineBuffer::_checkbounds(int index) const {
    // Bounds checking is handled in operator[] implementation
    // This is a placeholder for additional validation if needed
}

// LineActions implementation
LineActions::LineActions() : LineBuffer() {
}

std::shared_ptr<LineActions> LineActions::operator+(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Add
    );
}

std::shared_ptr<LineActions> LineActions::operator-(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Sub
    );
}

std::shared_ptr<LineActions> LineActions::operator*(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Mul
    );
}

std::shared_ptr<LineActions> LineActions::operator/(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Div
    );
}

std::shared_ptr<LineActions> LineActions::operator<(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Lt
    );
}

std::shared_ptr<LineActions> LineActions::operator<=(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Le
    );
}

std::shared_ptr<LineActions> LineActions::operator>(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Gt
    );
}

std::shared_ptr<LineActions> LineActions::operator>=(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Ge
    );
}

std::shared_ptr<LineActions> LineActions::operator==(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Eq
    );
}

std::shared_ptr<LineActions> LineActions::operator!=(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Ne
    );
}

std::shared_ptr<LineActions> LineActions::operator&&(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::And
    );
}

std::shared_ptr<LineActions> LineActions::operator||(const LineActions& other) const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        std::const_pointer_cast<LineActions>(other.shared_from_this()),
        LinesOperation::Or
    );
}

std::shared_ptr<LineActions> LineActions::abs_action() const {
    return std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        LinesOperation::Abs
    );
}

std::shared_ptr<LineActions> LineActions::pow_action(double exponent) const {
    auto result = std::make_shared<LinesOperation>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        LinesOperation::Pow
    );
    // Note: This is a simplified implementation
    // In a complete implementation, we'd need to handle the exponent parameter
    return result;
}

std::shared_ptr<LineActions> LineActions::delay(int period) const {
    return std::make_shared<LineDelay>(
        std::const_pointer_cast<LineActions>(shared_from_this()),
        period
    );
}

std::shared_ptr<LineActions> LineActions::operator()(int period) const {
    return delay(period);
}

// LineNum implementation
LineNum::LineNum(double value) : LineActions(), value_(value) {
}

double LineNum::operator[](int index) const {
    return value_;
}

void LineNum::set(int index, double value) {
    if (index == 0) {
        value_ = value;
    }
}

size_t LineNum::size() const {
    return 1;
}

bool LineNum::empty() const {
    return false;
}

// LineDelay implementation
LineDelay::LineDelay(std::shared_ptr<LineActions> line, int period) 
    : LineActions(), line_(line), period_(period) {
}

double LineDelay::operator[](int index) const {
    return (*line_)[index + period_];
}

void LineDelay::set(int index, double value) {
    line_->set(index + period_, value);
}

size_t LineDelay::size() const {
    return line_->size();
}

bool LineDelay::empty() const {
    return line_->empty();
}

// LinesOperation implementation
LinesOperation::LinesOperation(std::shared_ptr<LineActions> lhs, std::shared_ptr<LineActions> rhs, OpType op)
    : LineActions(), lhs_(lhs), rhs_(rhs), op_(op), scalar_(0.0), use_scalar_(false) {
}

LinesOperation::LinesOperation(std::shared_ptr<LineActions> operand, OpType op)
    : LineActions(), lhs_(operand), rhs_(nullptr), op_(op), scalar_(0.0), use_scalar_(false) {
}

double LinesOperation::operator[](int index) const {
    if (rhs_ == nullptr) {
        // Unary operation
        return compute_unary((*lhs_)[index]);
    } else {
        // Binary operation
        return compute((*lhs_)[index], (*rhs_)[index]);
    }
}

void LinesOperation::set(int index, double value) {
    // Operations are read-only
    throw std::runtime_error("Cannot set values on operation results");
}

size_t LinesOperation::size() const {
    if (rhs_ == nullptr) {
        return lhs_->size();
    } else {
        return std::max(lhs_->size(), rhs_->size());
    }
}

bool LinesOperation::empty() const {
    if (rhs_ == nullptr) {
        return lhs_->empty();
    } else {
        return lhs_->empty() && rhs_->empty();
    }
}

double LinesOperation::compute(double a, double b) const {
    switch (op_) {
        case Add: return a + b;
        case Sub: return a - b;
        case Mul: return a * b;
        case Div: return (b != 0.0) ? a / b : NAN_VALUE;
        case Lt: return (a < b) ? 1.0 : 0.0;
        case Le: return (a <= b) ? 1.0 : 0.0;
        case Gt: return (a > b) ? 1.0 : 0.0;
        case Ge: return (a >= b) ? 1.0 : 0.0;
        case Eq: return (a == b) ? 1.0 : 0.0;
        case Ne: return (a != b) ? 1.0 : 0.0;
        case And: return (a != 0.0 && b != 0.0) ? 1.0 : 0.0;
        case Or: return (a != 0.0 || b != 0.0) ? 1.0 : 0.0;
        default: return NAN_VALUE;
    }
}

double LinesOperation::compute_unary(double a) const {
    switch (op_) {
        case Neg: return -a;
        case Not: return (a == 0.0) ? 1.0 : 0.0;
        case Abs: return std::abs(a);
        case Pow: return std::pow(a, scalar_);
        default: return NAN_VALUE;
    }
}

} // namespace backtrader