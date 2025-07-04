#pragma once

#include "../indicator.h"
#include "../lineseries.h"

namespace backtrader {
namespace indicators {

// UpMove Indicator - calculates upward price movement
class UpMove : public Indicator {
public:
    // Lines
    enum Lines { 
        upmove = 0
    };
    
    UpMove();
    UpMove(std::shared_ptr<LineSeries> data_source);
    virtual ~UpMove() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// DownMove Indicator - calculates downward price movement
class DownMove : public Indicator {
public:
    // Lines
    enum Lines { 
        downmove = 0
    };
    
    DownMove();
    DownMove(std::shared_ptr<LineSeries> data_source);
    virtual ~DownMove() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

} // namespace indicators
} // namespace backtrader