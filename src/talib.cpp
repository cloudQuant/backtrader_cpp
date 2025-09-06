#include "talib.h"
#include "dataseries.h"
#include <algorithm>
#include <stdexcept>

namespace backtrader {

// TAAbstract implementation
TAAbstract::TAAbstract(const std::string& function_name) {
    load_function_info(function_name);
}

void TAAbstract::set_parameters(const std::map<std::string, double>& params) {
    current_params_ = params;
}

int TAAbstract::get_lookback() const {
#ifdef HAVE_TALIB
    // If TA-Lib is available, calculate actual lookback
    // This is a simplified implementation - real TA-Lib integration would 
    // call the actual TA-Lib lookback functions
    return info_.lookback;
#else
    // Fallback for when TA-Lib is not available
    return info_.lookback;
#endif
}

std::vector<std::vector<double>> TAAbstract::execute(const std::vector<std::vector<double>>& inputs) const {
#ifdef HAVE_TALIB
    // Real TA-Lib integration would go here
    // For now, return empty results as placeholder
    std::vector<std::vector<double>> outputs(info_.output_names.size());
    if (!inputs.empty()) {
        for (auto& output : outputs) {
            output.resize(inputs[0].size(), 0.0);
        }
    }
    return outputs;
#else
    // When TA-Lib is not available, throw exception
    throw std::runtime_error("TA-Lib is not available. Cannot execute function: " + info_.name);
#endif
}

void TAAbstract::load_function_info(const std::string& function_name) {
    // This would normally load from TA-Lib, but for now we'll use hardcoded info
    info_.name = function_name;
    info_.group = "Math Operators";
    
    // Set up some common function information
    if (function_name == "SMA") {
        info_.input_names = {"real"};
        info_.output_names = {"real"};
        info_.parameters["timeperiod"] = 30;
        info_.lookback = 29;
        info_.doc = "Simple Moving Average";
    } else if (function_name == "EMA") {
        info_.input_names = {"real"};
        info_.output_names = {"real"};
        info_.parameters["timeperiod"] = 30;
        info_.lookback = 29;
        info_.doc = "Exponential Moving Average";
    } else if (function_name == "RSI") {
        info_.input_names = {"real"};
        info_.output_names = {"real"};
        info_.parameters["timeperiod"] = 14;
        info_.lookback = 14;
        info_.doc = "Relative Strength Index";
    } else if (function_name == "MACD") {
        info_.input_names = {"real"};
        info_.output_names = {"macd", "macdsignal", "macdhist"};
        info_.parameters["fastperiod"] = 12;
        info_.parameters["slowperiod"] = 26;
        info_.parameters["signalperiod"] = 9;
        info_.lookback = 33;
        info_.doc = "Moving Average Convergence/Divergence";
    } else if (function_name == "BBANDS") {
        info_.input_names = {"real"};
        info_.output_names = {"upperband", "middleband", "lowerband"};
        info_.parameters["timeperiod"] = 5;
        info_.parameters["nbdevup"] = 2;
        info_.parameters["nbdevdn"] = 2;
        info_.parameters["matype"] = static_cast<double>(MAType::SMA);
        info_.lookback = 4;
        info_.doc = "Bollinger Bands";
    } else if (function_name == "STOCH") {
        info_.input_names = {"high", "low", "close"};
        info_.output_names = {"slowk", "slowd"};
        info_.parameters["fastk_period"] = 5;
        info_.parameters["slowk_period"] = 3;
        info_.parameters["slowk_matype"] = static_cast<double>(MAType::SMA);
        info_.parameters["slowd_period"] = 3;
        info_.parameters["slowd_matype"] = static_cast<double>(MAType::SMA);
        info_.lookback = 8;
        info_.doc = "Stochastic";
    } else {
        // Default fallback
        info_.input_names = {"real"};
        info_.output_names = {"real"};
        info_.lookback = 1;
        info_.doc = "Unknown TA-Lib function: " + function_name;
    }
    
    // Check for candlestick patterns
    if (function_name.find("CDL") == 0) {
        info_.is_candle = true;
        info_.function_flags.push_back(FUNC_FLAGS_CANDLESTICK);
        info_.input_names = {"open", "high", "low", "close"};
        info_.output_names = {"integer"};
    }
    
    // Set current parameters to defaults
    current_params_ = info_.parameters;
}

// TALibIndicator implementation
TALibIndicator::TALibIndicator(const std::string& ta_function_name) 
    : Indicator(), ta_abstract_(ta_function_name) {
    
    const auto& info = ta_abstract_.get_info();
    is_candle_ = info.is_candle;
    is_unstable_ = info.is_unstable;
    lookback_ = ta_abstract_.get_lookback();
    
    setup_lines();
    setup_plotting();
    
    // Set minimum period
    set_minperiod(lookback_ + 1);
}

std::shared_ptr<TALibIndicator> TALibIndicator::create(const std::string& function_name) {
    return std::make_shared<TALibIndicator>(function_name);
}

void TALibIndicator::setup_lines() {
    const auto& output_names = ta_abstract_.get_output_names();
    auto line_names = output_names;
    
    // Add candle plot line if this is a candlestick indicator
    if (is_candle_) {
        line_names.push_back("_candleplot");
    }
    
    // Create lines
    lines = std::make_shared<Lines>(line_names.size());
    for (size_t i = 0; i < line_names.size(); ++i) {
        auto line = lines->getline(i);
        if (line) {
            // Set line name - would need Lines class to support this
        }
    }
}

void TALibIndicator::setup_plotting() {
    // This would set up plotting information based on function flags
    // Simplified implementation for now
    
    const auto& info = ta_abstract_.get_info();
    
    // Check function flags for plotting hints
    for (int flag : info.function_flags) {
        if (flag == FUNC_FLAGS_SAMESCALE) {
            // Plot on same scale as price data
        } else if (flag == FUNC_FLAGS_CANDLESTICK) {
            // Set up candlestick plotting
        }
    }
}

std::vector<std::vector<double>> TALibIndicator::prepare_input_data(int size) const {
    std::vector<std::vector<double>> input_data;
    
    const auto& input_names = ta_abstract_.get_input_names();
    
    for (const auto& input_name : input_names) {
        std::vector<double> data;
        data.reserve(size);
        
        // Map input name to data series line
        int line_idx = 0;
        if (input_name == "real" || input_name == "close") {
            line_idx = DataSeries::Close;
        } else if (input_name == "open") {
            line_idx = DataSeries::Open;
        } else if (input_name == "high") {
            line_idx = DataSeries::High;
        } else if (input_name == "low") {
            line_idx = DataSeries::Low;
        } else if (input_name == "volume") {
            line_idx = DataSeries::Volume;
        }
        
        // Get data from the appropriate line
        if (!datas.empty() && datas[0]->lines) {
            auto line = datas[0]->lines->getline(line_idx);
            if (line) {
                for (int i = 0; i < size; ++i) {
                    data.push_back((*line)[-(size - 1 - i)]);
                }
            }
        }
        
        input_data.push_back(data);
    }
    
    return input_data;
}

void TALibIndicator::prenext() {
    // Call parent prenext
    Indicator::prenext();
}

void TALibIndicator::oncestart(int start, int end) {
    // Implementation for once mode start
}

void TALibIndicator::once(int start, int end) {
    // Prepare input data
    int size = end - start;
    auto input_data = prepare_input_data(size + lookback_);
    
    // Execute TA-Lib function
    try {
        auto outputs = ta_abstract_.execute(input_data);
        
        // Copy results to our lines
        const auto& output_names = ta_abstract_.get_output_names();
        for (size_t i = 0; i < outputs.size() && i < output_names.size(); ++i) {
            if (i < lines->size()) {
                auto line = lines->getline(i);
                if (line && outputs[i].size() >= static_cast<size_t>(size)) {
                    for (int j = 0; j < size; ++j) {
                        (*line)[start + j] = outputs[i][lookback_ + j];
                    }
                }
            }
        }
        
        // Handle candlestick plotting if applicable
        if (is_candle_ && lines->size() > output_names.size()) {
            auto candle_line = lines->getline(output_names.size());
            if (candle_line && !outputs.empty()) {
                auto ref_data = prepare_input_data(size + lookback_);
                if (ref_data.size() > CANDLEREF) {
                    for (int j = 0; j < size; ++j) {
                        double ref_val = ref_data[CANDLEREF][lookback_ + j] * CANDLEOVER;
                        double candle_val = outputs[0][lookback_ + j] / 100.0;
                        (*candle_line)[start + j] = ref_val * candle_val;
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        // Handle TA-Lib errors gracefully
        // For now, just fill with NaN
        for (size_t i = 0; i < lines->size(); ++i) {
            auto line = lines->getline(i);
            if (line) {
                for (int j = start; j < end; ++j) {
                    (*line)[j] = std::numeric_limits<double>::quiet_NaN();
                }
            }
        }
    }
}

void TALibIndicator::next() {
    // Prepare input data for single calculation
    int size = lookback_ > 0 ? lookback_ + 1 : length();
    auto input_data = prepare_input_data(size);
    
    try {
        auto outputs = ta_abstract_.execute(input_data);
        
        // Set current values
        const auto& output_names = ta_abstract_.get_output_names();
        for (size_t i = 0; i < outputs.size() && i < output_names.size(); ++i) {
            if (i < lines->size()) {
                auto line = lines->getline(i);
                if (line && !outputs[i].empty()) {
                    (*line)[0] = outputs[i].back();
                }
            }
        }
        
        // Handle candlestick plotting
        if (is_candle_ && lines->size() > output_names.size()) {
            auto candle_line = lines->getline(output_names.size());
            if (candle_line && !outputs.empty() && !input_data.empty()) {
                if (input_data.size() > CANDLEREF && !input_data[CANDLEREF].empty()) {
                    double ref_val = input_data[CANDLEREF].back() * CANDLEOVER;
                    double candle_val = outputs[0].back() / 100.0;
                    (*candle_line)[0] = ref_val * candle_val;
                }
            }
        }
        
    } catch (const std::exception& e) {
        // Handle errors gracefully
        for (size_t i = 0; i < lines->size(); ++i) {
            auto line = lines->getline(i);
            if (line) {
                (*line)[0] = std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
}

// TALibRegistry implementation
TALibRegistry& TALibRegistry::instance() {
    static TALibRegistry registry;
    return registry;
}

void TALibRegistry::register_function(const std::string& name, const TAFunctionInfo& info) {
    functions_[name] = info;
}

bool TALibRegistry::has_function(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

const TAFunctionInfo& TALibRegistry::get_function_info(const std::string& name) const {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw std::runtime_error("TA-Lib function not found: " + name);
    }
    return it->second;
}

std::vector<std::string> TALibRegistry::get_function_names() const {
    std::vector<std::string> names;
    names.reserve(functions_.size());
    for (const auto& pair : functions_) {
        names.push_back(pair.first);
    }
    return names;
}

std::shared_ptr<TALibIndicator> TALibRegistry::create_indicator(const std::string& function_name) const {
    return TALibIndicator::create(function_name);
}

// Utility functions
std::string ma_type_to_string(MAType type) {
    switch (type) {
        case MAType::SMA: return "SMA";
        case MAType::EMA: return "EMA";
        case MAType::WMA: return "WMA";
        case MAType::DEMA: return "DEMA";
        case MAType::TEMA: return "TEMA";
        case MAType::TRIMA: return "TRIMA";
        case MAType::KAMA: return "KAMA";
        case MAType::MAMA: return "MAMA";
        case MAType::T3: return "T3";
        default: return "SMA";
    }
}

MAType string_to_ma_type(const std::string& str) {
    if (str == "EMA") return MAType::EMA;
    if (str == "WMA") return MAType::WMA;
    if (str == "DEMA") return MAType::DEMA;
    if (str == "TEMA") return MAType::TEMA;
    if (str == "TRIMA") return MAType::TRIMA;
    if (str == "KAMA") return MAType::KAMA;
    if (str == "MAMA") return MAType::MAMA;
    if (str == "T3") return MAType::T3;
    return MAType::SMA; // default
}

bool is_talib_available() {
#ifdef HAVE_TALIB
    return true;
#else
    return false;
#endif
}

std::vector<std::string> get_talib_functions() {
#ifdef HAVE_TALIB
    // This would return actual TA-Lib functions
    // For now, return a predefined list
    return {
        "SMA", "EMA", "WMA", "DEMA", "TEMA", "TRIMA", "KAMA", "MAMA", "T3",
        "RSI", "MACD", "BBANDS", "STOCH", "STOCHF", "CCI", "WILLR",
        "ADX", "ADXR", "APO", "AROON", "AROONOSC", "BOP", "CMO",
        "DX", "MFI", "MINUS_DI", "MINUS_DM", "MOM", "PLUS_DI", "PLUS_DM",
        "PPO", "ROC", "ROCP", "ROCR", "ROCR100", "TRIX", "ULTOSC"
    };
#else
    return {}; // Empty list when TA-Lib is not available
#endif
}

std::shared_ptr<TALibIndicator> create_talib_indicator(const std::string& function_name) {
    return TALibRegistry::instance().create_indicator(function_name);
}

} // namespace backtrader