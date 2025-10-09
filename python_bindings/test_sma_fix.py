import backtrader_cpp as bt

def test_sma_fix():
    """Test that SMA can be created with data parameter"""
    data = bt.create_sample_data(100)
    print(f"Created data with {len(data)} bars")
    
    # This should work now
    sma = bt.indicators.SMA(data, period=20)
    print(f"Created SMA: {sma}")
    print(f"SMA period: {sma.period}")
    
    # Test indicator access
    try:
        val = sma[0]
        print(f"SMA value: {val}")
    except Exception as e:
        print(f"Error accessing SMA value: {e}")

if __name__ == "__main__":
    test_sma_fix()