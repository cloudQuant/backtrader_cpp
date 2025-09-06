#!/usr/bin/env python3
"""
Backtrader Correct Test Suite
正确理解原版backtrader测试逻辑的测试套件
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


class TestBacktraderCorrectBase(unittest.TestCase):
    """正确理解backtrader逻辑的测试基类"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载原版测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)  # 使用第一个数据文件
        
        # 验证数据加载正确
        self.assertEqual(len(self.test_data), 255)  # 2006年交易日数据
        self.assertAlmostEqual(self.test_data[0], 3604.33, places=2)  # 第一个收盘价
        self.assertAlmostEqual(self.test_data[-1], 4119.94, places=2)  # 最后一个收盘价
    
    def verify_backtrader_checkpoints(self, 
                                    indicator_values: List[float],
                                    expected_values: List[str],
                                    min_period: int):
        """
        根据原版Python测试输出验证检查点
        检查点逻辑:
        - 检查点[0, -225, -113]对应原版测试中的具体值
        - 检查点0: 最新值 (index -1)
        - 检查点-225: 第一个有效值后某个位置
        - 检查点-113: 中间位置
        """
        data_length = len(indicator_values)
        
        # 原版测试的检查点计算
        # chkpts = [0, -l + mp, (-l + mp) // 2]
        # 其中l是指标长度，mp是最小周期
        l = data_length  # 255
        mp = min_period  # 30
        
        # 按照原版算法计算检查点
        check_points = [
            0,                      # 最新值
            -l + mp,               # -255 + 30 = -225
            (-l + mp) // 2         # (-255 + 30) // 2 = -225 // 2 = -113 (向下取整)
        ]
        
        print(f"\nBacktrader检查点验证:")
        print(f"  数据长度: {l}")
        print(f"  最小周期: {mp}")
        print(f"  计算的检查点: {check_points}")
        
        # 验证检查点数量
        self.assertEqual(len(expected_values), len(check_points))
        
        for i, (cp, expected_str) in enumerate(zip(check_points, expected_values)):
            # 将负索引转换为正索引
            actual_index = cp if cp >= 0 else data_length + cp
            
            self.assertGreaterEqual(actual_index, 0, 
                                  f"检查点{cp}对应的索引{actual_index}无效")
            self.assertLess(actual_index, data_length, 
                           f"检查点{cp}对应的索引{actual_index}超出范围")
            
            actual_value = indicator_values[actual_index]
            expected_value = float(expected_str)
            
            print(f"  检查点{cp} -> 索引{actual_index}: 期望{expected_value}, 实际{actual_value:.6f}")
            
            # 验证值
            if math.isnan(expected_value):
                self.assertTrue(math.isnan(actual_value))
            else:
                # 使用适当的容差
                tolerance = 0.000001  # 6位小数精度
                self.assertAlmostEqual(actual_value, expected_value, places=6,
                                     msg=f"检查点{cp}值不匹配: 期望{expected_value}, 实际{actual_value}")


class TestSMA_Correct(TestBacktraderCorrectBase):
    """正确的SMA测试"""
    
    def test_sma_30_original_values(self):
        """测试30周期SMA，使用原版期望值"""
        period = 30
        expected_values = ['4063.463000', '3644.444667', '3554.693333']
        min_period = 30
        
        # 计算SMA
        sma_values = bt.calculate_sma(self.test_data, period)
        
        # 基本验证
        self.assertEqual(len(sma_values), len(self.test_data))
        
        # 验证前29个值为NaN
        for i in range(period - 1):
            self.assertTrue(math.isnan(sma_values[i]), 
                          f"SMA[{i}]应该是NaN")
        
        # 验证第30个值开始有效
        for i in range(period - 1, len(sma_values)):
            self.assertFalse(math.isnan(sma_values[i]), 
                           f"SMA[{i}]应该有有效值")
        
        # 验证关键检查点
        self.verify_backtrader_checkpoints(sma_values, expected_values, min_period)
        
        # 额外验证：检查特定值
        # 根据原版测试，第一个有效值应该在索引29（第30个值）
        first_valid_index = 29
        first_valid_expected = 3644.444667
        self.assertAlmostEqual(sma_values[first_valid_index], first_valid_expected, places=6)
        
        # 最后一个值
        last_value_expected = 4063.463000
        self.assertAlmostEqual(sma_values[-1], last_value_expected, places=6)
    
    def test_sma_calculation_verification(self):
        """验证SMA计算的正确性"""
        # 使用简单数据验证算法
        simple_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        period = 3
        
        sma = bt.calculate_sma(simple_data, period)
        
        # 验证计算正确性
        self.assertTrue(math.isnan(sma[0]))
        self.assertTrue(math.isnan(sma[1]))
        self.assertAlmostEqual(sma[2], 2.0, places=6)  # (1+2+3)/3
        self.assertAlmostEqual(sma[3], 3.0, places=6)  # (2+3+4)/3
        self.assertAlmostEqual(sma[4], 4.0, places=6)  # (3+4+5)/3


