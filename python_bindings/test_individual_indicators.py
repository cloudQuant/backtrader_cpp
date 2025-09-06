#!/usr/bin/env python3
"""
Test individual indicators to debug differences
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

def test_rsi_detailed():
    """Detailed RSI test to understand the difference"""
    print("Detailed RSI Analysis")
    print("="*60)
    
    # Simple test data
    closes = [44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.10, 45.42,
              45.84, 46.08, 45.89, 46.03, 45.61, 46.28, 46.28, 46.00,
              46.03, 46.41, 46.22, 45.64, 46.21, 46.25, 45.71, 46.45,
              45.78, 45.35, 44.03, 44.18, 44.22, 44.57, 43.42, 42.66, 43.13]
    
    period = 14
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_rsi(closes, period)
    
    # Manual RSI calculation (standard method)
    def calculate_rsi_manual(prices, period=14):
        deltas = np.diff(prices)
        seed = deltas[:period]
        up = seed[seed >= 0].sum() / period
        down = -seed[seed < 0].sum() / period
        rs = up / down if down != 0 else 0
        rsi = np.zeros_like(prices)
        rsi[:period] = np.nan
        rsi[period] = 100. - 100. / (1. + rs)
        
        for i in range(period + 1, len(prices)):
            delta = deltas[i - 1]
            if delta > 0:
                upval = delta
                downval = 0.
            else:
                upval = 0.
                downval = -delta
            
            up = (up * (period - 1) + upval) / period
            down = (down * (period - 1) + downval) / period
            rs = up / down if down != 0 else 0
            rsi[i] = 100. - 100. / (1. + rs)
        
        return rsi
    
    manual_result = calculate_rsi_manual(closes, period)
    
    print("\nResults comparison (last 10 values):")
    print(f"C++ Binding: {cpp_result[-10:]}")
    print(f"Manual Calc: {manual_result[-10:]}")
    
    # Calculate using Backtrader
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
    
    print(f"Backtrader:  {bt_result[-10:]}")

def test_ema_detailed():
    """Detailed EMA test to understand the difference"""
    print("\nDetailed EMA Analysis")
    print("="*60)
    
    # Simple test data
    closes = [22.27, 22.19, 22.08, 22.17, 22.18, 22.13, 22.23, 22.43,
              22.24, 22.29, 22.15, 22.39, 22.38, 22.61, 23.36, 24.05,
              23.75, 23.83, 23.95, 23.63, 23.82, 23.87, 23.65, 23.19,
              23.10, 23.33, 22.68, 23.10, 22.40, 22.17]
    
    period = 10
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_ema(closes, period)
    
    # Manual EMA calculation
    def calculate_ema_manual(prices, period):
        ema = np.zeros_like(prices)
        ema[:period-1] = np.nan
        
        # Start with SMA for first value
        ema[period-1] = np.mean(prices[:period])
        
        # EMA formula: price * k + ema_prev * (1 - k)
        k = 2.0 / (period + 1)
        
        for i in range(period, len(prices)):
            ema[i] = prices[i] * k + ema[i-1] * (1 - k)
        
        return ema
    
    manual_result = calculate_ema_manual(closes, period)
    
    print("\nResults comparison (last 10 values):")
    print(f"C++ Binding: {cpp_result[-10:]}")
    print(f"Manual Calc: {manual_result[-10:]}")
    
    # Backtrader result
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
    
    print(f"Backtrader:  {bt_result[-10:]}")
    
    # Check alpha/multiplier
    print(f"\nAlpha/Multiplier check:")
    print(f"Standard EMA multiplier (2/(n+1)): {2.0/(period+1)}")
    print(f"Backtrader might use: Check source code")

def test_stochastic_detailed():
    """Detailed Stochastic test"""
    print("\nDetailed Stochastic Analysis")  
    print("="*60)
    
    # Test data
    highs = [127.01, 127.62, 126.59, 127.35, 128.17, 128.43, 127.37, 126.42,
             126.90, 126.85, 125.65, 125.72, 127.16, 127.72, 127.69, 128.22,
             128.27, 128.09, 128.27, 127.74, 128.77, 129.29, 130.06, 129.12,
             129.29, 128.47, 128.09, 128.65, 129.14, 128.64]
    
    lows = [125.36, 126.16, 124.93, 126.09, 126.82, 126.48, 126.03, 124.83,
            126.39, 125.72, 124.56, 124.57, 125.07, 126.86, 126.63, 126.80,
            126.71, 126.80, 126.13, 125.92, 126.99, 127.81, 128.47, 128.06,
            127.61, 127.60, 127.00, 126.90, 127.49, 127.40]
    
    closes = [125.36, 126.16, 124.93, 126.09, 126.82, 126.48, 126.03, 124.83,
              126.39, 125.72, 124.56, 124.57, 125.07, 126.86, 126.63, 126.80,
              126.71, 126.80, 126.13, 125.92, 126.99, 127.81, 128.47, 128.06,
              127.61, 127.60, 127.00, 126.90, 127.49, 127.40]
    
    period = 14
    period_dfast = 3
    
    # C++ binding calculation
    cpp_result = backtrader_cpp.calculate_stochastic(highs, lows, closes, period, period_dfast)
    
    # Manual calculation
    def calculate_stochastic_manual(highs, lows, closes, period=14, period_dfast=3):
        k_values = []
        
        for i in range(len(closes)):
            if i < period - 1:
                k_values.append(np.nan)
            else:
                highest = max(highs[i-period+1:i+1])
                lowest = min(lows[i-period+1:i+1])
                
                if highest != lowest:
                    k = 100 * (closes[i] - lowest) / (highest - lowest)
                else:
                    k = 50
                k_values.append(k)
        
        # Calculate %D as SMA of %K
        d_values = []
        for i in range(len(k_values)):
            if i < period + period_dfast - 2:
                d_values.append(np.nan)
            else:
                valid_k = [k for k in k_values[i-period_dfast+1:i+1] if not np.isnan(k)]
                if valid_k:
                    d_values.append(np.mean(valid_k))
                else:
                    d_values.append(np.nan)
        
        return k_values, d_values
    
    manual_k, manual_d = calculate_stochastic_manual(highs, lows, closes, period, period_dfast)
    
    print("\nResults comparison (last 10 values):")
    print(f"C++ %K: {cpp_result['k'][-10:]}")
    print(f"Manual %K: {manual_k[-10:]}")
    print(f"\nC++ %D: {cpp_result['d'][-10:]}")
    print(f"Manual %D: {manual_d[-10:]}")

if __name__ == "__main__":
    test_rsi_detailed()
    test_ema_detailed()
    test_stochastic_detailed()