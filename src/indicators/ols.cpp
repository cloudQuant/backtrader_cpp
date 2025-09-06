#include "indicators/ols.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {

// OLS_Slope_InterceptN implementation
OLS_Slope_InterceptN::OLS_Slope_InterceptN() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void OLS_Slope_InterceptN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void OLS_Slope_InterceptN::next() {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);  // Y variable
    auto data1_line = datas[1]->lines->getline(0);  // X variable
    auto slope_line = lines->getline(slope);
    auto intercept_line = lines->getline(intercept);
    
    if (!data0_line || !data1_line || !slope_line || !intercept_line) return;
    
    // Get period data
    std::vector<double> y_data, x_data;
    for (int i = params.period - 1; i >= 0; --i) {
        y_data.push_back((*data0_line)[-i]);
        x_data.push_back((*data1_line)[-i]);
    }
    
    // Calculate regression
    auto [slope, intercept] = calculate_regression(x_data, y_data);
    
    slope_line->set(0, slope);
    intercept_line->set(0, intercept);
}

void OLS_Slope_InterceptN::once(int start, int end) {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);  // Y variable
    auto data1_line = datas[1]->lines->getline(0);  // X variable
    auto slope_line = lines->getline(slope);
    auto intercept_line = lines->getline(intercept);
    
    if (!data0_line || !data1_line || !slope_line || !intercept_line) return;
    
    for (int i = start; i < end; ++i) {
        // Get period data
        std::vector<double> y_data, x_data;
        for (int j = 0; j < params.period; ++j) {
            y_data.push_back((*data0_line)[i - j]);
            x_data.push_back((*data1_line)[i - j]);
        }
        
        // Calculate regression
        auto [slope, intercept] = calculate_regression(x_data, y_data);
        
        slope_line->set(i, slope);
        intercept_line->set(i, intercept);
    }

std::pair<double, double> OLS_Slope_InterceptN::calculate_regression(const std::vector<double>& x, 
                                                                     const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) {
        return {0.0, 0.0};
    }
    
    size_t n = x.size();
    
    // Calculate means
    double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / n;
    double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / n;
    
    // Calculate slope and intercept
    double numerator = 0.0;
    double denominator = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double x_diff = x[i] - mean_x;
        double y_diff = y[i] - mean_y;
        
        numerator += x_diff * y_diff;
        denominator += x_diff * x_diff;
    }
    
    double slope = (denominator != 0.0) ? numerator / denominator : 0.0;
    double intercept = mean_y - slope * mean_x;
    
    return {slope, intercept};
}

// OLS_TransformationN implementation
OLS_TransformationN::OLS_TransformationN() : Indicator() {
    setup_lines();
    _minperiod(params.period);
    
    // Create component indicators
    ols_si_ = std::make_shared<OLS_Slope_InterceptN>();
    ols_si_->params.period = params.period;
    
    spread_sma_ = std::make_shared<SMA>();
    spread_sma_->params.period = params.period;
    
    spread_std_ = std::make_shared<StandardDeviation>();
    spread_std_->params.period = params.period;
}

void OLS_TransformationN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void OLS_TransformationN::next() {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);  // Y variable
    auto data1_line = datas[1]->lines->getline(0);  // X variable
    
    auto spread_line = lines->getline(spread);
    auto spread_mean_line = lines->getline(spread_mean);
    auto spread_std_line = lines->getline(spread_std);
    auto zscore_line = lines->getline(zscore);
    
    if (!data0_line || !data1_line || !spread_line || 
        !spread_mean_line || !spread_std_line || !zscore_line) return;
    
    // Set data for OLS indicator
    ols_si_->datas = datas;
    ols_si_->next();
    
    // Get slope and intercept
    double slope = 0.0, intercept = 0.0;
    if (ols_si_->lines && ols_si_->lines->getline(0) && ols_si_->lines->getline(1)) {
        slope = (*ols_si_->lines->getline(0))[0];
        intercept = (*ols_si_->lines->getline(1))[0];
    }
    
    // Calculate spread
    double y = (*data0_line)[0];
    double x = (*data1_line)[0];
    double spread = y - (slope * x + intercept);
    
    spread_line->set(0, spread);
    spread_values_.push_back(spread);
    
    // Calculate spread statistics
    if (spread_values_.size() >= params.period) {
        // Calculate SMA of spread
        double sum = 0.0;
        int start_idx = spread_values_.size() - params.period;
        for (int i = start_idx; i < spread_values_.size(); ++i) {
            sum += spread_values_[i];
        }
        double spread_mean = sum / params.period;
        spread_mean_line->set(0, spread_mean);
        
        // Calculate standard deviation of spread
        double variance = 0.0;
        for (int i = start_idx; i < spread_values_.size(); ++i) {
            double diff = spread_values_[i] - spread_mean;
            variance += diff * diff;
        }
        double spread_std = std::sqrt(variance / params.period);
        spread_std_line->set(0, spread_std);
        
        // Calculate z-score
        if (spread_std > 0.0) {
            zscore_line->set(0, (spread - spread_mean) / spread_std);
        } else {
            zscore_line->set(0, 0.0);
        }
    } else {
        spread_mean_line->set(0, spread);
        spread_std_line->set(0, 0.0);
        zscore_line->set(0, 0.0);
    }
    
    // Keep history manageable
    if (spread_values_.size() > params.period * 2) {
        spread_values_.erase(spread_values_.begin());
    }
}

