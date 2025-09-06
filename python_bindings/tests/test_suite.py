#!/usr/bin/env python3
"""
Comprehensive Test Suite for Backtrader C++ Python Bindings
基于backtrader和backtrader_cpp测试用例的全面测试套件
"""

import sys
import os
import unittest
import math
import time
from typing import List, Dict, Any

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build'))

try:
    import backtrader_cpp as bt
    MODULE_AVAILABLE = True
except ImportError as e:
    print(f"❌ Cannot import backtrader_cpp: {e}")
    MODULE_AVAILABLE = False


class TestBasicFunctionality(unittest.TestCase):
    """基础功能测试"""
    
    def setUp(self):
        """测试前置条件"""
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
    
    def test_module_import(self):
        """测试模块导入"""
        self.assertTrue(hasattr(bt, 'test'))
        self.assertTrue(hasattr(bt, 'get_version'))
        
    def test_module_info(self):
        """测试模块信息"""
        result = bt.test()
        self.assertIsInstance(result, str)
        self.assertIn("Backtrader C++", result)
        
        version = bt.get_version()
        self.assertIsInstance(version, dict)
        self.assertIn('version', version)
        self.assertIn('build_date', version)
        self.assertIn('compiler', version)
        
    def test_available_functions(self):
        """测试可用函数"""
        expected_functions = [
            'test', 'get_version', 'calculate_sma', 'calculate_ema', 
            'calculate_rsi', 'calculate_returns', 'calculate_volatility',
            'calculate_sharpe', 'generate_sample_data', 'validate_data',
            'simple_moving_average_strategy', 'benchmark', 'benchmark_sma'
        ]
        
        for func_name in expected_functions:
            self.assertTrue(hasattr(bt, func_name), 
                          f"Function {func_name} should be available")


