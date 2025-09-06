#pragma once

#include "indicator.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace backtrader {

// Note: This is a C++ adaptation of Python backtrader's TA-Lib integration.
// It provides a framework for TA-Lib integration but requires actual TA-Lib library to be installed.

#ifdef HAVE_TALIB
// Only compile TA-Lib integration if TA-Lib is available
#include <ta_libc.h>
#include <ta_func.h>
#endif

// MA Type constants (matching TA-Lib)
enum class MAType : int {
    SMA = 0,    // Simple Moving Average
    EMA = 1,    // Exponential Moving Average
    WMA = 2,    // Weighted Moving Average
    DEMA = 3,   // Double Exponential Moving Average
    TEMA = 4,   // Triple Exponential Moving Average
    TRIMA = 5,  // Triangular Moving Average
    KAMA = 6,   // Kaufman Adaptive Moving Average
    MAMA = 7,   // MESA Adaptive Moving Average
    T3 = 8      // Triple Exponential Moving Average (T3)
};

// Function flag constants
const int FUNC_FLAGS_SAMESCALE = 16777216;
const int FUNC_FLAGS_UNSTABLE = 134217728;
const int FUNC_FLAGS_CANDLESTICK = 268435456;

// Output flag constants
const int OUT_FLAGS_LINE = 1;
const int OUT_FLAGS_DOTTED = 2;
const int OUT_FLAGS_DASH = 4;
const int OUT_FLAGS_HISTO = 16;
const int OUT_FLAGS_UPPER = 2048;
const int OUT_FLAGS_LOWER = 4096;

// Forward declarations
class TALibIndicator;

// TA-Lib function information
struct TAFunctionInfo {
    std::string name;
    std::string group;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<int> function_flags;
    std::map<std::string, std::vector<int>> output_flags;
    std::map<std::string, double> parameters;
    int lookback = 0;
    bool is_candle = false;
    bool is_unstable = false;
    std::string doc;
};

// TA-Lib abstract interface (simplified)
class TAAbstract {
public:
    TAAbstract(const std::string& function_name);
    virtual ~TAAbstract() = default;
    
    // Function information
    const TAFunctionInfo& get_info() const { return info_; }
    std::string get_name() const { return info_.name; }
    std::vector<std::string> get_input_names() const { return info_.input_names; }
    std::vector<std::string> get_output_names() const { return info_.output_names; }
    std::map<std::string, double> get_parameters() const { return info_.parameters; }
    
    // Set parameters
    void set_parameters(const std::map<std::string, double>& params);
    
    // Calculate lookback period
    int get_lookback() const;
    
    // Execute function (simplified interface)
    std::vector<std::vector<double>> execute(const std::vector<std::vector<double>>& inputs) const;
    
private:
    TAFunctionInfo info_;
    std::map<std::string, double> current_params_;
    
    void load_function_info(const std::string& function_name);
};

// Base TA-Lib indicator class
class TALibIndicator : public Indicator {
public:
    static constexpr double CANDLEOVER = 1.02;  // 2% over
    static constexpr int CANDLEREF = 1;         // Open, High, Low, Close (0, 1, 2, 3)
    
    TALibIndicator(const std::string& ta_function_name);
    virtual ~TALibIndicator() = default;
    
    // Factory method for creating TA-Lib indicators
    static std::shared_ptr<TALibIndicator> create(const std::string& function_name);
    
    // Override indicator methods
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    void oncestart(int start, int end) override;
    
    // TA-Lib specific methods
    const TAAbstract& get_ta_abstract() const { return ta_abstract_; }
    bool is_candle() const { return is_candle_; }
    bool is_unstable() const { return is_unstable_; }
    
protected:
    TAAbstract ta_abstract_;
    bool is_candle_;
    bool is_unstable_;
    int lookback_;
    
#ifdef HAVE_TALIB
    // TA-Lib function pointer (when available)
    std::function<TA_RetCode(int, int, const double*, const double*, const double*, const double*, 
                            int*, int*, double*)> ta_function_;
#endif
    
    void setup_plotting();
    void setup_lines();
    std::vector<std::vector<double>> prepare_input_data(int size) const;
};

// Registry for TA-Lib functions
class TALibRegistry {
public:
    static TALibRegistry& instance();
    
    // Register/get functions
    void register_function(const std::string& name, const TAFunctionInfo& info);
    bool has_function(const std::string& name) const;
    const TAFunctionInfo& get_function_info(const std::string& name) const;
    std::vector<std::string> get_function_names() const;
    
    // Create indicator instance
    std::shared_ptr<TALibIndicator> create_indicator(const std::string& function_name) const;
    
private:
    TALibRegistry() = default;
    std::map<std::string, TAFunctionInfo> functions_;
    
    void initialize_functions();
};

// Specific TA-Lib indicator implementations (examples)
class TALIB_SMA : public TALibIndicator {
public:
    TALIB_SMA() : TALibIndicator("SMA") {}
};

class TALIB_EMA : public TALibIndicator {
public:
    TALIB_EMA() : TALibIndicator("EMA") {}
};

class TALIB_RSI : public TALibIndicator {
public:
    TALIB_RSI() : TALibIndicator("RSI") {}
};

class TALIB_MACD : public TALibIndicator {
public:
    TALIB_MACD() : TALibIndicator("MACD") {}
};

class TALIB_BBANDS : public TALibIndicator {
public:
    TALIB_BBANDS() : TALibIndicator("BBANDS") {}
};

class TALIB_STOCH : public TALibIndicator {
public:
    TALIB_STOCH() : TALibIndicator("STOCH") {}
};

// Utility functions
std::string ma_type_to_string(MAType type);
MAType string_to_ma_type(const std::string& str);

// Check if TA-Lib is available
bool is_talib_available();

// Get all available TA-Lib functions
std::vector<std::string> get_talib_functions();

// Factory function
std::shared_ptr<TALibIndicator> create_talib_indicator(const std::string& function_name);

} // namespace backtrader