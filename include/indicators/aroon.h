#pragma once

#include "../indicator.h"
#include "../lineroot.h"
#include "../lineseries.h"
#include <memory>

namespace backtrader {

// Base class for Aroon indicators
class AroonBase : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for calculation
        double upperband = 70.0;  // Upper band level
        double lowerband = 30.0;  // Lower band level
    } params;
    
    AroonBase(bool calc_up = false, bool calc_down = false);
    virtual ~AroonBase() = default;
    
    // Calculate method (called by tests)
    void calculate() override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    virtual void setup_lines() = 0;
    virtual void calculate_lines() = 0;
    
    // Helper methods
    int find_highest_index(int period);
    int find_lowest_index(int period);
    int find_highest_index_at_position(int position, int period);
    int find_lowest_index_at_position(int position, int period);
    virtual void calculate_lines_at_position(int position);
    virtual void set_nan_values_at_position(int position);
    
    // Flags for what to calculate
    bool calc_up_;
    bool calc_down_;
    
    // Intermediate values
    double up_value_;
    double down_value_;
};

// AroonUp indicator
class AroonUp : public AroonBase {
public:
    // Lines
    enum Lines { 
        aroonup = 0
    };
    
    AroonUp();
    virtual ~AroonUp() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get minimum period
    int getMinPeriod() const;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// AroonDown indicator
class AroonDown : public AroonBase {
public:
    // Lines
    enum Lines { 
        aroondown = 0
    };
    
    AroonDown();
    virtual ~AroonDown() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get minimum period
    int getMinPeriod() const;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// AroonUpDown indicator (combines both AroonUp and AroonDown)
class AroonUpDown : public AroonBase {
public:
    // Lines
    enum Lines { 
        aroonup = 0,
        aroondown = 1
    };
    
    AroonUpDown();
    AroonUpDown(std::shared_ptr<DataSeries> data_source, int period = 14);
    AroonUpDown(std::shared_ptr<LineSeries> data_source, int period = 14); // Constructor for single LineSeries (test framework compatibility)
    AroonUpDown(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low, int period = 14); // Constructor for LineSeries
    virtual ~AroonUpDown() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get AroonUp and AroonDown values
    double getAroonUp(int ago = 0) const;
    double getAroonDown(int ago = 0) const;
    
    // Get minimum period
    int getMinPeriod() const;
    
    // Size method
    size_t size() const;
    
    // Calculate method (override for test framework compatibility)
    void calculate() override;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
    
private:

};

// Aroon Oscillator (AroonUp - AroonDown) - standalone implementation
class AroonOscillator : public Indicator {
public:
    struct Params {
        int period = 14;          // Period for calculation
        double upperband = 70.0;  // Upper band level
        double lowerband = 30.0;  // Lower band level
    } params;
    
    // Lines
    enum Lines { 
        aroonosc = 0
    };

private:

public:
    AroonOscillator();
    AroonOscillator(std::shared_ptr<LineSeries> data); // Constructor for test framework compatibility
    AroonOscillator(std::shared_ptr<LineSeries> data, int period); // Constructor with period for test framework
    AroonOscillator(std::shared_ptr<LineSeries> high, std::shared_ptr<LineSeries> low, int period = 14); // Constructor for high/low LineSeries
    virtual ~AroonOscillator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get minimum period
    int getMinPeriod() const;
    
    // Size method
    size_t size() const override;
    
    // Calculate method (override for test framework compatibility)
    void calculate() override;
    
protected:
    void setup_lines();
    void calculate_lines();
    void calculate_lines_at_position(int position);
};

// Combined AroonUpDown and AroonOscillator
class AroonUpDownOscillator : public AroonBase {
public:
    // Lines
    enum Lines { 
        aroonup = 0,
        aroondown = 1,
        aroonosc = 2
    };
    
    AroonUpDownOscillator();
    virtual ~AroonUpDownOscillator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get minimum period
    int getMinPeriod() const;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// Aliases
using AroonIndicator = AroonUpDown;
using AroonOsc = AroonOscillator;
using AroonUpDownOsc = AroonUpDownOscillator;

} // namespace backtrader