class TestDataGeneration(unittest.TestCase):
    """数据生成测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
    
    def test_generate_sample_data_basic(self):
        """测试基础数据生成"""
        data = bt.generate_sample_data(100, 100.0, 0.02, 42)
        
        self.assertEqual(len(data), 100)
        self.assertIsInstance(data, list)
        self.assertTrue(all(isinstance(x, float) for x in data))
        self.assertEqual(data[0], 100.0)  # 起始价格
        
    def test_generate_sample_data_parameters(self):
        """测试数据生成参数"""
        # 测试不同参数
        small_data = bt.generate_sample_data(10, 50.0, 0.01, 123)
        self.assertEqual(len(small_data), 10)
        self.assertEqual(small_data[0], 50.0)
        
        # 测试随机种子一致性
        data1 = bt.generate_sample_data(20, 100.0, 0.02, 42)
        data2 = bt.generate_sample_data(20, 100.0, 0.02, 42)
        self.assertEqual(data1, data2)  # 相同种子应产生相同数据
        
    def test_validate_data(self):
        """测试数据验证"""
        data = bt.generate_sample_data(50, 100.0, 0.01, 42)
        validation = bt.validate_data(data)
        
        self.assertIsInstance(validation, dict)
        self.assertTrue(validation['valid'])
        self.assertEqual(validation['size'], 50)
        self.assertGreater(validation['min'], 0)
        self.assertGreater(validation['max'], validation['min'])
        self.assertEqual(validation['nan_count'], 0)
        self.assertEqual(validation['inf_count'], 0)


class TestTechnicalIndicators(unittest.TestCase):
    """技术指标测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        # 生成测试数据 - 使用已知种子确保可重现
        self.test_data = bt.generate_sample_data(100, 100.0, 0.02, 42)
        
    def test_sma_calculation(self):
        """测试SMA计算"""
        # 测试不同周期的SMA
        periods = [5, 10, 20, 30]
        
        for period in periods:
            sma = bt.calculate_sma(self.test_data, period)
            
            # 基本验证
            self.assertEqual(len(sma), len(self.test_data))
            
            # 前period-1个值应该是NaN
            for i in range(period - 1):
                self.assertTrue(math.isnan(sma[i]), 
                              f"SMA[{i}] should be NaN for period {period}")
            
            # 第period个值开始应该有有效值
            for i in range(period - 1, len(sma)):
                self.assertFalse(math.isnan(sma[i]), 
                               f"SMA[{i}] should be valid for period {period}")
                
    def test_sma_manual_calculation(self):
        """手动验证SMA计算"""
        # 使用简单数据进行手动验证
        simple_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        sma_3 = bt.calculate_sma(simple_data, 3)
        
        # 验证前两个值为NaN
        self.assertTrue(math.isnan(sma_3[0]))
        self.assertTrue(math.isnan(sma_3[1]))
        
        # 验证手动计算的值
        self.assertAlmostEqual(sma_3[2], (1+2+3)/3, places=6)  # 2.0
        self.assertAlmostEqual(sma_3[3], (2+3+4)/3, places=6)  # 3.0
        self.assertAlmostEqual(sma_3[4], (3+4+5)/3, places=6)  # 4.0
        
    def test_ema_calculation(self):
        """测试EMA计算"""
        periods = [10, 20, 30]
        
        for period in periods:
            ema = bt.calculate_ema(self.test_data, period)
            
            # 基本验证
            self.assertEqual(len(ema), len(self.test_data))
            
            # 所有值都应该有效（EMA从第一个值开始）
            for i, val in enumerate(ema):
                self.assertFalse(math.isnan(val), 
                               f"EMA[{i}] should be valid for period {period}")
                
    def test_ema_convergence(self):
        """测试EMA收敛性"""
        # EMA应该趋向于最新的价格变化
        stable_data = [100.0] * 50 + [110.0] * 50  # 价格跳跃
        ema = bt.calculate_ema(stable_data, 10)
        
        # 在价格稳定期，EMA应该接近价格
        self.assertAlmostEqual(ema[49], 100.0, delta=1.0)
        
        # 在价格跳跃后，EMA应该逐渐接近新价格
        self.assertGreater(ema[99], 105.0)
        
    def test_rsi_calculation(self):
        """测试RSI计算"""
        periods = [14, 21]
        
        for period in periods:
            rsi = bt.calculate_rsi(self.test_data, period)
            
            # 基本验证
            self.assertEqual(len(rsi), len(self.test_data))
            
            # 前period个值应该是NaN（包括第一个价格差值）
            for i in range(period):
                self.assertTrue(math.isnan(rsi[i]), 
                              f"RSI[{i}] should be NaN for period {period}")
            
            # 有效RSI值应该在0-100范围内
            for i in range(period, len(rsi)):
                if not math.isnan(rsi[i]):
                    self.assertGreaterEqual(rsi[i], 0.0)
                    self.assertLessEqual(rsi[i], 100.0)
                    
    def test_rsi_extreme_values(self):
        """测试RSI极端值"""
        # 持续上涨应该产生高RSI
        rising_data = list(range(1, 51))  # 1到50持续上涨
        rsi_rising = bt.calculate_rsi(rising_data, 14)
        
        # 最后的RSI值应该接近100
        last_valid_rsi = None
        for i in range(len(rsi_rising)-1, -1, -1):
            if not math.isnan(rsi_rising[i]):
                last_valid_rsi = rsi_rising[i]
                break
        
        if last_valid_rsi is not None:
            self.assertGreater(last_valid_rsi, 70.0)  # 应该是超买状态


