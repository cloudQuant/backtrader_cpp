#pragma once

#include "indicator.h"
#include <vector>

namespace backtrader {

// Signal type constants
enum class SignalType : int {
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

// Signal types list
extern const std::vector<SignalType> SignalTypes;

// Signal indicator class
class Signal : public Indicator {
public:
    Signal();
    virtual ~Signal() = default;
    
    // Signal types
    static const std::vector<SignalType> signal_types;
    
    // Lines
    enum Lines { signal = 0 };
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
};

// Helper functions for signal types
std::string signal_type_to_string(SignalType type);
bool is_long_signal(SignalType type);
bool is_short_signal(SignalType type);
bool is_exit_signal(SignalType type);
bool is_inverted_signal(SignalType type);
bool is_any_signal(SignalType type);

} // namespace backtrader