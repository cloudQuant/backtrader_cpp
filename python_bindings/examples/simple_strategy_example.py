#!/usr/bin/env python3
"""
Simple SMA Crossover Strategy Example
Demonstrates basic usage of backtrader-cpp Python bindings
"""

import sys
import os
import numpy as np
import pandas as pd

# Add the path to import backtrader_cpp (when built)
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

try:
    import backtrader_cpp as bt
    print(f"Successfully imported backtrader_cpp version {bt.__version__}")
    print(f"Build info: {bt.get_build_info()}")
except ImportError as e:
    print(f"Could not import backtrader_cpp: {e}")
    print("Please build the Python bindings first using: python setup.py build_ext --inplace")
    sys.exit(1)


class SMAStrategy(bt.Strategy):
    """Simple SMA crossover strategy"""
    
    params = dict(
        fast_period=10,
        slow_period=30,
    )
    
    def init(self):
        """Initialize strategy indicators"""
        print(f"Initializing strategy with fast_period={self.params['fast_period']}, slow_period={self.params['slow_period']}")
        
        # Create moving averages
        self.fast_sma = bt.SMA(self.data, period=self.params['fast_period'])
        self.slow_sma = bt.SMA(self.data, period=self.params['slow_period'])
        
        # Create crossover signal
        self.crossover = bt.CrossOver(self.fast_sma, self.slow_sma)
        
        print("Strategy indicators created successfully")
    
    def next(self):
        """Process each new bar"""
        current_bar = len(self)
        
        # Get current values
        close_price = self.data.close[0]
        fast_sma_value = self.fast_sma[0]
        slow_sma_value = self.slow_sma[0]
        cross_value = self.crossover[0]
        
        # Log every 50 bars to avoid spam
        if current_bar % 50 == 0:
            print(f"Bar {current_bar}: Close={close_price:.2f}, Fast SMA={fast_sma_value:.2f}, Slow SMA={slow_sma_value:.2f}")
        
        # Trading logic
        position = self.getposition()
        
        if not position and cross_value > 0:
            # No position and fast SMA crosses above slow SMA -> Buy
            order = self.buy()
            print(f"Bar {current_bar}: BUY signal - Close={close_price:.2f}")
            
        elif position and cross_value < 0:
            # Have position and fast SMA crosses below slow SMA -> Sell
            order = self.close()
            print(f"Bar {current_bar}: SELL signal - Close={close_price:.2f}")
    
    def notify_order(self, order):
        """Handle order notifications"""
        if order.status == bt.OrderStatus.Completed:
            if order.isbuy():
                print(f"BUY EXECUTED: Price={order.executed.price:.2f}, Size={order.executed.size:.2f}")
            else:
                print(f"SELL EXECUTED: Price={order.executed.price:.2f}, Size={order.executed.size:.2f}")
    
    def notify_trade(self, trade):
        """Handle trade notifications"""
        if trade.isclosed():
            print(f"TRADE CLOSED: PnL={trade.pnl:.2f}, PnL%={trade.pnl/trade.value*100:.2f}%")
    
    def stop(self):
        """Called when strategy stops"""
        final_value = self.broker.getvalue()
        print(f"Strategy finished. Final portfolio value: ${final_value:.2f}")


def create_sample_data():
    """Create sample OHLCV data for testing"""
    print("Creating sample market data...")
    
    # Generate 500 days of sample data
    np.random.seed(42)  # For reproducible results
    dates = pd.date_range('2020-01-01', periods=500, freq='D')
    
    # Generate realistic price data with trend and volatility
    base_price = 100.0
    returns = np.random.normal(0.0005, 0.02, len(dates))  # Small positive drift, 2% daily volatility
    prices = base_price * np.exp(np.cumsum(returns))
    
    # Create OHLCV data
    df = pd.DataFrame(index=dates)
    df['close'] = prices
    df['open'] = df['close'].shift(1) + np.random.normal(0, 0.5, len(df))
    df['high'] = np.maximum(df['open'], df['close']) + np.abs(np.random.normal(0, 1, len(df)))
    df['low'] = np.minimum(df['open'], df['close']) - np.abs(np.random.normal(0, 1, len(df)))
    df['volume'] = np.random.randint(100000, 1000000, len(df))
    
    # Clean up first row
    df.iloc[0, df.columns.get_loc('open')] = df.iloc[0, df.columns.get_loc('close')]
    
    print(f"Created {len(df)} days of sample data")
    print(f"Price range: ${df['close'].min():.2f} - ${df['close'].max():.2f}")
    
    return df


