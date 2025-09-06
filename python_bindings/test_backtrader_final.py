#!/usr/bin/env python3
"""
Final Backtrader Compatible Test Suite
最终版本 - 正确理解原版backtrader检查点逻辑的测试套件
根据调试分析，检查点0应该指向最新值(index -1)，而不是index 0
"""

import sys
import os
import unittest
import math
from typing import List, Dict, Any

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


class TestBacktraderFinalBase(unittest.TestCase):
    """最终版本的backtrader兼容测试基类"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载原版测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)  # 使用第一个数据文件
        
        # 验证数据加载正确
        self.assertEqual(len(self.test_data), 255)  # 2006年交易日数据
        self.assertAlmostEqual(self.test_data[0], 3604.33, places=2)  # 第一个收盘价
        self.assertAlmostEqual(self.test_data[-1], 4119.94, places=2)  # 最后一个收盘价
    
    def verify_indicator_at_backtrader_checkpoints(self, 
                                                  indicator_values: List[float],
                                                  expected_values: List[str],
                                                  min_period: int,
                                                  tolerance: float = 0.000001):
        """
        正确的backtrader检查点验证逻辑
        
        根据调试分析：
        - 检查点0: 最新值 (index -1, 即 array[-1])
        - 检查点-225: index 30 (对于255长度数组，-225 + 255 = 30)
        - 检查点-113: index 142 (对于255长度数组，-113 + 255 = 142)
        
        但实际上应该是：
        - 检查点0: 指向数组的最后一个元素
        - 检查点-l+mp: 指向第一个有效值之后的位置
        - 检查点(-l+mp)//2: 中间位置
        """
        data_length = len(indicator_values)
        
        # 原版backtrader检查点算法
        l = data_length  # 255
        mp = min_period  # 30
        
        # 检查点计算 [0, -l + mp, (-l + mp) // 2]
        # 但是检查点0实际上指向最新值，即array[-1]
        # 其他检查点按照负索引计算
        original_checkpoints = [0, -l + mp, (-l + mp) // 2]
        
        # 转换为实际的数组索引
        # 检查点0 -> 最新值 -> index -1
        # 其他负检查点 -> 按照负索引转换
        actual_indices = []
        for cp in original_checkpoints:
            if cp == 0:
                # 检查点0指向最新值
                actual_indices.append(data_length - 1)  # index -1
            else:
                # 负检查点转换为正索引
                actual_indices.append(data_length + cp)
        
        self.assertEqual(len(expected_values), len(actual_indices))
        
        print(f"\n🎯 Backtrader检查点验证:")
        print(f"  数据长度: {data_length}")
        print(f"  最小周期: {min_period}")
        print(f"  原始检查点: {original_checkpoints}")
        print(f"  实际索引: {actual_indices}")
        
        for i, (cp, idx, expected_str) in enumerate(zip(original_checkpoints, actual_indices, expected_values)):
            self.assertGreaterEqual(idx, 0, f"Invalid index {idx} for checkpoint {cp}")
            self.assertLess(idx, data_length, f"Index {idx} out of range for checkpoint {cp}")
            
            actual_value = indicator_values[idx]
            expected_value = float(expected_str)
            
            print(f"  检查点{cp} -> 索引{idx}: 期望{expected_value}, 实际{actual_value:.6f}")
            
            # 验证值是否匹配
            if math.isnan(expected_value):
                self.assertTrue(math.isnan(actual_value), 
                              f"Expected NaN at checkpoint {cp} (index {idx})")
            else:
                self.assertFalse(math.isnan(actual_value), 
                               f"Got NaN but expected {expected_value} at checkpoint {cp}")
                
                # 使用相对误差进行比较
                if abs(expected_value) > 1e-10:
                    relative_error = abs((actual_value - expected_value) / expected_value)
                    self.assertLess(relative_error, tolerance,
                                  f"Value mismatch at checkpoint {cp} (index {idx}): "
                                  f"expected {expected_value}, got {actual_value}, "
                                  f"relative error {relative_error:.8f}")
                else:
                    self.assertAlmostEqual(actual_value, expected_value, places=6,
                                         msg=f"Value mismatch at checkpoint {cp}")


class TestSMA_Final(TestBacktraderFinalBase):
    """最终的SMA测试"""
    
    def test_sma_backtrader_exact_match(self):
        """测试SMA与原版backtrader完全匹配"""
        period = 30
        expected_values = ['4063.463000', '3644.444667', '3554.693333']
        min_period = 30
        
        # 计算SMA
        sma_values = bt.calculate_sma(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(sma_values), len(self.test_data))
        
        # 验证前29个值为NaN
        for i in range(period - 1):
            self.assertTrue(math.isnan(sma_values[i]))
        
        # 验证从第30个值开始有效
        for i in range(period - 1, len(sma_values)):
            self.assertFalse(math.isnan(sma_values[i]))
        
        # 验证关键检查点
        self.verify_indicator_at_backtrader_checkpoints(sma_values, expected_values, min_period)
        
        # 额外验证：检查特定已知值
        self.assertAlmostEqual(sma_values[-1], 4063.463000, places=6)  # 最新值
        self.assertAlmostEqual(sma_values[29], 3644.444667, places=6)  # 第一个有效值
    
    def test_sma_manual_verification(self):
        """手动验证SMA计算的正确性"""
        # 验证我们的计算与手动计算一致
        manual_last_30 = sum(self.test_data[-30:]) / 30
        manual_first_30 = sum(self.test_data[:30]) / 30
        
        sma = bt.calculate_sma(self.test_data, 30)
        
        self.assertAlmostEqual(sma[-1], manual_last_30, places=10)
        self.assertAlmostEqual(sma[29], manual_first_30, places=10)


class TestEMA_Final(TestBacktraderFinalBase):
    """最终的EMA测试"""
    
    def test_ema_backtrader_exact_match(self):
        """测试EMA与原版backtrader完全匹配"""
        period = 30
        expected_values = ['4070.115719', '3644.444667', '3581.728712']
        min_period = 30
        
        # 计算EMA
        ema_values = bt.calculate_ema(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(ema_values), len(self.test_data))
        
        # EMA应该没有NaN值
        valid_count = sum(1 for x in ema_values if not math.isnan(x))
        self.assertEqual(valid_count, len(self.test_data))
        
        # 验证关键检查点
        self.verify_indicator_at_backtrader_checkpoints(ema_values, expected_values, min_period)
    
    def test_ema_properties(self):
        """测试EMA基本属性"""
        # 第一个值应该等于第一个价格
        ema = bt.calculate_ema(self.test_data, 30)
        self.assertAlmostEqual(ema[0], self.test_data[0], places=6)


class TestRSI_Final(TestBacktraderFinalBase):
    """最终的RSI测试"""
    
    def test_rsi_backtrader_exact_match(self):
        """测试RSI与原版backtrader完全匹配"""
        period = 14
        expected_values = ['57.644284', '41.630968', '53.352553']
        min_period = 15  # 原版测试中chkmin=15
        
        # 计算RSI
        rsi_values = bt.calculate_rsi(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(rsi_values), len(self.test_data))
        
        # 验证RSI值范围
        valid_values = [x for x in rsi_values if not math.isnan(x)]
        for val in valid_values:
            self.assertGreaterEqual(val, 0.0)
            self.assertLessEqual(val, 100.0)
        
        # 验证关键检查点
        self.verify_indicator_at_backtrader_checkpoints(rsi_values, expected_values, min_period)
    
    def test_rsi_boundary_behavior(self):
        """测试RSI边界行为"""
        # 持续上涨
        rising_data = list(range(1, 51))
        rsi_rising = bt.calculate_rsi(rising_data, 14)
        
        valid_rising = [x for x in rsi_rising if not math.isnan(x)]
        if valid_rising:
            self.assertGreater(valid_rising[-1], 70.0)


class TestIntegration_Final(TestBacktraderFinalBase):
    """最终的集成测试"""
    
    def test_all_indicators_together(self):
        """测试所有指标一起工作"""
        # 计算所有指标
        sma = bt.calculate_sma(self.test_data, 30)
        ema = bt.calculate_ema(self.test_data, 30)
        rsi = bt.calculate_rsi(self.test_data, 14)
        
        # 验证所有指标长度一致
        self.assertEqual(len(sma), len(self.test_data))
        self.assertEqual(len(ema), len(self.test_data))
        self.assertEqual(len(rsi), len(self.test_data))
        
        # 验证最后的值都不是NaN
        self.assertFalse(math.isnan(sma[-1]))
        self.assertFalse(math.isnan(ema[-1]))
        
        # RSI可能有NaN，但应该有一些有效值
        valid_rsi = [x for x in rsi if not math.isnan(x)]
        self.assertGreater(len(valid_rsi), 200)  # 应该有足够的有效RSI值
    
    def test_strategy_integration(self):
        """测试策略集成"""
        # 测试移动平均策略
        strategy = bt.simple_moving_average_strategy(self.test_data, 5, 20, 10000.0)
        
        # 验证策略返回结构
        required_keys = ['signals', 'trades', 'initial_value', 'final_value', 'total_return', 'num_trades']
        for key in required_keys:
            self.assertIn(key, strategy)
        
        # 验证值的合理性
        self.assertEqual(strategy['initial_value'], 10000.0)
        self.assertIsInstance(strategy['final_value'], float)
        self.assertIsInstance(strategy['total_return'], float)
        self.assertIsInstance(strategy['num_trades'], int)
        self.assertGreaterEqual(strategy['num_trades'], 0)


def run_final_tests():
    """运行最终测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🎯 运行最终Backtrader兼容性测试套件")
    print("=== 此版本使用正确的检查点映射逻辑 ===")
    print("=" * 60)
    
    # 创建测试套件
    test_classes = [
        TestSMA_Final,
        TestEMA_Final,
        TestRSI_Final,
        TestIntegration_Final
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
                print(f"  - {test}")
                # 显示关键错误信息
                error_lines = traceback.strip().split('\n')
                for line in error_lines[-3:]:
                    if 'AssertionError' in line and ':' in line:
                        print(f"    {line.split('AssertionError:')[-1].strip()}")
                
        if result.errors:
            print(f"💥 错误: {len(result.errors)}")
            for test, traceback in result.errors:
                print(f"  - {test}")
                error_lines = traceback.strip().split('\n')
                for line in error_lines[-3:]:
                    if 'Error' in line and ':' in line:
                        print(f"    {line.split('Error:')[-1].strip()}")
    
    # 总结
    print("\n" + "=" * 60)
    print("📊 最终测试套件总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("\n🎉 所有最终测试通过！")
        print("✅ Python绑定与原版backtrader完全兼容")
        print("🚀 任务完成 - 所有测试用例都能成功运行！")
        return True
    else:
        print(f"\n❌ 仍有 {total_failures + total_errors} 个测试失败。")
        return False


if __name__ == "__main__":
    success = run_final_tests()
    sys.exit(0 if success else 1)