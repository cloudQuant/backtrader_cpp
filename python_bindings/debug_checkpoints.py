#!/usr/bin/env python3
"""
Debug checkpoint mapping to understand the original backtrader test logic
"""

import sys
import os
import math

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

# Import data loader
from data_loader import BacktraderTestData, OriginalTestParameters

try:
    import backtrader_cpp as bt
    MODULE_AVAILABLE = True
except ImportError as e:
    print(f"❌ Cannot import backtrader_cpp: {e}")
    MODULE_AVAILABLE = False


def debug_checkpoint_mapping():
    """Debug the checkpoint mapping logic"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available.")
        return
    
    # Load test data
    data_loader = BacktraderTestData()
    test_data = data_loader.get_close_prices(0)
    
    print("🔍 Debug Checkpoint Mapping")
    print("=" * 60)
    print(f"Data length: {len(test_data)}")
    print(f"First price: {test_data[0]}")
    print(f"Last price: {test_data[-1]}")
    
    # SMA Debug
    print("\n📊 SMA (30-period) Debug:")
    period = 30
    min_period = 30
    expected_values = ['4063.463000', '3644.444667', '3554.693333']
    
    sma_values = bt.calculate_sma(test_data, period)
    
    # Debug nan values
    nan_count = sum(1 for x in sma_values if math.isnan(x))
    valid_start = period - 1  # First valid SMA index should be 29
    
    print(f"  SMA array length: {len(sma_values)}")
    print(f"  NaN values: {nan_count} (expected: {period-1})")
    print(f"  First valid index: {valid_start}")
    print(f"  First valid value: {sma_values[valid_start] if valid_start < len(sma_values) else 'N/A'}")
    print(f"  Last value: {sma_values[-1]}")
    
    # Calculate original checkpoints
    l = len(sma_values)  # 255
    mp = min_period      # 30
    
    # Original algorithm: [0, -l + mp, (-l + mp) // 2]
    check_points = [0, -l + mp, (-l + mp) // 2]
    print(f"  Original checkpoints: {check_points}")
    
    # Convert to positive indices
    positive_indices = []
    for cp in check_points:
        if cp >= 0:
            positive_indices.append(cp)
        else:
            positive_indices.append(l + cp)
    
    print(f"  Positive indices: {positive_indices}")
    
    # Check values at these positions
    for i, (cp, pos_idx, expected) in enumerate(zip(check_points, positive_indices, expected_values)):
        if 0 <= pos_idx < len(sma_values):
            actual = sma_values[pos_idx]
            expected_val = float(expected)
            print(f"  Checkpoint {i}: cp={cp}, idx={pos_idx}, expected={expected_val}, actual={actual:.6f}, NaN={math.isnan(actual)}")
        else:
            print(f"  Checkpoint {i}: cp={cp}, idx={pos_idx} - OUT OF RANGE")
    
    # Try different interpretation - maybe checkpoints are relative to valid data only?
    print("\n🔄 Alternative interpretation - Valid data only:")
    valid_sma = [x for x in sma_values if not math.isnan(x)]
    valid_length = len(valid_sma)
    print(f"  Valid SMA length: {valid_length}")
    
    # Checkpoints relative to valid data
    valid_checkpoints = [0, -valid_length + mp, (-valid_length + mp) // 2]
    print(f"  Valid data checkpoints: {valid_checkpoints}")
    
    for i, (cp, expected) in enumerate(zip(valid_checkpoints, expected_values)):
        if cp >= 0:
            valid_idx = cp
        else:
            valid_idx = valid_length + cp
        
        if 0 <= valid_idx < valid_length:
            actual = valid_sma[valid_idx]
            expected_val = float(expected)
            print(f"  Valid checkpoint {i}: cp={cp}, valid_idx={valid_idx}, expected={expected_val}, actual={actual:.6f}")
        else:
            print(f"  Valid checkpoint {i}: cp={cp}, valid_idx={valid_idx} - OUT OF RANGE")
    
    # Manual calculation verification
    print("\n🧮 Manual SMA verification:")
    # Check if our calculations match expected manually
    manual_last_30 = sum(test_data[-30:]) / 30
    manual_first_30 = sum(test_data[:30]) / 30
    
    print(f"  Manual last 30 periods SMA: {manual_last_30:.6f}")
    print(f"  Manual first 30 periods SMA: {manual_first_30:.6f}")
    print(f"  SMA at index 29 (first valid): {sma_values[29]:.6f}")
    print(f"  SMA at index 254 (last): {sma_values[254]:.6f}")
    
    # Check what the expected checkpoint 0 should be
    print("\n🎯 Expected vs Actual Analysis:")
    print(f"  Expected checkpoint 0: 4063.463000")
    print(f"  Our last SMA value: {sma_values[-1]:.6f}")
    print(f"  Match? {abs(sma_values[-1] - 4063.463000) < 0.000001}")


if __name__ == "__main__":
    debug_checkpoint_mapping()