# Immediate Fixes for Backtrader C++ Python Bindings

## Problem Analysis

The current backtrader_cpp Python bindings have several compatibility issues with the original backtrader API:

1. **Indicator constructors don't accept data parameters** - Backtrader indicators are created with data as the first parameter, but the current C++ bindings only accept parameters like period.

2. **Missing data access patterns** - Backtrader supports `data.close[0]`, `data[-1]`, etc., but the current bindings may not fully implement these patterns.

3. **Incomplete Strategy lifecycle** - The Strategy class is missing several lifecycle methods and notification systems.

4. **Missing Cerebro features** - The Cerebro engine lacks many advanced features like optimization, resampling, etc.

## Immediate Fixes Needed

### 1. Fix Indicator Constructors

**Current Problem**: 
```python
# This doesn't work in backtrader_cpp
sma = bt.indicators.SMA(data, period=20)
# TypeError: __init__(): incompatible constructor arguments
```

**Solution**: Modify the PySMA class to accept data as the first parameter:

```cpp
// In PySMA class
py::class_<PySMA, std::shared_ptr<PySMA>>(indicators, "SMA")
    .def(py::init<std::shared_ptr<PyDataSeries>, int>(), 
         py::arg("data"), py::arg("period") = 20, "Create SMA indicator")
    // ... rest of bindings
```

But this requires modifying the PySMA class to store and process the data.

### 2. Implement Data Access Patterns

**Current Problem**:
```python
# This should work
close_price = data.close[0]
prev_close = data.close[-1]
```

**Solution**: Ensure PyDataSeries properly implements `__getitem__` and line access.

### 3. Complete Strategy Lifecycle

**Current Problem**: Missing methods like `nextstart`, `notify_order`, etc.

**Solution**: Add all lifecycle methods to PyStrategy class.

## Implementation Plan

### Phase 1: Fix Core API Compatibility (1-2 weeks)

#### Task 1.1: Fix Indicator Constructor Pattern (3 days)
- Modify all indicator classes to accept data as first parameter
- Update PySMA, PyEMA, PyRSI, etc. constructors
- Implement data processing logic in C++
- Update Python bindings

#### Task 1.2: Implement Data Access (2 days)
- Fix PyDataSeries `__getitem__` implementation
- Add proper line access (`data.close`, `data.open`, etc.)
- Implement negative indexing
- Add slice support

#### Task 1.3: Complete Strategy Methods (3 days)
- Add missing lifecycle methods (`nextstart`, `prenextstart`, etc.)
- Implement notification methods (`notify_order`, `notify_trade`, etc.)
- Add order management methods (`buy`, `sell`, `close`)
- Implement position management

### Phase 2: Fix Test Compatibility (1 week)

#### Task 2.1: Make Existing Tests Pass (5 days)
- Run each test file and identify specific failures
- Fix compatibility issues one by one
- Ensure test results match expected values
- Document any remaining incompatibilities

### Phase 3: Performance Optimization (1 week)

#### Task 3.1: Optimize Data Access (3 days)
- Implement zero-copy data access patterns
- Optimize memory usage for large datasets
- Add caching for frequently accessed data

#### Task 3.2: Benchmark Performance (2 days)
- Compare performance with original backtrader
- Ensure 8-25x performance improvement is maintained
- Identify and fix performance bottlenecks

## Technical Implementation Details

### Fixing Indicator Constructors

The key issue is that backtrader indicators are created with a pattern like:
```python
sma = bt.indicators.SMA(data, period=20)
```

But the current C++ implementation only supports:
```python
sma = bt.indicators.SMA(period=20)
```

To fix this, we need to:

1. Modify the PySMA class to accept a data parameter:
```cpp
class PySMA : public PyIndicator {
private:
    std::shared_ptr<PyDataSeries> data_;
    int period_;
    std::shared_ptr<PyLineBuffer> output_;
    
public:
    PySMA(std::shared_ptr<PyDataSeries> data, int period = 20) 
        : PyIndicator("sma"), data_(data), period_(period) {
        PyValidator::validate_period(period, "period");
        output_ = std::make_shared<PyLineBuffer>();
        lines_.push_back(output_);
        
        // Process the data to calculate SMA values
        calculate_sma();
    }
    
    void calculate_sma() {
        // Implementation of SMA calculation using data_
        // This would populate the output_ line buffer
    }
};
```

2. Update the Python binding:
```cpp
py::class_<PySMA, std::shared_ptr<PySMA>>(indicators, "SMA")
    .def(py::init<std::shared_ptr<PyDataSeries>, int>(), 
         py::arg("data"), py::arg("period") = 20, "Create SMA indicator")
    // ... rest of bindings
```

### Fixing Data Access Patterns

The PyDataSeries needs to properly implement line access:

```cpp
// In PyDataSeries class
.def("__getattr__", [](PyDataSeries& self, const std::string& name) {
    // Handle line access like data.close, data.open, etc.
    if (name == "close") {
        return self.get_close_line();
    } else if (name == "open") {
        return self.get_open_line();
    }
    // ... other lines
    throw py::attribute_error("No such attribute: " + name);
})
```

### Completing Strategy Lifecycle

Add missing methods to PyStrategy:

```cpp
// In PyStrategy class
.def("nextstart", &PyStrategy::nextstart, "Called once when minimum period is reached")
.def("prenextstart", &PyStrategy::prenextstart, "Called before prenext starts")
.def("notify_order", &PyStrategy::notify_order, "Called when order status changes")
.def("notify_trade", &PyStrategy::notify_trade, "Called when trade is executed")
```

## Testing Approach

1. **Unit Testing**: Test each fix individually
2. **Integration Testing**: Test complete workflows
3. **Compatibility Testing**: Compare results with original backtrader
4. **Performance Testing**: Ensure performance improvements are maintained

## Expected Outcomes

1. **API Compatibility**: 95%+ compatibility with original backtrader
2. **Test Pass Rate**: All existing tests pass
3. **Performance**: Maintain 8-25x performance improvement
4. **Usability**: Python developers can use backtrader_cpp as a drop-in replacement