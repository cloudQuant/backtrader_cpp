# Lines Architecture Refactoring Plan

## Overview
This document outlines the refactoring plan to align the C++ backtrader implementation with the Python backtrader's Lines architecture.

## Current Architecture Issues

### 1. Inheritance Hierarchy Mismatch
**Python backtrader:**
```
LineRoot (base interface)
├── LineSingle (single line operations)
│   └── LineBuffer (data storage)
└── LineMultiple (multi-line operations)
    └── LineSeries (manages multiple LineBuffers)
        └── LineIterator (execution engine)
            ├── DataBase (OHLCV data feeds)
            ├── IndicatorBase (technical indicators)
            └── StrategyBase (trading strategies)
```

**Current C++ implementation:**
```
LineRoot (contains CircularBuffer directly)
├── IndicatorBase (direct inheritance)
└── DataFeed (unclear hierarchy)
```

### 2. Key Missing Components
- **LineSingle/LineMultiple distinction**: No differentiation between single and multi-line objects
- **LineSeries**: No proper multi-line management with named access
- **LineIterator**: Missing execution engine with next/once modes
- **Proper data line management**: Indicators can't handle multiple inputs/outputs properly

### 3. Design Pattern Mismatches
- Python uses metaclasses for dynamic line creation
- Python uses descriptors for line access (self.close, self.lines.close, etc.)
- Python has lazy evaluation through LineActions
- Python supports line binding and synchronization

## Refactoring Goals

1. **Maintain API compatibility** with Python backtrader where possible
2. **Support multi-line indicators** (like AroonOscillator with high/low inputs)
3. **Enable proper execution modes** (next/once)
4. **Preserve performance** while adding flexibility
5. **Keep the design C++-idiomatic** while matching Python functionality

## Refactoring Steps

### Phase 1: Core Infrastructure (Steps 1-4)

#### Step 1: Create New LineBuffer Class
```cpp
// Separate data storage from LineRoot
class LineBuffer {
    CircularBuffer<double> buffer_;
    size_t min_period_;
    std::string name_;
    // Data access methods
    // Buffer operations
};
```

#### Step 2: Create LineSingle and LineMultiple Base Classes
```cpp
// Single line operations
class LineSingle : public LineRoot {
    std::shared_ptr<LineBuffer> line_;
    // Single line specific operations
};

// Multi-line operations
class LineMultiple : public LineRoot {
    // Delegates to first line (line 0)
    virtual std::shared_ptr<LineBuffer> getLine(size_t idx) = 0;
};
```

#### Step 3: Refactor LineSeries
```cpp
// Manages multiple named LineBuffers
class LineSeries : public LineMultiple {
    std::vector<std::shared_ptr<LineBuffer>> lines_;
    std::unordered_map<std::string, size_t> line_names_;
    // Named access: open(), high(), low(), close()
    // Line management
};
```

#### Step 4: Refactor LineIterator
```cpp
// Execution engine
class LineIterator : public LineSeries {
    // next() and once() execution modes
    // Minimum period handling
    // Data synchronization
};
```

### Phase 2: Component Updates (Steps 5-7)

#### Step 5: Update IndicatorBase
```cpp
// Indicators inherit from LineIterator
class IndicatorBase : public LineIterator {
    std::vector<std::shared_ptr<LineRoot>> inputs_;
    // Support multiple inputs
    // Automatic minperiod calculation
};
```

#### Step 6: Update DataBase
```cpp
// Data feeds inherit from LineSeries
class DataBase : public LineSeries {
    // OHLCV line management
    // Data loading interface
};
```

#### Step 7: Update Existing Indicators
- Modify all indicators to work with new architecture
- Support multi-input indicators properly
- Ensure backward compatibility where possible

### Phase 3: Testing and Validation (Step 8)

#### Step 8: Comprehensive Testing
- Ensure all original tests pass
- Add new tests for multi-line functionality
- Performance benchmarking
- Integration testing

## Implementation Strategy

### 1. Incremental Refactoring
- Start with core classes (LineBuffer, LineSingle, LineMultiple)
- Build up to LineSeries and LineIterator
- Update components gradually
- Maintain working state throughout

### 2. Compatibility Layer
- Create adapter classes if needed
- Provide migration path for existing code
- Document breaking changes

### 3. C++ Specific Considerations
- Use templates for compile-time optimization
- Leverage move semantics for performance
- Consider constexpr where applicable
- Use smart pointers for memory management

## Example: AroonOscillator After Refactoring

```cpp
class AroonOscillator : public IndicatorBase {
public:
    AroonOscillator(std::shared_ptr<LineRoot> high, 
                    std::shared_ptr<LineRoot> low, 
                    size_t period = 14)
        : IndicatorBase({high, low}, "AroonOscillator") {
        
        // Multiple inputs supported naturally
        // Output lines managed by LineSeries
        addLine("aroonup");
        addLine("aroondown");
        addLine("oscillator");
        
        setMinPeriod(period + 1);
    }
    
    void next() override {
        // Access inputs naturally
        auto high = inputs_[0];
        auto low = inputs_[1];
        
        // Calculate and set outputs
        setLine("oscillator", aroon_up - aroon_down);
    }
};
```

## Benefits of Refactoring

1. **Consistency**: Matches Python backtrader architecture
2. **Flexibility**: Supports complex multi-line indicators
3. **Maintainability**: Clear separation of concerns
4. **Extensibility**: Easy to add new indicator types
5. **Performance**: Optimized data access patterns

## Migration Guide

### For Indicator Developers
```cpp
// Old style
class MyIndicator : public IndicatorBase {
    MyIndicator(std::shared_ptr<LineRoot> input)
        : IndicatorBase(input, "MyIndicator") {}
};

// New style
class MyIndicator : public IndicatorBase {
    MyIndicator(std::shared_ptr<LineRoot> input)
        : IndicatorBase({input}, "MyIndicator") {
        addLine("output");  // Explicit line management
    }
};
```

### For Strategy Developers
```cpp
// Access remains similar
double value = indicator->get(0);  // Current value
double prev = indicator->get(1);   // Previous value

// New multi-line access
double high = data->line("high")->get(0);
double low = data->line("low")->get(0);
```

## Timeline Estimate

- Phase 1 (Core Infrastructure): 2-3 days
- Phase 2 (Component Updates): 3-4 days  
- Phase 3 (Testing & Validation): 2-3 days
- Total: 7-10 days for complete refactoring

## Risks and Mitigation

1. **Breaking existing code**: Provide compatibility layer
2. **Performance regression**: Benchmark throughout
3. **Complex migration**: Detailed documentation and examples
4. **Test coverage**: Ensure comprehensive test suite

## Conclusion

This refactoring will align the C++ implementation with Python backtrader's proven architecture, enabling full feature parity while maintaining C++ performance characteristics.