/**
 * @file main_extended.cpp
 * @brief Extended backtrader-cpp Python bindings with more indicators
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <limits>

// Include backtrader headers
#include "../../include/indicators/sma.h"
#include "../../include/indicators/ema.h"
#include "../../include/indicators/rsi.h"
#include "../../include/indicators/macd.h"
#include "../../include/indicators/bollinger.h"
#include "../../include/indicators/stochastic.h"
#include "../../include/indicators/atr.h"
#include "../../include/indicators/cci.h"
#include "../../include/indicators/dema.h"
#include "../../include/indicators/tema.h"
#include "../../include/indicators/wma.h"
#include "../../include/indicators/hma.h"
#include "../../include/indicators/kama.h"
#include "../../include/indicators/roc.h"
#include "../../include/indicators/momentum.h"
#include "../../include/indicators/williamsr.h"
#include "../../include/indicators/basicops.h"
#include "../../include/indicators/aroon.h"
#include "../../include/indicators/dma.h"
#include "../../include/linebuffer.h"
#include "../../include/lineseries.h"
#include "../../include/dataseries.h"

namespace py = pybind11;
using namespace pybind11::literals;
using namespace backtrader;
using namespace backtrader::indicators;

// Helper function to create a LineSeries from Python list
std::shared_ptr<LineSeries> create_lineseries_from_list(const std::vector<double>& data) {
    auto line_series = std::make_shared<LineSeries>();
    line_series->lines = std::make_shared<Lines>();
    
    auto close_buffer = std::make_shared<LineBuffer>();
    if (!data.empty()) {
        close_buffer->set(0, data[0]);
        for (size_t i = 1; i < data.size(); ++i) {
            close_buffer->append(data[i]);
        }
    }
    
    line_series->lines->add_line(close_buffer);
    line_series->lines->add_alias("close", 0);
    
    return line_series;
}

// Helper function to extract values from indicator
std::vector<double> extract_indicator_values(std::shared_ptr<Indicator> indicator, size_t data_size) {
    std::vector<double> result;
    result.reserve(data_size);
    
    // Calculate the indicator
    indicator->calculate();
    
    // Extract values from the indicator's line
    auto line = indicator->lines ? indicator->lines->getline(0) : nullptr;
    if (line) {
        for (size_t i = 0; i < data_size; ++i) {
            int ago = -static_cast<int>(data_size - 1 - i);
            double value = line->get(ago);
            result.push_back(value);
        }
    } else {
        // If no line, return NaN values
        for (size_t i = 0; i < data_size; ++i) {
            result.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    return result;
}

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "Backtrader C++ - Extended Python Bindings";
    
    // Module metadata
    m.attr("__version__") = "0.4.0";
    m.attr("__author__") = "Backtrader C++ Team";
    
    // ==================== SIMPLE MOVING AVERAGE ====================
    m.def("calculate_sma", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto sma = std::make_shared<SMA>(line_series, period);
        return extract_indicator_values(sma, prices.size());
    }, "Calculate Simple Moving Average", "prices"_a, "period"_a = 30);
    
    // ==================== EXPONENTIAL MOVING AVERAGE ====================
    m.def("calculate_ema", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto ema = std::make_shared<EMA>(line_series, period);
        return extract_indicator_values(ema, prices.size());
    }, "Calculate Exponential Moving Average", "prices"_a, "period"_a = 30);
    
    // ==================== RELATIVE STRENGTH INDEX ====================
    m.def("calculate_rsi", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto rsi = std::make_shared<RSI>(line_series, period);
        return extract_indicator_values(rsi, prices.size());
    }, "Calculate Relative Strength Index", "prices"_a, "period"_a = 14);
    
    // ==================== MACD ====================
    m.def("calculate_macd", [](const std::vector<double>& prices, int fast_period, int slow_period, int signal_period) {
        auto line_series = create_lineseries_from_list(prices);
        auto macd = std::make_shared<MACD>(line_series, fast_period, slow_period, signal_period);
        macd->calculate();
        
        // MACD returns three lines: MACD, Signal, Histogram
        py::dict result;
        
        // Extract MACD line
        std::vector<double> macd_line, signal_line, histogram;
        for (size_t i = 0; i < prices.size(); ++i) {
            int ago = -static_cast<int>(prices.size() - 1 - i);
            macd_line.push_back(macd->getMACDLine(ago));
            signal_line.push_back(macd->getSignalLine(ago));
            histogram.push_back(macd->getHistogram(ago));
        }
        
        result["macd"] = macd_line;
        result["signal"] = signal_line;
        result["histogram"] = histogram;
        
        return result;
    }, "Calculate MACD", "prices"_a, "fast_period"_a = 12, "slow_period"_a = 26, "signal_period"_a = 9);
    
    // ==================== BOLLINGER BANDS ====================
    m.def("calculate_bollinger", [](const std::vector<double>& prices, int period, double devfactor) {
        auto line_series = create_lineseries_from_list(prices);
        auto bb = std::make_shared<BollingerBands>(line_series, period, devfactor);
        bb->calculate();
        
        py::dict result;
        std::vector<double> upper, middle, lower;
        
        for (size_t i = 0; i < prices.size(); ++i) {
            int ago = -static_cast<int>(prices.size() - 1 - i);
            upper.push_back(bb->getUpperBand(ago));
            middle.push_back(bb->getMiddleBand(ago));
            lower.push_back(bb->getLowerBand(ago));
        }
        
        result["upper"] = upper;
        result["middle"] = middle;
        result["lower"] = lower;
        
        return result;
    }, "Calculate Bollinger Bands", "prices"_a, "period"_a = 20, "devfactor"_a = 2.0);
    
    // ==================== STOCHASTIC ====================
    m.def("calculate_stochastic", [](const std::vector<double>& highs, const std::vector<double>& lows, 
                                     const std::vector<double>& closes, int period, int period_dfast) {
        // Create a DataSeries with high/low/close
        auto data_series = std::make_shared<DataSeries>();
        data_series->lines = std::make_shared<Lines>();
        
        // Add high, low, close lines
        auto high_buffer = std::make_shared<LineBuffer>();
        auto low_buffer = std::make_shared<LineBuffer>();
        auto close_buffer = std::make_shared<LineBuffer>();
        
        if (!highs.empty()) {
            high_buffer->set(0, highs[0]);
            low_buffer->set(0, lows[0]);
            close_buffer->set(0, closes[0]);
            
            for (size_t i = 1; i < highs.size(); ++i) {
                high_buffer->append(highs[i]);
                low_buffer->append(lows[i]);
                close_buffer->append(closes[i]);
            }
        }
        
        data_series->lines->add_line(high_buffer);
        data_series->lines->add_line(low_buffer);
        data_series->lines->add_line(close_buffer);
        data_series->lines->add_alias("high", 0);
        data_series->lines->add_alias("low", 1);
        data_series->lines->add_alias("close", 2);
        
        auto stoch = std::make_shared<Stochastic>(data_series, period, period_dfast);
        stoch->calculate();
        
        py::dict result;
        std::vector<double> k_line, d_line;
        
        for (size_t i = 0; i < highs.size(); ++i) {
            int ago = -static_cast<int>(highs.size() - 1 - i);
            k_line.push_back(stoch->getPercentK(ago));
            d_line.push_back(stoch->getPercentD(ago));
        }
        
        result["k"] = k_line;
        result["d"] = d_line;
        
        return result;
    }, "Calculate Stochastic Oscillator", "highs"_a, "lows"_a, "closes"_a, "period"_a = 14, "period_dfast"_a = 3);
    
    // ==================== ATR (Average True Range) ====================
    m.def("calculate_atr", [](const std::vector<double>& highs, const std::vector<double>& lows,
                              const std::vector<double>& closes, int period) {
        auto data_series = std::make_shared<DataSeries>();
        data_series->lines = std::make_shared<Lines>();
        
        auto high_buffer = std::make_shared<LineBuffer>();
        auto low_buffer = std::make_shared<LineBuffer>();
        auto close_buffer = std::make_shared<LineBuffer>();
        
        if (!highs.empty()) {
            high_buffer->set(0, highs[0]);
            low_buffer->set(0, lows[0]);
            close_buffer->set(0, closes[0]);
            
            for (size_t i = 1; i < highs.size(); ++i) {
                high_buffer->append(highs[i]);
                low_buffer->append(lows[i]);
                close_buffer->append(closes[i]);
            }
        }
        
        data_series->lines->add_line(high_buffer);
        data_series->lines->add_line(low_buffer);
        data_series->lines->add_line(close_buffer);
        data_series->lines->add_alias("high", 0);
        data_series->lines->add_alias("low", 1);
        data_series->lines->add_alias("close", 2);
        
        auto atr = std::make_shared<ATR>(data_series, period);
        return extract_indicator_values(atr, highs.size());
    }, "Calculate Average True Range", "highs"_a, "lows"_a, "closes"_a, "period"_a = 14);
    
    // ==================== MORE INDICATORS ====================
    
    // DEMA (Double Exponential Moving Average)
    m.def("calculate_dema", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto dema = std::make_shared<DEMA>(line_series, period);
        return extract_indicator_values(dema, prices.size());
    }, "Calculate Double Exponential Moving Average", "prices"_a, "period"_a = 30);
    
    // TEMA (Triple Exponential Moving Average)
    m.def("calculate_tema", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto tema = std::make_shared<TEMA>(line_series, period);
        return extract_indicator_values(tema, prices.size());
    }, "Calculate Triple Exponential Moving Average", "prices"_a, "period"_a = 30);
    
    // WMA (Weighted Moving Average)
    m.def("calculate_wma", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto wma = std::make_shared<WMA>(line_series, period);
        return extract_indicator_values(wma, prices.size());
    }, "Calculate Weighted Moving Average", "prices"_a, "period"_a = 30);
    
    // HMA (Hull Moving Average)
    m.def("calculate_hma", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto hma = std::make_shared<HMA>(line_series, period);
        return extract_indicator_values(hma, prices.size());
    }, "Calculate Hull Moving Average", "prices"_a, "period"_a = 30);
    
    // KAMA (Kaufman Adaptive Moving Average)
    m.def("calculate_kama", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto kama = std::make_shared<KAMA>(line_series, period);
        return extract_indicator_values(kama, prices.size());
    }, "Calculate Kaufman Adaptive Moving Average", "prices"_a, "period"_a = 30);
    
    // ROC (Rate of Change)
    m.def("calculate_roc", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto roc = std::make_shared<ROC>(line_series, period);
        return extract_indicator_values(roc, prices.size());
    }, "Calculate Rate of Change", "prices"_a, "period"_a = 10);
    
    // Momentum
    m.def("calculate_momentum", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto momentum = std::make_shared<Momentum>(line_series, period);
        return extract_indicator_values(momentum, prices.size());
    }, "Calculate Momentum", "prices"_a, "period"_a = 10);
    
    // Williams %R
    m.def("calculate_williamsr", [](const std::vector<double>& highs, const std::vector<double>& lows,
                                    const std::vector<double>& closes, int period) {
        auto data_series = std::make_shared<DataSeries>();
        data_series->lines = std::make_shared<Lines>();
        
        auto high_buffer = std::make_shared<LineBuffer>();
        auto low_buffer = std::make_shared<LineBuffer>();
        auto close_buffer = std::make_shared<LineBuffer>();
        
        if (!highs.empty()) {
            high_buffer->set(0, highs[0]);
            low_buffer->set(0, lows[0]);
            close_buffer->set(0, closes[0]);
            
            for (size_t i = 1; i < highs.size(); ++i) {
                high_buffer->append(highs[i]);
                low_buffer->append(lows[i]);
                close_buffer->append(closes[i]);
            }
        }
        
        data_series->lines->add_line(high_buffer);
        data_series->lines->add_line(low_buffer);
        data_series->lines->add_line(close_buffer);
        data_series->lines->add_alias("high", 0);
        data_series->lines->add_alias("low", 1);
        data_series->lines->add_alias("close", 2);
        
        auto williamsr = std::make_shared<WilliamsR>(data_series, period);
        return extract_indicator_values(williamsr, highs.size());
    }, "Calculate Williams %R", "highs"_a, "lows"_a, "closes"_a, "period"_a = 14);
    
    // Highest
    m.def("calculate_highest", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto highest = std::make_shared<Highest>(line_series, period);
        return extract_indicator_values(highest, prices.size());
    }, "Calculate Highest", "prices"_a, "period"_a = 30);
    
    // Lowest
    m.def("calculate_lowest", [](const std::vector<double>& prices, int period) {
        auto line_series = create_lineseries_from_list(prices);
        auto lowest = std::make_shared<Lowest>(line_series, period);
        return extract_indicator_values(lowest, prices.size());
    }, "Calculate Lowest", "prices"_a, "period"_a = 30);
    
    // ==================== UTILITY FUNCTIONS ====================
    
    m.def("test", []() {
        return "Backtrader C++ extended bindings loaded successfully!";
    }, "Test function");
    
    m.def("get_version", []() {
        return py::dict(
            "version"_a="0.4.0",
            "build_date"_a=__DATE__,
            "build_time"_a=__TIME__,
            "compiler"_a="C++20",
            "status"_a="Extended Integration",
            "indicators"_a=py::list(py::make_tuple(
                "SMA", "EMA", "RSI", "MACD", "Bollinger", "Stochastic",
                "ATR", "DEMA", "TEMA", "WMA", "HMA", "KAMA", "ROC",
                "Momentum", "Williams%R", "Highest", "Lowest"
            ))
        );
    }, "Get version and feature information");
    
    // ==================== DATA PROCESSING ====================
    
    m.def("calculate_returns", [](const std::vector<double>& prices) {
        std::vector<double> returns;
        if (prices.size() <= 1) {
            return returns;
        }
        
        returns.reserve(prices.size() - 1);
        for (size_t i = 1; i < prices.size(); ++i) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
        }
        return returns;
    }, "Calculate returns from prices", "prices"_a);
    
    m.def("calculate_volatility", [](const std::vector<double>& returns, int window) {
        std::vector<double> volatility;
        volatility.reserve(returns.size());
        
        for (size_t i = 0; i < returns.size(); ++i) {
            if (i < static_cast<size_t>(window - 1)) {
                volatility.push_back(std::numeric_limits<double>::quiet_NaN());
            } else {
                double mean = 0.0;
                for (int j = 0; j < window; ++j) {
                    mean += returns[i - j];
                }
                mean /= window;
                
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
    
    m.def("calculate_sharpe", [](const std::vector<double>& returns, double risk_free_rate) {
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
        
        double sharpe = (mean_return - risk_free_rate/252) / std_dev * std::sqrt(252);
        return sharpe;
    }, "Calculate Sharpe ratio", "returns"_a, "risk_free_rate"_a = 0.0);
    
    // ==================== SIMPLE STRATEGY ====================
    
    m.def("simple_moving_average_strategy", [](
        const std::vector<double>& prices, 
        int short_period, 
        int long_period, 
        double initial_capital) {
        
        // Calculate SMAs
        auto line_series = create_lineseries_from_list(prices);
        auto sma_short = std::make_shared<SMA>(line_series, short_period);
        auto sma_long = std::make_shared<SMA>(line_series, long_period);
        
        auto short_values = extract_indicator_values(sma_short, prices.size());
        auto long_values = extract_indicator_values(sma_long, prices.size());
        
        // Generate signals
        std::vector<int> signals(prices.size(), 0);
        std::vector<py::dict> trades;
        
        double capital = initial_capital;
        double position = 0.0;
        double entry_price = 0.0;
        int num_trades = 0;
        
        for (size_t i = 1; i < prices.size(); ++i) {
            if (!std::isnan(short_values[i]) && !std::isnan(long_values[i])) {
                // Buy signal
                if (short_values[i] > long_values[i] && short_values[i-1] <= long_values[i-1]) {
                    if (position == 0) {
                        signals[i] = 1;
                        position = capital / prices[i];
                        entry_price = prices[i];
                        capital = 0;
                        num_trades++;
                        
                        py::dict trade;
                        trade["type"] = "BUY";
                        trade["price"] = prices[i];
                        trade["index"] = i;
                        trades.push_back(trade);
                    }
                }
                // Sell signal
                else if (short_values[i] < long_values[i] && short_values[i-1] >= long_values[i-1]) {
                    if (position > 0) {
                        signals[i] = -1;
                        capital = position * prices[i];
                        
                        py::dict trade;
                        trade["type"] = "SELL";
                        trade["price"] = prices[i];
                        trade["index"] = i;
                        trade["profit"] = (prices[i] - entry_price) * position;
                        trades.push_back(trade);
                        
                        position = 0;
                        entry_price = 0;
                    }
                }
            }
        }
        
        // Close position at end
        if (position > 0) {
            capital = position * prices.back();
        }
        
        py::dict result;
        result["signals"] = signals;
        result["trades"] = trades;
        result["initial_value"] = initial_capital;
        result["final_value"] = capital;
        result["total_return"] = (capital - initial_capital) / initial_capital;
        result["num_trades"] = num_trades;
        
        return result;
    }, "Simple moving average crossover strategy", 
       "prices"_a, "short_period"_a = 10, "long_period"_a = 30, "initial_capital"_a = 10000.0);
}