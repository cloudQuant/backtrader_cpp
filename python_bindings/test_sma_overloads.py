import backtrader_cpp as bt

# Create sample data
data = bt.create_sample_data(100)
print(f"Created data with {len(data)} bars")

# Try to create SMA with both constructors
try:
    # Try with old constructor (period only)
    sma1 = bt.indicators.SMA(20)
    print(f"Created SMA with old constructor: {sma1}")
    
    # Try with new constructor (data and period)
    sma2 = bt.indicators.SMA(data, 20)
    print(f"Created SMA with new constructor: {sma2}")
    
except Exception as e:
    print(f"Error creating SMA: {e}")
    import traceback
    traceback.print_exc()