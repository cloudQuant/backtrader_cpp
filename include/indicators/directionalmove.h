#pragma once

#include "../indicator.h"
#include "atr.h"
#include "smma.h"
#include <memory>

namespace backtrader {

// Up Move indicator
class UpMove : public Indicator {
public:
    // Lines
    enum Lines { 
        upmove = 0
    };
    
    UpMove();
    virtual ~UpMove() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Down Move indicator
class DownMove : public Indicator {
public:
    // Lines
    enum Lines { 
        downmove = 0
    };
    
    DownMove();
    virtual ~DownMove() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
};

// Base class for Directional Indicators
class DirectionalIndicatorBase : public Indicator {
public:
    struct Params {
        int period = 14;  // Period for calculation
    } params;
    
    DirectionalIndicatorBase(bool calc_plus = true, bool calc_minus = true);
    virtual ~DirectionalIndicatorBase() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    virtual void setup_lines() = 0;
    virtual void calculate_indicators() = 0;
    
    // Helper methods
    double calculate_plus_dm();
    double calculate_minus_dm();
    
    // Sub-indicators
    std::shared_ptr<indicators::AverageTrueRange> atr_;
    std::shared_ptr<indicators::SmoothedMovingAverage> plus_dm_smma_;
    std::shared_ptr<indicators::SmoothedMovingAverage> minus_dm_smma_;
    
    // Flags for what to calculate
    bool calc_plus_;
    bool calc_minus_;
    
    // Intermediate values
    double di_plus_;
    double di_minus_;
    
    // Storage for DM values
    std::vector<double> plus_dm_values_;
    std::vector<double> minus_dm_values_;
};

// Plus Directional Indicator (+DI)
class PlusDirectionalIndicator : public DirectionalIndicatorBase {
public:
    // Lines
    enum Lines { 
        plusDI = 0
    };
    
    PlusDirectionalIndicator();
    virtual ~PlusDirectionalIndicator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void setup_lines() override;
    void calculate_indicators() override;
};

// Minus Directional Indicator (-DI)
class MinusDirectionalIndicator : public DirectionalIndicatorBase {
public:
    // Lines
    enum Lines { 
        minusDI = 0
    };
    
    MinusDirectionalIndicator();
    virtual ~MinusDirectionalIndicator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void setup_lines() override;
    void calculate_indicators() override;
};

// Directional Indicator (both +DI and -DI)
class DirectionalIndicator : public DirectionalIndicatorBase {
public:
    // Lines
    enum Lines { 
        plusDI = 0,
        minusDI = 1
    };
    
    DirectionalIndicator();
    virtual ~DirectionalIndicator() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void setup_lines() override;
    void calculate_indicators() override;
};

// Average Directional Movement Index (ADX)
class AverageDirectionalMovementIndex : public DirectionalIndicatorBase {
public:
    // Lines
    enum Lines { 
        adx = 0
    };
    
    AverageDirectionalMovementIndex();
    virtual ~AverageDirectionalMovementIndex() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    void setup_lines() override;
    void calculate_indicators() override;
    
private:
    std::shared_ptr<indicators::SmoothedMovingAverage> dx_smma_;
    std::vector<double> dx_values_;
};

// Average Directional Movement Index Rating (ADXR)
class AverageDirectionalMovementIndexRating : public AverageDirectionalMovementIndex {
public:
    // Lines
    enum Lines { 
        adx = 0,
        adxr = 1
    };
    
    AverageDirectionalMovementIndexRating();
    virtual ~AverageDirectionalMovementIndexRating() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
    void setup_lines() override;
};

// Directional Movement Index (DMI) - combines ADX with +DI and -DI
class DirectionalMovementIndex : public AverageDirectionalMovementIndex {
public:
    // Lines
    enum Lines { 
        adx = 0,
        plusDI = 1,
        minusDI = 2
    };
    
    DirectionalMovementIndex();
    virtual ~DirectionalMovementIndex() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void setup_lines() override;
    void calculate_indicators() override;
};

// Directional Movement (DM) - combines ADXR with +DI and -DI
class DirectionalMovement : public AverageDirectionalMovementIndexRating {
public:
    // Lines
    enum Lines { 
        adx = 0,
        adxr = 1,
        plusDI = 2,
        minusDI = 3
    };
    
    DirectionalMovement();
    virtual ~DirectionalMovement() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
protected:
    void setup_lines() override;
    void calculate_indicators() override;
};

// Aliases
using DI = DirectionalIndicator;
using PlusDI = PlusDirectionalIndicator;
using MinusDI = MinusDirectionalIndicator;
using ADX = AverageDirectionalMovementIndex;
using ADXR = AverageDirectionalMovementIndexRating;
using DMI = DirectionalMovementIndex;
using DM = DirectionalMovement;

} // namespace backtrader