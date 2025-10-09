import backtrader_cpp as bt

# Create sample data
data = bt.create_sample_data(100)
print(f"Created data with {len(data)} bars")

# Try to create SMA with data parameter
try:
    # Try with both constructors
    sma1 = bt.indicators.SMA(20)  # Old constructor
    print(f"Created SMA with old constructor: {sma1}")
    
    sma2 = bt.indicators.SMA(data, 20)  # New constructor
    print(f"Created SMA with new constructor: {sma2}")
    
except Exception as e:
    print(f"Error creating SMA: {e}")
    import traceback
    traceback.print_exc()