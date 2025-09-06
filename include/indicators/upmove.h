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
    UpMove(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructors for disambiguation
    UpMove(std::shared_ptr<DataSeries> data_source);
    UpMove(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~UpMove() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
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
    DownMove(std::shared_ptr<LineSeries> data_source, int period);
    // DataSeries constructors for disambiguation
    DownMove(std::shared_ptr<DataSeries> data_source);
    DownMove(std::shared_ptr<DataSeries> data_source, int period);
    virtual ~DownMove() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
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