#pragma once

#include "kama.h"
#include "envelope.h"

namespace backtrader {
namespace indicators {

// KAMA Envelope - Combines KAMA with envelope bands
class KAMAEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;     // KAMA period
        int fast = 2;        // KAMA fast parameter
        int slow = 30;       // KAMA slow parameter
        double perc = 2.5;   // Envelope percentage
    } params;
    
    // Lines
    enum Lines {
        mid = 0,    // KAMA line (middle)
        top = 1,    // Upper envelope
        bot = 2     // Lower envelope
    };
    
    KAMAEnvelope();
    KAMAEnvelope(std::shared_ptr<LineRoot> data, int period = 30, int fast = 2, int slow = 30, double perc = 2.5);
    virtual ~KAMAEnvelope() = default;
    
    // Utility methods for test framework compatibility
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    
    // Envelope specific methods
    double getMidLine(int ago = 0) const;
    double getUpperLine(int ago = 0) const;
    double getLowerLine(int ago = 0) const;
    
protected:
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // Component indicator
    std::shared_ptr<KAMA> kama_;
};

// Aliases
using KamaEnvelope = KAMAEnvelope;

} // namespace indicators
} // namespace backtrader