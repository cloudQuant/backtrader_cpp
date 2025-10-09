# Concrete Implementation Plan for Backtrader C++ Python Bindings

## Immediate Priority: Fix SMA Indicator Constructor

### Current Issue
The SMA indicator constructor in backtrader_cpp only accepts a period parameter:
```cpp
py::class_<PySMA, std::shared_ptr<PySMA>>(indicators, "SMA")
    .def(py::init<int>(), py::arg("period") = 20, "Create SMA indicator")
```

But backtrader expects:
```python
sma = bt.indicators.SMA(data, period=20)
```

### Fix Implementation

#### Step 1: Modify PySMA Class

In `/home/yun/Documents/refactor_backtrader/backtrader_cpp/python_bindings/src/backtrader_cpp_bindings.cpp`, modify the PySMA class:

```cpp
/** 
 * @brief SMA - Simple Moving Average Indicator
 */
class PySMA : public PyIndicator {
private:
    std::shared_ptr<PyDataSeries> data_;
    int period_;
    std::shared_ptr<PyLineBuffer> output_;
    
public:
    // Constructor that accepts data as first parameter (backtrader compatible)
    PySMA(std::shared_ptr<PyDataSeries> data, int period = 20) 
        : PyIndicator("sma"), data_(data), period_(period) {
        PyValidator::validate_period(period, "period");
        output_ = std::make_shared<PyLineBuffer>();
        lines_.push_back(output_);
        
        // Initialize with NaN values
        // In a full implementation, this would be calculated in the next() method
    }
    
    // Original constructor for backward compatibility
    PySMA(int period = 20) : PyIndicator("sma"), data_(nullptr), period_(period) {
        PyValidator::validate_period(period, "period");
        output_ = std::make_shared<PyLineBuffer>();
        lines_.push_back(output_);
    }
    
    void next() {
        // For now, just append NaN - full implementation would calculate SMA
        if (data_ && data_->size() >= static_cast<size_t>(period_)) {
            // Calculate actual SMA
            double sum = 0.0;
            for (int i = 0; i < period_; ++i) {
                sum += data_->close(data_->size() - period_ + i);
            }
            output_->append(sum / period_);
        } else {
            output_->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    std::shared_ptr<PyLineBuffer> line(int idx = 0) const {
        if (idx == 0 && !lines_.empty()) {
            return lines_[0];
        }
        return nullptr;
    }
    
    int period() const { return period_; }
};
```

#### Step 2: Update Python Binding

Replace the current SMA binding with:

```cpp
// SMA Indicator
py::class_<PySMA, std::shared_ptr<PySMA>>(indicators, "SMA")
    .def(py::init<std::shared_ptr<PyDataSeries>, int>(), 
         py::arg("data"), py::arg("period") = 20, "Create SMA indicator with data")
    .def(py::init<int>(), py::arg("period") = 20, "Create SMA indicator")
    .def("line", &PySMA::line, py::arg("idx") = 0, "Get SMA line")
    .def("__getitem__", [](PySMA& self, int idx) {
        auto line = self.line(0);
        if (line) {
            // Handle negative indexing
            if (idx < 0) {
                idx += line->len();
            }
            if (idx >= 0 && static_cast<size_t>(idx) < line->len()) {
                return line->get(idx);
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    })
    .def_property_readonly("period", &PySMA::period, "Get SMA period")
    .def("__repr__", [](const PySMA& sma) {
        return "<backtrader.indicators.SMA period=" + std::to_string(sma.period()) + ">";
    });
```

## Immediate Priority: Fix Data Access Patterns

### Current Issue
Data access patterns like `data.close[0]` don't work properly.

### Fix Implementation

#### Step 1: Enhance PyDataSeries

Add proper line access methods:

```cpp
// In PyDataSeries class
.def("__getattr__", [](PyDataSeries& self, const std::string& name) -> py::object {
    // Return line objects for backtrader compatibility
    if (name == "close") {
        // Create a proxy object that behaves like a line
        class LineProxy {
        private:
            PyDataSeries& data_;
            std::string line_name_;
        public:
            LineProxy(PyDataSeries& data, const std::string& line_name) 
                : data_(data), line_name_(line_name) {}
                
            double operator[](int idx) const {
                if (line_name_ == "close") {
                    if (idx < 0) idx += data_.size();
                    if (idx >= 0 && static_cast<size_t>(idx) < data_.size()) {
                        return data_.close(idx);
                    }
                }
                return std::numeric_limits<double>::quiet_NaN();
            }
        };
        
        // Return a lambda that acts like a line
        auto line_proxy = [self_ptr = &self](int idx) -> double {
            if (idx < 0) idx += self_ptr->size();
            if (idx >= 0 && static_cast<size_t>(idx) < self_ptr->size()) {
                return self_ptr->close(idx);
            }
            return std::numeric_limits<double>::quiet_NaN();
        };
        
        return py::cast(line_proxy);
    }
    // Handle other line types (open, high, low, volume, etc.)
    throw py::attribute_error("DataSeries has no attribute '" + name + "'");
})
```

Actually, a better approach is to create line objects:

```cpp
// Add a Line class to represent data lines
class PyDataLine {
private:
    PyDataSeries& data_;
    std::function<double(size_t)> getter_;
    
public:
    PyDataLine(PyDataSeries& data, std::function<double(size_t)> getter) 
        : data_(data), getter_(getter) {}
        
    double operator[](int idx) const {
        if (idx < 0) idx += data_.size();
        if (idx >= 0 && static_cast<size_t>(idx) < data_.size()) {
            return getter_(idx);
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    size_t size() const { return data_.size(); }
};

// In PyDataSeries class
.def("__getattr__", [](PyDataSeries& self, const std::string& name) {
    if (name == "close") {
        return py::cast(PyDataLine(self, [&self](size_t idx) { return self.close(idx); }));
    } else if (name == "open") {
        return py::cast(PyDataLine(self, [&self](size_t idx) { return self.open(idx); }));
    } else if (name == "high") {
        return py::cast(PyDataLine(self, [&self](size_t idx) { return self.high(idx); }));
    } else if (name == "low") {
        return py::cast(PyDataLine(self, [&self](size_t idx) { return self.low(idx); }));
    } else if (name == "volume") {
        return py::cast(PyDataLine(self, [&self](size_t idx) { return self.volume(idx); }));
    }
    throw py::attribute_error("DataSeries has no attribute '" + name + "'");
})
```

## Immediate Priority: Complete Strategy Lifecycle Methods

### Current Issue
Missing strategy lifecycle methods and notification systems.

### Fix Implementation

#### Step 1: Add Missing Methods to PyStrategy

```cpp
// In PyStrategy class
class PyStrategy {
    // ... existing members ...
    
public:
    // Additional lifecycle methods
    virtual void nextstart() {}
    virtual void prenextstart() {}
    virtual void notify_order(std::shared_ptr<PyOrder> order) {}
    virtual void notify_trade(std::shared_ptr<PyTrade> trade) {}
    virtual void notify_cashvalue(double cash, double value) {}
    virtual void notify_fund(double cash, double value, double fundvalue, int shares) {}
    
    // Parameter access (backtrader compatibility)
    py::object __getattr__(const std::string& name) {
        if (name == "p") {
            return params_dict_;
        }
        // Try to find in params
        try {
            return params_dict_[py::str(name)];
        } catch (...) {
            throw py::attribute_error("Strategy has no attribute '" + name + "'");
        }
    }
    
    // Data access
    py::object __getitem__(int idx) {
        if (idx >= 0 && static_cast<size_t>(idx) < datas_.size()) {
            return py::cast(datas_[idx]);
        }
        throw py::index_error("Index out of range");
    }
};
```

#### Step 2: Update Python Binding

