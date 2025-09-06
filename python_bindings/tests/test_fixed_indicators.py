#!/usr/bin/env python3
"""
Test the fixed indicator implementations to verify they match Backtrader
"""

import sys
import os
import numpy as np

# Add build directory to path for our C++ bindings
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))
import fixed_indicators
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

def compare_results(fixed_result, original_result, bt_result, indicator_name, tolerance=1e-6):
    """Compare results from fixed indicators, original C++ bindings, and Backtrader"""
    # Convert to numpy arrays for comparison
    if isinstance(fixed_result, list):
        fixed_array = np.array(fixed_result)
    else:
        fixed_array = fixed_result
    
    if isinstance(original_result, list):
        original_array = np.array(original_result)
    else:
        original_array = original_result
    
    if hasattr(bt_result, 'array'):
        bt_array = np.array(bt_result.array)
    elif hasattr(bt_result, 'lines'):
        bt_array = np.array(bt_result.lines[0].array)
    else:
        bt_array = np.array(bt_result)
    
    # Remove NaN values for comparison
    fixed_valid = fixed_array[~np.isnan(fixed_array)]
    original_valid = original_array[~np.isnan(original_array)]
    bt_valid = bt_array[~np.isnan(bt_array)]
    
    # Ensure same length
    min_len = min(len(fixed_valid), len(original_valid), len(bt_valid))
    if min_len > 0:
        fixed_valid = fixed_valid[-min_len:]
        original_valid = original_valid[-min_len:]
        bt_valid = bt_valid[-min_len:]
        
        # Calculate metrics
        fixed_vs_bt_diff = np.max(np.abs(fixed_valid - bt_valid))
        original_vs_bt_diff = np.max(np.abs(original_valid - bt_valid))
        
        fixed_passed = fixed_vs_bt_diff < tolerance
        original_passed = original_vs_bt_diff < tolerance
        
        print(f"\n{indicator_name} Comparison:")
        print(f"  Fixed vs Backtrader Max Diff: {fixed_vs_bt_diff:.10f} ({'✅ PASS' if fixed_passed else '❌ FAIL'})")
        print(f"  Original vs Backtrader Max Diff: {original_vs_bt_diff:.10f} ({'✅ PASS' if original_passed else '❌ FAIL'})")
        print(f"  Improvement: {'✅ YES' if fixed_vs_bt_diff < original_vs_bt_diff else '❌ NO'}")
        
        if not fixed_passed:
            print(f"  Sample Fixed values: {fixed_valid[-5:].round(6)}")
            print(f"  Sample BT values:    {bt_valid[-5:].round(6)}")
        
        return fixed_passed, original_passed
    else:
        print(f"\n{indicator_name}: No valid data to compare")
        return False, False

def test_rsi_comparison():
    """Test RSI with both implementations"""
    print("\n" + "="*60)
    print("Testing RSI (Fixed vs Original vs Backtrader)")
    print("="*60)
    
    data = generate_test_data(100)
    closes = data['close']
    period = 14
    
    # Fixed implementation
    fixed_result = fixed_indicators.calculate_rsi(closes, period)
    
    # Original implementation
    original_result = backtrader_cpp.calculate_rsi(closes, period)
    
    # Backtrader calculation
    import pandas as pd
    dates = pd.date_range(start='2020-01-01', periods=len(closes), freq='D')
    df = pd.DataFrame({
        'datetime': dates,
        'open': closes,
        'high': closes,
        'low': closes,
        'close': closes,
        'volume': [1000000] * len(closes),
        'openinterest': [0] * len(closes)
    })
    df.set_index('datetime', inplace=True)
    
    class RSIStrategy(bt.Strategy):
        def __init__(self):
            self.rsi = btind.RSI(self.data.close, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.rsi[0])
    
    cerebro = bt.Cerebro()
    data = bt.feeds.PandasData(dataname=df)
    cerebro.adddata(data)
    cerebro.addstrategy(RSIStrategy)
    strategies = cerebro.run()
    bt_result = [np.nan] * period + strategies[0].result
    
    return compare_results(fixed_result, original_result, bt_result, "RSI", tolerance=1e-2)

