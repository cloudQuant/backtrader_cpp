#include "signal.h"
#include "dataseries.h"

namespace backtrader {

// Signal types list
const std::vector<SignalType> SignalTypes = {
    SignalType::SIGNAL_NONE,
    SignalType::SIGNAL_LONGSHORT,
    SignalType::SIGNAL_LONG,
    SignalType::SIGNAL_LONG_INV,
    SignalType::SIGNAL_LONG_ANY,
    SignalType::SIGNAL_SHORT,
    SignalType::SIGNAL_SHORT_INV,
    SignalType::SIGNAL_SHORT_ANY,
    SignalType::SIGNAL_LONGEXIT,
    SignalType::SIGNAL_LONGEXIT_INV,
    SignalType::SIGNAL_LONGEXIT_ANY,
    SignalType::SIGNAL_SHORTEXIT,
    SignalType::SIGNAL_SHORTEXIT_INV,
    SignalType::SIGNAL_SHORTEXIT_ANY
};

// Signal static members
const std::vector<SignalType> Signal::signal_types = SignalTypes;

Signal::Signal() : Indicator() {
    // Initialize lines
    lines = std::make_shared<Lines>(1); // One line: signal
    
    // Set signal line to data0's first line (typically close)
    if (datas.size() > 0 && datas[0]->lines) {
        auto signal_line = lines->getline(Lines::signal);
        auto data_line = datas[0]->lines->getline(DataSeries::Close);
        if (signal_line && data_line) {
            // Copy data from data line to signal line
            signal_line->bind(data_line);
        }
    }
}

void Signal::prenext() {
    // Call parent prenext
    Indicator::prenext();
}

void Signal::next() {
    // Signal line is bound to data line, so no additional processing needed
    Indicator::next();
}

void Signal::once(int start, int end) {
    // Signal line is bound to data line, so no additional processing needed
    Indicator::once(start, end);
}

// Helper functions
std::string signal_type_to_string(SignalType type) {
    switch (type) {
        case SignalType::SIGNAL_NONE: return "SIGNAL_NONE";
        case SignalType::SIGNAL_LONGSHORT: return "SIGNAL_LONGSHORT";
        case SignalType::SIGNAL_LONG: return "SIGNAL_LONG";
        case SignalType::SIGNAL_LONG_INV: return "SIGNAL_LONG_INV";
        case SignalType::SIGNAL_LONG_ANY: return "SIGNAL_LONG_ANY";
        case SignalType::SIGNAL_SHORT: return "SIGNAL_SHORT";
        case SignalType::SIGNAL_SHORT_INV: return "SIGNAL_SHORT_INV";
        case SignalType::SIGNAL_SHORT_ANY: return "SIGNAL_SHORT_ANY";
        case SignalType::SIGNAL_LONGEXIT: return "SIGNAL_LONGEXIT";
        case SignalType::SIGNAL_LONGEXIT_INV: return "SIGNAL_LONGEXIT_INV";
        case SignalType::SIGNAL_LONGEXIT_ANY: return "SIGNAL_LONGEXIT_ANY";
        case SignalType::SIGNAL_SHORTEXIT: return "SIGNAL_SHORTEXIT";
        case SignalType::SIGNAL_SHORTEXIT_INV: return "SIGNAL_SHORTEXIT_INV";
        case SignalType::SIGNAL_SHORTEXIT_ANY: return "SIGNAL_SHORTEXIT_ANY";
        default: return "UNKNOWN_SIGNAL";
    }
}

bool is_long_signal(SignalType type) {
    return type == SignalType::SIGNAL_LONG ||
           type == SignalType::SIGNAL_LONG_INV ||
           type == SignalType::SIGNAL_LONG_ANY ||
           type == SignalType::SIGNAL_LONGSHORT;
}

bool is_short_signal(SignalType type) {
    return type == SignalType::SIGNAL_SHORT ||
           type == SignalType::SIGNAL_SHORT_INV ||
           type == SignalType::SIGNAL_SHORT_ANY ||
           type == SignalType::SIGNAL_LONGSHORT;
}

bool is_exit_signal(SignalType type) {
    return type == SignalType::SIGNAL_LONGEXIT ||
           type == SignalType::SIGNAL_LONGEXIT_INV ||
           type == SignalType::SIGNAL_LONGEXIT_ANY ||
           type == SignalType::SIGNAL_SHORTEXIT ||
           type == SignalType::SIGNAL_SHORTEXIT_INV ||
           type == SignalType::SIGNAL_SHORTEXIT_ANY;
}

bool is_inverted_signal(SignalType type) {
    return type == SignalType::SIGNAL_LONG_INV ||
           type == SignalType::SIGNAL_SHORT_INV ||
           type == SignalType::SIGNAL_LONGEXIT_INV ||
           type == SignalType::SIGNAL_SHORTEXIT_INV;
}

bool is_any_signal(SignalType type) {
    return type == SignalType::SIGNAL_LONG_ANY ||
           type == SignalType::SIGNAL_SHORT_ANY ||
           type == SignalType::SIGNAL_LONGEXIT_ANY ||
           type == SignalType::SIGNAL_SHORTEXIT_ANY;
}

} // namespace backtrader