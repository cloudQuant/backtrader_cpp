#!/usr/bin/env python3
"""
Test Available Functions in Python Bindings
测试当前Python绑定中可用的功能
"""

import sys
import os
import unittest
import math

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

# Import data loader
from data_loader import BacktraderTestData

try:
    import backtrader_cpp as bt
    MODULE_AVAILABLE = True
except ImportError as e:
    print(f"❌ Cannot import backtrader_cpp: {e}")
    MODULE_AVAILABLE = False


class TestAvailableFunctions(unittest.TestCase):
    """测试当前可用的函数"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
        
        # 验证数据
        self.assertEqual(len(self.test_data), 255)
        self.assertTrue(all(not math.isnan(x) for x in self.test_data))
    
    def test_calculate_sma(self):
        """测试SMA计算"""
        # 测试不同周期
        periods = [5, 10, 20, 30]
        
        for period in periods:
            with self.subTest(period=period):
                result = bt.calculate_sma(self.test_data, period)
                
                # 基本验证
                self.assertEqual(len(result), len(self.test_data))
                
                # 验证NaN值数量
                nan_count = sum(1 for x in result if math.isnan(x))
                self.assertEqual(nan_count, period - 1)
                
                # 验证有效值
                valid_values = [x for x in result if not math.isnan(x)]
                self.assertGreater(len(valid_values), 0)
                
                print(f"✅ SMA({period}): {len(valid_values)} valid values")
    
    def test_calculate_ema(self):
        """测试EMA计算"""
        periods = [10, 20, 30]
        
        for period in periods:
            with self.subTest(period=period):
                result = bt.calculate_ema(self.test_data, period)
                
                # 基本验证
                self.assertEqual(len(result), len(self.test_data))
                
                # EMA应该没有NaN值
                nan_count = sum(1 for x in result if math.isnan(x))
                self.assertEqual(nan_count, 0)
                
                # 第一个值应该等于第一个价格
                self.assertAlmostEqual(result[0], self.test_data[0], places=6)
                
                print(f"✅ EMA({period}): All {len(result)} values valid")
    
    def test_calculate_rsi(self):
        """测试RSI计算"""
        periods = [14, 21]
        
        for period in periods:
            with self.subTest(period=period):
                result = bt.calculate_rsi(self.test_data, period)
                
                # 基本验证
                self.assertEqual(len(result), len(self.test_data))
                
                # 验证RSI值范围
                valid_values = [x for x in result if not math.isnan(x)]
                
                # 应该有足够的有效值
                self.assertGreater(len(valid_values), len(self.test_data) * 0.8)
                
                # 所有有效值应该在0-100范围内
                for val in valid_values:
                    self.assertGreaterEqual(val, 0.0)
                    self.assertLessEqual(val, 100.0)
                
                print(f"✅ RSI({period}): {len(valid_values)} valid values")
    
    def test_calculate_returns(self):
        """测试收益率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        # 验证基本属性
        self.assertEqual(len(returns), len(self.test_data) - 1)
        
        # 手动验证前几个收益率
        for i in range(min(3, len(returns))):
            expected = (self.test_data[i+1] - self.test_data[i]) / self.test_data[i]
            self.assertAlmostEqual(returns[i], expected, places=10)
        
        print(f"✅ Returns: {len(returns)} calculated")
    
    def test_calculate_volatility(self):
        """测试波动率计算"""
        returns = bt.calculate_returns(self.test_data)
        volatility = bt.calculate_volatility(returns, 20)
        
        # 验证基本属性
        self.assertEqual(len(volatility), len(returns))
        
        # 前19个应该是NaN
        for i in range(19):
            self.assertTrue(math.isnan(volatility[i]))
        
        # 有效值应该为正
        valid_vol = [x for x in volatility if not math.isnan(x)]
        for vol in valid_vol:
            self.assertGreater(vol, 0.0)
        
        print(f"✅ Volatility: {len(valid_vol)} valid values")
    
    def test_calculate_sharpe(self):
        """测试夏普比率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        risk_free_rates = [0.0, 0.02, 0.05]
        
        for rf in risk_free_rates:
            with self.subTest(risk_free=rf):
                sharpe = bt.calculate_sharpe(returns, rf)
                
                # 验证是有限数值
                self.assertIsInstance(sharpe, float)
                self.assertFalse(math.isnan(sharpe))
                self.assertFalse(math.isinf(sharpe))
                
                print(f"✅ Sharpe(rf={rf}): {sharpe:.4f}")
    
    def test_simple_moving_average_strategy(self):
        """测试移动平均策略"""
        strategy = bt.simple_moving_average_strategy(self.test_data, 5, 20, 10000.0)
        
        # 验证返回结构
        required_keys = ['signals', 'trades', 'initial_value', 'final_value', 'total_return', 'num_trades']
        for key in required_keys:
            self.assertIn(key, strategy)
        
        # 验证值
        self.assertEqual(strategy['initial_value'], 10000.0)
        self.assertIsInstance(strategy['final_value'], float)
        self.assertIsInstance(strategy['total_return'], float)
        self.assertIsInstance(strategy['num_trades'], int)
        
        print(f"✅ Strategy: {strategy['num_trades']} trades, {strategy['total_return']:.2%} return")
    
    def test_utility_functions(self):
        """测试工具函数"""
        # 测试版本信息
        version = bt.get_version()
        self.assertIsInstance(version, dict)
        self.assertIn('version', version)
        
        # 测试基本函数
        test_result = bt.test()
        self.assertIsInstance(test_result, str)
        
        print(f"✅ Version: {version['version']}")
        print(f"✅ Test: {test_result}")


def run_available_function_tests():
    """运行可用函数测试"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available.")
        return False
    
    print("🧪 测试当前可用的Python绑定函数")
    print("=" * 60)
    
    # 显示可用函数
    available_functions = [f for f in dir(bt) if not f.startswith('_')]
    print(f"📋 可用函数 ({len(available_functions)} 个):")
    for func in sorted(available_functions):
        print(f"  - {func}")
    
    print("\n" + "=" * 60)
    
    suite = unittest.TestLoader().loadTestsFromTestCase(TestAvailableFunctions)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    success = len(result.failures) == 0 and len(result.errors) == 0
    
    print("\n" + "=" * 60)
    print("📊 测试结果总结")
    print("=" * 60)
    
    print(f"总测试数: {result.testsRun}")
    print(f"成功: {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"失败: {len(result.failures)}")
    print(f"错误: {len(result.errors)}")
    
    if success:
        print("\n🎉 所有可用函数测试通过！")
        print("✅ 当前Python绑定功能正常工作")
    else:
        print("\n❌ 部分测试失败")
        
    return success


if __name__ == "__main__":
    success = run_available_function_tests()
    sys.exit(0 if success else 1)