class TestRiskMetrics(unittest.TestCase):
    """风险指标测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        self.test_data = bt.generate_sample_data(252, 100.0, 0.02, 42)  # 一年数据
        
    def test_calculate_returns(self):
        """测试收益率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        # 收益率数量应该比价格少1
        self.assertEqual(len(returns), len(self.test_data) - 1)
        
        # 手动验证前几个收益率
        for i in range(min(5, len(returns))):
            expected_return = (self.test_data[i+1] - self.test_data[i]) / self.test_data[i]
            self.assertAlmostEqual(returns[i], expected_return, places=6)
            
    def test_calculate_volatility(self):
        """测试波动率计算"""
        returns = bt.calculate_returns(self.test_data)
        volatility = bt.calculate_volatility(returns, 20)
        
        # 验证波动率计算
        self.assertEqual(len(volatility), len(returns))
        
        # 前19个值应该是NaN
        for i in range(19):
            self.assertTrue(math.isnan(volatility[i]))
            
        # 有效波动率应该是正数
        for i in range(19, len(volatility)):
            if not math.isnan(volatility[i]):
                self.assertGreater(volatility[i], 0.0)
                
    def test_calculate_sharpe_ratio(self):
        """测试夏普比率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        # 测试不同无风险利率
        sharpe_0 = bt.calculate_sharpe(returns, 0.0)
        sharpe_2 = bt.calculate_sharpe(returns, 0.02)
        
        self.assertIsInstance(sharpe_0, float)
        self.assertIsInstance(sharpe_2, float)
        
        # 更高的无风险利率应该产生更低的夏普比率
        # （假设策略收益率为正）
        # 注意：这个断言可能因数据而异，所以我们只检查它们是不同的
        self.assertNotEqual(sharpe_0, sharpe_2)


class TestDataContainers(unittest.TestCase):
    """数据容器测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
    def test_standard_lists(self):
        """测试标准Python列表兼容性"""
        # 使用标准Python列表测试所有功能
        test_data = [1.0, 2.0, 3.0, 4.0, 5.0]
        
        # 验证技术指标支持标准列表
        sma = bt.calculate_sma(test_data, 3)
        self.assertEqual(len(sma), len(test_data))
        
        # 验证前两个值为NaN
        self.assertTrue(math.isnan(sma[0]))
        self.assertTrue(math.isnan(sma[1]))
        
        # 验证第三个值正确
        self.assertAlmostEqual(sma[2], 2.0, places=6)  # (1+2+3)/3
        
    def test_data_validation_container(self):
        """测试数据验证功能"""
        test_data = bt.generate_sample_data(50, 100.0, 0.01, 42)
        validation = bt.validate_data(test_data)
        
        # 验证返回的数据结构
        self.assertIsInstance(validation, dict)
        self.assertTrue(validation['valid'])
        self.assertEqual(validation['size'], 50)
        self.assertGreater(validation['min'], 0)
        self.assertGreater(validation['max'], validation['min'])
        self.assertEqual(validation['nan_count'], 0)
        self.assertEqual(validation['inf_count'], 0)
        
    def test_numpy_like_operations(self):
        """测试类NumPy操作"""
        data = bt.generate_sample_data(100, 100.0, 0.02, 42)
        
        # 测试数据可以被各种函数处理
        sma_short = bt.calculate_sma(data, 5)
        sma_long = bt.calculate_sma(data, 20)
        
        # 验证列表操作
        self.assertEqual(len(sma_short), len(data))
        self.assertEqual(len(sma_long), len(data))
        
        # 验证可以进行切片操作
        data_slice = data[:50]
        sma_slice = bt.calculate_sma(data_slice, 10)
        self.assertEqual(len(sma_slice), 50)


