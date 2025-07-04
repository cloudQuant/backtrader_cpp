#pragma once

#include "../analyzer.h"
#include <memory>
#include <limits>

namespace backtrader {
namespace analyzers {

/**
 * DrawDown analyzer - calculates trading system drawdown statistics
 * 
 * This analyzer calculates trading system drawdowns stats such as drawdown
 * values in %s and in dollars, max drawdown in %s and in dollars, drawdown
 * length and drawdown max length
 * 
 * Params:
 *   - fund (default: None) - If None the actual mode of the broker (fundmode)
 *     will be autodetected. Set to True/False for specific behavior
 * 
 * Analysis Results:
 *   - drawdown - current drawdown value in % (0.xx format)
 *   - moneydown - current drawdown value in monetary units  
 *   - len - current drawdown length
 *   - max.drawdown - maximum drawdown value in %
 *   - max.moneydown - maximum drawdown value in monetary units
 *   - max.len - maximum drawdown length
 */
class DrawDown : public Analyzer {
public:
    // Parameters structure
    struct Params {
        bool fund;      // Use fund mode
        bool auto_fund;  // Auto-detect fund mode from broker
        
        Params() : fund(false), auto_fund(true) {}
    };
    
    DrawDown();
    DrawDown(const Params& params);
    virtual ~DrawDown() = default;
    
    // Override analyzer interface
    void start() override;
    void stop() override;
    void create_analysis() override;
    void notify_fund(double cash, double value, double fundvalue, double shares) override;
    void next() override;
    
    AnalysisResult get_analysis() const override;
    
protected:
    Params p;  // Parameters
    
    // Current state tracking
    bool _fundmode = false;
    double _value = 0.0;
    double _maxvalue = std::numeric_limits<double>::lowest();
    
    // Analysis results structure (matching Python AutoOrderedDict behavior)
    struct DrawDownResults {
        // Current values
        double drawdown = 0.0;
        double moneydown = 0.0;
        int len = 0;
        
        // Maximum values
        struct Max {
            double drawdown = 0.0;
            double moneydown = 0.0;
            int len = 0;
        } max;
        
        bool _closed = false;  // Equivalent to Python ._close()
    };
    
    DrawDownResults rets;
    
private:
    void update_drawdown_stats();
};

/**
 * TimeDrawDown analyzer - calculates drawdown on chosen timeframe
 * 
 * This analyzer calculates trading system drawdowns on the chosen
 * timeframe which can be different from the one used in the underlying data
 */
class TimeDrawDown : public TimeFrameAnalyzerBase {
public:
    // Parameters structure extending TimeFrameAnalyzerBase::Params
    struct Params : public TimeFrameAnalyzerBase::Params {
        bool fund = false;      // Use fund mode
        bool auto_fund = true;  // Auto-detect fund mode from broker
    };
    
    TimeDrawDown(const Params& params = Params{});
    virtual ~TimeDrawDown() = default;
    
    // Override analyzer interface
    void start() override;
    void stop() override;
    void on_dt_over() override;
    
    AnalysisResult get_analysis() const override;
    
    // Public attributes (matching Python interface)
    double dd = 0.0;          // Current drawdown
    double maxdd = 0.0;       // Maximum drawdown
    int maxddlen = 0;         // Maximum drawdown length
    
protected:
    Params p;  // Parameters
    
    // State tracking
    bool _fundmode = false;
    double peak = std::numeric_limits<double>::lowest();
    int ddlen = 0;
    
private:
    double get_current_value() const;
};

} // namespace analyzers
} // namespace backtrader

// Register the analyzers  
REGISTER_ANALYZER(backtrader::analyzers::DrawDown);
REGISTER_ANALYZER(backtrader::analyzers::TimeDrawDown);