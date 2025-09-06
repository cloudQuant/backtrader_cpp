#!/usr/bin/env python3
"""
Core Functionality Test - Backtrader C++ Python Bindings
测试核心功能，避免有问题的组件
"""

import sys
import os
import unittest
import math
import time

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

try:
    import backtrader_cpp as bt
    MODULE_AVAILABLE = True
except ImportError as e:
    print(f"❌ Cannot import backtrader_cpp: {e}")
    MODULE_AVAILABLE = False


class TestCoreFunctionality(unittest.TestCase):
    """核心功能测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
    
    def test_module_import_and_info(self):
        """测试模块导入和信息"""
        # Test basic functionality
        result = bt.test()
        self.assertIsInstance(result, str)
        self.assertIn("Backtrader C++", result)
        
        # Test version info
        version = bt.get_version()
        self.assertIsInstance(version, dict)
        self.assertIn('version', version)
        self.assertEqual(version['version'], '0.3.0')
        
        print(f"Module version: {version['version']}")
        print(f"Status: {version['status']}")

    def test_data_generation(self):
        """测试数据生成"""
        # Generate test data
        data = bt.generate_sample_data(100, 100.0, 0.02, 42)
        self.assertEqual(len(data), 100)
        self.assertIsInstance(data, list)
        self.assertTrue(all(isinstance(x, float) for x in data))
        self.assertEqual(data[0], 100.0)
        
        # Test validation
        validation = bt.validate_data(data)
        self.assertIsInstance(validation, dict)
        self.assertTrue(validation['valid'])
        self.assertEqual(validation['size'], 100)
        
        print(f"Generated data: {len(data)} points")
        print(f"Price range: {min(data):.2f} - {max(data):.2f}")

    def test_technical_indicators(self):
        """测试技术指标"""
        test_data = bt.generate_sample_data(100, 100.0, 0.02, 42)
        
        # Test SMA
        sma_20 = bt.calculate_sma(test_data, 20)
        self.assertEqual(len(sma_20), len(test_data))
        sma_valid = [x for x in sma_20 if not math.isnan(x)]
        self.assertEqual(len(sma_valid), len(test_data) - 19)  # 20-1 NaN values
        
        # Test EMA
        ema_20 = bt.calculate_ema(test_data, 20)
        self.assertEqual(len(ema_20), len(test_data))
        # EMA should have no NaN values
        ema_valid = [x for x in ema_20 if not math.isnan(x)]
        self.assertEqual(len(ema_valid), len(test_data))
        
        # Test RSI
        rsi_14 = bt.calculate_rsi(test_data, 14)
        self.assertEqual(len(rsi_14), len(test_data))
        rsi_valid = [x for x in rsi_14 if not math.isnan(x)]
        self.assertGreater(len(rsi_valid), 0)
        
        # Verify RSI values are in correct range
        for val in rsi_valid:
            self.assertGreaterEqual(val, 0.0)
            self.assertLessEqual(val, 100.0)
        
        print(f"SMA valid values: {len(sma_valid)}")
        print(f"EMA valid values: {len(ema_valid)}")
        print(f"RSI valid values: {len(rsi_valid)}")

    def test_risk_metrics(self):
        """测试风险指标"""
        test_data = bt.generate_sample_data(252, 100.0, 0.02, 42)  # One year of data
        
        # Test returns calculation
        returns = bt.calculate_returns(test_data)
        self.assertEqual(len(returns), len(test_data) - 1)
        
        # Test volatility
        volatility = bt.calculate_volatility(returns, 20)
        self.assertEqual(len(volatility), len(returns))
        vol_valid = [x for x in volatility if not math.isnan(x)]
        self.assertGreater(len(vol_valid), 0)
        
        # Test Sharpe ratio
        sharpe = bt.calculate_sharpe(returns, 0.02)
        self.assertIsInstance(sharpe, float)
        
        print(f"Returns: {len(returns)} values")
        print(f"Volatility valid: {len(vol_valid)} values")
        print(f"Sharpe ratio: {sharpe:.4f}")

    def test_strategy_framework(self):
        """测试策略框架"""
        test_data = bt.generate_sample_data(100, 100.0, 0.02, 42)
        
        # Test moving average strategy
        strategy = bt.simple_moving_average_strategy(test_data, 5, 20, 10000.0)
        
        # Verify result structure
        expected_keys = ['signals', 'trades', 'initial_value', 'final_value', 'total_return', 'num_trades']
        for key in expected_keys:
            self.assertIn(key, strategy)
        
        # Verify values
        self.assertEqual(strategy['initial_value'], 10000.0)
        self.assertIsInstance(strategy['final_value'], float)
        self.assertIsInstance(strategy['total_return'], float)
        self.assertIsInstance(strategy['num_trades'], int)
        self.assertGreaterEqual(strategy['num_trades'], 0)
        
        print(f"Strategy return: {strategy['total_return']:.2%}")
        print(f"Number of trades: {strategy['num_trades']}")

    def test_performance_benchmarks(self):
        """测试性能基准"""
        # Test calculation benchmark
        benchmark = bt.benchmark(100000)
        
        expected_keys = ['result', 'time_us', 'iterations', 'ops_per_second']
        for key in expected_keys:
            self.assertIn(key, benchmark)
        
        self.assertEqual(benchmark['iterations'], 100000)
        self.assertGreater(benchmark['time_us'], 0)
        self.assertGreater(benchmark['ops_per_second'], 1000)
        
        # Test SMA benchmark
        test_data = bt.generate_sample_data(1000, 100.0, 0.02, 42)
        sma_benchmark = bt.benchmark_sma(test_data, 20, 100)
        
        expected_keys = ['data_points', 'period', 'iterations', 'time_us', 
                        'time_per_calculation_us', 'calculations_per_second']
        for key in expected_keys:
            self.assertIn(key, sma_benchmark)
        
        print(f"Performance: {benchmark['ops_per_second']:.0f} ops/sec")
        print(f"SMA performance: {sma_benchmark['calculations_per_second']:.0f} calc/sec")

    def test_accuracy_verification(self):
        """测试计算精度验证"""
        # Use simple known data for manual verification
        simple_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        
        # Test SMA precision
        sma_3 = bt.calculate_sma(simple_data, 3)
        
        # Verify first valid SMA values
        self.assertTrue(math.isnan(sma_3[0]))
        self.assertTrue(math.isnan(sma_3[1]))
        self.assertAlmostEqual(sma_3[2], 2.0, places=6)  # (1+2+3)/3
        self.assertAlmostEqual(sma_3[3], 3.0, places=6)  # (2+3+4)/3
        self.assertAlmostEqual(sma_3[4], 4.0, places=6)  # (3+4+5)/3
        
        # Test returns precision
        prices = [100.0, 110.0, 99.0, 108.9]
        returns = bt.calculate_returns(prices)
        
        expected_returns = [0.1, -0.1, 0.1]  # Simplified
        for i, (actual, expected) in enumerate(zip(returns, expected_returns)):
            self.assertAlmostEqual(actual, expected, places=6)
        
        print("Accuracy verification passed")


def run_core_tests():
    """运行核心测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🚀 运行 Backtrader C++ 核心功能测试")
    print("=" * 60)
    
    # Run the test suite
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(TestCoreFunctionality)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    # Print summary
    print("\n" + "=" * 60)
    print("📊 测试结果总结")
    print("=" * 60)
    
    success_rate = ((result.testsRun - len(result.failures) - len(result.errors)) / result.testsRun * 100) if result.testsRun > 0 else 0
    
    print(f"总测试数: {result.testsRun}")
    print(f"成功: {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"失败: {len(result.failures)}")
    print(f"错误: {len(result.errors)}")
    print(f"成功率: {success_rate:.1f}%")
    
    if len(result.failures) == 0 and len(result.errors) == 0:
        print("🎉 所有核心测试通过！")
        return True
    else:
        print("❌ 部分测试失败，需要检查。")
        if result.failures:
            print("\n失败详情:")
            for test, traceback in result.failures:
                print(f"  - {test}: {traceback}")
        if result.errors:
            print("\n错误详情:")
            for test, traceback in result.errors:
                print(f"  - {test}: {traceback}")
        return False


if __name__ == "__main__":
    success = run_core_tests()
    sys.exit(0 if success else 1)