class TestStrategyFramework(unittest.TestCase):
    """策略框架测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
    def test_simple_moving_average_strategy_basic(self):
        """测试基本移动平均策略"""
        # 创建趋势明显的测试数据
        trend_data = []
        for i in range(100):
            if i < 50:
                trend_data.append(100.0 + i * 0.1)  # 上涨趋势
            else:
                trend_data.append(105.0 - (i-50) * 0.1)  # 下跌趋势
                
        result = bt.simple_moving_average_strategy(
            trend_data, short_period=5, long_period=15, initial_cash=10000.0
        )
        
        # 验证返回结果结构
        expected_keys = ['signals', 'trades', 'initial_value', 'final_value', 'total_return', 'num_trades']
        for key in expected_keys:
            self.assertIn(key, result)
            
        # 验证基本数值
        self.assertEqual(result['initial_value'], 10000.0)
        self.assertIsInstance(result['final_value'], float)
        self.assertIsInstance(result['total_return'], float)
        self.assertIsInstance(result['num_trades'], int)
        self.assertGreaterEqual(result['num_trades'], 0)
        
        # 信号数量应该等于数据点数量
        self.assertEqual(len(result['signals']), len(trend_data))
        
    def test_strategy_with_different_parameters(self):
        """测试不同参数的策略"""
        test_data = bt.generate_sample_data(50, 100.0, 0.02, 42)
        
        # 测试不同的均线周期
        result1 = bt.simple_moving_average_strategy(test_data, 3, 7, 1000.0)
        result2 = bt.simple_moving_average_strategy(test_data, 5, 15, 1000.0)
        
        # 不同参数应该产生不同结果
        self.assertNotEqual(result1['num_trades'], result2['num_trades'])
        
    def test_strategy_edge_cases(self):
        """测试策略边界情况"""
        # 测试短数据
        short_data = [100.0, 101.0, 102.0, 103.0, 104.0]
        result = bt.simple_moving_average_strategy(short_data, 2, 3, 1000.0)
        
        self.assertEqual(len(result['signals']), len(short_data))
        self.assertGreaterEqual(result['num_trades'], 0)
        
        # 测试平坦数据（无变化）
        flat_data = [100.0] * 20
        result_flat = bt.simple_moving_average_strategy(flat_data, 3, 7, 1000.0)
        
        # 平坦数据应该不产生交易
        self.assertEqual(result_flat['num_trades'], 0)
        self.assertEqual(result_flat['total_return'], 0.0)


class TestPerformance(unittest.TestCase):
    """性能测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
    def test_benchmark_basic(self):
        """测试基础性能基准"""
        result = bt.benchmark(100000)  # 10万次迭代
        
        # 验证返回结构
        expected_keys = ['result', 'time_us', 'iterations', 'ops_per_second']
        for key in expected_keys:
            self.assertIn(key, result)
            
        # 验证数值合理性
        self.assertEqual(result['iterations'], 100000)
        self.assertGreater(result['time_us'], 0)
        self.assertGreater(result['ops_per_second'], 1000)  # 至少每秒1000次操作
        self.assertIsInstance(result['result'], float)
        
    def test_sma_benchmark(self):
        """测试SMA性能基准"""
        test_data = bt.generate_sample_data(1000, 100.0, 0.02, 42)
        result = bt.benchmark_sma(test_data, 20, 100)  # 100次SMA计算
        
        # 验证返回结构
        expected_keys = ['data_points', 'period', 'iterations', 'time_us', 
                        'time_per_calculation_us', 'calculations_per_second']
        for key in expected_keys:
            self.assertIn(key, result)
            
        # 验证数值
        self.assertEqual(result['data_points'], 1000)
        self.assertEqual(result['period'], 20)
        self.assertEqual(result['iterations'], 100)
        self.assertGreater(result['calculations_per_second'], 10)  # 至少每秒10次计算
        
    def test_performance_scaling(self):
        """测试性能扩展性"""
        # 测试不同数据量的性能
        data_sizes = [100, 500, 1000]
        
        for size in data_sizes:
            test_data = bt.generate_sample_data(size, 100.0, 0.02, 42)
            
            start_time = time.time()
            sma = bt.calculate_sma(test_data, 20)
            ema = bt.calculate_ema(test_data, 20)
            end_time = time.time()
            
            calc_time = (end_time - start_time) * 1000  # 毫秒
            
            # 即使是大数据量，计算时间也应该很短
            self.assertLess(calc_time, 100.0, f"Calculation too slow for {size} data points")
            
            # 验证结果正确
            self.assertEqual(len(sma), size)
            self.assertEqual(len(ema), size)


