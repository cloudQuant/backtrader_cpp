/**
 * @file main_working.cpp
 * @brief Working backtrader-cpp Python bindings - focused on core functionality
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "Backtrader C++ - Working Python Bindings";
    
    // Module metadata
    m.attr("__version__") = "0.3.0";
    m.attr("__author__") = "Backtrader C++ Team";
    
    // ==================== UTILITY FUNCTIONS ====================
    
    m.def("test", []() {
        return "Backtrader C++ working bindings loaded successfully!";
    }, "Test function");
    
    m.def("get_version", []() {
        return py::dict(
            "version"_a="0.3.0",
            "build_date"_a=__DATE__,
            "build_time"_a=__TIME__,
            "compiler"_a="C++20",
            "status"_a="Working Integration",
            "features"_a=py::list(py::make_tuple("Math Functions", "Performance Tests", "Data Containers"))
        );
    }, "Get version and feature information");

    // ==================== MATH FUNCTIONS ====================
    
    m.def("calculate_sma", [](const std::vector<double>& prices, int period) {
        std::vector<double> result;
        result.reserve(prices.size());
        
        for (size_t i = 0; i < prices.size(); ++i) {
            if (i < static_cast<size_t>(period - 1)) {
                result.push_back(std::numeric_limits<double>::quiet_NaN());
            } else {
                double sum = 0.0;
                for (int j = 0; j < period; ++j) {
                    sum += prices[i - j];
                }
                result.push_back(sum / period);
            }
        }
        return result;
    }, "Calculate Simple Moving Average", "prices"_a, "period"_a);

    m.def("calculate_ema", [](const std::vector<double>& prices, int period) {
        std::vector<double> result;
        result.reserve(prices.size());
        
        if (prices.empty()) return result;
        
        double alpha = 2.0 / (period + 1.0);
        double ema = prices[0];
        
        for (size_t i = 0; i < prices.size(); ++i) {
            if (i == 0) {
                ema = prices[0];
            } else {
                ema = alpha * prices[i] + (1.0 - alpha) * ema;
            }
            result.push_back(ema);
        }
        return result;
    }, "Calculate Exponential Moving Average", "prices"_a, "period"_a);

    m.def("calculate_rsi", [](const std::vector<double>& prices, int period = 14) {
        std::vector<double> result;
        result.reserve(prices.size());
        
        if (prices.size() < 2) return result;
        
        // Calculate price changes
        std::vector<double> gains, losses;
        for (size_t i = 1; i < prices.size(); ++i) {
            double change = prices[i] - prices[i-1];
            gains.push_back(change > 0 ? change : 0);
            losses.push_back(change < 0 ? -change : 0);
        }
        
        // Calculate RSI
        result.push_back(std::numeric_limits<double>::quiet_NaN()); // First value is NaN
        
        for (size_t i = 0; i < gains.size(); ++i) {
            if (i < static_cast<size_t>(period - 1)) {
                result.push_back(std::numeric_limits<double>::quiet_NaN());
            } else {
                // Calculate average gains and losses
                double avg_gain = 0.0, avg_loss = 0.0;
                for (int j = 0; j < period; ++j) {
                    avg_gain += gains[i - j];
                    avg_loss += losses[i - j];
                }
                avg_gain /= period;
                avg_loss /= period;
                
                if (avg_loss == 0) {
                    result.push_back(100.0);
                } else {
                    double rs = avg_gain / avg_loss;
                    double rsi = 100.0 - (100.0 / (1.0 + rs));
                    result.push_back(rsi);
                }
            }
        }
        
        return result;
    }, "Calculate Relative Strength Index", "prices"_a, "period"_a = 14);

    m.def("calculate_returns", [](const std::vector<double>& prices) {
        std::vector<double> returns;
        if (prices.size() <= 1) {
            return returns;  // Return empty vector for insufficient data
        }
        
        returns.reserve(prices.size() - 1);
        
        for (size_t i = 1; i < prices.size(); ++i) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
        }
        return returns;
    }, "Calculate returns from prices", "prices"_a);

    m.def("calculate_volatility", [](const std::vector<double>& returns, int window = 20) {
        std::vector<double> volatility;
        volatility.reserve(returns.size());
        
        for (size_t i = 0; i < returns.size(); ++i) {
            if (i < static_cast<size_t>(window - 1)) {
                volatility.push_back(std::numeric_limits<double>::quiet_NaN());
            } else {
                // Calculate mean
                double mean = 0.0;
                for (int j = 0; j < window; ++j) {
                    mean += returns[i - j];
                }
                mean /= window;
                
                // Calculate variance
                double variance = 0.0;
                for (int j = 0; j < window; ++j) {
                    double diff = returns[i - j] - mean;
                    variance += diff * diff;
                }
                variance /= window;
                
                volatility.push_back(std::sqrt(variance));
            }
        }
        return volatility;
    }, "Calculate rolling volatility", "returns"_a, "window"_a = 20);

    m.def("calculate_sharpe", [](const std::vector<double>& returns, double risk_free_rate = 0.0) {
        if (returns.empty()) return 0.0;
        
        double mean_return = 0.0;
        for (double ret : returns) {
            mean_return += ret;
        }
        mean_return /= returns.size();
        
        double variance = 0.0;
        for (double ret : returns) {
            double diff = ret - mean_return;
            variance += diff * diff;
        }
        variance /= returns.size();
        double std_dev = std::sqrt(variance);
        
        if (std_dev == 0.0) return 0.0;
        
        // Annualized Sharpe ratio (assuming daily returns)
        double sharpe = (mean_return - risk_free_rate/252) / std_dev * std::sqrt(252);
        return sharpe;
    }, "Calculate Sharpe ratio", "returns"_a, "risk_free_rate"_a = 0.0);

    // ==================== DATA CONTAINERS ====================
    // Note: DoubleVector temporarily disabled due to memory issues
    // Core functionality works perfectly without it

    // ==================== STRATEGY SIMULATION ====================
    
    m.def("simple_moving_average_strategy", [](
        const std::vector<double>& prices, 
        int short_period = 5, 
        int long_period = 20,
        double initial_cash = 10000.0
    ) {
        // Calculate moving averages directly (avoiding module reference)
        auto calculate_sma = [](const std::vector<double>& data, int period) {
            std::vector<double> result;
            result.reserve(data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                if (i < static_cast<size_t>(period - 1)) {
                    result.push_back(std::numeric_limits<double>::quiet_NaN());
                } else {
                    double sum = 0.0;
                    for (int j = 0; j < period; ++j) {
                        sum += data[i - j];
                    }
                    result.push_back(sum / period);
                }
            }
            return result;
        };
        
        std::vector<double> short_ma = calculate_sma(prices, short_period);
        std::vector<double> long_ma = calculate_sma(prices, long_period);
        
        std::vector<int> signals;
        std::vector<py::dict> trades;
        
        double cash = initial_cash;
        double shares = 0;
        int position = 0; // 0: no position, 1: long, -1: short
        
        for (size_t i = 0; i < prices.size(); ++i) {
            int signal = 0;
            
            if (i >= static_cast<size_t>(long_period - 1)) {
                double short_val = short_ma[i];
                double long_val = long_ma[i];
                
                if (!std::isnan(short_val) && !std::isnan(long_val)) {
                    if (short_val > long_val && position <= 0) {
                        signal = 1; // Buy signal
                        if (position == 0) {
                            shares = cash / prices[i];
                            cash = 0;
                            position = 1;
                            trades.push_back(py::dict(
                                "type"_a="BUY",
                                "index"_a=i,
                                "price"_a=prices[i],
                                "shares"_a=shares
                            ));
                        }
                    } else if (short_val < long_val && position >= 0) {
                        signal = -1; // Sell signal
                        if (position == 1) {
                            cash = shares * prices[i];
                            shares = 0;
                            position = 0;
                            trades.push_back(py::dict(
                                "type"_a="SELL",
                                "index"_a=i,
                                "price"_a=prices[i],
                                "cash"_a=cash
                            ));
                        }
                    }
                }
            }
            signals.push_back(signal);
        }
        
        // Final value
        double final_value = cash + shares * prices.back();
        double total_return = (final_value - initial_cash) / initial_cash;
        
        return py::dict(
            "signals"_a=signals,
            "trades"_a=trades,
            "initial_value"_a=initial_cash,
            "final_value"_a=final_value,
            "total_return"_a=total_return,
            "num_trades"_a=trades.size()
        );
    }, "Simple moving average crossover strategy", 
       "prices"_a, "short_period"_a = 5, "long_period"_a = 20, "initial_cash"_a = 10000.0);

    // ==================== PERFORMANCE TESTING ====================
    
    m.def("benchmark", [](int iterations = 1000000) {
        auto start = std::chrono::high_resolution_clock::now();
        
        double sum = 0.0;
        for (int i = 0; i < iterations; ++i) {
            sum += std::sin(i * 0.001) * std::cos(i * 0.001);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return py::dict(
            "result"_a=sum,
            "time_us"_a=duration.count(),
            "iterations"_a=iterations,
            "ops_per_second"_a=static_cast<double>(iterations) * 1000000.0 / duration.count()
        );
    }, "Performance benchmark", "iterations"_a = 1000000);

    m.def("benchmark_sma", [](const std::vector<double>& prices, int period, int iterations = 100) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Inline SMA calculation for benchmarking
        for (int i = 0; i < iterations; ++i) {
            std::vector<double> result;
            result.reserve(prices.size());
            for (size_t j = 0; j < prices.size(); ++j) {
                if (j < static_cast<size_t>(period - 1)) {
                    result.push_back(std::numeric_limits<double>::quiet_NaN());
                } else {
                    double sum = 0.0;
                    for (int k = 0; k < period; ++k) {
                        sum += prices[j - k];
                    }
                    result.push_back(sum / period);
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return py::dict(
            "data_points"_a=prices.size(),
            "period"_a=period,
            "iterations"_a=iterations,
            "time_us"_a=duration.count(),
            "time_per_calculation_us"_a=duration.count() / iterations,
            "calculations_per_second"_a=static_cast<double>(iterations) * 1000000.0 / duration.count()
        );
    }, "Benchmark SMA calculation performance", "prices"_a, "period"_a, "iterations"_a = 100);

    // ==================== UTILITY HELPERS ====================
    
    m.def("generate_sample_data", [](int size = 252, double start_price = 100.0, double volatility = 0.02, int seed = 42) {
        std::srand(seed);
        std::vector<double> prices;
        prices.reserve(size);
        
        double price = start_price;
        prices.push_back(price);
        
        for (int i = 1; i < size; ++i) {
            // Simple random walk with noise
            double change = (static_cast<double>(std::rand()) / RAND_MAX - 0.5) * volatility * 2;
            price *= (1.0 + change);
            prices.push_back(price);
        }
        
        return prices;
    }, "Generate sample price data for testing", 
       "size"_a = 252, "start_price"_a = 100.0, "volatility"_a = 0.02, "seed"_a = 42);

    m.def("validate_data", [](const std::vector<double>& data) {
        py::dict stats;
        
        if (data.empty()) {
            stats["valid"] = false;
            stats["reason"] = "Empty data";
            return stats;
        }
        
        double min_val = data[0], max_val = data[0];
        int nan_count = 0, inf_count = 0;
        
        for (double val : data) {
            if (std::isnan(val)) nan_count++;
            else if (std::isinf(val)) inf_count++;
            else {
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
            }
        }
        
        stats["valid"] = true;
        stats["size"] = data.size();
        stats["min"] = min_val;
        stats["max"] = max_val;
        stats["nan_count"] = nan_count;
        stats["inf_count"] = inf_count;
        stats["valid_count"] = data.size() - nan_count - inf_count;
        
        return stats;
    }, "Validate data quality", "data"_a);
}