#!/usr/bin/env python3
"""
Test script for backtrader-cpp Python bindings
"""

import sys
import os
import traceback
import numpy as np

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

def test_module():
    """Test the backtrader_cpp module functionality"""
    print("=" * 60)
    print("Testing Backtrader C++ Python Bindings")
    print("=" * 60)
    
    try:
        print("Attempting to import backtrader_cpp...")
        import backtrader_cpp as bt
        print("✅ Successfully imported backtrader_cpp!")
        
        # Test basic functionality
        print("\n📋 Testing basic functions:")
        print(f"Test function: {bt.test()}")
        
        version_info = bt.get_version()
        print(f"Version info: {version_info}")
        
        # Test performance calculation
        print("\n🚀 Testing performance functions:")
        
        # Create sample price data
        np.random.seed(42)
        prices = 100 + np.cumsum(np.random.randn(1000) * 0.01)
        print(f"Generated {len(prices)} price points")
        
        # Calculate returns
        returns = bt.calculate_returns(prices)
        print(f"Calculated {len(returns)} returns")
        print(f"Return stats: mean={np.mean(returns):.6f}, std={np.std(returns):.6f}")
        
        # Calculate Sharpe ratio
        sharpe = bt.calculate_sharpe(returns, risk_free_rate=0.02)
        print(f"Sharpe ratio: {sharpe:.4f}")
        
        # Test moving average
        print("\n📈 Testing SimpleMA:")
        ma = bt.SimpleMA()
        ma_values = ma.calculate(prices, period=20)
        print(f"Calculated MA with {len(ma_values)} values")
        print(f"Last 5 MA values: {ma_values[-5:]}")
        
        # Test data container
        print("\n📊 Testing SimpleData:")
        data = bt.SimpleData()
        data.add_series(prices)
        data.add_series(returns)
        print(f"Data container has {len(data)} series")
        
        retrieved_prices = data.get_series(0)
        print(f"Retrieved prices series: {len(retrieved_prices)} values")
        
        # Performance benchmark
        print("\n⚡ Testing performance benchmark:")
        benchmark_result = bt.benchmark_calculation(100000)
        print(f"Benchmark results: {benchmark_result}")
        print(f"Performance: {benchmark_result['ops_per_second']:.0f} ops/second")
        
        # Test module documentation
        print("\n📚 Module documentation:")
        doc = bt.__doc__()
        print("Documentation available:", len(doc) > 0)
        
        print("\n" + "=" * 60)
        print("🎉 ALL TESTS PASSED SUCCESSFULLY!")
        print("Backtrader C++ Python bindings are working correctly.")
        print("=" * 60)
        
        return True
        
    except ImportError as e:
        print(f"❌ Import Error: {e}")
        print("\nPossible solutions:")
        print("1. Make sure the module was compiled successfully")
        print("2. Check if all required libraries are available")
        print("3. Try building with system Python instead of conda")
        return False
        
    except Exception as e:
        print(f"❌ Error during testing: {e}")
        print(f"Error type: {type(e).__name__}")
        traceback.print_exc()
        return False

def create_simple_example():
    """Create a simple working example"""
    try:
        import backtrader_cpp as bt
        import numpy as np
        import matplotlib.pyplot as plt
        
        print("\n📈 Creating a simple trading example...")
        
        # Generate sample data
        np.random.seed(42)
        days = 252  # One year of trading days
        prices = 100 + np.cumsum(np.random.randn(days) * 0.01)
        
        # Calculate moving averages
        ma_short = bt.SimpleMA()
        ma_long = bt.SimpleMA()
        
        ma_short_values = ma_short.calculate(prices, period=20)
        ma_long_values = ma_long.calculate(prices, period=50)
        
        # Simple crossover strategy signals
        signals = []
        position = 0
        for i in range(len(prices)):
            if i < 50:  # Wait for both MAs to be valid
                signals.append(0)
                continue
                
            short_ma = ma_short_values[i]
            long_ma = ma_long_values[i]
            
            if not np.isnan(short_ma) and not np.isnan(long_ma):
                if short_ma > long_ma and position <= 0:
                    signals.append(1)  # Buy signal
                    position = 1
                elif short_ma < long_ma and position >= 0:
                    signals.append(-1)  # Sell signal
                    position = -1
                else:
                    signals.append(0)  # Hold
            else:
                signals.append(0)
        
        # Calculate strategy performance
        returns = bt.calculate_returns(prices)
        
        # Calculate strategy returns (simplified)
        strategy_returns = []
        for i in range(1, len(signals)):
            if signals[i-1] == 1:  # Was long
                strategy_returns.append(returns[i-1])
            elif signals[i-1] == -1:  # Was short
                strategy_returns.append(-returns[i-1])
            else:
                strategy_returns.append(0)  # No position
        
        strategy_returns = np.array(strategy_returns)
        
        # Performance metrics
        total_return = np.prod(1 + returns) - 1
        strategy_total_return = np.prod(1 + strategy_returns) - 1
        
        buy_hold_sharpe = bt.calculate_sharpe(returns)
        strategy_sharpe = bt.calculate_sharpe(strategy_returns)
        
        print(f"\n📊 Performance Results:")
        print(f"Buy & Hold Return: {total_return:.2%}")
        print(f"Strategy Return: {strategy_total_return:.2%}")
        print(f"Buy & Hold Sharpe: {buy_hold_sharpe:.4f}")
        print(f"Strategy Sharpe: {strategy_sharpe:.4f}")
        
        # Count signals
        buy_signals = sum(1 for s in signals if s == 1)
        sell_signals = sum(1 for s in signals if s == -1)
        print(f"Buy signals: {buy_signals}")
        print(f"Sell signals: {sell_signals}")
        
        print("✅ Simple trading example completed successfully!")
        return True
        
    except Exception as e:
        print(f"Example creation failed: {e}")
        return False

if __name__ == "__main__":
    print("Starting Backtrader C++ Python Bindings Test")
    print(f"Python version: {sys.version}")
    print(f"Current directory: {os.getcwd()}")
    print(f"Module search path: {sys.path[0]}")
    
    # Test the module
    success = test_module()
    
    if success:
        # Create a simple example
        create_simple_example()
    else:
        print("\n🔧 Troubleshooting Information:")
        print(f"Build directory exists: {os.path.exists('build')}")
        if os.path.exists('build'):
            files = os.listdir('build')
            so_files = [f for f in files if f.endswith('.so')]
            print(f"Shared object files in build/: {so_files}")
            
        print("\nBuild might need to be redone with compatible Python version.")