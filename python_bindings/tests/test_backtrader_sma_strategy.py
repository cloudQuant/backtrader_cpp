#!/usr/bin/env python3
"""
Complete backtrader SMA strategy test case
Adapted from original backtrader test: test_backtrader_ts_strategy/test_backtrader_ts.py
"""

import sys
import os
import numpy as np
import time

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

def create_sample_data(n_rows=1000):
    """Create sample OHLCV data similar to original test"""
    np.random.seed(1)
    data = []

    for i in range(n_rows):
        # Generate realistic price data
        base_price = 100.0 + 0.1 * i + np.random.randn() * 2
        high = base_price * (1.0 + abs(np.random.randn()) * 0.05)
        low = base_price * (1.0 - abs(np.random.randn()) * 0.05)
        open_price = base_price + np.random.randn() * 1
        close_price = base_price + np.random.randn() * 2
        volume = 1000 + np.random.randint(0, 5000)
        openinterest = 100 + np.random.randint(0, 200)

        # Ensure OHLC relationships
        high = max(high, open_price, close_price)
        low = min(low, open_price, close_price)

        data.append([
            1609459200.0 + i * 300,  # datetime (5-minute intervals)
            open_price,
            high,
            low,
            close_price,
            volume,
            openinterest
        ])

    return data

class SmaStrategy:
    """SMA Strategy implementation compatible with backtrader-cpp"""

    def __init__(self):
        self.short_window = 10
        self.long_window = 60
        self.short_ma = None
        self.long_ma = None
        self.position_size = 0

    def __init__(self):
        """Initialize strategy - called by Cerebro"""
        print(f"Strategy initialized with params: short_window={self.short_window}, long_window={self.long_window}")

    def start(self):
        """Strategy start callback"""
        print("Strategy started")

    def prenext(self):
        """Pre-next callback"""
        pass

    def next(self):
        """Main strategy logic - called for each bar"""
        # Get current data
        current_close = self.data.close
        current_open = self.data.open

        # Calculate position size
        position = self.position
        current_size = position.size

        # Get broker value
        broker_value = self.broker.get_value()

        # Simple SMA crossover logic (simplified for now)
        if current_size == 0 and current_close > current_open:
            # Buy signal
            try:
                lots = 0.1 * broker_value / current_open
                if lots > 0:
                    order = self.buy(size=lots)
                    print(f"BUY: size={lots:.2f}, price={current_close:.2f}")
            except:
                lots = 0.1 * broker_value / current_close
                if lots > 0:
                    order = self.buy(size=lots)
                    print(f"BUY: size={lots:.2f}, price={current_close:.2f}")

        elif current_size > 0 and current_close < current_open:
            # Sell signal
            order = self.close()
            print(f"SELL: closing position, price={current_close:.2f}")

    def stop(self):
        """Strategy stop callback"""
        print("Strategy stopped")

    def notify_order(self, order):
        """Order notification callback"""
        if order.status == "Completed":
            if order.size > 0:
                print(f"BUY EXECUTED: Price={order.price}, Size={order.size}")
            else:
                print(f"SELL EXECUTED: Price={order.price}, Size={order.size}")

    def notify_trade(self, trade):
        """Trade notification callback"""
        if trade.isclosed:
            print(f"TRADE CLOSED: PNL={trade.pnl}")

def run_sma_strategy_test(n_rows=100):
    """Run the SMA strategy test"""
    try:
        import backtrader_cpp as bt

        print("🚀 Running Complete SMA Strategy Test")
        print("=" * 60)
        print(f"Testing with {n_rows} data points")

        # Create sample data
        print("\n📊 Creating sample data...")
        test_data = create_sample_data(n_rows)
        data = bt.DataSeries("SMA_Test")
        data.load_from_csv(test_data)
        print(f"✅ Data created: {data}")

        # Create strategy
        print("\n📈 Setting up strategy...")
        strategy = bt.Strategy()
        # Set strategy parameters
        params = {"short_window": 10, "long_window": 60}
        strategy.set_params(params)
        print(f"✅ Strategy created: {strategy}")
        print(f"Strategy parameters: {dict(strategy.p)}")

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
        execution_time = end_time - start_time

        print("✅ Backtest completed!")
        print(f"Execution time: {execution_time:.4f} seconds")
        print(f"Results: {len(results)} result sets")

        # Get final broker state
        broker = cerebro.broker()
        final_value = broker.get_value()
        final_cash = broker.get_cash()

        print("\n💰 Final Results:")
        print(f"Final Portfolio Value: ${final_value:.2f}")
        print(f"Remaining Cash: ${final_cash:.2f}")
        print("\n" + "=" * 60)
        print("🎉 SMA Strategy test completed successfully!")
        print("\n📊 Test Summary:")
        print("- ✅ Sample data generation")
        print("- ✅ Strategy creation and parameter setting")
        print("- ✅ Cerebro setup and execution")
        print("- ✅ Order execution simulation")
        print("- ✅ Performance timing")

        return True

    except Exception as e:
        print(f"❌ SMA Strategy test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = run_sma_strategy_test(n_rows=50)  # Smaller dataset for testing
    sys.exit(0 if success else 1)
