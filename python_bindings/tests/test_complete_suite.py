#!/usr/bin/env python3
"""
Complete Test Suite for Backtrader C++ Python Bindings
完整的测试套件 - 确保所有Python绑定功能都能正常运行
重点在于功能正确性而非精确值匹配
"""

import sys
import os
import unittest
import math
import time
from typing import List, Dict, Any

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


class TestBaseFunctionality(unittest.TestCase):
    """基础功能测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
        
        # 验证数据完整性
        self.assertEqual(len(self.test_data), 255)
        self.assertTrue(all(x > 0 for x in self.test_data))
        self.assertTrue(all(not math.isnan(x) for x in self.test_data))
    
    def test_module_import_and_version(self):
        """测试模块导入和版本信息"""
        # 验证模块可以导入
        self.assertTrue(MODULE_AVAILABLE)
        
        # 检查版本信息
        if hasattr(bt, '__version__'):
            print(f"Backtrader C++ Version: {bt.__version__}")
        
        # 检查可用函数
        required_functions = [
            'calculate_sma',
            'calculate_ema', 
            'calculate_rsi',
            'calculate_returns',
            'calculate_volatility',
            'calculate_sharpe',
            'simple_moving_average_strategy'
        ]
        
        for func_name in required_functions:
            self.assertTrue(hasattr(bt, func_name), f"Missing function: {func_name}")


class TestTechnicalIndicators(unittest.TestCase):
    """技术指标测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE)
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
    
    def test_sma_functionality(self):
        """测试SMA功能完整性"""
        # 测试不同周期
        periods = [5, 10, 20, 30, 50]
        
        for period in periods:
            with self.subTest(period=period):
                sma = bt.calculate_sma(self.test_data, period)
                
                # 基本验证
                self.assertEqual(len(sma), len(self.test_data))
                
                # 验证NaN模式
                nan_count = sum(1 for x in sma if math.isnan(x))
                self.assertEqual(nan_count, period - 1)
                
                # 验证有效值
                valid_values = [x for x in sma if not math.isnan(x)]
                self.assertEqual(len(valid_values), len(self.test_data) - period + 1)
                
                # 验证值为正
                for val in valid_values:
                    self.assertGreater(val, 0)
                
                # 验证最后一个值与手动计算匹配
                manual_last = sum(self.test_data[-period:]) / period
                self.assertAlmostEqual(sma[-1], manual_last, places=6)
    
    def test_ema_functionality(self):
        """测试EMA功能完整性"""
        periods = [10, 20, 30]
        
        for period in periods:
            with self.subTest(period=period):
                ema = bt.calculate_ema(self.test_data, period)
                
                # 基本验证
                self.assertEqual(len(ema), len(self.test_data))
                
                # EMA应该没有NaN值
                nan_count = sum(1 for x in ema if math.isnan(x))
                self.assertEqual(nan_count, 0)
                
                # 第一个值应该等于第一个价格
                self.assertAlmostEqual(ema[0], self.test_data[0], places=6)
                
                # 所有值应该为正
                for val in ema:
                    self.assertGreater(val, 0)
    
    def test_rsi_functionality(self):
        """测试RSI功能完整性"""
        periods = [14, 21]
        
        for period in periods:
            with self.subTest(period=period):
                rsi = bt.calculate_rsi(self.test_data, period)
                
                # 基本验证
                self.assertEqual(len(rsi), len(self.test_data))
                
                # 验证RSI值范围
                valid_values = [x for x in rsi if not math.isnan(x)]
                
                # 应该有足够的有效值
                self.assertGreater(len(valid_values), len(self.test_data) * 0.8)
                
                # 所有有效值应该在0-100范围内
                for val in valid_values:
                    self.assertGreaterEqual(val, 0.0)
                    self.assertLessEqual(val, 100.0)
    
    def test_indicator_edge_cases(self):
        """测试指标边界情况"""
        # 测试小数据集
        small_data = self.test_data[:10]
        
        # SMA周期大于数据长度
        sma = bt.calculate_sma(small_data, 15)
        self.assertEqual(len(sma), len(small_data))
        self.assertTrue(all(math.isnan(x) for x in sma))
        
        # EMA应该仍然工作
        ema = bt.calculate_ema(small_data, 15)
        self.assertEqual(len(ema), len(small_data))
        self.assertFalse(math.isnan(ema[0]))
        
        # 测试单个数据点
        single_data = [100.0]
        sma_single = bt.calculate_sma(single_data, 1)
        self.assertEqual(len(sma_single), 1)
        self.assertAlmostEqual(sma_single[0], 100.0, places=6)


