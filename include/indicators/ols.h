#pragma once

#include "../indicator.h"
#include "sma.h"
#include "deviation.h"
#include <vector>
#include <utility>

namespace backtrader {

// OLS Slope and Intercept Calculator
class OLS_Slope_InterceptN : public Indicator {
public:
    struct Params {
        int period = 10;
    } params;
    
    // Lines
    enum Lines { 
        slope = 0,
        intercept = 1
    };
    
    OLS_Slope_InterceptN();
    virtual ~OLS_Slope_InterceptN() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper method for linear regression
    std::pair<double, double> calculate_regression(const std::vector<double>& x, 
                                                   const std::vector<double>& y);
};

// OLS Transformation (Z-Score)
class OLS_TransformationN : public Indicator {
public:
    struct Params {
        int period = 10;
    } params;
    
    // Lines
    enum Lines { 
        spread = 0,
        spread_mean = 1,
        spread_std = 2,
        zscore = 3
    };
    
    OLS_TransformationN();
    virtual ~OLS_TransformationN() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicators
    std::shared_ptr<indicators::OLS_Slope_InterceptN> ols_si_;
    std::shared_ptr<indicators::SMA> spread_sma_;
    std::shared_ptr<indicators::StandardDeviation> spread_std_;
    
    // Spread calculation storage
    std::vector<double> spread_values_;
};

// OLS Beta Calculator (simplified version)
class OLS_BetaN : public Indicator {
public:
    struct Params {
        int period = 10;
    } params;
    
    // Lines
    enum Lines { 
        beta = 0
    };
    
    OLS_BetaN();
    virtual ~OLS_BetaN() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Helper method to calculate beta (correlation-based)
    double calculate_beta(const std::vector<double>& x, const std::vector<double>& y);
};

// Cointegration Test (simplified version)
class CointN : public Indicator {
public:
    struct Params {
        int period = 10;
    } params;
    
    // Lines
    enum Lines { 
        score = 0,
        pvalue = 1
    };
    
    CointN();
    virtual ~CointN() = default;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Simplified cointegration test
    std::pair<double, double> calculate_cointegration(const std::vector<double>& x, 
                                                      const std::vector<double>& y);
};

} // namespace backtrader