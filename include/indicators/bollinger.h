#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include <deque>

namespace backtrader {
namespace indicators {

// Bollinger Bands Indicator
class BollingerBands : public Indicator {
public:
    struct Params {
        int period = 20;            // Period for moving average and standard deviation
        double devfactor = 2.0;     // Standard deviation factor
        // For simplicity, always use SMA (can be extended later)
    } params;
    
    // Lines
    enum Lines { 
        mid = 0,   // Middle band (moving average)
        top = 1,   // Upper band
        bot = 2    // Lower band
    };
    
    // Constructor with default parameters (original)
    BollingerBands();
    // Constructor for test framework compatibility
    BollingerBands(std::shared_ptr<LineSeries> data_source, int period = 20, double devfactor = 2.0);
    BollingerBands(std::shared_ptr<DataSeries> data_source, int period = 20, double devfactor = 2.0);
    virtual ~BollingerBands() = default;
    
    // Utility methods for tests
    double get(int ago = 0) const;
    int getMinPeriod() const { return params.period; }
    void calculate() override;
    
    // Additional utility methods
    double getBandwidth(int ago = 0) const;
    double getPercentB(int ago = 0) const;
    
    // Band accessor methods
    double getMiddleBand(int ago = 0) const;
    double getUpperBand(int ago = 0) const;
    double getLowerBand(int ago = 0) const;
    
    // Size method
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    double calculate_sma(int period, int current_index) const;
    double calculate_stddev(int period, int current_index, double mean) const;
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
    std::deque<double> values_;  // Rolling window of values for incremental calculation
};

// Bollinger Bands Percentage
class BollingerBandsPct : public BollingerBands {
public:
    // Additional line for percentage
    enum Lines { 
        mid = 0,   // Middle band 
        top = 1,   // Upper band
        bot = 2,   // Lower band
        pctb = 3   // Percentage B
    };
    
    BollingerBandsPct();
    virtual ~BollingerBandsPct() = default;
    
    size_t size() const override;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void calculate_percentage();
};

// Alias
using BBands = BollingerBands;

} // namespace indicators
} // namespace backtrader