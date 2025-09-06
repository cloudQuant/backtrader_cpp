#!/usr/bin/env python3
"""
Comprehensive validation test suite for Python bindings
Compares outputs with original Backtrader implementation
"""

import sys
import os
import numpy as np

# Add build directory to path for our C++ bindings
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))
import backtrader_cpp

# Import original backtrader for comparison
sys.path.insert(0, '/home/yun/Documents/refactor_backtrader')
import backtrader as bt
import backtrader.indicators as btind

def generate_test_data(size=100, seed=42):
    """Generate consistent test data for both implementations"""
    np.random.seed(seed)
    
    # Generate realistic OHLCV data
    opens = []
    highs = []
    lows = []
    closes = []
    volumes = []
    
    base_price = 100.0
    for i in range(size):
        # Generate daily movement
        daily_return = np.random.normal(0.001, 0.02)
        open_price = base_price * (1 + np.random.normal(0, 0.005))
        close_price = base_price * (1 + daily_return)
        
        # High and low based on volatility
        volatility = abs(daily_return) + 0.01
        high_price = max(open_price, close_price) * (1 + np.random.uniform(0, volatility))
        low_price = min(open_price, close_price) * (1 - np.random.uniform(0, volatility))
        
        volume = 1000000 * (1 + np.random.uniform(-0.5, 1.5))
        
        opens.append(open_price)
        highs.append(high_price)
        lows.append(low_price)
        closes.append(close_price)
        volumes.append(volume)
        
        base_price = close_price
    
    return {
        'open': opens,
        'high': highs,
        'low': lows,
        'close': closes,
        'volume': volumes
    }

class BacktraderDataWrapper:
    """Wrapper to create Backtrader data feed from arrays"""
    def __init__(self, data_dict):
        import pandas as pd
        import datetime
        
        # Create pandas dataframe
        dates = pd.date_range(start='2020-01-01', periods=len(data_dict['close']), freq='D')
        df = pd.DataFrame({
            'datetime': dates,
            'open': data_dict['open'],
            'high': data_dict['high'],
            'low': data_dict['low'],
            'close': data_dict['close'],
            'volume': data_dict['volume'],
            'openinterest': [0] * len(data_dict['close'])
        })
        df.set_index('datetime', inplace=True)
        
        # Create Backtrader data feed
        self.data = bt.feeds.PandasData(dataname=df)

def compare_results(cpp_result, bt_result, indicator_name, tolerance=1e-6):
    """Compare results from C++ bindings and original Backtrader"""
    # Convert to numpy arrays for comparison
    if isinstance(cpp_result, list):
        cpp_array = np.array(cpp_result)
    else:
        cpp_array = cpp_result
    
    if hasattr(bt_result, 'array'):
        bt_array = np.array(bt_result.array)
    elif hasattr(bt_result, 'lines'):
        # Get the first line (main indicator output)
        bt_array = np.array(bt_result.lines[0].array)
    else:
        bt_array = np.array(bt_result)
    
    # Remove NaN values for comparison
    cpp_valid = cpp_array[~np.isnan(cpp_array)]
    bt_valid = bt_array[~np.isnan(bt_array)]
    
    # Ensure same length (might have different startup periods)
    min_len = min(len(cpp_valid), len(bt_valid))
    if min_len > 0:
        cpp_valid = cpp_valid[-min_len:]
        bt_valid = bt_valid[-min_len:]
        
        # Calculate metrics
        max_diff = np.max(np.abs(cpp_valid - bt_valid))
        mean_diff = np.mean(np.abs(cpp_valid - bt_valid))
        correlation = np.corrcoef(cpp_valid, bt_valid)[0, 1] if len(cpp_valid) > 1 else 1.0
        
        passed = max_diff < tolerance
        
        print(f"\n{indicator_name} Comparison:")
        print(f"  Max Difference: {max_diff:.10f}")
        print(f"  Mean Difference: {mean_diff:.10f}")
        print(f"  Correlation: {correlation:.6f}")
        print(f"  Status: {'✅ PASS' if passed else '❌ FAIL'}")
        
        if not passed and min_len > 0:
            print(f"  Sample CPP values: {cpp_valid[-5:].round(6)}")
            print(f"  Sample BT values:  {bt_valid[-5:].round(6)}")
        
        return passed
    else:
        print(f"\n{indicator_name}: No valid data to compare")
        return False