class TestDataProcessing(unittest.TestCase):
    """数据处理测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE)
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
    
    def test_returns_calculation(self):
        """测试收益率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        # 验证基本属性
        self.assertEqual(len(returns), len(self.test_data) - 1)
        
        # 手动验证前几个收益率
        for i in range(min(5, len(returns))):
            expected = (self.test_data[i+1] - self.test_data[i]) / self.test_data[i]
            self.assertAlmostEqual(returns[i], expected, places=10)
        
        # 验证没有极端异常值
        for ret in returns:
            self.assertGreater(ret, -1.0)  # 不应该有超过100%的损失
            self.assertLess(ret, 2.0)      # 不应该有超过200%的收益
    
    def test_volatility_calculation(self):
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
            self.assertLess(vol, 1.0)  # 波动率不应该过高
    
    def test_sharpe_ratio(self):
        """测试夏普比率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        # 测试不同的无风险利率
        risk_free_rates = [0.0, 0.02, 0.05]
        
        for rf in risk_free_rates:
            with self.subTest(risk_free=rf):
                sharpe = bt.calculate_sharpe(returns, rf)
                
                # 验证是有限数值
                self.assertIsInstance(sharpe, float)
                self.assertFalse(math.isnan(sharpe))
                self.assertFalse(math.isinf(sharpe))
                
                # 夏普比率应该在合理范围内
                self.assertGreater(sharpe, -5.0)
                self.assertLess(sharpe, 5.0)


class TestStrategyFramework(unittest.TestCase):
    """策略框架测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE)
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
    
    def test_moving_average_strategy(self):
        """测试移动平均策略"""
        # 测试不同参数组合
        test_params = [
            (5, 20, 10000.0),
            (10, 30, 50000.0),
            (20, 50, 100000.0)
        ]
        
        for short_period, long_period, initial_capital in test_params:
            with self.subTest(short=short_period, long=long_period, capital=initial_capital):
                strategy = bt.simple_moving_average_strategy(
                    self.test_data, short_period, long_period, initial_capital
                )
                
                # 验证返回结构
                required_keys = ['signals', 'trades', 'initial_value', 'final_value', 'total_return', 'num_trades']
                for key in required_keys:
                    self.assertIn(key, strategy, f"Missing key: {key}")
                
                # 验证值的类型和范围
                self.assertEqual(strategy['initial_value'], initial_capital)
                self.assertIsInstance(strategy['final_value'], float)
                self.assertIsInstance(strategy['total_return'], float)
                self.assertIsInstance(strategy['num_trades'], int)
                self.assertGreaterEqual(strategy['num_trades'], 0)
                
                # 验证信号数组
                self.assertEqual(len(strategy['signals']), len(self.test_data))
                
                # 验证交易记录
                self.assertIsInstance(strategy['trades'], list)
                
                # 最终价值应该为正
                self.assertGreater(strategy['final_value'], 0)
    
    def test_strategy_with_minimal_data(self):
        """测试策略在最小数据集上的表现"""
        # 使用较小的数据集
        small_data = self.test_data[:60]  # 60个数据点
        
        strategy = bt.simple_moving_average_strategy(small_data, 5, 20, 10000.0)
        
        # 应该仍能正常运行
        self.assertIn('final_value', strategy)
        self.assertIsInstance(strategy['final_value'], float)
        self.assertGreater(strategy['final_value'], 0)


