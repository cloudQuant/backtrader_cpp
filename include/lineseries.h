#pragma once

#include "lineroot.h"
#include "linebuffer.h"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <limits>

namespace backtrader {

class LineAlias {
public:
    explicit LineAlias(size_t line_idx) : line_idx_(line_idx) {}
    
    template<typename T>
    std::shared_ptr<LineSingle> get(T* obj) const {
        return obj->getline(line_idx_);
    }
    
    template<typename T>
    void set(T* obj, std::shared_ptr<LineSingle> line) const {
        if (auto multiple = dynamic_cast<LineMultiple*>(line.get())) {
            line = multiple->getline(0);
        }
        
        if (auto actions = dynamic_cast<LineActions*>(line.get())) {
            if (!dynamic_cast<LineActions*>(line.get())) {
                line = std::make_shared<LineDelay>(
                    std::dynamic_pointer_cast<LineActions>(line), 0);
            }
            actions->addbinding(obj->getline(line_idx_));
        }
    }
    
private:
    size_t line_idx_;
};

class Lines {
public:
    Lines();
    virtual ~Lines() = default;
    
    // Line access
    std::shared_ptr<LineSingle> operator[](size_t idx) const;
    std::shared_ptr<LineSingle> getline(size_t idx) const;
    
    // Size and capacity
    size_t size() const;
    bool empty() const;
    
    // Buffer operations
    void forward(size_t size = 1);
    void backward(size_t size = 1);
    void rewind(size_t size = 1);
    void extend(size_t size = 1);
    void reset();
    void home();
    void advance(size_t size = 1);
    
    // Binding operations
    void addbinding(std::shared_ptr<LineSingle> binding);
    void oncebinding();
    
    // Buffer management
    void qbuffer(size_t savemem = 0);
    void minbuffer(size_t size);
    virtual size_t buflen() const;
    
    // Line management
    void add_line(std::shared_ptr<LineSingle> line);
    void set_line(size_t idx, std::shared_ptr<LineSingle> line);
    
    // Aliases
    void add_alias(const std::string& name, size_t idx);
    size_t get_alias_idx(const std::string& name) const;
    bool has_alias(const std::string& name) const;
    std::vector<std::string> get_aliases() const;
    
    // Class derivation (simplified version of Python's _derive)
    static std::shared_ptr<Lines> derive(const std::string& name, 
                                        const std::vector<std::string>& line_names,
                                        size_t extra_lines = 0);
    
protected:
    std::vector<std::shared_ptr<LineSingle>> lines_;
    std::map<std::string, size_t> aliases_;
    std::vector<std::string> aliases_order_;  // Track insertion order of aliases
};

class LineSeries : public LineMultiple {
public:
    // Line types (matching Python's enum)
    enum LType { IndType = 0, StratType = 1, ObsType = 2 };
    
    LineSeries();
    virtual ~LineSeries() = default;
    
    // Line access
    std::shared_ptr<LineSingle> getline(size_t idx = 0) override;
    size_t getlinealiases() const override;
    std::string getlinealias(size_t idx) const override;
    size_t fullsize() const override;
    
    // Implement pure virtual methods from LineMultiple
    size_t size() const override;
    bool empty() const override;
    virtual size_t buflen() const override;
    
    // Lines management
    std::shared_ptr<Lines> lines;
    
    // Owner relationship
    LineSeries* _owner;
    
    // CSV output support
    bool csv = false;
    
    // OHLCV accessor methods (virtual, default NaN - override in DataSeries)
    virtual double datetime(int ago = 0) const { return 0.0; }
    virtual double open(int ago = 0) const { return std::numeric_limits<double>::quiet_NaN(); }
    virtual double high(int ago = 0) const { return std::numeric_limits<double>::quiet_NaN(); }
    virtual double low(int ago = 0) const { return std::numeric_limits<double>::quiet_NaN(); }
    virtual double close(int ago = 0) const { return std::numeric_limits<double>::quiet_NaN(); }
    virtual double volume(int ago = 0) const { return 0.0; }
    virtual double openinterest(int ago = 0) const { return 0.0; }
    
    // Forward method to advance the lines
    virtual void forward(size_t size = 1) {
        if (lines) {
            lines->forward(size);
        }
    }
    
protected:
    void _init_lines();
    virtual std::vector<std::string> _get_line_names() const;
};

// Utility class for creating LineSeries from various inputs
class LineSeriesMaker {
public:
    static std::shared_ptr<LineSeries> make(std::shared_ptr<LineRoot> source);
    static std::shared_ptr<LineSeries> make(double value);
    static std::shared_ptr<LineSeries> make(const std::vector<double>& values);
};

// Stub class for incomplete LineSeries
class LineSeriesStub : public LineSeries {
public:
    LineSeriesStub() : LineSeries() {}
    virtual ~LineSeriesStub() = default;
    
    // Minimal implementation
    void next() {}
    void once(int start, int end) {}
};

// Template class for creating Lines with specific line names
template<typename... LineNames>
class LinesTemplate : public Lines {
public:
    LinesTemplate() : Lines() {
        // This would be implemented with variadic templates
        // For now, we'll use a simpler approach
    }
};

// Common line configurations
class OHLCLines : public Lines {
public:
    OHLCLines() : Lines() {
        // Add standard OHLC lines
        add_alias("open", 0);
        add_alias("high", 1);
        add_alias("low", 2);
        add_alias("close", 3);
        
        // Add the actual line buffers
        for (int i = 0; i < 4; ++i) {
            add_line(std::make_shared<LineBuffer>());
        }
    }
};

class OHLCVLines : public Lines {
public:
    OHLCVLines() : Lines() {
        // Add standard OHLCV lines
        add_alias("open", 0);
        add_alias("high", 1);
        add_alias("low", 2);
        add_alias("close", 3);
        add_alias("volume", 4);
        
        // Add the actual line buffers
        for (int i = 0; i < 5; ++i) {
            add_line(std::make_shared<LineBuffer>());
        }
    }
};

class OHLCVILines : public Lines {
public:
    OHLCVILines() : Lines() {
        // Add standard OHLCVI lines
        add_alias("open", 0);
        add_alias("high", 1);
        add_alias("low", 2);
        add_alias("close", 3);
        add_alias("volume", 4);
        add_alias("openinterest", 5);
        
        // Add the actual line buffers
        for (int i = 0; i < 6; ++i) {
            add_line(std::make_shared<LineBuffer>());
        }
    }
};

} // namespace backtrader