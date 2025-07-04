#pragma once

#include "../indicator.h"
#include "../metabase.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <type_traits>

namespace backtrader {
namespace indicators {

// Forward declarations
class MovingAverageBase;

/**
 * MovingAverage - Registry class for all Moving Average types
 * 
 * This class serves as a central registry for all moving average indicators,
 * maintaining a list of registered classes and providing access through aliases.
 */
class MovingAverage {
public:
    using MovAvCreatorFunc = std::function<std::shared_ptr<MovingAverageBase>(const Params&)>;
    using MovAvRegistry = std::map<std::string, MovAvCreatorFunc>;
    
    // Registry management
    static void register_moving_average(const std::string& name, MovAvCreatorFunc creator);
    static void register_alias(const std::string& alias, const std::string& original_name);
    static std::shared_ptr<MovingAverageBase> create(const std::string& name, const Params& params = Params{});
    
    // Registry access
    static const MovAvRegistry& get_registry();
    static std::vector<std::string> get_available_types();
    static bool is_registered(const std::string& name);
    
    // Convenience creators for common moving averages
    static std::shared_ptr<MovingAverageBase> Simple(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> SMA(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> Exponential(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> EMA(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> Weighted(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> WMA(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> Adaptive(const Params& params = Params{});
    static std::shared_ptr<MovingAverageBase> KAMA(const Params& params = Params{});
    
private:
    static MovAvRegistry registry_;
    static bool initialized_;
    static void initialize_registry();
};

/**
 * MovAv - Alias for MovingAverage class
 * 
 * Provides the same functionality as MovingAverage with a shorter name.
 */
using MovAv = MovingAverage;

/**
 * MovingAverageBase - Base class for all moving average indicators
 * 
 * This class provides the common functionality and interface for all
 * moving average implementations.
 */
class MovingAverageBase : public Indicator {
public:
    // Parameters structure
    struct Params : public Indicator::Params {
        int period = 30;           // Moving average period
        bool _movav = true;        // Flag indicating this is a moving average
    };
    
    MovingAverageBase(const Params& params = Params{});
    virtual ~MovingAverageBase() = default;
    
    // Indicator interface
    void start() override;
    void stop() override;
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
    // Moving average specific interface
    virtual double calculate_value(int index) = 0;
    virtual void reset_calculation();
    virtual void update_calculation(double value);
    
    // Properties
    int get_period() const { return params_.period; }
    void set_period(int period);
    
    // Data access
    double get_ma_value(int ago = 0) const;
    std::vector<double> get_ma_values(int count) const;
    
    // Line definitions
    enum Lines {
        MA = 0  // Moving average line
    };
    
protected:
    Params params_;
    
    // Calculation state
    std::vector<double> values_;     // Historical values for calculation
    double current_sum_ = 0.0;       // Current sum for optimization
    bool is_ready_ = false;          // Whether enough data points available
    int calculation_start_ = -1;     // Start index for calculations
    
    // Helper methods
    void add_value(double value);
    void remove_oldest_value();
    bool has_enough_data() const;
    void ensure_capacity();
    
    // Optimization for simple moving averages
    virtual bool can_use_sum_optimization() const { return false; }
    virtual void initialize_sum_optimization();
    virtual void update_sum_optimization(double new_value, double old_value);
    
private:
    void initialize_lines();
    void setup_plot_info();
};

/**
 * Registration helper template for automatic registration of moving averages
 */
template<typename T>
class MovingAverageRegistrar {
public:
    MovingAverageRegistrar(const std::string& name) {
        MovingAverage::register_moving_average(name, [](const Params& params) {
            return std::make_shared<T>(static_cast<const typename T::Params&>(params));
        });
    }
    
    MovingAverageRegistrar(const std::string& name, const std::vector<std::string>& aliases) {
        MovingAverage::register_moving_average(name, [](const Params& params) {
            return std::make_shared<T>(static_cast<const typename T::Params&>(params));
        });
        
        for (const auto& alias : aliases) {
            MovingAverage::register_alias(alias, name);
        }
    }
};

/**
 * Macro for easy registration of moving average indicators
 */
#define REGISTER_MOVING_AVERAGE(ClassName, Name, ...) \
    static MovingAverageRegistrar<ClassName> registrar_##ClassName(Name, ##__VA_ARGS__)

/**
 * MetaMovingAverage - Template for creating moving average indicators
 * 
 * This template provides a standardized way to create moving average indicators
 * with automatic registration and proper type safety.
 */
template<typename Derived>
class MetaMovingAverage : public MovingAverageBase {
public:
    using DerivedParams = typename Derived::Params;
    
    MetaMovingAverage(const DerivedParams& params = DerivedParams{})
        : MovingAverageBase(params), derived_params_(params) {}
    
    virtual ~MetaMovingAverage() = default;
    
protected:
    DerivedParams derived_params_;
    
    // CRTP pattern for compile-time polymorphism
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

/**
 * SimpleMovingAverage - Basic simple moving average implementation
 */
class SimpleMovingAverage : public MetaMovingAverage<SimpleMovingAverage> {
public:
    struct Params : public MovingAverageBase::Params {
        // SMA specific parameters can be added here
    };
    
    SimpleMovingAverage(const Params& params = Params{});
    virtual ~SimpleMovingAverage() = default;
    
    double calculate_value(int index) override;
    bool can_use_sum_optimization() const override { return true; }
    void initialize_sum_optimization() override;
    void update_sum_optimization(double new_value, double old_value) override;
    
private:
    double sum_ = 0.0;
    bool sum_initialized_ = false;
};

/**
 * ExponentialMovingAverage - Exponential moving average implementation
 */
class ExponentialMovingAverage : public MetaMovingAverage<ExponentialMovingAverage> {
public:
    struct Params : public MovingAverageBase::Params {
        double alpha = -1.0;  // Smoothing factor (-1 = auto-calculate from period)
    };
    
    ExponentialMovingAverage(const Params& params = Params{});
    virtual ~ExponentialMovingAverage() = default;
    
    double calculate_value(int index) override;
    void reset_calculation() override;
    
private:
    double alpha_;
    double previous_ema_ = 0.0;
    bool first_value_ = true;
};

/**
 * WeightedMovingAverage - Linearly weighted moving average implementation
 */
class WeightedMovingAverage : public MetaMovingAverage<WeightedMovingAverage> {
public:
    struct Params : public MovingAverageBase::Params {
        // WMA specific parameters can be added here
    };
    
    WeightedMovingAverage(const Params& params = Params{});
    virtual ~WeightedMovingAverage() = default;
    
    double calculate_value(int index) override;
    
private:
    double weight_sum_;
    void calculate_weight_sum();
};

/**
 * AdaptiveMovingAverage - Adaptive moving average (Kaufman's AMA)
 */
class AdaptiveMovingAverage : public MetaMovingAverage<AdaptiveMovingAverage> {
public:
    struct Params : public MovingAverageBase::Params {
        int fast_period = 2;    // Fast EMA period
        int slow_period = 30;   // Slow EMA period
    };
    
    AdaptiveMovingAverage(const Params& params = Params{});
    virtual ~AdaptiveMovingAverage() = default;
    
    double calculate_value(int index) override;
    void reset_calculation() override;
    
private:
    double fast_alpha_;
    double slow_alpha_;
    double previous_ama_ = 0.0;
    bool first_value_ = true;
    
    double calculate_efficiency_ratio(int index);
    double calculate_smoothing_constant(double efficiency_ratio);
};

} // namespace indicators
} // namespace backtrader