import backtrader_cpp as bt
import numpy as np

# Create sample data
data = bt.create_sample_data(100)
print(f"Data created with {len(data)} bars")

# Create SMA indicator
sma = bt.indicators.SMA(data, period=20)
print(f"SMA created: {sma}")

# Try to access values (this is where we might have issues)
try:
    print(f"SMA value at index 0: {sma[0]}")
    print(f"SMA value at index -1: {sma[-1]}")
except Exception as e:
    print(f"Error accessing SMA values: {e}")

# Try to create a simple strategy
class TestStrategy(bt.Strategy):
    def __init__(self):
        self.sma = bt.indicators.SMA(self.data, period=20)
        
    def next(self):
        print(f"Close: {self.data.close[0]}, SMA: {self.sma[0]}")

# Try to run with cerebro
cerebro = bt.Cerebro()
cerebro.add_data(data)
cerebro.add_strategy(TestStrategy())
try:
    results = cerebro.run()
    print("Cerebro run successful")
except Exception as e:
    print(f"Error running cerebro: {e}")