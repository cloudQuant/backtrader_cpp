#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "sma.h"
#include "ema.h"
#include <memory>

namespace backtrader {
namespace indicators {

// Envelope MixIn functionality
template<typename BaseIndicator>
class EnvelopeMixIn : public BaseIndicator {
public:
    struct EnvelopeParams {
        double perc = 2.5;  // Percentage for envelope bands
    };
    
    EnvelopeParams envelope_params;
    
    // Additional lines for envelope
    enum EnvelopeLines {
        top = BaseIndicator::Lines::_count,
        bot = BaseIndicator::Lines::_count + 1
    };
    
    EnvelopeMixIn() : BaseIndicator() {
        // Will be set up in derived classes
    }
    
protected:
    void calculate_envelope_bands() {
        if (!this->lines) return;
        
        auto base_line = this->lines->getline(0);  // First line from base indicator
        auto top_line = this->lines->getline(EnvelopeLines::top);
        auto bot_line = this->lines->getline(EnvelopeLines::bot);
        
        if (base_line && top_line && bot_line) {
            double base_value = (*base_line)[0];
            double perc = envelope_params.perc / 100.0;
            
            (*top_line)[0] = base_value * (1.0 + perc);
            (*bot_line)[0] = base_value * (1.0 - perc);
        }
    }
};

// Basic Envelope indicator
class Envelope : public Indicator {
public:
    struct Params {
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Lines
    enum Lines {
        src = 0,  // Source line
        top = 1,  // Top envelope band
        bot = 2   // Bottom envelope band
    };
    
    Envelope();
    Envelope(std::shared_ptr<LineSeries> data_source, double perc = 2.5);
    virtual ~Envelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate();
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// SMA Envelope
class SimpleMovingAverageEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;    // SMA period
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Lines
    enum Lines {
        sma = 0,  // SMA line
        top = 1,  // Top envelope band
        bot = 2   // Bottom envelope band
    };
    
    SimpleMovingAverageEnvelope();
    virtual ~SimpleMovingAverageEnvelope() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::SMA> sma_;
};

// EMA Envelope
class ExponentialMovingAverageEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;    // EMA period
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Lines
    enum Lines {
        ema = 0,  // EMA line
        top = 1,  // Top envelope band
        bot = 2   // Bottom envelope band
    };
    
    ExponentialMovingAverageEnvelope();
    virtual ~ExponentialMovingAverageEnvelope() = default;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::EMA> ema_;
};

// Aliases
using SMAEnvelope = SimpleMovingAverageEnvelope;
using EMAEnvelope = ExponentialMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader