#pragma once

#include "../dataseries.h"

#include "../indicator.h"
#include "../lineseries.h"
#include "sma.h"
#include <memory>

namespace backtrader {

// Base class for Stochastic indicators
class StochasticBase : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for highest/lowest calculation
        int period_dfast = 3;     // Period for fast %D calculation
        double upperband = 80.0;  // Upper band level
        double lowerband = 20.0;  // Lower band level
        bool safediv = false;     // Use safe division
        double safezero = 0.0;    // Safe division zero value
    } params;
    
    // Lines
    enum Lines { 
        percK = 0,  // %K line
        percD = 1   // %D line
    };
    
    StochasticBase();
    virtual ~StochasticBase() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    double get_d(int ago = 0) const;
    int getMinPeriod() const;
    virtual void calculate() override;
    size_t size() const override;
    
    // Line access methods for tests
    double getPercentK(int ago = 0) const;
    double getPercentD(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    virtual void once(int start, int end) override;
    
    virtual void setup_lines() = 0;
    virtual void calculate_lines() = 0;
    
    // Helper methods
    double get_highest(int period, int offset = 0);
    double get_lowest(int period, int offset = 0);
    
    // Sub-indicators
    std::shared_ptr<indicators::SMA> sma_fast_;
    std::shared_ptr<indicators::SMA> sma_slow_;
    
    // Intermediate values
    std::vector<double> k_values_;
    std::vector<double> d_values_;
};

// Fast Stochastic Oscillator
class StochasticFast : public StochasticBase {
public:
    StochasticFast();
    virtual ~StochasticFast() = default;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// Regular (Slow) Stochastic Oscillator
class Stochastic : public StochasticBase {
public:
    struct Params : public StochasticBase::Params {
        int period_dslow = 3;  // Period for slow %D calculation
    } params;
    
    Stochastic();
    Stochastic(std::shared_ptr<LineSeries> data_source, int period);
    Stochastic(std::shared_ptr<DataSeries> data_source);
    Stochastic(std::shared_ptr<DataSeries> data_source, int period);
    // Constructor for test framework with separate high/low/close lines
    Stochastic(std::shared_ptr<LineSeries> high_line, 
               std::shared_ptr<LineSeries> low_line,
               std::shared_ptr<LineSeries> close_line,
               int period = 14, int period_dfast = 3, int period_dslow = 3);
    virtual ~Stochastic() = default;
    
    // Add calculate and once methods for test framework  
    void calculate() override;
    void next() override;
    virtual void once(int start, int end) override;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
    
    // New calculation methods
    void calculate_with_separate_lines();
    void calculate_with_single_datasource();
    void calculate_stochastic_values(const std::vector<double>& high_array, 
                                   const std::vector<double>& low_array,
                                   const std::vector<double>& close_array,
                                   int data_size);
    
private:
    std::shared_ptr<indicators::SMA> sma_dslow_;
};

// Full Stochastic Oscillator (shows all three lines)
class StochasticFull : public StochasticBase {
public:
    struct Params : public StochasticBase::Params {
        int period_dslow = 3;  // Period for slow %D calculation
    } params;
    
    // Lines
    enum Lines { 
        percK = 0,     // %K line
        percD = 1,     // %D line
        percDSlow = 2  // %D Slow line
    };
    
    StochasticFull();
    StochasticFull(std::shared_ptr<LineSeries> data_source);
    StochasticFull(std::shared_ptr<DataSeries> data_source);
    StochasticFull(std::shared_ptr<DataSeries> data_source, int period, int period_dfast = 3, int period_dslow = 3);
    virtual ~StochasticFull() = default;
    
    void calculate() override;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
    
private:
    std::shared_ptr<indicators::SMA> sma_dslow_;
};

// Aliases
using StochasticSlow = Stochastic;

} // namespace backtrader