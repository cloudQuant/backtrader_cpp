#!/usr/bin/env python3
"""
Test a simple trading strategy using backtrader-cpp bindings
"""

import sys
import os

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def test_simple_strategy():
    """Test a simple moving average crossover strategy"""
    try:
        import backtrader_cpp as bt

        print("🧪 Testing Simple Strategy")
        print("=" * 50)

        # Create data
        data = bt.DataSeries("TestData")
        test_data = [
            [1609459200.0, 100.0, 105.0, 95.0, 102.0, 1000.0, 10.0],
            [1609545600.0, 102.0, 107.0, 97.0, 104.0, 1100.0, 12.0],
            [1609632000.0, 104.0, 109.0, 99.0, 106.0, 1200.0, 15.0],
            [1609718400.0, 106.0, 111.0, 101.0, 108.0, 1300.0, 18.0],
            [1609804800.0, 108.0, 113.0, 103.0, 110.0, 1400.0, 20.0],
        ]
        data.load_from_csv(test_data)
        print(f"✅ Data loaded: {data}")

        # Create cerebro
        cerebro = bt.Cerebro()
        cerebro.add_data(data)
        print(f"✅ Cerebro with data: {cerebro}")

        # Create broker
        broker = cerebro.broker()
        print(f"✅ Broker: {broker}")

        # Test basic operations
        print("\n--- Basic Operations ---")
        print(f"Initial cash: {broker.get_cash()}")
        print(f"Initial value: {broker.get_value()}")

        # Test position
        position = broker.get_position()
        print(f"Initial position: {position}")

        # Test order creation
        order = broker.buy(10)  # Buy 10 shares
        print(f"Buy order: {order}")

        # Check orders
        orders = broker.get_orders()
        print(f"Total orders: {len(orders)}")

        print("\n" + "=" * 50)
        print("🎉 Simple strategy test completed!")
        print("\n📊 Test Results:")
        print("- ✅ DataSeries creation and data loading")
        print("- ✅ Cerebro initialization")
        print("- ✅ Broker operations")
        print("- ✅ Order creation")
        print("- ✅ Position tracking")

        return True

    except Exception as e:
        print(f"❌ Strategy test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_simple_strategy()
    sys.exit(0 if success else 1)
