#!/usr/bin/env python3
"""
Test script for new indicator functions
"""

import sys
import os
import numpy as np

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def test_new_indicators():
    """Test all new indicator functions"""
    try:
        import backtrader_cpp as bt

        print("🧪 Testing New Indicators")
        print("=" * 40)

        # Generate test data
        prices = np.random.uniform(100, 200, 50).tolist()

        # Test SMA
        print("Testing SMA...")
        sma = bt.sma(prices, 10)
        print(f"✅ SMA: length={len(sma)}, last={sma[-1]:.2f}")

        # Test EMA
        print("Testing EMA...")
        ema = bt.ema(prices, 10)
        print(f"✅ EMA: length={len(ema)}, last={ema[-1]:.2f}")

        # Test RSI
        print("Testing RSI...")
        rsi = bt.rsi(prices, 14)
        print(f"✅ RSI: length={len(rsi)}, last={rsi[-1]:.2f}")

        # Test MACD
        print("Testing MACD...")
        macd, signal, hist = bt.macd(prices)
        print(f"✅ MACD: macd={macd[-1]:.2f}, signal={signal[-1]:.2f}, hist={hist[-1]:.2f}")

        # Test Bollinger Bands
        print("Testing Bollinger Bands...")
        upper, middle, lower = bt.bollinger_bands(prices)
        print(f"✅ Bollinger: upper={upper[-1]:.2f}, middle={middle[-1]:.2f}, lower={lower[-1]:.2f}")

        # Test Stochastic
        print("Testing Stochastic...")
        high = np.random.uniform(150, 200, 50).tolist()
        low = np.random.uniform(100, 150, 50).tolist()
        close = np.random.uniform(125, 175, 50).tolist()
        k, d = bt.stochastic(high, low, close)
        print(f"✅ Stochastic: K={k[-1]:.2f}, D={d[-1]:.2f}")

        print("=" * 40)
        print("🎉 All indicator tests passed!")
        return True

    except Exception as e:
        print(f"❌ Indicator test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_new_indicators()
    sys.exit(0 if success else 1)