```cpp
// Strategy
py::class_<PyStrategy, std::shared_ptr<PyStrategy>>(m, "Strategy")
    .def(py::init<>(), "Create Strategy")
    .def("data", &PyStrategy::data, py::arg("idx") = 0, "Get data series")
    .def("broker", &PyStrategy::broker, "Get broker")
    .def("position", &PyStrategy::position, py::arg("idx") = 0, "Get position")
    .def("buy", &PyStrategy::buy, py::arg("size") = 0.0, py::arg("price") = 0.0, "Buy order")
    .def("sell", &PyStrategy::sell, py::arg("size") = 0.0, py::arg("price") = 0.0, "Sell order")
    .def("close", &PyStrategy::close, py::arg("data") = nullptr, "Close position")
    .def("getposition", &PyStrategy::position, py::arg("data") = nullptr, "Get position (alias)")
    .def("set_params", &PyStrategy::set_params, py::arg("params"), "Set strategy parameters")
    .def("__getattr__", &PyStrategy::__getattr__, "Get attribute or parameter")
    .def("__getitem__", &PyStrategy::__getitem__, "Get data by index")
    // Additional lifecycle methods
    .def("nextstart", &PyStrategy::nextstart, "Called when minimum period is reached")
    .def("prenextstart", &PyStrategy::prenextstart, "Called before prenext starts")
    .def("notify_order", &PyStrategy::notify_order, py::arg("order"), "Called when order status changes")
    .def("notify_trade", &PyStrategy::notify_trade, py::arg("trade"), "Called when trade is executed")
    .def("notify_cashvalue", &PyStrategy::notify_cashvalue, 
         py::arg("cash"), py::arg("value"), "Called when cash or value changes")
    .def("notify_fund", &PyStrategy::notify_fund, 
         py::arg("cash"), py::arg("value"), py::arg("fundvalue"), py::arg("shares"), 
         "Called for fund notifications")
    .def("__init__", &PyStrategy::__init__, "Initialize strategy")
    .def("start", &PyStrategy::start, "Start strategy")
    .def("prenext", &PyStrategy::prenext, "Pre-next callback")
    .def("next", &PyStrategy::next, "Next callback")
    .def("stop", &PyStrategy::stop, "Stop strategy")
    .def("__repr__", &PyStrategy::repr);
```

## Testing Plan

### Step 1: Create Simple Test

Create a test file to verify the fixes:

```python
# test_fixes.py
import backtrader_cpp as bt

def test_sma_constructor():
    """Test that SMA can be created with data parameter"""
    data = bt.create_sample_data(100)
    print(f"Created data with {len(data)} bars")
    
    # This should work now
    sma = bt.indicators.SMA(data, period=20)
    print(f"Created SMA: {sma}")
    print(f"SMA period: {sma.period}")
    
    # Test data access
    print(f"First close price: {data.close[0]}")
    print(f"Last close price: {data.close[-1]}")
    
    # Test indicator access
    try:
        val = sma[0]
        print(f"SMA value: {val}")
    except Exception as e:
        print(f"Error accessing SMA value: {e}")

def test_strategy():
    """Test basic strategy functionality"""
    class TestStrategy(bt.Strategy):
        def __init__(self):
            self.sma = bt.indicators.SMA(self.data, period=20)
            
        def next(self):
            print(f"Close: {self.data.close[0]}, SMA: {self.sma[0]}")
            
        def nextstart(self):
            print("Next start called")
            
        def notify_order(self, order):
            print(f"Order notification: {order}")
    
    data = bt.create_sample_data(100)
    cerebro = bt.Cerebro()
    cerebro.add_data(data)
    cerebro.add_strategy(TestStrategy)
    
    try:
        results = cerebro.run()
        print("Strategy run successful")
    except Exception as e:
        print(f"Error running strategy: {e}")

if __name__ == "__main__":
    test_sma_constructor()
    test_strategy()
```

## Implementation Steps

1. **Backup current bindings file**
2. **Modify PySMA class to accept data parameter**
3. **Update SMA Python binding**
4. **Enhance PyDataSeries for proper line access**
5. **Add missing Strategy methods**
6. **Update Strategy Python binding**
7. **Test with simple test file**
8. **Run original backtrader tests**
9. **Iterate and fix any remaining issues**

## Expected Timeline

- **Day 1-2**: Fix SMA constructor and data access
- **Day 3**: Complete Strategy lifecycle methods
- **Day 4**: Testing and debugging
- **Day 5**: Run original tests and fix compatibility issues