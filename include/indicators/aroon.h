#pragma once

#include "../indicator.h"
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
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    virtual void setup_lines() = 0;
    virtual void calculate_lines() = 0;
    
    // Helper methods
    int find_highest_index(int period);
    int find_lowest_index(int period);
    
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
    virtual ~AroonUpDown() = default;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// Aroon Oscillator (AroonUp - AroonDown)
class AroonOscillator : public AroonBase {
public:
    // Lines
    enum Lines { 
        aroonosc = 0
    };
    
    AroonOscillator();
    virtual ~AroonOscillator() = default;
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
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
    
protected:
    void setup_lines() override;
    void calculate_lines() override;
};

// Aliases
using AroonIndicator = AroonUpDown;
using AroonOsc = AroonOscillator;
using AroonUpDownOsc = AroonUpDownOscillator;

} // namespace backtrader