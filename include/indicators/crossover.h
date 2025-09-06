#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include <iostream>
#include <algorithm>

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
    double get(int ago = 0) const override;
    size_t size() const override;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineSeries> data);
    
public:
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    void oncestart(int start, int end) override;
    void _once() override;
    
private:
    void setup_lines();
    std::shared_ptr<LineSeries> data0_;
    std::shared_ptr<LineSeries> data1_;
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
    double get(int ago = 0) const override;
    size_t size() const override;
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineSeries> data);
    
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    void _once() override;
    
private:
    void setup_lines();
    bool crossup_;
    std::shared_ptr<LineSeries> data0_;
    std::shared_ptr<LineSeries> data1_;
    std::shared_ptr<NonZeroDifference> nzd_;
    int bar_count_ = 0;
    double last_signal_ = 0.0;  // Track last signal to prevent duplicates
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
    // Constructor for indicator compatibility - accepts any types that can be converted to LineSeries
    template<typename T1, typename T2>
    CrossOver(std::shared_ptr<T1> data0, std::shared_ptr<T2> data1, bool ignored = false) : Indicator(), data0_(nullptr), data1_(nullptr), upcross_(nullptr), downcross_(nullptr) {
        setup_lines();
        _minperiod(2); // Needs previous value to detect crossover
        _ltype = LineRoot::IndType::IndType;  // Ensure CrossOver is marked as an indicator
        std::cout << "DEBUG CrossOver template constructor called with data0=" << data0.get() << ", data1=" << data1.get() << std::endl;
        // Try to convert to LineSeries and add
        auto line0 = std::dynamic_pointer_cast<LineSeries>(data0);
        auto line1 = std::dynamic_pointer_cast<LineSeries>(data1);
        std::cout << "DEBUG CrossOver: line0=" << line0.get() << ", line1=" << line1.get() << std::endl;
        if (line0 && line1) {
            data0_ = line0;
            data1_ = line1;
            std::cout << "DEBUG CrossOver template: Set data0_=" << data0_.get() << ", data1_=" << data1_.get() << std::endl;
            // Create CrossUp and CrossDown indicators
            upcross_ = std::make_shared<CrossUp>();
            upcross_->add_data(data0_);
            upcross_->add_data(data1_);
            addindicator(upcross_);
            std::cout << "DEBUG CrossOver template: Created and added upcross_=" << upcross_.get() 
                      << ", upcross minperiod=" << upcross_->getMinPeriod() << std::endl;
            
            downcross_ = std::make_shared<CrossDown>();
            downcross_->add_data(data0_);
            downcross_->add_data(data1_);
            addindicator(downcross_);
            std::cout << "DEBUG CrossOver template: Created and added downcross_=" << downcross_.get() 
                      << ", downcross minperiod=" << downcross_->getMinPeriod() << std::endl;
            
            // Update minimum period based on child indicators
            _periodrecalc();
            
            // Also consider the minimum period of data sources
            int data0_minperiod = 1;
            int data1_minperiod = 1;
            
            if (auto ind0 = std::dynamic_pointer_cast<Indicator>(data0_)) {
                data0_minperiod = ind0->getMinPeriod();
            }
            if (auto ind1 = std::dynamic_pointer_cast<Indicator>(data1_)) {
                data1_minperiod = ind1->getMinPeriod();
            }
            
            int data_minperiod = std::max(data0_minperiod, data1_minperiod);
            // CrossOver needs at least one additional period to compare current vs previous
            int required_minperiod = data_minperiod + 1;
            if (required_minperiod > minperiod_) {
                _minperiod(required_minperiod);
            }
            
            std::cout << "DEBUG CrossOver template: Final minperiod=" << minperiod_ 
                      << ", calling _periodrecalc() on child indicators" << std::endl;
            
            // Make sure child indicators have their minperiods calculated
            if (upcross_) {
                upcross_->_periodrecalc();
                std::cout << "DEBUG CrossOver template: After _periodrecalc, upcross minperiod=" 
                          << upcross_->_minperiod() << std::endl;
            }
            if (downcross_) {
                downcross_->_periodrecalc();
                std::cout << "DEBUG CrossOver template: After _periodrecalc, downcross minperiod=" 
                          << downcross_->_minperiod() << std::endl;
            }
        } else {
            std::cout << "DEBUG CrossOver template: FAILED to cast data sources to LineSeries!" << std::endl;
        }
    }
    virtual ~CrossOver() = default;
    
    // Get method for accessing the indicator value
    double get(int ago = 0) const override;
    size_t size() const override;
    
    // Get minimum period required for the indicator
    int getMinPeriod() const override { return static_cast<int>(minperiod_); }
    
    // Requires two data sources
    void add_data(std::shared_ptr<LineSeries> data);
    
    void prenext() override;
    void nextstart() override;
    void next() override;
    void once(int start, int end) override;
    void _once() override;
    
private:
    void setup_lines();
    std::shared_ptr<LineSeries> data0_;
    std::shared_ptr<LineSeries> data1_;
    std::shared_ptr<CrossUp> upcross_;
    std::shared_ptr<CrossDown> downcross_;
    int crossover_bar_count_ = 0;
};

// Alias
using NZD = NonZeroDifference;

} // namespace backtrader