void OLS_TransformationN::once(int start, int end) {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    // Set data for OLS indicator
    ols_si_->datas = datas;
    ols_si_->once(0, end);
    
    // Calculate for all bars (simplified implementation)
    for (int i = start; i < end; ++i) {
        // This is a simplified version - in practice would need full implementation
        (*lines->getline(spread))[i] = 0.0;
        (*lines->getline(spread_mean))[i] = 0.0;
        (*lines->getline(spread_std))[i] = 0.0;
        (*lines->getline(zscore))[i] = 0.0;
    }
}

// OLS_BetaN implementation
OLS_BetaN::OLS_BetaN() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void OLS_BetaN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void OLS_BetaN::next() {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);  // Y variable
    auto data1_line = datas[1]->lines->getline(0);  // X variable
    auto beta_line = lines->getline(beta);
    
    if (!data0_line || !data1_line || !beta_line) return;
    
    // Get period data
    std::vector<double> y_data, x_data;
    for (int i = params.period - 1; i >= 0; --i) {
        y_data.push_back((*data0_line)[-i]);
        x_data.push_back((*data1_line)[-i]);
    }
    
    // Calculate beta
    double beta = calculate_beta(x_data, y_data);
    beta_line->set(0, beta);
}

void OLS_BetaN::once(int start, int end) {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);  // Y variable
    auto data1_line = datas[1]->lines->getline(0);  // X variable
    auto beta_line = lines->getline(beta);
    
    if (!data0_line || !data1_line || !beta_line) return;
    
    for (int i = start; i < end; ++i) {
        // Get period data
        std::vector<double> y_data, x_data;
        for (int j = 0; j < params.period; ++j) {
            y_data.push_back((*data0_line)[i - j]);
            x_data.push_back((*data1_line)[i - j]);
        }
        
        // Calculate beta
        double beta = calculate_beta(x_data, y_data);
        beta_line->set(i, beta);
    }
}

double OLS_BetaN::calculate_beta(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) {
        return 1.0; // Default beta
    }
    
    size_t n = x.size();
    
    // Calculate means
    double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / n;
    double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / n;
    
    // Calculate covariance and variance
    double covariance = 0.0;
    double variance_x = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double x_diff = x[i] - mean_x;
        double y_diff = y[i] - mean_y;
        
        covariance += x_diff * y_diff;
        variance_x += x_diff * x_diff;
    }
    
    // Beta = Cov(X,Y) / Var(X)
    return (variance_x != 0.0) ? covariance / variance_x : 1.0;
}

// CointN implementation
CointN::CointN() : Indicator() {
    setup_lines();
    _minperiod(params.period);
}

void CointN::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void CointN::next() {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);
    auto data1_line = datas[1]->lines->getline(0);
    auto score_line = lines->getline(score);
    auto pvalue_line = lines->getline(pvalue);
    
    if (!data0_line || !data1_line || !score_line || !pvalue_line) return;
    
    // Get period data
    std::vector<double> x_data, y_data;
    for (int i = params.period - 1; i >= 0; --i) {
        x_data.push_back((*data0_line)[-i]);
        y_data.push_back((*data1_line)[-i]);
    }
    
    // Calculate simplified cointegration test
    auto [score, pvalue] = calculate_cointegration(x_data, y_data);
    
    score_line->set(0, score);
    pvalue_line->set(0, pvalue);
}

void CointN::once(int start, int end) {
    if (datas.size() < 2 || !datas[0]->lines || !datas[1]->lines) return;
    
    auto data0_line = datas[0]->lines->getline(0);
    auto data1_line = datas[1]->lines->getline(0);
    auto score_line = lines->getline(score);
    auto pvalue_line = lines->getline(pvalue);
    
    if (!data0_line || !data1_line || !score_line || !pvalue_line) return;
    
    for (int i = start; i < end; ++i) {
        // Get period data
        std::vector<double> x_data, y_data;
        for (int j = 0; j < params.period; ++j) {
            x_data.push_back((*data0_line)[i - j]);
            y_data.push_back((*data1_line)[i - j]);
        }
        
        // Calculate simplified cointegration test
        auto [score, pvalue] = calculate_cointegration(x_data, y_data);
        
        score_line->set(i, score);
        pvalue_line->set(i, pvalue);
    }

std::pair<double, double> CointN::calculate_cointegration(const std::vector<double>& x, 
                                                          const std::vector<double>& y) {
    // This is a simplified version of cointegration test
    // In practice, would need Augmented Dickey-Fuller test implementation
    
    if (x.size() != y.size() || x.empty()) {
        return {0.0, 1.0}; // No cointegration
    }
    
    // Calculate correlation as a proxy for cointegration
    size_t n = x.size();
    double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / n;
    double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / n;
    
    double covariance = 0.0;
    double variance_x = 0.0;
    double variance_y = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double x_diff = x[i] - mean_x;
        double y_diff = y[i] - mean_y;
        
        covariance += x_diff * y_diff;
        variance_x += x_diff * x_diff;
        variance_y += y_diff * y_diff;
    }
    
    double correlation = 0.0;
    if (variance_x > 0.0 && variance_y > 0.0) {
        correlation = covariance / std::sqrt(variance_x * variance_y);
    }
    
    // Simple heuristic: higher correlation = higher cointegration score
    double score = std::abs(correlation) * 10.0; // Scale to reasonable range
    double pvalue = 1.0 - std::abs(correlation); // Inverse relationship
    
    return {score, pvalue};
}
}}
} // namespace backtrader