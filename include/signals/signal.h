#pragma once

#include "../indicator.h"
#include <array>

namespace backtrader {
namespace signals {

/**
 * Signal types enumeration
 * 
 * Defines different types of trading signals that can be generated.
 */
enum class SignalType {
    SIGNAL_NONE = 0,
    SIGNAL_LONGSHORT = 1,
    SIGNAL_LONG = 2,
    SIGNAL_LONG_INV = 3,
    SIGNAL_LONG_ANY = 4,
    SIGNAL_SHORT = 5,
    SIGNAL_SHORT_INV = 6,
    SIGNAL_SHORT_ANY = 7,
    SIGNAL_LONGEXIT = 8,
    SIGNAL_LONGEXIT_INV = 9,
    SIGNAL_LONGEXIT_ANY = 10,
    SIGNAL_SHORTEXIT = 11,
    SIGNAL_SHORTEXIT_INV = 12,
    SIGNAL_SHORTEXIT_ANY = 13
};

/**
 * Signal - Base signal indicator class
 * 
 * Inherits from Indicator to create trading signals.
 * Provides a framework for signal generation and interpretation.
 */
class Signal : public Indicator {
public:
    // Signal types array
    static const std::array<SignalType, 14> SignalTypes;

    // Lines
    enum Lines {
        SIGNAL = 0
    };

    Signal(std::shared_ptr<DataSeries> data);
    virtual ~Signal() = default;

    // Indicator interface
    void next() override;

    // Signal-specific methods
    SignalType get_current_signal() const;
    SignalType get_signal(int lookback = 0) const;
    void set_signal(SignalType signal_type);
    void set_signal_value(double value);
    
    // Signal interpretation
    bool is_long_signal(SignalType signal = SignalType::SIGNAL_NONE) const;
    bool is_short_signal(SignalType signal = SignalType::SIGNAL_NONE) const;
    bool is_exit_signal(SignalType signal = SignalType::SIGNAL_NONE) const;
    bool is_entry_signal(SignalType signal = SignalType::SIGNAL_NONE) const;
    
    // Signal conversion
    static double signal_to_value(SignalType signal);
    static SignalType value_to_signal(double value);
    static std::string signal_to_string(SignalType signal);

private:
    // Internal methods
    void initialize_signal_line();
    void update_plot_master();
    
    // Signal validation
    bool is_valid_signal(SignalType signal) const;
    bool is_valid_signal_value(double value) const;
};

/**
 * SimpleSignal - Simple signal generator based on crossovers
 * 
 * Generates signals based on crossovers between two data series or indicators.
 */
class SimpleSignal : public Signal {
public:
    // Parameters structure
    struct Params {
        SignalType long_signal = SignalType::SIGNAL_LONG;
        SignalType short_signal = SignalType::SIGNAL_SHORT;
        SignalType exit_signal = SignalType::SIGNAL_NONE;
        bool use_exit_signals = false;
    };

    SimpleSignal(std::shared_ptr<DataSeries> data1,
                 std::shared_ptr<DataSeries> data2,
                 const Params& params = Params{});
    virtual ~SimpleSignal() = default;

    // Indicator interface
    void next() override;

private:
    // Parameters
    Params params_;
    
    // Data sources
    std::shared_ptr<DataSeries> data1_;
    std::shared_ptr<DataSeries> data2_;
    
    // State tracking
    bool was_above_ = false;
    bool first_calculation_ = true;
    
    // Internal methods
    void detect_crossover();
    bool is_crossover_up() const;
    bool is_crossover_down() const;
};

/**
 * ThresholdSignal - Signal generator based on threshold levels
 * 
 * Generates signals when data crosses above or below specified threshold levels.
 */
class ThresholdSignal : public Signal {
public:
    // Parameters structure
    struct Params {
        double upper_threshold = 70.0;
        double lower_threshold = 30.0;
        SignalType long_signal = SignalType::SIGNAL_LONG;
        SignalType short_signal = SignalType::SIGNAL_SHORT;
        SignalType exit_signal = SignalType::SIGNAL_NONE;
        bool use_exit_signals = false;
        bool reverse_signals = false;  // Reverse signal logic
    };

    ThresholdSignal(std::shared_ptr<DataSeries> data,
                    const Params& params);
    virtual ~ThresholdSignal() = default;

    // Indicator interface
    void next() override;

    // Threshold management
    void set_thresholds(double upper, double lower);
    void set_upper_threshold(double threshold);
    void set_lower_threshold(double threshold);

private:
    // Parameters
    Params params_;
    
    // State tracking
    bool above_upper_ = false;
    bool below_lower_ = false;
    bool first_calculation_ = true;
    
    // Internal methods
    void detect_threshold_cross();
    void generate_threshold_signals();
};

/**
 * CompositeSignal - Combines multiple signals using logical operations
 * 
 * Allows combining multiple signal sources using AND, OR, XOR logic.
 */
class CompositeSignal : public Signal {
public:
    // Logic operations
    enum class LogicOp {
        AND,
        OR,
        XOR,
        MAJORITY  // Signal if majority of inputs signal
    };

    // Parameters structure
    struct Params {
        LogicOp operation = LogicOp::AND;
        SignalType output_signal = SignalType::SIGNAL_LONGSHORT;
        bool invert_result = false;
    };

    CompositeSignal(const std::vector<std::shared_ptr<Signal>>& signals,
                    const Params& params = Params{});
    virtual ~CompositeSignal() = default;

    // Indicator interface
    void next() override;

    // Signal management
    void add_signal(std::shared_ptr<Signal> signal);
    void remove_signal(std::shared_ptr<Signal> signal);
    void clear_signals();
    int get_signal_count() const;

private:
    // Parameters
    Params params_;
    
    // Signal sources
    std::vector<std::shared_ptr<Signal>> input_signals_;
    
    // Internal methods
    void combine_signals();
    bool apply_logic_operation(const std::vector<bool>& signal_states) const;
    std::vector<bool> get_current_signal_states() const;
    
    // Logic operations
    bool apply_and_logic(const std::vector<bool>& states) const;
    bool apply_or_logic(const std::vector<bool>& states) const;
    bool apply_xor_logic(const std::vector<bool>& states) const;
    bool apply_majority_logic(const std::vector<bool>& states) const;
};

} // namespace signals
} // namespace backtrader