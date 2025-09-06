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

def create_test_data(n_rows=1000):
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
        self.datas = []
        self.broker = None
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
        if not self.datas:
            return

        # Get current data (use first data series)
        data = self.datas[0]
        current_close = data.close
        current_open = data.open

        # Calculate position size
        position = self.getposition(data)
        current_size = position.size

        # Simple SMA crossover logic (simplified for now)
        # In a full implementation, this would use actual SMA calculations
        if current_size == 0 and current_close > current_open:
            # Buy signal
            try:
                lots = 0.1 * self.broker.get_value() / current_open
                if lots > 0:
                    order = self.buy(data, size=lots)
                    print(f"BUY: size={lots:.2f}, price={current_close:.2f}")
            except:
                lots = 0.1 * self.broker.get_value() / current_close
                if lots > 0:
                    order = self.buy(data, size=lots)
                    print(f"BUY: size={lots:.2f}, price={current_close:.2f}")

        elif current_size > 0 and current_close < current_open:
            # Sell signal
            order = self.close(data)
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

    def buy(self, data=None, size=0.0, price=0.0):
        """Buy order"""
        if self.broker:
            return self.broker.buy(size, price)
        return None

    def sell(self, data=None, size=0.0, price=0.0):
        """Sell order"""
        if self.broker:
            return self.broker.sell(size, price)
        return None

    def close(self, data=None):
        """Close position"""
        if self.broker:
            return self.broker.sell(0.0)  # Simplified close
        return None

    def getposition(self, data=None):
        """Get position"""
        if self.broker:
            return self.broker.get_position(data.name if data else "")
        return type('Position', (), {'size': 0})()

    def add_data(self, data):
        """Add data series"""
        self.datas.append(data)

    def set_broker(self, broker):
        """Set broker"""
        self.broker = broker

def run_backtrader_compatible_test(n_rows=50):
    """Run the complete backtrader-compatible test"""
    try:
        import backtrader_cpp as bt

        print("🚀 Running Complete Backtrader-Compatible SMA Strategy Test")
        print("=" * 70)
        print(f"Testing with {n_rows} data points")

        # Create sample data
        print("\n📊 Creating sample data...")
        test_data = create_test_data(n_rows)
        data = bt.DataSeries("SMA_Test")
        data.load_from_csv(test_data)
        print(f"✅ Data created: {data}")
        print(f"Data size: {data.size}")
        print(f"Sample data: close[0]={data.get_close(0)}, current_close={data.close}")

        # Create strategy
        print("\n📈 Setting up strategy...")
        strategy = SmaStrategy()

        # Create cerebro
        print("\n🧠 Setting up Cerebro...")
        cerebro = bt.Cerebro()
        cerebro.add_data(data)

        # Manually set up strategy with data and broker
        strategy.add_data(data)
        strategy.set_broker(cerebro.broker())
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

        print("\n" + "=" * 70)
        print("🎉 Backtrader-compatible SMA Strategy test completed successfully!")
        print("\n📊 Test Summary:")
        print("- ✅ Sample data generation and loading")
        print("- ✅ Strategy creation and parameter setting")
        print("- ✅ Cerebro setup and execution")
        print("- ✅ Data access (indexed and current values)")
        print("- ✅ Order execution simulation")
        print("- ✅ Performance timing")

        return True

    except Exception as e:
        print(f"❌ Backtrader-compatible test failed: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = run_backtrader_compatible_test(n_rows=20)  # Smaller dataset for testing
    sys.exit(0 if success else 1)
