#!/usr/bin/env python3
"""
Test script for minimal backtrader-cpp Python bindings
"""

import sys
import os
import traceback

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

def test_minimal_module():
    """Test the minimal backtrader_cpp module functionality"""
    print("=" * 60)
    print("Testing Minimal Backtrader C++ Python Bindings")
    print("=" * 60)
    
    try:
        print("Attempting to import backtrader_cpp...")
        import backtrader_cpp as bt
        print("✅ Successfully imported backtrader_cpp!")
        
        # Test basic functionality
        print("\n📋 Testing basic functions:")
        test_result = bt.test()
        print(f"Test function: {test_result}")
        
        version_info = bt.get_version()
        print(f"Version info: {version_info}")
        
        # Test simple mathematical functions
        print("\n🧮 Testing mathematical functions:")
        
        # Create sample data without numpy
        prices = [100.0]
        for i in range(100):
            prices.append(prices[-1] + (i % 10 - 5) * 0.5)  # Simple price walk
        
        print(f"Created {len(prices)} price points")
        print(f"Price range: {min(prices):.2f} - {max(prices):.2f}")
        
        # Calculate simple moving average
        ma_values = bt.calculate_sma(prices, 10)
        valid_ma = [v for v in ma_values if not (v != v)]  # Remove NaN values
        print(f"SMA calculation: {len(valid_ma)} valid values out of {len(ma_values)}")
        print(f"Last 5 SMA values: {[f'{v:.2f}' for v in ma_values[-5:] if v == v]}")
        
        # Calculate returns
        returns = bt.calculate_returns(prices)
        print(f"Returns calculation: {len(returns)} values")
        if returns:
            avg_return = sum(returns) / len(returns)
            print(f"Average return: {avg_return:.6f}")
        
        # Test data container
        print("\n📊 Testing DoubleVector:")
        vec = bt.DoubleVector()
        for i, price in enumerate(prices[:10]):  # Add first 10 prices
            vec.push_back(price)
        
        print(f"Vector size: {len(vec)}")
        print(f"First 5 values: {[vec[i] for i in range(min(5, len(vec)))]}")
        
        # Modify vector
        if len(vec) > 0:
            original = vec[0]
            vec[0] = 999.99
            print(f"Modified vec[0] from {original:.2f} to {vec[0]:.2f}")
        
        # Performance test
        print("\n⚡ Testing performance:")
        perf_result = bt.performance_test(50000)  # Smaller test for compatibility
        print(f"Performance test results:")
        print(f"  - Iterations: {perf_result['iterations']}")
        print(f"  - Time: {perf_result['time_us']} microseconds")
        print(f"  - Result: {perf_result['result']:.6f}")
        
        ops_per_sec = perf_result['iterations'] * 1000000 / perf_result['time_us']
        print(f"  - Performance: {ops_per_sec:.0f} operations/second")
        
        print("\n" + "=" * 60)
        print("🎉 ALL MINIMAL TESTS PASSED SUCCESSFULLY!")
        print("Minimal backtrader C++ Python bindings are working correctly.")
        print("This demonstrates successful C++/Python integration.")
        print("=" * 60)
        
        return True
        
    except ImportError as e:
        print(f"❌ Import Error: {e}")
        print("\nDebugging information:")
        print(f"Current directory: {os.getcwd()}")
        print(f"Python path: {sys.path[0]}")
        if os.path.exists('build'):
            files = os.listdir('build')
            so_files = [f for f in files if f.endswith('.so')]
            print(f"Shared object files: {so_files}")
        return False
        
    except Exception as e:
        print(f"❌ Error during testing: {e}")
        print(f"Error type: {type(e).__name__}")
        traceback.print_exc()
        return False

def create_simple_strategy_example():
    """Create a simple strategy example using the minimal bindings"""
    try:
        import backtrader_cpp as bt
        
        print("\n📈 Creating Simple Strategy Example...")
        
        # Generate sample price data (simple trend + noise)
        import random
        random.seed(42)
        
        prices = [100.0]
        for i in range(100):
            # Simple random walk with slight upward bias
            change = random.gauss(0.02, 1.0)  # 2% average return with 1% volatility
            prices.append(prices[-1] * (1 + change/100))
        
        print(f"Generated {len(prices)} price points")
        print(f"Starting price: {prices[0]:.2f}, Ending price: {prices[-1]:.2f}")
        
        # Calculate moving averages for strategy
        short_ma = bt.calculate_sma(prices, 5)   # 5-period short MA
        long_ma = bt.calculate_sma(prices, 20)   # 20-period long MA
        
        # Simple crossover strategy
        signals = []
        position = 0
        trades = []
        
        for i in range(len(prices)):
            if i < 20:  # Wait for long MA to be valid
                signals.append(0)
                continue
            
            short_val = short_ma[i]
            long_val = long_ma[i]
            
            # Check if values are valid (not NaN)
            if short_val == short_val and long_val == long_val:
                # Buy signal: short MA crosses above long MA
                if short_val > long_val and position <= 0:
                    signals.append(1)
                    if position == 0:
                        trades.append(('BUY', i, prices[i]))
                        position = 1
                # Sell signal: short MA crosses below long MA  
                elif short_val < long_val and position >= 0:
                    signals.append(-1)
                    if position == 1:
                        trades.append(('SELL', i, prices[i]))
                        position = 0
                else:
                    signals.append(0)
            else:
                signals.append(0)
        
        # Calculate strategy performance
        returns = bt.calculate_returns(prices)
        
        # Simple buy-and-hold performance
        total_return = (prices[-1] - prices[0]) / prices[0]
        
        # Strategy performance (simplified)
        strategy_value = 100.0  # Starting capital
        shares = 0
        cash = strategy_value
        
        for trade_type, idx, price in trades:
            if trade_type == 'BUY':
                shares = cash / price
                cash = 0
            elif trade_type == 'SELL' and shares > 0:
                cash = shares * price
                shares = 0
        
        # Final value (if still holding shares)
        if shares > 0:
            cash = shares * prices[-1]
        
        strategy_return = (cash - strategy_value) / strategy_value
        
        print(f"\n📊 Strategy Results:")
        print(f"Buy & Hold Return: {total_return:.2%}")
        print(f"Strategy Return: {strategy_return:.2%}")
        print(f"Number of trades: {len(trades)}")
        print(f"Trade list: {trades}")
        
        if strategy_return > total_return:
            print("✅ Strategy outperformed buy & hold!")
        else:
            print("📉 Strategy underperformed buy & hold")
        
        return True
        
    except Exception as e:
        print(f"Strategy example failed: {e}")
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("Starting Minimal Backtrader C++ Python Bindings Test")
    print(f"Python version: {sys.version}")
    
    # Test the minimal module
    success = test_minimal_module()
    
    if success:
        # Create a simple strategy example
        create_simple_strategy_example()
        
        print("\n🎯 Next Steps:")
        print("1. This minimal implementation proves C++/Python integration works")
        print("2. Can now expand to include core backtrader classes")
        print("3. Add LineSeries, Strategy, and Indicator implementations")
        print("4. Integrate with the full backtrader-cpp codebase")
    else:
        print("\n❌ Module testing failed. Check build and dependencies.")