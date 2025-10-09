import backtrader_cpp as bt

# Create sample data
data = bt.create_sample_data(100)
print(f"Created data with {len(data)} bars")

# Try to create SMA with data parameter using factory function
try:
    # Try with factory function
    sma = bt.indicators.SMA(data, 20)
    print(f"Created SMA with factory function: {sma}")
    print(f"SMA period: {sma.period}")
    
except Exception as e:
    print(f"Error creating SMA with factory function: {e}")
    import traceback
    traceback.print_exc()