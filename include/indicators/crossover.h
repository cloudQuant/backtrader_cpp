#pragma once

#include "../indicator.h"

namespace backtrader {

// Non-Zero Difference Indicator
class NonZeroDifference : public Indicator {
public:
    // Lines
    enum Lines { 
        nzd = 0  // Non-zero difference
    };
    
    NonZeroDifference();
    virtual ~NonZeroDifference() = default;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineActions> data);
    
protected:
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    void oncestart(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<LineActions> data0_;
    std::shared_ptr<LineActions> data1_;
    double last_nzd_;
};

// Base class for cross indicators
class CrossBase : public Indicator {
public:
    // Lines
    enum Lines { 
        cross = 0
    };
    
    CrossBase(bool crossup);
    virtual ~CrossBase() = default;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineActions> data);
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    bool crossup_;
    std::shared_ptr<LineActions> data0_;
    std::shared_ptr<LineActions> data1_;
    std::shared_ptr<NonZeroDifference> nzd_;
};

// Cross Up Indicator - detects upward crossovers
class CrossUp : public CrossBase {
public:
    CrossUp();
    virtual ~CrossUp() = default;
};

// Cross Down Indicator - detects downward crossovers  
class CrossDown : public CrossBase {
public:
    CrossDown();
    virtual ~CrossDown() = default;
};

// CrossOver Indicator - detects both up and down crossovers
class CrossOver : public Indicator {
public:
    // Lines
    enum Lines { 
        crossover = 0  // +1.0 for up cross, -1.0 for down cross, 0.0 for no cross
    };
    
    CrossOver();
    CrossOver(std::shared_ptr<LineActions> data0, std::shared_ptr<LineActions> data1);
    virtual ~CrossOver() = default;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineActions> data);
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<LineActions> data0_;
    std::shared_ptr<LineActions> data1_;
    std::shared_ptr<CrossUp> upcross_;
    std::shared_ptr<CrossDown> downcross_;
};

// Alias
using NZD = NonZeroDifference;

} // namespace backtrader