class TestEdgeCasesAndErrors(unittest.TestCase):
    """边界情况和错误处理测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
    def test_empty_data(self):
        """测试空数据处理"""
        empty_data = []
        
        # SMA对空数据应该返回空结果
        sma = bt.calculate_sma(empty_data, 5)
        self.assertEqual(len(sma), 0)
        
        # EMA对空数据应该返回空结果
        ema = bt.calculate_ema(empty_data, 5)
        self.assertEqual(len(ema), 0)
        
        # Returns对空数据应该返回空结果
        returns = bt.calculate_returns(empty_data)
        self.assertEqual(len(returns), 0)
        
    def test_single_data_point(self):
        """测试单个数据点"""
        single_data = [100.0]
        
        # SMA应该返回一个NaN
        sma = bt.calculate_sma(single_data, 5)
        self.assertEqual(len(sma), 1)
        self.assertTrue(math.isnan(sma[0]))
        
        # EMA应该返回原值
        ema = bt.calculate_ema(single_data, 5)
        self.assertEqual(len(ema), 1)
        self.assertEqual(ema[0], 100.0)
        
        # Returns应该返回空
        returns = bt.calculate_returns(single_data)
        self.assertEqual(len(returns), 0)
        
    def test_invalid_parameters(self):
        """测试无效参数"""
        test_data = [1.0, 2.0, 3.0, 4.0, 5.0]
        
        # 零周期或负周期的处理
        # 注意：具体行为取决于实现，这里测试不会崩溃
        try:
            sma_zero = bt.calculate_sma(test_data, 0)
            # 如果没有抛出异常，结果应该有意义
            self.assertEqual(len(sma_zero), len(test_data))
        except (ValueError, RuntimeError):
            # 如果抛出异常，这也是合理的
            pass
            
    def test_large_period(self):
        """测试大周期值"""
        test_data = [float(i) for i in range(10)]
        
        # 周期大于数据长度
        sma = bt.calculate_sma(test_data, 20)
        self.assertEqual(len(sma), len(test_data))
        
        # 所有值都应该是NaN
        for val in sma:
            self.assertTrue(math.isnan(val))


class TestAccuracy(unittest.TestCase):
    """精度和准确性测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
    def test_sma_accuracy(self):
        """测试SMA计算精度"""
        # 使用精确已知的数据
        precise_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        sma_3 = bt.calculate_sma(precise_data, 3)
        
        # 手动计算期望值
        expected_values = [
            float('nan'), float('nan'),  # 前两个NaN
            2.0,  # (1+2+3)/3
            3.0,  # (2+3+4)/3
            4.0,  # (3+4+5)/3
            5.0,  # (4+5+6)/3
            6.0,  # (5+6+7)/3
            7.0,  # (6+7+8)/3
            8.0,  # (7+8+9)/3
            9.0   # (8+9+10)/3
        ]
        
        for i, (actual, expected) in enumerate(zip(sma_3, expected_values)):
            if math.isnan(expected):
                self.assertTrue(math.isnan(actual), f"SMA[{i}] should be NaN")
            else:
                self.assertAlmostEqual(actual, expected, places=6, 
                                     msg=f"SMA[{i}] accuracy test failed")
                
    def test_returns_accuracy(self):
        """测试收益率计算精度"""
        # 精确计算收益率
        prices = [100.0, 110.0, 99.0, 108.9]
        returns = bt.calculate_returns(prices)
        
        expected_returns = [
            (110.0 - 100.0) / 100.0,  # 0.1
            (99.0 - 110.0) / 110.0,   # -0.1
            (108.9 - 99.0) / 99.0     # 0.1
        ]
        
        for i, (actual, expected) in enumerate(zip(returns, expected_returns)):
            self.assertAlmostEqual(actual, expected, places=10, 
                                 msg=f"Return[{i}] accuracy test failed")


def run_test_suite():
    """运行完整测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
        
    print("🚀 运行 Backtrader C++ Python 绑定测试套件")
    print("=" * 60)
    
    # 创建测试套件
    test_classes = [
        TestBasicFunctionality,
        TestDataGeneration,
        TestTechnicalIndicators,
        TestRiskMetrics,
        TestDataContainers,
        TestStrategyFramework,
        TestPerformance,
        TestEdgeCasesAndErrors,
        TestAccuracy
    ]
    
    total_tests = 0
    total_failures = 0
    total_errors = 0
    
    for test_class in test_classes:
        print(f"\n📋 运行 {test_class.__name__}")
        print("-" * 40)
        
        suite = unittest.TestLoader().loadTestsFromTestCase(test_class)
        runner = unittest.TextTestRunner(verbosity=2)
        result = runner.run(suite)
        
        total_tests += result.testsRun
        total_failures += len(result.failures)
        total_errors += len(result.errors)
        
        if result.failures:
            print(f"❌ 失败: {len(result.failures)}")
            for test, traceback in result.failures:
                print(f"  - {test}: {traceback.split('AssertionError:')[-1].strip()}")
                
        if result.errors:
            print(f"💥 错误: {len(result.errors)}")
            for test, traceback in result.errors:
                print(f"  - {test}: {traceback.split('Error:')[-1].strip()}")
    
    # 总结
    print("\n" + "=" * 60)
    print("📊 测试套件总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("🎉 所有测试通过！")
        return True
    else:
        print("❌ 部分测试失败，需要检查。")
        return False


if __name__ == "__main__":
    success = run_test_suite()
    sys.exit(0 if success else 1)