def run_backtest():
    """Run the complete backtest"""
    print("=" * 60)
    print("Backtrader C++ Python Bindings Example")
    print("Simple SMA Crossover Strategy")
    print("=" * 60)
    
    # Create sample data
    data_df = create_sample_data()
    
    # Create Cerebro engine
    print("\nInitializing Cerebro engine...")
    cerebro = bt.Cerebro()
    
    # Add data
    print("Adding data feed...")
    data_feed = bt.PandasData(data_df)
    cerebro.adddata(data_feed)
    
    # Add strategy
    print("Adding strategy...")
    cerebro.addstrategy(SMAStrategy, fast_period=10, slow_period=30)
    
    # Set initial cash
    initial_cash = 100000.0
    cerebro.broker.setcash(initial_cash)
    print(f"Initial cash: ${initial_cash:,.2f}")
    
    # Add analyzers
    print("Adding analyzers...")
    cerebro.addanalyzer(bt.analyzers.TradeAnalyzer, _name='trades')
    cerebro.addanalyzer(bt.analyzers.SharpeRatio, _name='sharpe')
    cerebro.addanalyzer(bt.analyzers.DrawDown, _name='drawdown')
    
    # Run backtest
    print("\nRunning backtest...")
    print("-" * 40)
    
    start_time = time.time() if 'time' in dir() else 0
    results = cerebro.run()
    end_time = time.time() if 'time' in dir() else 0
    
    print("-" * 40)
    print("Backtest completed!")
    
    # Print results
    final_value = cerebro.broker.getvalue()
    total_return = (final_value - initial_cash) / initial_cash * 100
    
    print(f"\nResults Summary:")
    print(f"Initial Value: ${initial_cash:,.2f}")
    print(f"Final Value:   ${final_value:,.2f}")
    print(f"Total Return:  {total_return:.2f}%")
    
    # Analyzer results
    strategy = results[0]
    
    try:
        trade_analysis = strategy.analyzers.trades.get_analysis()
        print(f"\nTrade Analysis:")
        print(f"Total Trades: {trade_analysis.get('total', {}).get('total', 0)}")
        print(f"Winning Trades: {trade_analysis.get('won', {}).get('total', 0)}")
        print(f"Losing Trades: {trade_analysis.get('lost', {}).get('total', 0)}")
    except:
        print("Trade analysis not available")
    
    try:
        sharpe_ratio = strategy.analyzers.sharpe.get_analysis().get('sharperatio', 'N/A')
        print(f"Sharpe Ratio: {sharpe_ratio}")
    except:
        print("Sharpe ratio not available")
    
    try:
        drawdown = strategy.analyzers.drawdown.get_analysis()
        max_dd = drawdown.get('max', {}).get('drawdown', 'N/A')
        print(f"Max Drawdown: {max_dd}")
    except:
        print("Drawdown analysis not available")
    
    if 'time' in dir():
        print(f"\nExecution time: {end_time - start_time:.2f} seconds")
    
    print("\nBacktest completed successfully!")
    return results


def benchmark_performance():
    """Benchmark the performance of key operations"""
    print("\n" + "=" * 60)
    print("Performance Benchmark")
    print("=" * 60)
    
    try:
        import time
        
        # Test indicator calculation performance
        data_df = create_sample_data()
        data_feed = bt.PandasData(data_df)
        
        print("Benchmarking SMA calculation...")
        start_time = time.time()
        sma = bt.SMA(data_feed, period=20)
        sma.calculate()
        sma_time = time.time() - start_time
        print(f"SMA(20) calculation time: {sma_time*1000:.2f} ms")
        
        print("Benchmarking RSI calculation...")
        start_time = time.time()
        rsi = bt.RSI(data_feed, period=14)
        rsi.calculate()
        rsi_time = time.time() - start_time
        print(f"RSI(14) calculation time: {rsi_time*1000:.2f} ms")
        
        print("Benchmarking MACD calculation...")
        start_time = time.time()
        macd = bt.MACD(data_feed)
        macd.calculate()
        macd_time = time.time() - start_time
        print(f"MACD calculation time: {macd_time*1000:.2f} ms")
        
        print(f"\nTotal benchmark time: {(sma_time + rsi_time + macd_time)*1000:.2f} ms")
        
    except Exception as e:
        print(f"Benchmark failed: {e}")


if __name__ == "__main__":
    try:
        # Run the main backtest
        results = run_backtest()
        
        # Run performance benchmark
        benchmark_performance()
        
        print("\n" + "=" * 60)
        print("Example completed successfully!")
        print("For more examples, check the examples/ directory")
        print("=" * 60)
        
    except Exception as e:
        print(f"\nError running example: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)