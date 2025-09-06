#pragma once

#include "../indicator.h"
#include "../lineseries.h"
#include "../dataseries.h"
#include "sma.h"
#include "ema.h"
#include "dema.h"
#include "tema.h"
#include "smma.h"
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
        int period = 30;    // Period for SMA calculation
    } params;
    
    // Line indices
    enum LineIndex {
        src = 0,  // Source line
        top = 1,  // Top envelope band
        bot = 2   // Bottom envelope band
    };
    
    Envelope();
    // Constructor for test framework compatibility
    Envelope(std::shared_ptr<LineSeries> data);  // Default period and perc
    Envelope(std::shared_ptr<LineSeries> data, double perc);
    // Constructor with period parameter for SMA-based envelope
    Envelope(std::shared_ptr<LineSeries> data, int period, double perc = 2.5);
    // DataSeries constructor for test framework compatibility
    Envelope(std::shared_ptr<DataSeries> data);  // Default period and perc
    Envelope(std::shared_ptr<DataSeries> data, int period, double perc = 2.5);
    virtual ~Envelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Multi-line access methods for test compatibility
    double getMidLine(int ago = 0) const;
    double getUpperLine(int ago = 0) const;
    double getLowerLine(int ago = 0) const;
    
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
    
    // Line indices
    enum LineIndex {
        sma = 0,  // SMA line
        top = 1,  // Top envelope band
        bot = 2   // Bottom envelope band
    };
    
    SimpleMovingAverageEnvelope();
    SimpleMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source);
    SimpleMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc = 2.5);
    SimpleMovingAverageEnvelope(std::shared_ptr<LineBuffer> data_source, int period, double perc);
    SimpleMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc = 2.5);
    virtual ~SimpleMovingAverageEnvelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Get specific line for multi-line access
    std::shared_ptr<LineBuffer> getLine(int index) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::SMA> sma_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// EMA Envelope
class ExponentialMovingAverageEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;    // EMA period
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Line indices
    enum LineIndex {
        ema = 0,  // EMA line
        top = 1,  // Top envelope band
        bot = 2   // Bottom envelope band
    };
    
    ExponentialMovingAverageEnvelope();
    ExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source);
    ExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc = 2.5);
    // DataSeries constructors for disambiguation
    ExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source);
    ExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc = 2.5);
    virtual ~ExponentialMovingAverageEnvelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Get specific line for multi-line access
    std::shared_ptr<LineBuffer> getLine(int index) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::EMA> ema_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// DEMA Envelope
class DoubleExponentialMovingAverageEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;    // DEMA period
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Line indices
    enum LineIndex {
        dema = 0,  // DEMA line
        top = 1,   // Top envelope band
        bot = 2    // Bottom envelope band
    };
    
    DoubleExponentialMovingAverageEnvelope();
    DoubleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source);
    DoubleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc = 2.5);
    DoubleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source);
    DoubleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc = 2.5);
    virtual ~DoubleExponentialMovingAverageEnvelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Get specific line for multi-line access
    std::shared_ptr<LineBuffer> getLine(int index) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::DoubleExponentialMovingAverage> dema_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// TEMA Envelope
class TripleExponentialMovingAverageEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;    // TEMA period
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Line indices
    enum LineIndex {
        tema = 0,  // TEMA line
        top = 1,   // Top envelope band
        bot = 2    // Bottom envelope band
    };
    
    TripleExponentialMovingAverageEnvelope();
    TripleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source);
    TripleExponentialMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc = 2.5);
    TripleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source);
    TripleExponentialMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc = 2.5);
    virtual ~TripleExponentialMovingAverageEnvelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Get specific line for multi-line access
    std::shared_ptr<LineSingle> getLine(int index) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::TEMA> tema_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// SMMA Envelope
class SmoothedMovingAverageEnvelope : public Indicator {
public:
    struct Params {
        int period = 30;    // SMMA period
        double perc = 2.5;  // Percentage for envelope bands
    } params;
    
    // Line indices
    enum LineIndex {
        smma = 0,  // SMMA line
        top = 1,   // Top envelope band
        bot = 2    // Bottom envelope band
    };
    
    SmoothedMovingAverageEnvelope();
    SmoothedMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source);
    SmoothedMovingAverageEnvelope(std::shared_ptr<LineSeries> data_source, int period, double perc = 2.5);
    SmoothedMovingAverageEnvelope(std::shared_ptr<DataSeries> data_source, int period, double perc = 2.5);
    virtual ~SmoothedMovingAverageEnvelope() = default;
    
    // Utility methods
    double get(int ago = 0) const;
    int getMinPeriod() const;
    void calculate() override;
    size_t size() const override;
    
    // Get specific line for multi-line access
    std::shared_ptr<LineBuffer> getLine(int index) const;
    
protected:
    void prenext() override;
    void next() override;
    void once(int start, int end) override;
    
private:
    void setup_lines();
    std::shared_ptr<indicators::SMMA> smma_;
    
    // LineSeries support
    std::shared_ptr<LineSeries> data_source_;
    size_t current_index_;
};

// Aliases
using SMAEnvelope = SimpleMovingAverageEnvelope;
using EMAEnvelope = ExponentialMovingAverageEnvelope;
using DEMAEnvelope = DoubleExponentialMovingAverageEnvelope;
using TEMAEnvelope = TripleExponentialMovingAverageEnvelope;
using SMMAEnvelope = SmoothedMovingAverageEnvelope;

} // namespace indicators
} // namespace backtrader