#include "lineroot.h"
#include "linebuffer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace backtrader {

size_t LineRoot::next_id_ = 0;

LineRoot::LineRoot() 
    : _owner(nullptr), _opstage(0), _ltype(IndType::IndType), minperiod_(1), id_(next_id_++) {
}

LineRoot::LineRoot(size_t size, const std::string& name) 
    : _owner(nullptr), _opstage(0), _ltype(IndType::IndType), minperiod_(1), id_(next_id_++) {
    // Basic constructor for test compatibility
    // Note: This constructor creates a basic LineRoot but it's not fully functional
    // Tests should use LineBuffer instead of LineRoot for concrete functionality
}


void LineRoot::updateminperiod(size_t minperiod) {
    minperiod_ = std::max(minperiod_, minperiod);
}

void LineRoot::addminperiod(size_t minperiod) {
    minperiod_ += minperiod;
}

void LineRoot::incminperiod(size_t minperiod) {
    minperiod_ += minperiod;
}

// Arithmetic operations - placeholders for now
LineRoot* LineRoot::operator+(const LineRoot& other) const {
    throw std::runtime_error("Arithmetic operations not implemented in base class");
}

LineRoot* LineRoot::operator-(const LineRoot& other) const {
    throw std::runtime_error("Arithmetic operations not implemented in base class");
}

LineRoot* LineRoot::operator*(const LineRoot& other) const {
    throw std::runtime_error("Arithmetic operations not implemented in base class");
}

LineRoot* LineRoot::operator/(const LineRoot& other) const {
    throw std::runtime_error("Arithmetic operations not implemented in base class");
}

LineRoot* LineRoot::operator<(const LineRoot& other) const {
    throw std::runtime_error("Comparison operations not implemented in base class");
}

LineRoot* LineRoot::operator<=(const LineRoot& other) const {
    throw std::runtime_error("Comparison operations not implemented in base class");
}

LineRoot* LineRoot::operator>(const LineRoot& other) const {
    throw std::runtime_error("Comparison operations not implemented in base class");
}

LineRoot* LineRoot::operator>=(const LineRoot& other) const {
    throw std::runtime_error("Comparison operations not implemented in base class");
}

LineRoot* LineRoot::operator==(const LineRoot& other) const {
    throw std::runtime_error("Comparison operations not implemented in base class");
}

LineRoot* LineRoot::operator!=(const LineRoot& other) const {
    throw std::runtime_error("Comparison operations not implemented in base class");
}

LineRoot* LineRoot::operator&&(const LineRoot& other) const {
    throw std::runtime_error("Logical operations not implemented in base class");
}

LineRoot* LineRoot::operator||(const LineRoot& other) const {
    throw std::runtime_error("Logical operations not implemented in base class");
}

LineRoot* LineRoot::operator-() const {
    throw std::runtime_error("Unary operations not implemented in base class");
}

LineRoot* LineRoot::operator!() const {
    throw std::runtime_error("Unary operations not implemented in base class");
}

LineRoot* LineRoot::abs() const {
    throw std::runtime_error("Math functions not implemented in base class");
}

LineRoot* LineRoot::pow(double exponent) const {
    throw std::runtime_error("Math functions not implemented in base class");
}

// LineSingle implementation
LineSingle::LineSingle() : LineRoot() {
}

void LineSingle::updateminperiod(size_t minperiod) {
    LineRoot::updateminperiod(minperiod);
}

void LineSingle::addminperiod(size_t minperiod) {
    LineRoot::addminperiod(minperiod);
}

void LineSingle::incminperiod(size_t minperiod) {
    LineRoot::incminperiod(minperiod);
}

// LineMultiple implementation
LineMultiple::LineMultiple() : LineRoot() {
}

double LineMultiple::operator[](int index) const {
    if (lines_.empty()) {
        throw std::runtime_error("No lines available");
    }
    return (*lines_[0])[index];
}

void LineMultiple::set(int index, double value) {
    if (lines_.empty()) {
        throw std::runtime_error("No lines available");
    }
    lines_[0]->set(index, value);
}

void LineMultiple::forward(size_t size) {
    for (auto& line : lines_) {
        line->forward(size);
    }
}

void LineMultiple::backward(size_t size) {
    for (auto& line : lines_) {
        line->backward(size);
    }
}

void LineMultiple::rewind(size_t size) {
    for (auto& line : lines_) {
        line->rewind(size);
    }
}

void LineMultiple::extend(size_t size) {
    for (auto& line : lines_) {
        line->extend(size);
    }
}

void LineMultiple::reset() {
    for (auto& line : lines_) {
        line->reset();
    }
}

void LineMultiple::home() {
    for (auto& line : lines_) {
        line->home();
    }
}

size_t LineMultiple::buflen() const {
    if (lines_.empty()) {
        return 0;
    }
    return lines_[0]->buflen();
}

void LineMultiple::advance(size_t size) {
    for (auto& line : lines_) {
        line->advance(size);
    }
}

void LineMultiple::addbinding(std::shared_ptr<LineSingle> binding) {
    if (!lines_.empty()) {
        lines_[0]->addbinding(binding);
    }
}

void LineMultiple::oncebinding() {
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

void LineMultiple::bind2line(std::shared_ptr<LineSingle> binding) {
    if (!lines_.empty()) {
        lines_[0]->bind2line(binding);
    }
}

void LineMultiple::updateminperiod(size_t minperiod) {
    LineRoot::updateminperiod(minperiod);
    for (auto& line : lines_) {
        line->updateminperiod(minperiod);
    }
}

void LineMultiple::addminperiod(size_t minperiod) {
    LineRoot::addminperiod(minperiod);
    for (auto& line : lines_) {
        line->addminperiod(minperiod);
    }
}

void LineMultiple::incminperiod(size_t minperiod) {
    LineRoot::incminperiod(minperiod);
    for (auto& line : lines_) {
        line->incminperiod(minperiod);
    }
}

size_t LineMultiple::size() const {
    if (lines_.empty()) {
        return 0;
    }
    return lines_[0]->size();
}

bool LineMultiple::empty() const {
    if (lines_.empty()) {
        return true;
    }
    return lines_[0]->empty();
}

} // namespace backtrader