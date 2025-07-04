#pragma once

#include "linebuffer.h"
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

namespace backtrader {

// Forward declarations
class LineActions;

// List class equivalent that uses hash for contains
class List : public std::vector<std::shared_ptr<LineActions>> {
public:
    bool contains(std::shared_ptr<LineActions> other) const;
};

// Base class for logical operations
class Logic : public LineActions {
public:
    Logic(const std::vector<std::shared_ptr<LineActions>>& args);
    virtual ~Logic() = default;

protected:
    std::vector<std::shared_ptr<LineActions>> args;
};

// Division by zero safe operation
class DivByZero : public Logic {
public:
    DivByZero(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b, double zero = 0.0);
    
    void next() override;
    void once(int start, int end) override;

private:
    std::shared_ptr<LineActions> a;
    std::shared_ptr<LineActions> b;
    double zero;
};

// Division handling both numerator and denominator being zero
class DivZeroByZero : public Logic {
public:
    DivZeroByZero(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b,
                  double single = std::numeric_limits<double>::infinity(),
                  double dual = 0.0);
    
    void next() override;
    void once(int start, int end) override;

private:
    std::shared_ptr<LineActions> a;
    std::shared_ptr<LineActions> b;
    double single;
    double dual;
};

// Comparison operation
class Cmp : public Logic {
public:
    Cmp(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b);
    
    void next() override;
    void once(int start, int end) override;

private:
    std::shared_ptr<LineActions> a;
    std::shared_ptr<LineActions> b;
};

// Extended comparison with different return values
class CmpEx : public Logic {
public:
    CmpEx(std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b,
          std::shared_ptr<LineActions> r1, std::shared_ptr<LineActions> r2, 
          std::shared_ptr<LineActions> r3);
    
    void next() override;
    void once(int start, int end) override;

private:
    std::shared_ptr<LineActions> a;
    std::shared_ptr<LineActions> b;
    std::shared_ptr<LineActions> r1;
    std::shared_ptr<LineActions> r2;
    std::shared_ptr<LineActions> r3;
};

// If-else conditional operation
class If : public Logic {
public:
    If(std::shared_ptr<LineActions> cond, std::shared_ptr<LineActions> a, std::shared_ptr<LineActions> b);
    
    void next() override;
    void once(int start, int end) override;

private:
    std::shared_ptr<LineActions> cond;
    std::shared_ptr<LineActions> a;
    std::shared_ptr<LineActions> b;
};

// Base class for multi-argument logical operations
class MultiLogic : public Logic {
public:
    MultiLogic(const std::vector<std::shared_ptr<LineActions>>& args);
    
    void next() override;
    void once(int start, int end) override;

protected:
    virtual double flogic(const std::vector<double>& values) = 0;
};

// Reduce operation with function
class MultiLogicReduce : public MultiLogic {
public:
    MultiLogicReduce(const std::vector<std::shared_ptr<LineActions>>& args, double initializer = 0.0);

protected:
    double initializer_;
    bool use_initializer_;
    
    virtual double reduce_function(double x, double y) = 0;
    double flogic(const std::vector<double>& values) override;
};

// Generic reduce operation
class Reduce : public MultiLogicReduce {
public:
    Reduce(std::function<double(double, double)> func, 
           const std::vector<std::shared_ptr<LineActions>>& args, 
           double initializer = 0.0);

protected:
    std::function<double(double, double)> func_;
    double reduce_function(double x, double y) override;
};

// Logical AND operation
class And : public MultiLogicReduce {
public:
    And(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double reduce_function(double x, double y) override;
};

// Logical OR operation
class Or : public MultiLogicReduce {
public:
    Or(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double reduce_function(double x, double y) override;
};

// Maximum operation
class Max : public MultiLogic {
public:
    Max(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double flogic(const std::vector<double>& values) override;
};

// Minimum operation
class Min : public MultiLogic {
public:
    Min(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double flogic(const std::vector<double>& values) override;
};

// Sum operation
class Sum : public MultiLogic {
public:
    Sum(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double flogic(const std::vector<double>& values) override;
};

// Any operation (returns 1.0 if any value is true/non-zero)
class Any : public MultiLogic {
public:
    Any(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double flogic(const std::vector<double>& values) override;
};

// All operation (returns 1.0 if all values are true/non-zero)
class All : public MultiLogic {
public:
    All(const std::vector<std::shared_ptr<LineActions>>& args);

protected:
    double flogic(const std::vector<double>& values) override;
};

// Helper function for comparison (equivalent to Python's cmp)
int cmp(double a, double b);

} // namespace backtrader