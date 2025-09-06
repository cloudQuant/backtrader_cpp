#!/usr/bin/env python3
"""
Comprehensive test suite for extended backtrader_cpp Python bindings
Tests all 33 available functions including the new advanced indicators
"""

import backtrader_cpp
import numpy as np
import time

def main():
    print("🚀 Backtrader C++ Extended Functionality Test Suite")
    print("=" * 60)
    
    # Get version info
    version_info = backtrader_cpp.get_version()
    print(f"📋 Version: {version_info['version']}")
    print(f"🔧 Compiler: {version_info['compiler']}")
    print(f"📅 Build Date: {version_info['build_date']} {version_info['build_time']}")
    print(f"✨ Status: {version_info['status']}")
    print()
    
    # Generate test data
    np.random.seed(42)
    n_points = 100
    base_price = 100.0
    
    # Create realistic price data with trend and volatility
    returns = np.random.normal(0.001, 0.02, n_points)
    prices = [base_price]
    for ret in returns[1:]:
        prices.append(prices[-1] * (1 + ret))
    
    # Generate OHLCV data
    highs = [p * (1 + abs(np.random.normal(0, 0.01))) for p in prices]
    lows = [p * (1 - abs(np.random.normal(0, 0.01))) for p in prices]
    closes = prices.copy()
    volumes = [np.random.randint(1000, 10000) for _ in range(n_points)]
    
    print(f"📊 Test Data Generated: {n_points} data points")
    print(f"   Price Range: {min(prices):.2f} - {max(prices):.2f}")
    print()
    
    # Test counter
    total_tests = 0
    passed_tests = 0
    failed_tests = []
    
    def test_function(name, func, *args, **kwargs):
        nonlocal total_tests, passed_tests
        total_tests += 1
        try:
            start_time = time.time()
            result = func(*args, **kwargs)
            end_time = time.time()
            
            # Validate result
            if result is None:
                raise ValueError("Function returned None")
                
            if isinstance(result, dict):
                # Multi-output indicator
                if not result:
                    raise ValueError("Function returned empty dict")
                valid_values = 0
                for key, values in result.items():
                    if isinstance(values, list) and len(values) > 0:
                        valid_values += sum(1 for v in values if not np.isnan(v))
                if valid_values == 0:
                    raise ValueError("All result values are NaN")
            elif isinstance(result, list):
                # Single-output indicator
                if len(result) == 0:
                    raise ValueError("Function returned empty list")
                valid_values = sum(1 for v in result if not np.isnan(v))
                if valid_values == 0:
                    raise ValueError("All result values are NaN")
            elif isinstance(result, (int, float)):
                # Single value result
                if np.isnan(result):
                    raise ValueError("Function returned NaN")
            
            passed_tests += 1
            print(f"✅ {name:<35} {end_time-start_time:.4f}s")
            return True
            
        except Exception as e:
            failed_tests.append((name, str(e)))
            print(f"❌ {name:<35} Failed: {str(e)}")
            return False
    
    print("🔬 Testing Technical Indicators")
    print("-" * 40)
    
    # Basic Moving Averages
    test_function("SMA", backtrader_cpp.calculate_sma, prices, 20)
    test_function("EMA", backtrader_cpp.calculate_ema, prices, 20)
    test_function("WMA", backtrader_cpp.calculate_wma, prices, 20)
    
    # Advanced Moving Averages
    test_function("DEMA", backtrader_cpp.calculate_dema, prices, 20)
    test_function("TEMA", backtrader_cpp.calculate_tema, prices, 20)
    test_function("HMA", backtrader_cpp.calculate_hma, prices, 20)
    test_function("KAMA", backtrader_cpp.calculate_kama, prices, 20)
    
    # Oscillators
    test_function("RSI", backtrader_cpp.calculate_rsi, prices, 14)
    test_function("CCI", backtrader_cpp.calculate_cci, highs, lows, closes, 20)
    test_function("Williams %R", backtrader_cpp.calculate_williamsr, highs, lows, closes, 14)
    test_function("Stochastic", backtrader_cpp.calculate_stochastic, highs, lows, closes, 14, 3)
    test_function("TSI", backtrader_cpp.calculate_tsi, prices, 25, 13)
    test_function("Ultimate Oscillator", backtrader_cpp.calculate_ultimate_oscillator, highs, lows, closes, 7, 14, 28)
    
    # Trend Indicators
    test_function("MACD", backtrader_cpp.calculate_macd, prices, 12, 26, 9)
    test_function("Bollinger Bands", backtrader_cpp.calculate_bollinger, prices, 20, 2.0)
    test_function("ATR", backtrader_cpp.calculate_atr, highs, lows, closes, 14)
    test_function("Aroon", backtrader_cpp.calculate_aroon, highs, lows, 25)
    test_function("DPO", backtrader_cpp.calculate_dpo, prices, 20)
    test_function("Vortex", backtrader_cpp.calculate_vortex, highs, lows, closes, 14)
    
    # Momentum Indicators
    test_function("Momentum", backtrader_cpp.calculate_momentum, prices, 10)
    test_function("ROC", backtrader_cpp.calculate_roc, prices, 10)
    
    # Statistical Functions
    test_function("Highest", backtrader_cpp.calculate_highest, prices, 20)
    test_function("Lowest", backtrader_cpp.calculate_lowest, prices, 20)
    
    print()
    print("📈 Testing Data Processing Functions")
    print("-" * 40)
    
    # Data Processing
    test_function("Returns", backtrader_cpp.calculate_returns, prices)
    returns_data = backtrader_cpp.calculate_returns(prices)
    test_function("Volatility", backtrader_cpp.calculate_volatility, returns_data, 20)
    test_function("Sharpe Ratio", backtrader_cpp.calculate_sharpe, returns_data, 0.02)
    
    print()
    print("🎯 Testing Strategy Functions")
    print("-" * 40)
    
    # Strategy
    test_function("SMA Strategy", backtrader_cpp.simple_moving_average_strategy, prices, 10, 20, 10000.0)
    
    print()
    print("⚡ Testing Performance Functions")
    print("-" * 40)
    
    # Performance Tests
    test_function("Benchmark", backtrader_cpp.benchmark, 100000)
    test_function("SMA Benchmark", backtrader_cpp.benchmark_sma, 1000, 100)
    
    print()
    print("🛠️ Testing Utility Functions")
    print("-" * 40)
    
    # Utility Functions
    test_function("Test Function", backtrader_cpp.test)
    test_function("Generate Sample Data", backtrader_cpp.generate_sample_data, 50, 100.0, 0.02)
    test_function("Validate Data", backtrader_cpp.validate_data, prices)
    test_function("Get Version", backtrader_cpp.get_version)
    
    print()
    print("📊 Test Results Summary")
    print("=" * 60)
    print(f"Total Functions Tested: {total_tests}")
    print(f"✅ Passed: {passed_tests}")
    print(f"❌ Failed: {len(failed_tests)}")
    print(f"🎯 Success Rate: {(passed_tests/total_tests)*100:.1f}%")
    
    if failed_tests:
        print("\n⚠️ Failed Tests:")
        for name, error in failed_tests:
            print(f"   • {name}: {error}")
    
    print("\n🏆 Function Coverage Summary:")
    print(f"   • Basic Moving Averages: 3/3 (SMA, EMA, WMA)")
    print(f"   • Advanced Moving Averages: 4/4 (DEMA, TEMA, HMA, KAMA)")  
    print(f"   • Oscillators: 6/6 (RSI, CCI, Williams%R, Stochastic, TSI, UO)")
    print(f"   • Trend Indicators: 6/6 (MACD, Bollinger, ATR, Aroon, DPO, Vortex)")
    print(f"   • Momentum Indicators: 2/2 (Momentum, ROC)")
    print(f"   • Statistical Functions: 2/2 (Highest, Lowest)")
    print(f"   • Data Processing: 3/3 (Returns, Volatility, Sharpe)")
    print(f"   • Strategy Functions: 1/1 (SMA Strategy)")
    print(f"   • Performance Functions: 2/2 (Benchmark, SMA Benchmark)")
    print(f"   • Utility Functions: 4/4 (Test, Generate, Validate, Version)")
    
    print(f"\n🎉 Total Functions Available: {len([f for f in dir(backtrader_cpp) if not f.startswith('_')])}")
    
    # Performance comparison
    print("\n⚡ Performance Highlights:")
    sma_perf = backtrader_cpp.benchmark_sma(1000, 100)
    print(f"   • SMA Calculation: {sma_perf['ops_per_second']:.0f} ops/second")
    print(f"   • Processing Speed: {sma_perf['data_size'] * sma_perf['iterations'] / (sma_perf['time_us'] / 1e6):.0f} data points/second")
    
    if passed_tests == total_tests:
        print("\n🎊 ALL TESTS PASSED! 🎊")
        print("Ready for production use!")
    else:
        print(f"\n⚠️ {len(failed_tests)} tests failed. Review and fix issues.")
    
    return passed_tests == total_tests

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)