# Backtrader C++ Test Suite

This directory contains the comprehensive test suite for the Backtrader C++ refactoring project, following test-driven development (TDD) principles.

## Test Organization

### Test Framework Structure
- **test_framework.h**: Core testing infrastructure with utilities, data generators, and base classes
- **test_indicators.cpp**: Unit tests for all technical indicators (SMA, RSI, MACD, Bollinger Bands, Ichimoku, CCI, etc.)
- **test_integration.cpp**: Integration tests for system components working together
- **test_performance.cpp**: Performance benchmarks and optimization validation
- **test_stress.cpp**: Stress testing, concurrency testing, and boundary condition testing

### Test Categories

#### 1. Unit Tests (`test_indicators.cpp`)
- **SMA Tests**: Basic calculation, insufficient data handling, edge cases, performance
- **RSI Tests**: Range validation (0-100), overbought/oversold detection
- **MACD Tests**: Multi-line output validation, crossover signal detection
- **Bollinger Bands Tests**: Band relationships, bandwidth calculations
- **Ichimoku Tests**: Five-line calculation, cloud analysis
- **CCI Tests**: Typical price calculation, overbought/oversold detection
- **Parameterized Tests**: Multiple period and data configurations

#### 2. Integration Tests (`test_integration.cpp`)
- **Strategy Behavior**: SimpleTestStrategy and RSITestStrategy logic validation
- **Cerebro Engine**: Basic backtest execution, multiple strategies
- **Indicator Chains**: Multiple indicators working together, consistency validation
- **Memory Management**: Large-scale memory usage, cleanup verification
- **Error Handling**: Invalid input handling, boundary conditions
- **Concurrency**: Basic thread safety for read operations

#### 3. Performance Tests (`test_performance.cpp`)
- **SMA Performance**: Different dataset sizes and periods
- **Complex Indicators**: RSI, MACD, Bollinger Bands, Ichimoku benchmarks
- **Multi-Indicator**: Sequential and parallel calculation performance
- **Memory Performance**: Buffer size impact, large-scale memory management
- **Real-Time Performance**: Simulated real-time data processing
- **Comparison Tests**: Optimized vs naive implementation validation

#### 4. Stress Tests (`test_stress.cpp`)
- **High Volume Data**: 1M+ data points processing
- **Continuous Data Stream**: 1KHz real-time simulation
- **Multi-Threading**: Concurrent indicator calculations
- **Memory Stress**: Large-scale indicator creation and cleanup
- **Boundary Conditions**: Extreme values, rapid parameter changes
- **Endurance Tests**: Long-running calculations (2+ minutes)

## Building and Running Tests

### Prerequisites
- CMake 3.16+
- Google Test and Google Mock
- C++17 compatible compiler
- Optional: lcov (coverage), cppcheck (static analysis), valgrind (memory checking)

### Build Commands
```bash
# Create build directory
mkdir build && cd build

# Configure with testing enabled
cmake .. -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug

# Build all tests
make -j4

# Run all tests
make run_all_tests

# Run specific test categories
make run_unit_tests
make run_performance_tests
make run_stress_tests

# Generate coverage report (Debug build only)
make coverage

# Run static analysis
make static_analysis

# Run memory check
make memcheck
```

### CI/CD Integration
The test suite is integrated with GitHub Actions for:
- **Multi-platform testing**: Linux (GCC/Clang), macOS, Windows
- **Sanitizer builds**: AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer
- **Code coverage**: Automated coverage reporting with Codecov
- **Performance regression**: Comparison with base branch
- **Nightly stress tests**: Extended stress testing
- **Security scanning**: CodeQL analysis

## Test Data Generation

### TestDataGenerator Class
- **generateRandomOHLCV()**: Random walk price data with configurable volatility
- **generateSineWaveOHLCV()**: Predictable sine wave patterns for deterministic testing
- **generateTrendOHLCV()**: Trending data with configurable slope and noise
- **loadFromCSV()**: Load real market data from CSV files

### TestUtils Class
- **Statistical calculations**: Mean, standard deviation, min/max
- **Array comparison**: Approximate equality with tolerance
- **Performance timing**: High-resolution benchmarking
- **Memory monitoring**: Memory usage tracking

## Test Macros and Utilities

### Custom Assertions
- `EXPECT_DOUBLE_NEAR(expected, actual, tolerance)`: Floating-point comparison with tolerance
- `EXPECT_ARRAY_NEAR(expected, actual, tolerance)`: Array comparison with tolerance
- `EXPECT_NO_NAN(value)`: Ensure value is not NaN
- `EXPECT_FINITE(value)`: Ensure value is finite
- `BENCHMARK_START()` / `BENCHMARK_END(max_time_ms)`: Performance timing

### Mock Classes
- **MockDataLine**: Simulated data line for isolated testing
- **BacktraderTestBase**: Base test class with setup/teardown and timing

## Performance Expectations

### Target Performance Metrics
- **SMA (10K points)**: < 50ms
- **RSI (10K points)**: < 100ms
- **MACD (10K points)**: < 150ms
- **Ichimoku (10K points)**: < 300ms
- **Real-time processing**: < 1ms per tick for multiple indicators
- **Memory usage**: Linear with data size, minimal fragmentation

### Stress Test Thresholds
- **Success rate**: > 95% for all operations
- **Memory leaks**: Zero tolerance
- **Concurrent access**: Thread-safe read operations
- **Extreme values**: Graceful handling of NaN, infinity, extreme ranges

## Test-Driven Development Approach

### TDD Workflow
1. **Red**: Write failing test for new functionality
2. **Green**: Implement minimal code to pass the test
3. **Refactor**: Improve code while maintaining test passage
4. **Repeat**: Continue cycle for each feature

### Test Coverage Goals
- **Line coverage**: > 90%
- **Branch coverage**: > 85%
- **Function coverage**: > 95%
- **Critical paths**: 100% coverage for error handling and edge cases

## Troubleshooting

### Common Issues
- **Google Test not found**: Install libgtest-dev package or build from source
- **Compilation errors**: Ensure C++17 support and proper include paths
- **Test timeouts**: Adjust timeout values for slower systems
- **Memory check failures**: Review memory management in indicator implementations

### Debug Options
- **Verbose output**: Use `ctest --verbose` for detailed test output
- **Single test**: Run individual test executables directly
- **GDB debugging**: Build with `-DCMAKE_BUILD_TYPE=Debug` and use GDB
- **Sanitizer builds**: Enable with `-DENABLE_SANITIZERS=ON`

## Contributing to Tests

### Adding New Tests
1. Follow existing naming conventions (`TestClass_TestFunction`)
2. Use appropriate test fixtures for setup/teardown
3. Include both positive and negative test cases
4. Add performance benchmarks for new indicators
5. Document test purpose and expected behavior

### Test Requirements
- All tests must be deterministic (use fixed random seeds)
- Tests should complete within reasonable time limits
- Include boundary condition and error case testing
- Verify both correctness and performance
- Use descriptive test names and error messages

This test suite ensures the reliability, performance, and correctness of the Backtrader C++ refactoring while maintaining compatibility with the original Python implementation.