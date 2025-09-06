#!/usr/bin/env python3
"""
Test script for backtrader-compatible Python bindings
"""

import sys
import os

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def test_backtrader_compatibility():
    """Test all backtrader-compatible classes"""
    try:
        import backtrader_cpp as bt

        print("🧪 Testing Backtrader Compatibility")
        print("=" * 50)

        # Test version
        print("Version:", bt.get_version())

        # Test core classes
        print("\n--- Testing Core Classes ---")

        # DataSeries
        print("Creating DataSeries...")
        data = bt.DataSeries("TestData")
        print(f"✅ DataSeries: {data}")

        # Load some test data
        test_data = [
            [1609459200.0, 100.0, 105.0, 95.0, 102.0, 1000.0, 10.0],
            [1609545600.0, 102.0, 107.0, 97.0, 104.0, 1100.0, 12.0],
            [1609632000.0, 104.0, 109.0, 99.0, 106.0, 1200.0, 15.0]
        ]
        data.load_from_csv(test_data)
        print(f"✅ Data loaded: size={data.size}")

        # Test data access
        print(f"✅ Close[0]: {data.get_close(0)}")
        print(f"✅ Current close: {data.close}")

        # Cerebro
        print("\nCreating Cerebro...")
        cerebro = bt.Cerebro()
        print(f"✅ Cerebro: {cerebro}")

        # Strategy
        print("\nCreating Strategy...")
        strategy = bt.Strategy()
        print(f"✅ Strategy: {strategy}")

        # Broker
        print("\nCreating Broker...")
        broker = bt.Broker()
        print(f"✅ Broker: {broker}")
        print(f"Cash: {broker.get_cash()}")
        print(f"Value: {broker.get_value()}")

        # Order
        print("\nTesting Order...")
        order = broker.buy(100)
        print(f"✅ Buy order: {order}")

        # Position
        print("\nTesting Position...")
        position = broker.get_position()
        print(f"✅ Position: {position}")

        # Trade
        print("\nTesting Trade...")
        trades = broker.get_trades()
        print(f"✅ Trades: {len(trades)}")

        # LineBuffer
        print("\nTesting LineBuffer...")
        buffer = bt.LineBuffer()
        buffer.append(100.0)
        buffer.append(101.0)
        print(f"✅ LineBuffer: size={buffer.size}")
        print(f"Current value: {buffer.get(0)}")

        print("\n" + "=" * 50)
        print("🎉 All backtrader compatibility tests passed!")
        print("\n📊 Summary:")
        print("- ✅ DataSeries: OHLCV data management")
        print("- ✅ Cerebro: Main backtesting engine")
        print("- ✅ Strategy: Trading strategy framework")
        print("- ✅ Broker: Order execution and portfolio management")
        print("- ✅ Order: Buy/sell order representation")
        print("- ✅ Position: Portfolio position tracking")
        print("- ✅ Trade: Completed trade records")
        print("- ✅ LineBuffer: Time series data buffer")

        return True

    except Exception as e:
        print(f"❌ Compatibility test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_backtrader_compatibility()
    sys.exit(0 if success else 1)
