#!/usr/bin/env python3
"""
Complete backtrader-compatible test case adapted from original backtrader test
Adapted from: test_backtrader_ts_strategy/test_backtrader_ts.py
"""

import sys
import os
import numpy as np
import time

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def create_test_data(n_rows=50):
    """Create sample OHLCV data similar to original test"""
    np.random.seed(1)
    data = []

    for i in range(n_rows):
        base_price = 100.0 + 0.1 * i + np.random.randn() * 2
        high = base_price * (1.0 + abs(np.random.randn()) * 0.05)
        low = base_price * (1.0 - abs(np.random.randn()) * 0.05)
        open_price = base_price + np.random.randn() * 1
        close_price = base_price + np.random.randn() * 2
        volume = 1000 + np.random.randint(0, 5000)
        openinterest = 100 + np.random.randint(0, 200)

        high = max(high, open_price, close_price)
        low = min(low, open_price, close_price)

        data.append([
            1609459200.0 + i * 300,  # datetime
            open_price,
            high,
            low,
            close_price,
            volume,
            openinterest
        ])

    return data

def test_backtrader_compatibility():
    """Test backtrader-cpp compatibility"""
    try:
        import backtrader_cpp as bt

        print("🚀 Running Backtrader-Compatible Test")
        print("=" * 60)

        # Create sample data
        print("\n📊 Creating sample data...")
        test_data = create_test_data(20)
        data = bt.DataSeries("Test")
        data.load_from_csv(test_data)
        print(f"✅ Data created: size={data.size}")
        print(f"Sample: close[0]={data.get_close(0)}, current_close={data.close}")

        # Test SMA indicator
        print("\n📈 Testing SMA indicator...")
        sma = bt.indicators.SMA(period=10)
        print(f"✅ SMA created: {sma}")

        # Create strategy
        print("\n📋 Creating strategy...")
        strategy = bt.Strategy()
        print(f"✅ Strategy: {strategy}")

        # Create cerebro
        print("\n🧠 Setting up Cerebro...")
        cerebro = bt.Cerebro()
        cerebro.add_data(data)
        cerebro.add_strategy(strategy)
        print(f"✅ Cerebro setup: {cerebro}")

        # Run backtest
        print("\n⚡ Running backtest...")
        start_time = time.perf_counter()
        results = cerebro.run()
        end_time = time.perf_counter()

        print("✅ Backtest completed!")
        print(f"Execution time: {end_time - start_time:.4f} seconds")
        print(f"Results: {len(results)} result sets")

        print("\n" + "=" * 60)
        print("🎉 Backtrader-compatible test completed successfully!")
        return True

    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_backtrader_compatibility()
    sys.exit(0 if success else 1)
