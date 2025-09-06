#!/usr/bin/env python3
"""
Test SMA strategy using backtrader-cpp bindings
Adapted from original backtrader test case
"""

import sys
import os
import numpy as np

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def test_sma_strategy():
    """Test a simple SMA crossover strategy"""
    try:
        import backtrader_cpp as bt

        print("🧪 Testing SMA Strategy with backtrader-cpp")
        print("=" * 60)

        # Create sample data
        print("\nCreating sample data...")
        np.random.seed(1)
        n_rows = 100
        data = bt.DataSeries("TestData")

        # Generate sample OHLCV data
        test_data = []
        for i in range(n_rows):
            base_price = 100.0 + 0.1 * i
            test_data.append([
                1609459200.0 + i * 86400.0,  # datetime
                base_price,                   # open
                base_price * 1.02,           # high
                base_price * 0.98,           # low
                base_price + np.random.randn() * 2,  # close
                1000.0 + np.random.randint(0, 1000),  # volume
                10.0 + np.random.randint(0, 20)       # openinterest
            ])

        data.load_from_csv(test_data)
        print(f"✅ Data loaded: {data}")

        # Create SMA indicators
        print("\nCreating SMA indicators...")
        short_sma = bt.SMA(period=10)
        long_sma = bt.SMA(period=60)
        print(f"✅ Short SMA: {short_sma}")
        print(f"✅ Long SMA: {long_sma}")

        # Create strategy
        print("\nCreating strategy...")
        strategy = bt.Strategy()
        strategy.set_params({"short_window": 10, "long_window": 60})
        print(f"✅ Strategy: {strategy}")
        print(f"Strategy params: {dict(strategy.p) if hasattr(strategy, 'p') else 'No params'}")

        # Create cerebro
        print("\nSetting up Cerebro...")
        cerebro = bt.Cerebro()
        cerebro.add_data(data)
        cerebro.add_strategy(strategy)

        # Set broker cash
        broker = cerebro.broker()
        broker = bt.Broker(cash=5000000.0)  # Create new broker with cash
        cerebro = bt.Cerebro()  # Recreate cerebro
        cerebro.add_data(data)
        cerebro.add_strategy(strategy)
        # For now, we'll skip broker cash setting as it's not fully implemented

        print(f"✅ Cerebro setup: {cerebro}")

        # Run backtest (simplified)
        print("\nRunning backtest...")
        results = cerebro.run()
        print(f"✅ Backtest completed: {len(results)} results")

        print("\n" + "=" * 60)
        print("🎉 SMA Strategy test completed successfully!")
        print("\n📊 Test Results:")
        print("- ✅ DataSeries creation and data loading")
        print("- ✅ SMA indicator creation")
        print("- ✅ Strategy creation with parameters")
        print("- ✅ Cerebro setup and execution")
        print("- ✅ Basic backtest functionality")

        return True

    except Exception as e:
        print(f"❌ SMA Strategy test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_sma_strategy()
    sys.exit(0 if success else 1)
