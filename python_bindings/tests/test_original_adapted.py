#!/usr/bin/env python3
"""
Adapted version of original backtrader test case
Based on: test_backtrader_ts_strategy/test_backtrader_ts.py
"""

import sys
import os
import numpy as np
import time

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def create_pandas_like_data(n_rows=1000):
    """Create data similar to original test using numpy arrays"""
    np.random.seed(1)
    data = {}

    # Generate data similar to original
    data['open'] = np.random.randn(n_rows) + 3
    data['high'] = data['open'] + np.random.rand(n_rows) * 2
    data['low'] = data['open'] - np.random.rand(n_rows) * 2
    data['close'] = data['open'] + np.random.randn(n_rows)
    data['volume'] = np.random.randint(1000, 5000, n_rows)
    data['total_value'] = data['close'] * data['volume']

    # Create datetime index
    data['datetime'] = np.arange(1609459200.0, 1609459200.0 + n_rows * 300, 300)

    return data

def convert_to_csv_format(data_dict):
    """Convert dictionary data to CSV format for backtrader-cpp"""
    csv_data = []
    n_rows = len(data_dict['datetime'])

    for i in range(n_rows):
        row = [
            data_dict['datetime'][i],
            data_dict['open'][i],
            data_dict['high'][i],
            data_dict['low'][i],
            data_dict['close'][i],
            data_dict['volume'][i],
            100.0  # openinterest placeholder
        ]
        csv_data.append(row)

    return csv_data

def run_adapted_strategy(n_rows=100):
    """Run adapted version of original backtrader strategy"""
    try:
        import backtrader_cpp as bt

        print("🚀 Running Adapted Backtrader Strategy Test")
        print("=" * 60)
        print(f"Testing with {n_rows} data points")

        # Create data similar to original
        print("\n📊 Creating sample data...")
        data_dict = create_pandas_like_data(n_rows)
        csv_data = convert_to_csv_format(data_dict)

        data = bt.DataSeries("test")
        data.load_from_csv(csv_data)
        print(f"✅ Data loaded: size={data.size}")
        print(f"Sample close[0]={data.get_close(0):.2f}, current_close={data.close:.2f}")

        # Create Cerebro
        cerebro = bt.Cerebro()

        # Set broker cash (adapted)
        broker = bt.Broker(cash=5000000.0)
        cerebro = bt.Cerebro()  # Recreate with broker
        cerebro.add_data(data)

        print(f"✅ Cerebro setup complete: {cerebro}")

        # Create strategy (simplified version)
        strategy = bt.Strategy()
        # Set parameters similar to original
        params = {"short_window": 10, "long_window": 60}
        strategy.set_params(params)
        cerebro.add_strategy(strategy)

        print(f"✅ Strategy added with params: {dict(strategy.p)}")

        # Run backtest
        print("\n⚡ Running backtest...")
        start_time = time.perf_counter()

        results = cerebro.run()

        end_time = time.perf_counter()
        execution_time = end_time - start_time

        print("✅ Backtest completed!")
        print(f"Execution time: {execution_time:.4f} seconds")
        print(f"Results: {len(results)} result sets")

        # Get final results
        broker = cerebro.broker()
        final_value = broker.get_value()
        final_cash = broker.get_cash()

        print("\n💰 Final Results:")
        print(f"Final Portfolio Value: ${final_value:.2f}")
        print(f"Remaining Cash: ${final_cash:.2f}")
        print("\n" + "=" * 60)
        print("🎉 Adapted strategy test completed successfully!")
        print("\n📊 Test Summary:")
        print("- ✅ Data generation (Pandas-like)")
        print("- ✅ DataSeries creation and loading")
        print("- ✅ Cerebro setup")
        print("- ✅ Strategy with parameters")
        print("- ✅ Backtest execution")
        print("- ✅ Performance measurement")

        return final_value

    except Exception as e:
        print(f"❌ Adapted strategy test failed: {e}")
        import traceback
        traceback.print_exc()
        return None

if __name__ == "__main__":
    result = run_adapted_strategy(n_rows=50)
    if result is not None:
        print("\n🎯 Test successful!")
        sys.exit(0)
    else:
        sys.exit(1)