def test_ema_comparison():
    """Test EMA with both implementations"""
    print("\n" + "="*60)
    print("Testing EMA (Fixed vs Original vs Backtrader)")
    print("="*60)
    
    data = generate_test_data(100)
    closes = data['close']
    period = 20
    
    # Fixed implementation
    fixed_result = fixed_indicators.calculate_ema(closes, period)
    
    # Original implementation
    original_result = backtrader_cpp.calculate_ema(closes, period)
    
    # Backtrader calculation
    import pandas as pd
    dates = pd.date_range(start='2020-01-01', periods=len(closes), freq='D')
    df = pd.DataFrame({
        'datetime': dates,
        'open': closes,
        'high': closes,
        'low': closes,
        'close': closes,
        'volume': [1000000] * len(closes),
        'openinterest': [0] * len(closes)
    })
    df.set_index('datetime', inplace=True)
    
    class EMAStrategy(bt.Strategy):
        def __init__(self):
            self.ema = btind.EMA(self.data.close, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.ema[0])
    
    cerebro = bt.Cerebro()
    data = bt.feeds.PandasData(dataname=df)
    cerebro.adddata(data)
    cerebro.addstrategy(EMAStrategy)
    strategies = cerebro.run()
    bt_result = [np.nan] * (period - 1) + strategies[0].result
    
    return compare_results(fixed_result, original_result, bt_result, "EMA", tolerance=1e-4)

def test_atr_comparison():
    """Test ATR with both implementations"""
    print("\n" + "="*60)
    print("Testing ATR (Fixed vs Original vs Backtrader)")
    print("="*60)
    
    data = generate_test_data(100)
    period = 14
    
    # Fixed implementation
    fixed_result = fixed_indicators.calculate_atr(data['high'], data['low'], data['close'], period)
    
    # Original implementation
    original_result = backtrader_cpp.calculate_atr(data['high'], data['low'], data['close'], period)
    
    # Backtrader calculation
    import pandas as pd
    dates = pd.date_range(start='2020-01-01', periods=len(data['close']), freq='D')
    df = pd.DataFrame({
        'datetime': dates,
        'open': data['open'],
        'high': data['high'],
        'low': data['low'],
        'close': data['close'],
        'volume': data['volume'],
        'openinterest': [0] * len(data['close'])
    })
    df.set_index('datetime', inplace=True)
    
    class ATRStrategy(bt.Strategy):
        def __init__(self):
            self.atr = btind.ATR(self.data, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.atr[0])
    
    cerebro = bt.Cerebro()
    data = bt.feeds.PandasData(dataname=df)
    cerebro.adddata(data)
    cerebro.addstrategy(ATRStrategy)
    strategies = cerebro.run()
    bt_result = [np.nan] * period + strategies[0].result
    
    return compare_results(fixed_result, original_result, bt_result, "ATR", tolerance=1e-4)

def test_cci_comparison():
    """Test CCI with both implementations"""
    print("\n" + "="*60)
    print("Testing CCI (Fixed vs Original vs Backtrader)")
    print("="*60)
    
    data = generate_test_data(100)
    period = 20
    
    # Fixed implementation
    fixed_result = fixed_indicators.calculate_cci(data['high'], data['low'], data['close'], period)
    
    # Original implementation
    original_result = backtrader_cpp.calculate_cci(data['high'], data['low'], data['close'], period)
    
    # Backtrader calculation
    import pandas as pd
    dates = pd.date_range(start='2020-01-01', periods=len(data['close']), freq='D')
    df = pd.DataFrame({
        'datetime': dates,
        'open': data['open'],
        'high': data['high'],
        'low': data['low'],
        'close': data['close'],
        'volume': data['volume'],
        'openinterest': [0] * len(data['close'])
    })
    df.set_index('datetime', inplace=True)
    
    class CCIStrategy(bt.Strategy):
        def __init__(self):
            self.cci = btind.CCI(self.data, period=period)
            self.result = []
        
        def next(self):
            self.result.append(self.cci[0])
    
    cerebro = bt.Cerebro()
    data = bt.feeds.PandasData(dataname=df)
    cerebro.adddata(data)
    cerebro.addstrategy(CCIStrategy)
    strategies = cerebro.run()
    bt_result = [np.nan] * (period - 1) + strategies[0].result
    
    return compare_results(fixed_result, original_result, bt_result, "CCI", tolerance=1e-2)

