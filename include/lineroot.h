#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <string>

namespace backtrader {

class LineRoot {
public:
    enum class IndType : int { IndType = 0, StratType = 1, ObsType = 2 };
    
    LineRoot();
    LineRoot(size_t size, const std::string& name = "");
    virtual ~LineRoot() = default;
    
    // Basic lifecycle hooks
    virtual void _stage1() {}
    virtual void _stage2() {}
    virtual void _periodrecalc() {}
    virtual void updateminperiod(size_t minperiod);
    virtual void addminperiod(size_t minperiod);
    virtual void incminperiod(size_t minperiod);
    virtual void _start() {}
    virtual void _stop() {}
    virtual void _notify() {}
    virtual void _clk_update() {}
    
    // Period management
    size_t _minperiod() const { return minperiod_; }
    void _minperiod(size_t period) { minperiod_ = period; }
    
    // Ownership and hierarchy
    LineRoot* _owner;
    int _opstage;
    IndType _ltype;
    
    // Note: These operators are intentionally overridden in derived classes with different return types
    // to support method chaining. The overloaded-virtual warnings are expected and can be ignored.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
    // Arithmetic operations
    virtual LineRoot* operator+(const LineRoot& other) const;
    virtual LineRoot* operator-(const LineRoot& other) const;
    virtual LineRoot* operator*(const LineRoot& other) const;
    virtual LineRoot* operator/(const LineRoot& other) const;
    
    // Comparison operations
    virtual LineRoot* operator<(const LineRoot& other) const;
    virtual LineRoot* operator<=(const LineRoot& other) const;
    virtual LineRoot* operator>(const LineRoot& other) const;
    virtual LineRoot* operator>=(const LineRoot& other) const;
    virtual LineRoot* operator==(const LineRoot& other) const;
    virtual LineRoot* operator!=(const LineRoot& other) const;
    
    // Logical operations
    virtual LineRoot* operator&&(const LineRoot& other) const;
    virtual LineRoot* operator||(const LineRoot& other) const;
    
    // Unary operations
    virtual LineRoot* operator-() const;
    virtual LineRoot* operator!() const;
    
    // Math functions
    virtual LineRoot* abs() const;
    virtual LineRoot* pow(double exponent) const;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    
    // Basic data operations (for compatibility)
    virtual void calculate() {}
    virtual void reset() {}
    
    // Forward/backward operations (default implementations for compatibility)
    virtual void forward(double /*value*/) {}
    virtual void forward(size_t /*size*/ = 1) {}
    virtual void backward(size_t /*size*/ = 1) {}
    virtual void rewind(size_t /*size*/ = 1) {}
    virtual void extend(size_t /*size*/ = 1) {}
    virtual void advance(size_t /*size*/ = 1) {}
    
    // Aliasing support
    bool aliased = false;
    
protected:
    size_t minperiod_;
    
private:
    static size_t next_id_;
    size_t id_;
};

class LineSingle : public LineRoot {
public:
    LineSingle();
    virtual ~LineSingle() = default;
    
    // Single line operations
    virtual double operator[](int index) const = 0;
    virtual void set(int index, double value) = 0;
    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
    
    // Convenience method for testing
    virtual double get(int index) const { return (*this)[index]; }
    
    // Forward/backward operations
    virtual void forward(size_t size = 1) override = 0;
    virtual void backward(size_t size = 1) override = 0;
    virtual void rewind(size_t size = 1) override = 0;
    virtual void extend(size_t size = 1) override = 0;
    virtual void reset() override = 0;

    // Buffer operations
    virtual void home() = 0;
    virtual size_t buflen() const = 0;
    virtual void advance(size_t size = 1) override = 0;
    
    // Binding operations
    virtual void addbinding(std::shared_ptr<LineSingle> binding) = 0;
    virtual void oncebinding() = 0;
    virtual void bind2line(std::shared_ptr<LineSingle> binding) = 0;
    
    // Period operations
    void updateminperiod(size_t minperiod) override;
    void addminperiod(size_t minperiod) override;
    void incminperiod(size_t minperiod) override;
};

class LineMultiple : public LineRoot {
public:
    LineMultiple();
    virtual ~LineMultiple() = default;
    
    // Multiple lines management
    virtual std::shared_ptr<LineSingle> getline(size_t idx = 0) = 0;
    virtual size_t getlinealiases() const = 0;
    virtual std::string getlinealias(size_t idx) const = 0;
    virtual size_t fullsize() const = 0;
    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
    
    // Delegated operations to line[0]
    virtual double operator[](int index) const;
    virtual void set(int index, double value);
    
    // Buffer operations
    virtual void forward(size_t size = 1) override;
    virtual void backward(size_t size = 1) override;
    // Note: LineIterator overloads rewind - this is intentional
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
    virtual void rewind(size_t size = 1) override;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    virtual void extend(size_t size = 1) override;
    void reset() override;
    virtual void home();
    virtual size_t buflen() const;
    virtual void advance(size_t size = 1) override;
    
    // Binding operations
    virtual void addbinding(std::shared_ptr<LineSingle> binding);
    virtual void oncebinding();
    // Note: LineIterator overloads bind2line with different signature - this is intentional
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
    virtual void bind2line(std::shared_ptr<LineSingle> binding);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    
    // Period operations
    void updateminperiod(size_t minperiod) override;
    void addminperiod(size_t minperiod) override;
    void incminperiod(size_t minperiod) override;
    
protected:
    std::vector<std::shared_ptr<LineSingle>> lines_;
};

} // namespace backtrader