#pragma once

#include "../indicator.h"

namespace backtrader {

// Standard Pivot Point indicator
class PivotPoint : public Indicator {
public:
    struct Params {
        bool open = false;     // Add opening price to the pivot point
        bool close = false;    // Use close twice in the calculations
        bool _autoplot = true; // Attempt to plot on real target data
    } params;
    
    // Lines
    enum Lines {
        p = 0,   // Pivot point
        s1 = 1,  // Support 1
        s2 = 2,  // Support 2
        r1 = 3,  // Resistance 1
        r2 = 4   // Resistance 2
    };
    
    PivotPoint();
    virtual ~PivotPoint() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Fibonacci Pivot Point indicator
class FibonacciPivotPoint : public Indicator {
public:
    struct Params {
        bool open = false;      // Add opening price to the pivot point
        bool close = false;     // Use close twice in the calculations
        bool _autoplot = true;  // Attempt to plot on real target data
        double level1 = 0.382;  // Fibonacci level 1
        double level2 = 0.618;  // Fibonacci level 2
        double level3 = 1.0;    // Fibonacci level 3
    } params;
    
    // Lines
    enum Lines {
        p = 0,   // Pivot point
        s1 = 1,  // Support 1
        s2 = 2,  // Support 2
        s3 = 3,  // Support 3
        r1 = 4,  // Resistance 1
        r2 = 5,  // Resistance 2
        r3 = 6   // Resistance 3
    };
    
    FibonacciPivotPoint();
    virtual ~FibonacciPivotPoint() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Demark Pivot Point indicator
class DemarkPivotPoint : public Indicator {
public:
    struct Params {
        bool _autoplot = true;  // Attempt to plot on real target data
    } params;
    
    // Lines
    enum Lines {
        p = 0,   // Pivot point
        s1 = 1,  // Support 1
        r1 = 2   // Resistance 1
    };
    
    DemarkPivotPoint();
    virtual ~DemarkPivotPoint() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper method for Demark calculation
    double calculate_x_value(double open, double high, double low, double close);
};

// Aliases
using PP = PivotPoint;
using FibPP = FibonacciPivotPoint;
using DemarkPP = DemarkPivotPoint;

} // namespace backtrader