import backtrader_cpp as bt

# Create sample data
data = bt.create_sample_data(100)
print(f"Created data with {len(data)} bars")

# Try to create SMA with both approaches
try:
    # Try with constructor (period only)
    sma1 = bt.indicators.SMA(20)
    print(f"Created SMA with constructor: {sma1}")
    
    # Try with factory function (data and period)
    sma2 = bt.indicators.SMA(data, 20)
    print(f"Created SMA with factory function: {sma2}")
    
except Exception as e:
    print(f"Error creating SMA: {e}")
    import traceback
    traceback.print_exc()