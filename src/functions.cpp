#include "functions.h"
#include <stdexcept>
#include <functional>

namespace backtrader {

// Helper function for comparison (equivalent to Python's cmp)
int cmp(double a, double b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

// List implementation
bool List::contains(std::shared_ptr<LineActions> other) const {
    if (!other) return false;
    auto other_hash = std::hash<void*>{}(other.get());
    for (const auto& item : *this) {
        if (item && std::hash<void*>{}(item.get()) == other_hash) {
            return true;
        }
    }
    return false;
}

// Logic implementation
Logic::Logic(const std::vector<std::shared_ptr<LineActions>>& args) : LineActions(), args(args) {
}

// DivByZero implementation
DivByZero::DivByZero(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b, double zero)
    : Logic({a, b}), a(a), b(b), zero(zero) {
}

void DivByZero::next() {
    double b_val = (*b)[0];
    (*this)[0] = (b_val != 0.0) ? (*a)[0] / b_val : zero;
}

void DivByZero::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        double b_val = (*b)[i];
        (*this)[i] = (b_val != 0.0) ? (*a)[i] / b_val : zero;
    }
}

// DivZeroByZero implementation
DivZeroByZero::DivZeroByZero(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b,
                             double single, double dual)
    : Logic({a, b}), a(a), b(b), single(single), dual(dual) {
}

void DivZeroByZero::next() {
    double b_val = (*b)[0];
    double a_val = (*a)[0];
    
    if (b_val == 0.0) {
        (*this)[0] = (a_val == 0.0) ? dual : single;
    } else {
        (*this)[0] = a_val / b_val;
    }
}

void DivZeroByZero::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        double b_val = (*b)[i];
        double a_val = (*a)[i];
        
        if (b_val == 0.0) {
            (*this)[i] = (a_val == 0.0) ? dual : single;
        } else {
            (*this)[i] = a_val / b_val;
        }
    }
}

// Cmp implementation
Cmp::Cmp(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b)
    : Logic({a, b}), a(a), b(b) {
}

void Cmp::next() {
    (*this)[0] = cmp((*a)[0], (*b)[0]);
}

void Cmp::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        (*this)[i] = cmp((*a)[i], (*b)[i]);
    }
}

// CmpEx implementation
CmpEx::CmpEx(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b,
             std::shared_ptr<LineActions> r1, std::shared_ptr<LineActions> r2, 
             std::shared_ptr<LineActions> r3)
    : Logic({a, b, r1, r2, r3}), a(a), b(b), r1(r1), r2(r2), r3(r3) {
}

void CmpEx::next() {
    double a_val = (*a)[0];
    double b_val = (*b)[0];
    
    if (a_val < b_val) {
        (*this)[0] = (*r1)[0];
    } else if (a_val > b_val) {
        (*this)[0] = (*r3)[0];
    } else {
        (*this)[0] = (*r2)[0];
    }
}

void CmpEx::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        double a_val = (*a)[i];
        double b_val = (*b)[i];
        
        if (a_val < b_val) {
            (*this)[i] = (*r1)[i];
        } else if (a_val > b_val) {
            (*this)[i] = (*r3)[i];
        } else {
            (*this)[i] = (*r2)[i];
        }
    }
}

// If implementation
If::If(std::shared_ptr<LineActions> cond, std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b)
    : Logic({a, b}), cond(cond), a(a), b(b) {
}

void If::next() {
    (*this)[0] = ((*cond)[0] != 0.0) ? (*a)[0] : (*b)[0];
}

void If::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        (*this)[i] = ((*cond)[i] != 0.0) ? (*a)[i] : (*b)[i];
    }
}

// MultiLogic implementation
MultiLogic::MultiLogic(const std::vector<std::shared_ptr<LineActions>>& args)
    : Logic(args) {
}

void MultiLogic::next() {
    std::vector<double> values;
    values.reserve(args.size());
    for (const auto& arg : args) {
        values.push_back((*arg)[0]);
    }
    (*this)[0] = flogic(values);
}

void MultiLogic::once(int start, int end) {
    for (int i = start; i < end; ++i) {
        std::vector<double> values;
        values.reserve(args.size());
        for (const auto& arg : args) {
            values.push_back((*arg)[i]);
        }
        (*this)[i] = flogic(values);
    }
}

// MultiLogicReduce implementation
MultiLogicReduce::MultiLogicReduce(const std::vector<std::shared_ptr<LineActions>>& args, double initializer)
    : MultiLogic(args), initializer_(initializer), use_initializer_(true) {
}

double MultiLogicReduce::flogic(const std::vector<double>& values) {
    if (values.empty()) {
        return use_initializer_ ? initializer_ : 0.0;
    }
    
    double result = use_initializer_ ? initializer_ : values[0];
    size_t start_idx = use_initializer_ ? 0 : 1;
    
    for (size_t i = start_idx; i < values.size(); ++i) {
        result = reduce_function(result, values[i]);
    }
    
    return result;
}

// Reduce implementation
Reduce::Reduce(std::function<double(double, double)> func, 
               const std::vector<std::shared_ptr<LineActions>>& args, 
               double initializer)
    : MultiLogicReduce(args, initializer), func_(func) {
}

double Reduce::reduce_function(double x, double y) {
    return func_(x, y);
}

// And implementation
And::And(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogicReduce(args, 1.0) {
}

double And::reduce_function(double x, double y) {
    return (x != 0.0 && y != 0.0) ? 1.0 : 0.0;
}

// Or implementation
Or::Or(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogicReduce(args, 0.0) {
}

double Or::reduce_function(double x, double y) {
    return (x != 0.0 || y != 0.0) ? 1.0 : 0.0;
}

// Max implementation
Max::Max(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogic(args) {
}

double Max::flogic(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return *std::max_element(values.begin(), values.end());
}

// Min implementation
Min::Min(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogic(args) {
}

double Min::flogic(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    return *std::min_element(values.begin(), values.end());
}

// Sum implementation
Sum::Sum(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogic(args) {
}

double Sum::flogic(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0);
}

// Any implementation
Any::Any(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogic(args) {
}

double Any::flogic(const std::vector<double>& values) {
    for (double val : values) {
        if (val != 0.0) return 1.0;
    }
    return 0.0;
}

// All implementation
All::All(const std::vector<std::shared_ptr<LineActions>>& args)
    : MultiLogic(args) {
}

double All::flogic(const std::vector<double>& values) {
    for (double val : values) {
        if (val == 0.0) return 0.0;
    }
    return 1.0;
}

} // namespace backtrader