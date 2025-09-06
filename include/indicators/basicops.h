#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <functional>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {
namespace indicators {

// Base class for indicators which take a period
class PeriodN : public Indicator {
public:
    struct Params {
        int period = 1;
    } params;
    
    PeriodN();
    virtual ~PeriodN() = default;
    
protected:
    void setup_minperiod();
};

// Base class for operations that work with a period and a function
class OperationN : public PeriodN {
public:
    OperationN();
    virtual ~OperationN() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
    // Function to be implemented by subclasses
    virtual double calculate_func(const std::vector<double>& data) = 0;
    
    // Override this to specify which line to use for DataSeries
    // Default is 4 (close), but Lowest uses 3 (low), Highest uses 2 (high)
    virtual int get_dataseries_line_index() const { return 4; }
    
private:
    void setup_lines();
};

// Base class for indicators that take a function as parameter
class BaseApplyN : public OperationN {
public:
    struct Params {
        int period = 1;
        std::function<double(const std::vector<double>&)> func;
    } params;
    
    BaseApplyN();
    virtual ~BaseApplyN() = default;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
};

// Apply function to data over period
class ApplyN : public BaseApplyN {
public:
    enum LineIndex { apply = 0 };
    
    ApplyN();
    virtual ~ApplyN() = default;
    
private:
    void setup_lines();
};

// Calculate highest value over period
class Highest : public OperationN {
public:
    enum LineIndex { highest = 0 };
    
    Highest();
    // Constructor with LineSeries
    Highest(std::shared_ptr<LineSeries> data_source, int period = 14);
    // Constructor with DataSeries (for test framework compatibility)
    Highest(std::shared_ptr<DataSeries> data_source, int period = 14);
    virtual ~Highest() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Streaming execution methods
    void prenext() override;
    void nextstart() override;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Note: Lowest class moved to dedicated lowest.h file

// Calculate sum over period
class SumN : public OperationN {
public:
    enum LineIndex { sumn = 0 };
    
    SumN();
    SumN(std::shared_ptr<LineSeries> data_source, int period = 14);
    SumN(std::shared_ptr<DataSeries> data_source, int period = 14);
    virtual ~SumN() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Check if any value is non-zero over period
class AnyN : public OperationN {
public:
    enum LineIndex { anyn = 0 };
    
    AnyN();
    AnyN(std::shared_ptr<LineSeries> data_source, int period = 14);
    virtual ~AnyN() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Check if all values are non-zero over period
class AllN : public OperationN {
public:
    enum LineIndex { alln = 0 };
    
    AllN();
    AllN(std::shared_ptr<LineSeries> data_source, int period = 14);
    virtual ~AllN() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Find first index of condition over period
class FindFirstIndex : public OperationN {
public:
    struct Params {
        int period = 1;
        std::function<double(const std::vector<double>&)> evalfunc;
    } params;
    
    enum LineIndex { index = 0 };
    
    FindFirstIndex();
    virtual ~FindFirstIndex() = default;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    
private:
    void setup_lines();
};

// Find first index of highest value
class FindFirstIndexHighest : public FindFirstIndex {
public:
    FindFirstIndexHighest();
    virtual ~FindFirstIndexHighest() = default;
};

// Find first index of lowest value
class FindFirstIndexLowest : public FindFirstIndex {
public:
    FindFirstIndexLowest();
    virtual ~FindFirstIndexLowest() = default;
};

// Find last index of condition over period
class FindLastIndex : public OperationN {
public:
    struct Params {
        int period = 1;
        std::function<double(const std::vector<double>&)> evalfunc;
    } params;
    
    enum LineIndex { index = 0 };
    
    FindLastIndex();
    virtual ~FindLastIndex() = default;
    
protected:
    double calculate_func(const std::vector<double>& data) override;
    
private:
    void setup_lines();
};

// Find last index of highest value
class FindLastIndexHighest : public FindLastIndex {
public:
    FindLastIndexHighest();
    virtual ~FindLastIndexHighest() = default;
};

// Find last index of lowest value
class FindLastIndexLowest : public FindLastIndex {
public:
    FindLastIndexLowest();
    virtual ~FindLastIndexLowest() = default;
};

// Cumulative sum indicator
class Accum : public Indicator {
public:
    struct Params {
        double seed = 0.0;
    } params;
    
    enum LineIndex { accum = 0 };
    
    Accum();
    virtual ~Accum() = default;
    
protected:
    void nextstart() override;
    void next() override;
    void oncestart(int start, int end) override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Arithmetic average over period
class Average : public PeriodN {
public:
    enum LineIndex { av = 0 };
    
    Average();
    Average(std::shared_ptr<LineSeries> data_source, int period = 14);
    virtual ~Average() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Exponential smoothing average
class ExponentialSmoothing : public Average {
public:
    struct Params {
        int period = 1;
        double alpha = 0.0;  // If 0, will calculate as 2/(1+period)
    } params;
    
    ExponentialSmoothing();
    virtual ~ExponentialSmoothing() = default;
    
protected:
    void nextstart() override;
    void next() override;
    void oncestart(int start, int end) override;
    void once(int start, int end) override;
    
private:
    double alpha_;
    double alpha1_;
};

// Weighted average over period
class WeightedAverage : public PeriodN {
public:
    struct Params {
        int period = 1;
        double coef = 1.0;
        std::vector<double> weights;
    } params;
    
    enum LineIndex { av = 0 };
    
    WeightedAverage();
    virtual ~WeightedAverage() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Aliases
using MaxN = Highest;
// MinN alias moved to lowest.h
using CumSum = Accum;
using CumulativeSum = Accum;
using ArithmeticMean = Average;
using Mean = Average;
using ExpSmoothing = ExponentialSmoothing;
using AverageWeighted = WeightedAverage;

} // namespace indicators
} // namespace backtrader