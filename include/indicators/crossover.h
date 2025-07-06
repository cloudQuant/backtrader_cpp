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
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineActions> data);
    
public:
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
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineActions> data);
    
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
    // Constructor for indicator compatibility - accepts any types that can be converted to LineActions
    template<typename T1, typename T2>
    CrossOver(std::shared_ptr<T1> data0, std::shared_ptr<T2> data1, bool ignored = false) : CrossOver() {
        // Try to convert to LineActions and add
        auto line0 = std::dynamic_pointer_cast<LineActions>(data0);
        auto line1 = std::dynamic_pointer_cast<LineActions>(data1);
        if (line0 && line1) {
            data0_ = line0;
            data1_ = line1;
            setup_lines();
        }
    }
    virtual ~CrossOver() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const;
    
    // Get minimum period required for the indicator
    int getMinPeriod() const { return 2; }
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineActions> data);
    
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