def test_sma():
    """Test Simple Moving Average"""
    print("\n" + "="*60)
    print("Testing SMA (Simple Moving Average)")
    print("="*60)
    
    # Generate test data
    data = generate_test_data(100)
    closes = data['close']
    period = 20
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_sma(closes, period)
    
    # Original Backtrader calculation
    class SMAStrategy(bt.Strategy):
        def __init__(self):
            self.sma = btind.SMA(self.data.close, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.sma[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(SMAStrategy)
    strategies = cerebro.run()
    bt_result = strategies[0].result
    
    # Pad with NaN for initial period
    bt_result = [np.nan] * (period - 1) + bt_result
    
    return compare_results(cpp_result, bt_result, "SMA")

def test_ema():
    """Test Exponential Moving Average"""
    print("\n" + "="*60)
    print("Testing EMA (Exponential Moving Average)")
    print("="*60)
    
    data = generate_test_data(100)
    closes = data['close']
    period = 20
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_ema(closes, period)
    
    # Original Backtrader calculation
    class EMAStrategy(bt.Strategy):
        def __init__(self):
            self.ema = btind.EMA(self.data.close, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.ema[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(EMAStrategy)
    strategies = cerebro.run()
    bt_result = strategies[0].result
    
    # Pad with NaN for initial period
    bt_result = [np.nan] * (period - 1) + bt_result
    
    return compare_results(cpp_result, bt_result, "EMA", tolerance=1e-4)

def test_rsi():
    """Test Relative Strength Index"""
    print("\n" + "="*60)
    print("Testing RSI (Relative Strength Index)")
    print("="*60)
    
    data = generate_test_data(100)
    closes = data['close']
    period = 14
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_rsi(closes, period)
    
    # Original Backtrader calculation  
    class RSIStrategy(bt.Strategy):
        def __init__(self):
            self.rsi = btind.RSI(self.data.close, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.rsi[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(RSIStrategy)
    strategies = cerebro.run()
    bt_result = strategies[0].result
    
    # Pad with NaN for initial period
    bt_result = [np.nan] * (period) + bt_result
    
    return compare_results(cpp_result, bt_result, "RSI", tolerance=1e-2)

def test_macd():
    """Test MACD"""
    print("\n" + "="*60)
    print("Testing MACD (Moving Average Convergence Divergence)")
    print("="*60)
    
    data = generate_test_data(150)
    closes = data['close']
    fast = 12
    slow = 26
    signal = 9
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_macd(closes, fast, slow, signal)
    
    # Original Backtrader calculation
    class MACDStrategy(bt.Strategy):
        def __init__(self):
            self.macd = btind.MACD(self.data.close, period_me1=fast, period_me2=slow, period_signal=signal)
            self.macd_line = []
            self.signal_line = []
            self.histogram = []
        
        def next(self):
            self.macd_line.append(self.macd.macd[0])
            self.signal_line.append(self.macd.signal[0])
            self.histogram.append(self.macd.histo[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(MACDStrategy)
    strategies = cerebro.run()
    
    bt_macd = strategies[0].macd_line
    bt_signal = strategies[0].signal_line
    bt_histogram = strategies[0].histogram
    
    # Pad with NaN
    pad_size = slow + signal - 2
    bt_macd = [np.nan] * pad_size + bt_macd
    bt_signal = [np.nan] * pad_size + bt_signal
    bt_histogram = [np.nan] * pad_size + bt_histogram
    
    passed = True
    if 'macd' in cpp_result:
        passed &= compare_results(cpp_result['macd'], bt_macd, "MACD Line", tolerance=1e-3)
    if 'signal' in cpp_result:
        passed &= compare_results(cpp_result['signal'], bt_signal, "MACD Signal", tolerance=1e-3)
    if 'histogram' in cpp_result:
        passed &= compare_results(cpp_result['histogram'], bt_histogram, "MACD Histogram", tolerance=1e-3)
    
    return passed

def test_bollinger_bands():
    """Test Bollinger Bands"""
    print("\n" + "="*60)
    print("Testing Bollinger Bands")
    print("="*60)
    
    data = generate_test_data(100)
    closes = data['close']
    period = 20
    devfactor = 2.0
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_bollinger(closes, period, devfactor)
    
    # Original Backtrader calculation
    class BBStrategy(bt.Strategy):
        def __init__(self):
            self.bb = btind.BollingerBands(self.data.close, period=period, devfactor=devfactor)
            self.upper = []
            self.middle = []
            self.lower = []
        
        def next(self):
            self.upper.append(self.bb.top[0])
            self.middle.append(self.bb.mid[0])
            self.lower.append(self.bb.bot[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(BBStrategy)
    strategies = cerebro.run()
    
    bt_upper = [np.nan] * (period - 1) + strategies[0].upper
    bt_middle = [np.nan] * (period - 1) + strategies[0].middle
    bt_lower = [np.nan] * (period - 1) + strategies[0].lower
    
    passed = True
    if 'upper' in cpp_result:
        passed &= compare_results(cpp_result['upper'], bt_upper, "BB Upper", tolerance=1e-4)
    if 'middle' in cpp_result:
        passed &= compare_results(cpp_result['middle'], bt_middle, "BB Middle", tolerance=1e-4)
    if 'lower' in cpp_result:
        passed &= compare_results(cpp_result['lower'], bt_lower, "BB Lower", tolerance=1e-4)
    
    return passed

def test_atr():
    """Test Average True Range"""
    print("\n" + "="*60)
    print("Testing ATR (Average True Range)")
    print("="*60)
    
    data = generate_test_data(100)
    period = 14
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_atr(data['high'], data['low'], data['close'], period)
    
    # Original Backtrader calculation
    class ATRStrategy(bt.Strategy):
        def __init__(self):
            self.atr = btind.ATR(self.data, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.atr[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(ATRStrategy)
    strategies = cerebro.run()
    bt_result = strategies[0].result
    
    # Pad with NaN
    bt_result = [np.nan] * (period) + bt_result
    
    return compare_results(cpp_result, bt_result, "ATR", tolerance=1e-4)

def test_stochastic():
    """Test Stochastic Oscillator"""
    print("\n" + "="*60)
    print("Testing Stochastic Oscillator")
    print("="*60)
    
    data = generate_test_data(100)
    period = 14
    period_dfast = 3
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_stochastic(
        data['high'], data['low'], data['close'], 
        period, period_dfast
    )
    
    # Original Backtrader calculation
    class StochStrategy(bt.Strategy):
        def __init__(self):
            self.stoch = btind.Stochastic(self.data, period=period, period_dfast=period_dfast)
            self.k = []
            self.d = []
        
        def next(self):
            self.k.append(self.stoch.percK[0])
            self.d.append(self.stoch.percD[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(StochStrategy)
    strategies = cerebro.run()
    
    bt_k = [np.nan] * (period + period_dfast - 2) + strategies[0].k
    bt_d = [np.nan] * (period + period_dfast - 2) + strategies[0].d
    
    passed = True
    if 'k' in cpp_result:
        passed &= compare_results(cpp_result['k'], bt_k, "Stochastic %K", tolerance=1e-3)
    if 'd' in cpp_result:
        passed &= compare_results(cpp_result['d'], bt_d, "Stochastic %D", tolerance=1e-3)
    
    return passed

def test_cci():
    """Test Commodity Channel Index"""
    print("\n" + "="*60)
    print("Testing CCI (Commodity Channel Index)")
    print("="*60)
    
    data = generate_test_data(100)
    period = 20
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_cci(
        data['high'], data['low'], data['close'], period
    )
    
    # Original Backtrader calculation
    class CCIStrategy(bt.Strategy):
        def __init__(self):
            self.cci = btind.CCI(self.data, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.cci[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(CCIStrategy)
    strategies = cerebro.run()
    bt_result = strategies[0].result
    
    # Pad with NaN
    bt_result = [np.nan] * (period - 1) + bt_result
    
    return compare_results(cpp_result, bt_result, "CCI", tolerance=1e-2)

def test_williams_r():
    """Test Williams %R"""
    print("\n" + "="*60)
    print("Testing Williams %R")
    print("="*60)
    
    data = generate_test_data(100)
    period = 14
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_williamsr(
        data['high'], data['low'], data['close'], period
    )
    
    # Original Backtrader calculation
    class WilliamsRStrategy(bt.Strategy):
        def __init__(self):
            self.wr = btind.WilliamsR(self.data, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.wr[0])
    
    cerebro = bt.Cerebro()
    wrapper = BacktraderDataWrapper(data)
    cerebro.adddata(wrapper.data)
    cerebro.addstrategy(WilliamsRStrategy)
    strategies = cerebro.run()
    bt_result = strategies[0].result
    
    # Pad with NaN
    bt_result = [np.nan] * (period - 1) + bt_result
    
    return compare_results(cpp_result, bt_result, "Williams %R", tolerance=1e-3)

def run_all_tests():
    """Run all validation tests"""
    print("\n" + "🔬"*30)
    print("COMPREHENSIVE VALIDATION TEST SUITE")
    print("Comparing C++ Python Bindings with Original Backtrader")
    print("🔬"*30)
    
    test_results = []
    
    # Test each indicator
    tests = [
        ("SMA", test_sma),
        ("EMA", test_ema),
        ("RSI", test_rsi),
        ("MACD", test_macd),
        ("Bollinger Bands", test_bollinger_bands),
        ("ATR", test_atr),
        ("Stochastic", test_stochastic),
        ("CCI", test_cci),
        ("Williams %R", test_williams_r),
    ]
    
    for name, test_func in tests:
        try:
            result = test_func()
            test_results.append((name, result))
        except Exception as e:
            print(f"\n❌ {name} test failed with error: {str(e)}")
            test_results.append((name, False))
    
    # Summary
    print("\n" + "="*60)
    print("VALIDATION SUMMARY")
    print("="*60)
    
    passed = sum(1 for _, result in test_results if result)
    total = len(test_results)
    
    for name, result in test_results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{name:20s}: {status}")
    
    print(f"\nTotal: {passed}/{total} tests passed ({passed/total*100:.1f}%)")
    
    if passed == total:
        print("\n🎉 ALL TESTS PASSED! Python bindings are compatible with Backtrader!")
    else:
        print(f"\n⚠️ {total - passed} tests failed. Review the differences above.")
    
    return passed == total

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)