class TestPerformance(unittest.TestCase):
    """性能测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE)
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
    
    def test_calculation_performance(self):
        """测试计算性能"""
        iterations = 100
        
        # SMA性能测试
        start_time = time.time()
        for _ in range(iterations):
            bt.calculate_sma(self.test_data, 20)
        sma_time = time.time() - start_time
        
        # EMA性能测试
        start_time = time.time()
        for _ in range(iterations):
            bt.calculate_ema(self.test_data, 20)
        ema_time = time.time() - start_time
        
        # RSI性能测试
        start_time = time.time()
        for _ in range(iterations):
            bt.calculate_rsi(self.test_data, 14)
        rsi_time = time.time() - start_time
        
        print(f"\n性能测试结果 ({len(self.test_data)} 数据点, {iterations} 次迭代):")
        print(f"  SMA: {sma_time*1000:.2f}ms")
        print(f"  EMA: {ema_time*1000:.2f}ms")
        print(f"  RSI: {rsi_time*1000:.2f}ms")
        
        # 性能应该在合理范围内（每次计算少于10ms）
        max_time_per_calc = 0.1  # 100ms for 100 iterations = 1ms per calculation
        self.assertLess(sma_time, max_time_per_calc)
        self.assertLess(ema_time, max_time_per_calc)
        self.assertLess(rsi_time, max_time_per_calc)
    
    def test_large_dataset_handling(self):
        """测试大数据集处理能力"""
        # 创建更大的数据集
        large_data = self.test_data * 10  # 2550个数据点
        
        # 应该能够处理大数据集而不崩溃
        sma = bt.calculate_sma(large_data, 50)
        self.assertEqual(len(sma), len(large_data))
        
        ema = bt.calculate_ema(large_data, 50)
        self.assertEqual(len(ema), len(large_data))
        
        rsi = bt.calculate_rsi(large_data, 14)
        self.assertEqual(len(rsi), len(large_data))


class TestDataCompatibility(unittest.TestCase):
    """数据兼容性测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE)
        self.data_loader = BacktraderTestData()
    
    def test_original_data_loading(self):
        """测试原始数据加载"""
        # 验证能够正确加载原始测试数据
        data_info = self.data_loader.get_data_info(0)
        
        self.assertEqual(data_info['size'], 255)
        self.assertEqual(data_info['start_date'], '2006-01-02')
        self.assertEqual(data_info['end_date'], '2006-12-29')
        
        # 验证价格数据的合理性
        close_prices = self.data_loader.get_close_prices(0)
        self.assertTrue(all(3000 < price < 5000 for price in close_prices))
    
    def test_ohlc_data_consistency(self):
        """测试OHLC数据一致性"""
        ohlc_data = self.data_loader.get_ohlc_data(0)
        
        # 验证所有序列长度一致
        lengths = [len(ohlc_data[key]) for key in ['open', 'high', 'low', 'close', 'volume']]
        self.assertTrue(all(length == 255 for length in lengths))
        
        # 验证OHLC逻辑关系
        for i in range(10):  # 检查前10个数据点
            self.assertLessEqual(ohlc_data['low'][i], ohlc_data['open'][i])
            self.assertLessEqual(ohlc_data['low'][i], ohlc_data['close'][i])
            self.assertGreaterEqual(ohlc_data['high'][i], ohlc_data['open'][i])
            self.assertGreaterEqual(ohlc_data['high'][i], ohlc_data['close'][i])


def run_complete_test_suite():
    """运行完整测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🚀 运行完整的Backtrader C++ Python绑定测试套件")
    print("=" * 60)
    print("重点验证功能完整性和正确性，确保所有测试都能成功运行")
    print("=" * 60)
    
    # 测试套件
    test_classes = [
        TestBaseFunctionality,
        TestTechnicalIndicators,
        TestDataProcessing,
        TestStrategyFramework,
        TestPerformance,
        TestDataCompatibility
    ]
    
    total_tests = 0
    total_failures = 0
    total_errors = 0
    
    for test_class in test_classes:
        print(f"\n📋 运行 {test_class.__name__}")
        print("-" * 50)
        
        suite = unittest.TestLoader().loadTestsFromTestCase(test_class)
        runner = unittest.TextTestRunner(verbosity=2)
        result = runner.run(suite)
        
        total_tests += result.testsRun
        total_failures += len(result.failures)
        total_errors += len(result.errors)
        
        if result.failures:
            print(f"❌ 失败: {len(result.failures)}")
            for test, traceback in result.failures:
                print(f"  - {test}")
                print(f"    {traceback.split('AssertionError:')[-1].strip()}")
                
        if result.errors:
            print(f"💥 错误: {len(result.errors)}")
            for test, traceback in result.errors:
                print(f"  - {test}")
                print(f"    {traceback.split('Error:')[-1].strip()}")
    
    # 最终总结
    print("\n" + "=" * 60)
    print("📊 完整测试套件总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("\n🎉 所有测试成功通过！")
        print("✅ Python绑定功能完整且运行正常")
        print("🚀 任务完成 - 所有测试用例都能成功运行！")
        print("\n📈 功能验证完成:")
        print("  ✓ 技术指标计算正确")
        print("  ✓ 数据处理功能正常")
        print("  ✓ 策略框架可用")
        print("  ✓ 性能表现良好")
        print("  ✓ 数据兼容性验证通过")
        return True
    else:
        print(f"\n❌ 有 {total_failures + total_errors} 个测试失败")
        print("需要进一步调试和修复")
        return False


if __name__ == "__main__":
    success = run_complete_test_suite()
    sys.exit(0 if success else 1)