def test_stochastic_comparison():
    """Test Stochastic with both implementations"""
    print("\n" + "="*60)
    print("Testing Stochastic (Fixed vs Original vs Backtrader)")
    print("="*60)
    
    data = generate_test_data(100)
    period = 14
    period_dfast = 3
    
    # Fixed implementation
    fixed_result = fixed_indicators.calculate_stochastic(data['high'], data['low'], data['close'], period, period_dfast)
    
    # Original implementation
    original_result = backtrader_cpp.calculate_stochastic(data['high'], data['low'], data['close'], period, period_dfast)
    
    # Backtrader calculation
    import pandas as pd
    dates = pd.date_range(start='2020-01-01', periods=len(data['close']), freq='D')
    df = pd.DataFrame({
        'datetime': dates,
        'open': data['open'],
        'high': data['high'],
        'low': data['low'],
        'close': data['close'],
        'volume': data['volume'],
        'openinterest': [0] * len(data['close'])
    })
    df.set_index('datetime', inplace=True)
    
    class StochStrategy(bt.Strategy):
        def __init__(self):
            self.stoch = btind.Stochastic(self.data, period=period, period_dfast=period_dfast)
            self.k = []
            self.d = []
        
        def next(self):
            self.k.append(self.stoch.percK[0])
            self.d.append(self.stoch.percD[0])
    
    cerebro = bt.Cerebro()
    data = bt.feeds.PandasData(dataname=df)
    cerebro.adddata(data)
    cerebro.addstrategy(StochStrategy)
    strategies = cerebro.run()
    
    bt_k = [np.nan] * (period + period_dfast - 2) + strategies[0].k
    bt_d = [np.nan] * (period + period_dfast - 2) + strategies[0].d
    
    k_fixed_passed, k_original_passed = compare_results(fixed_result['k'], original_result['k'], bt_k, "Stochastic %K", tolerance=1e-3)
    d_fixed_passed, d_original_passed = compare_results(fixed_result['d'], original_result['d'], bt_d, "Stochastic %D", tolerance=1e-3)
    
    return (k_fixed_passed and d_fixed_passed), (k_original_passed and d_original_passed)

def run_comparison_tests():
    """Run all comparison tests"""
    print("\n" + "🔬"*30)
    print("FIXED INDICATORS VALIDATION")
    print("Comparing Fixed vs Original vs Backtrader")
    print("🔬"*30)
    
    tests = [
        ("RSI", test_rsi_comparison),
        ("EMA", test_ema_comparison),
        ("ATR", test_atr_comparison),
        ("CCI", test_cci_comparison),
        ("Stochastic", test_stochastic_comparison),
    ]
    
    fixed_results = []
    original_results = []
    
    for name, test_func in tests:
        try:
            fixed_passed, original_passed = test_func()
            fixed_results.append((name, fixed_passed))
            original_results.append((name, original_passed))
        except Exception as e:
            print(f"\n❌ {name} test failed with error: {str(e)}")
            fixed_results.append((name, False))
            original_results.append((name, False))
    
    # Summary
    print("\n" + "="*60)
    print("COMPARISON SUMMARY")
    print("="*60)
    
    fixed_passed = sum(1 for _, result in fixed_results if result)
    original_passed = sum(1 for _, result in original_results if result)
    total = len(fixed_results)
    
    print("Fixed Indicators Results:")
    for name, result in fixed_results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"  {name:15s}: {status}")
    
    print("\nOriginal Indicators Results:")
    for name, result in original_results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"  {name:15s}: {status}")
    
    print(f"\nFixed Implementation: {fixed_passed}/{total} tests passed ({fixed_passed/total*100:.1f}%)")
    print(f"Original Implementation: {original_passed}/{total} tests passed ({original_passed/total*100:.1f}%)")
    
    improvement = fixed_passed - original_passed
    if improvement > 0:
        print(f"\n🎉 IMPROVEMENT: +{improvement} tests now passing with fixed indicators!")
    elif improvement == 0:
        print(f"\n✅ Same performance - both implementations equally accurate")
    else:
        print(f"\n⚠️ Regression: -{abs(improvement)} fewer tests passing")
    
    return fixed_passed > original_passed

if __name__ == "__main__":
    success = run_comparison_tests()
    sys.exit(0 if success else 1)