class TestEMA_Correct(TestBacktraderCorrectBase):
    """正确的EMA测试"""
    
    def test_ema_30_original_values(self):
        """测试30周期EMA，使用原版期望值"""
        period = 30
        expected_values = ['4070.115719', '3644.444667', '3581.728712']
        min_period = 30
        
        # 计算EMA
        ema_values = bt.calculate_ema(self.test_data, period)
        
        # 基本验证
        self.assertEqual(len(ema_values), len(self.test_data))
        
        # EMA应该没有NaN值
        valid_count = sum(1 for x in ema_values if not math.isnan(x))
        self.assertEqual(valid_count, len(self.test_data))
        
        # 验证关键检查点
        self.verify_backtrader_checkpoints(ema_values, expected_values, min_period)
    
    def test_ema_properties(self):
        """测试EMA的基本属性"""
        # 恒定价格应该收敛
        constant_data = [100.0] * 50
        ema = bt.calculate_ema(constant_data, 10)
        
        self.assertAlmostEqual(ema[0], 100.0, places=6)  # 第一个值
        self.assertAlmostEqual(ema[-1], 100.0, places=6)  # 收敛值


class TestRSI_Correct(TestBacktraderCorrectBase):
    """正确的RSI测试"""
    
    def test_rsi_14_original_values(self):
        """测试14周期RSI，使用原版期望值"""
        period = 14
        expected_values = ['57.644284', '41.630968', '53.352553']
        min_period = 15  # 原版测试chkmin=15
        
        # 计算RSI
        rsi_values = bt.calculate_rsi(self.test_data, period)
        
        # 基本验证
        self.assertEqual(len(rsi_values), len(self.test_data))
        
        # 验证RSI值范围
        valid_values = [x for x in rsi_values if not math.isnan(x)]
        for val in valid_values:
            self.assertGreaterEqual(val, 0.0)
            self.assertLessEqual(val, 100.0)
        
        # 验证关键检查点
        self.verify_backtrader_checkpoints(rsi_values, expected_values, min_period)
    
    def test_rsi_extreme_cases(self):
        """测试RSI极端情况"""
        # 持续上涨
        rising_data = list(range(1, 51))
        rsi_rising = bt.calculate_rsi(rising_data, 14)
        
        valid_rising = [x for x in rsi_rising if not math.isnan(x)]
        if valid_rising:
            # 应该趋向高值
            self.assertGreater(valid_rising[-1], 50.0)
        
        # 持续下跌
        falling_data = list(range(50, 0, -1))
        rsi_falling = bt.calculate_rsi(falling_data, 14)
        
        valid_falling = [x for x in rsi_falling if not math.isnan(x)]
        if valid_falling:
            # 应该趋向低值
            self.assertLess(valid_falling[-1], 50.0)


class TestDataConsistency_Correct(TestBacktraderCorrectBase):
    """数据一致性测试"""
    
    def test_data_integrity(self):
        """验证数据完整性"""
        # 验证数据大小和范围
        self.assertEqual(len(self.test_data), 255)
        self.assertTrue(all(x > 0 for x in self.test_data))
        self.assertTrue(all(not math.isnan(x) for x in self.test_data))
        
        # 验证关键数据点（来自原版测试输出）
        self.assertAlmostEqual(self.test_data[0], 3604.33, places=2)    # 第一天
        self.assertAlmostEqual(self.test_data[29], 3695.63, places=2)   # 第30天
        self.assertAlmostEqual(self.test_data[-1], 4119.94, places=2)   # 最后一天
    
    def test_algorithm_consistency(self):
        """验证算法一致性"""
        # 测试不同数据规模
        for size in [50, 100, 200]:
            if size <= len(self.test_data):
                subset = self.test_data[:size]
                
                sma = bt.calculate_sma(subset, 10)
                ema = bt.calculate_ema(subset, 10)
                rsi = bt.calculate_rsi(subset, 14)
                
                self.assertEqual(len(sma), size)
                self.assertEqual(len(ema), size)
                self.assertEqual(len(rsi), size)


def run_correct_tests():
    """运行正确的测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🎯 运行正确理解的Backtrader测试套件")
    print("=" * 60)
    
    test_classes = [
        TestSMA_Correct,
        TestEMA_Correct,
        TestRSI_Correct,
        TestDataConsistency_Correct
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
                print(f"    {traceback.split('AssertionError:')[-1].strip()}")
                
        if result.errors:
            print(f"💥 错误: {len(result.errors)}")
            for test, traceback in result.errors:
                print(f"  - {test}")
                print(f"    {traceback.split('Error:')[-1].strip()}")
    
    # 总结
    print("\n" + "=" * 60)
    print("📊 正确测试套件总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("\n🎉 所有正确测试通过！")
        print("✅ Python绑定与原版backtrader完全匹配")
        return True
    else:
        print("\n❌ 仍有测试失败，需要进一步分析算法差异。")
        return False


if __name__ == "__main__":
    success = run_correct_tests()
    sys.exit(0 if success else 1)