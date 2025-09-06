#!/usr/bin/env python3
"""
Backtrader Compatible Test Suite - Fixed Version
修正的与原版backtrader测试用例完全兼容的测试套件
正确处理检查点映射和期望值验证
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


class TestBacktraderFixedBase(unittest.TestCase):
    """修正的backtrader兼容测试基类"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载原版测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)  # 使用第一个数据文件
        
        # 验证数据加载正确
        self.assertEqual(len(self.test_data), 255)  # 2006年交易日数据
        self.assertAlmostEqual(self.test_data[0], 3604.33, places=2)  # 第一个收盘价
        self.assertAlmostEqual(self.test_data[-1], 4119.94, places=2)  # 最后一个收盘价
    
    def verify_indicator_values_backtrader_style(self, 
                                                indicator_values: List[float],
                                                expected_values: List[str],
                                                min_period: int,
                                                tolerance: float = 0.000001):
        """
        按照backtrader原版逻辑验证指标值
        检查点: [0, -l + mp, (-l + mp) // 2]
        这里的l是指标的有效长度，mp是最小周期
        """
        # 找到有效值（非NaN）
        valid_indices = []
        valid_values = []
        
        for i, val in enumerate(indicator_values):
            if not math.isnan(val):
                valid_indices.append(i)
                valid_values.append(val)
        
        if len(valid_values) == 0:
            self.fail("No valid values found in indicator")
        
        # 计算有效数据的长度
        valid_length = len(valid_values)
        
        # 按照原版算法计算检查点
        # 注意：这里的检查点是相对于有效数据的
        check_points = [
            0,                                    # 最新值 (有效数据的最后一个)
            -valid_length + min_period,          # 第一个有效值之后min_period位置
            (-valid_length + min_period) // 2    # 中间位置
        ]
        
        self.assertEqual(len(expected_values), len(check_points), 
                        "Expected values count must match check points count")
        
        print(f"\nDebug Info:")
        print(f"  Total data length: {len(indicator_values)}")
        print(f"  Valid values: {valid_length}")
        print(f"  Min period: {min_period}")
        print(f"  Valid indices range: {valid_indices[0]} to {valid_indices[-1]}")
        print(f"  Check points (relative to valid data): {check_points}")
        
        for i, (check_point, expected_str) in enumerate(zip(check_points, expected_values)):
            # 转换检查点到有效值数组的索引
            if check_point >= 0:
                valid_index = check_point
            else:
                valid_index = valid_length + check_point
            
            self.assertGreaterEqual(valid_index, 0, 
                                  f"Invalid valid_index {valid_index} for check point {check_point}")
            self.assertLess(valid_index, valid_length, 
                           f"valid_index {valid_index} out of range for check point {check_point}")
            
            actual_value = valid_values[valid_index]
            expected_value = float(expected_str)
            
            print(f"  Check point {check_point} -> valid_index {valid_index}: "
                  f"expected {expected_value}, got {actual_value:.6f}")
            
            # 验证值是否匹配
            if math.isnan(expected_value):
                self.assertTrue(math.isnan(actual_value), 
                              f"Expected NaN at check point {check_point}")
            else:
                self.assertFalse(math.isnan(actual_value), 
                               f"Got NaN but expected {expected_value} at check point {check_point}")
                
                # 使用相对误差进行比较
                if abs(expected_value) > 1e-10:
                    relative_error = abs((actual_value - expected_value) / expected_value)
                    self.assertLess(relative_error, tolerance,
                                  f"Value mismatch at check point {check_point}: "
                                  f"expected {expected_value}, got {actual_value}, "
                                  f"relative error {relative_error:.8f}")
                else:
                    self.assertAlmostEqual(actual_value, expected_value, places=6,
                                         msg=f"Value mismatch at check point {check_point}")


class TestSMA_Fixed(TestBacktraderFixedBase):
    """修正的SMA测试"""
    
    def test_sma_with_original_expected_values(self):
        """使用原版期望值测试SMA"""
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
        valid_count = 0
        for i in range(period - 1, len(sma_values)):
            if not math.isnan(sma_values[i]):
                valid_count += 1
        
        expected_valid_count = len(self.test_data) - period + 1
        self.assertEqual(valid_count, expected_valid_count)
        
        # 使用修正的验证方法
        self.verify_indicator_values_backtrader_style(sma_values, expected_values, min_period)
    
    def test_sma_manual_calculation_check(self):
        """手动验证SMA计算（使用原版数据的一部分）"""
        # 取原版数据的前50个点进行手动验证
        test_subset = self.test_data[:50]
        period = 5
        
        sma = bt.calculate_sma(test_subset, period)
        
        # 手动计算第5个SMA值（index 4）
        manual_sma_5 = sum(test_subset[:5]) / 5
        self.assertAlmostEqual(sma[4], manual_sma_5, places=6)
        
        # 手动计算第10个SMA值（index 9）
        manual_sma_10 = sum(test_subset[5:10]) / 5
        self.assertAlmostEqual(sma[9], manual_sma_10, places=6)


class TestEMA_Fixed(TestBacktraderFixedBase):
    """修正的EMA测试"""
    
    def test_ema_with_original_expected_values(self):
        """使用原版期望值测试EMA"""
        period = 30
        expected_values = ['4070.115719', '3644.444667', '3581.728712']
        min_period = 30
        
        # 计算EMA
        ema_values = bt.calculate_ema(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(ema_values), len(self.test_data))
        
        # EMA应该从第一个值开始就有有效值
        valid_count = sum(1 for x in ema_values if not math.isnan(x))
        self.assertEqual(valid_count, len(self.test_data))
        
        # 使用修正的验证方法
        self.verify_indicator_values_backtrader_style(ema_values, expected_values, min_period)
    
    def test_ema_smoothing_behavior(self):
        """测试EMA平滑行为"""
        # 使用恒定价格测试
        constant_data = [100.0] * 50
        ema = bt.calculate_ema(constant_data, 10)
        
        # 应该收敛到恒定价格
        self.assertAlmostEqual(ema[-1], 100.0, places=3)
        
        # 第一个值应该等于第一个价格
        self.assertAlmostEqual(ema[0], 100.0, places=6)


class TestRSI_Fixed(TestBacktraderFixedBase):
    """修正的RSI测试"""
    
    def test_rsi_with_original_expected_values(self):
        """使用原版期望值测试RSI"""
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
        
        # 使用修正的验证方法
        self.verify_indicator_values_backtrader_style(rsi_values, expected_values, min_period)
    
    def test_rsi_boundary_values(self):
        """测试RSI边界值"""
        # 持续上涨
        rising_data = list(range(1, 101))
        rsi_rising = bt.calculate_rsi(rising_data, 14)
        
        valid_rising = [x for x in rsi_rising if not math.isnan(x)]
        if valid_rising:
            self.assertGreater(valid_rising[-1], 70.0)  # 应该是超买
        
        # 持续下跌
        falling_data = list(range(100, 0, -1))
        rsi_falling = bt.calculate_rsi(falling_data, 14)
        
        valid_falling = [x for x in rsi_falling if not math.isnan(x)]
        if valid_falling:
            self.assertLess(valid_falling[-1], 30.0)  # 应该是超卖


class TestDataIntegrity_Fixed(TestBacktraderFixedBase):
    """数据完整性测试"""
    
    def test_data_matches_original(self):
        """确保我们使用的数据与原版一致"""
        # 检查关键数据点
        self.assertAlmostEqual(self.test_data[0], 3604.33, places=2)
        self.assertAlmostEqual(self.test_data[29], 3626.93, places=2)  # 第30个数据点
        self.assertAlmostEqual(self.test_data[-1], 4119.94, places=2)
        
        # 检查数据完整性
        self.assertEqual(len(self.test_data), 255)
        self.assertTrue(all(not math.isnan(x) for x in self.test_data))
        self.assertTrue(all(x > 0 for x in self.test_data))
    
    def test_indicator_output_format(self):
        """测试指标输出格式"""
        sma = bt.calculate_sma(self.test_data, 10)
        ema = bt.calculate_ema(self.test_data, 10)
        rsi = bt.calculate_rsi(self.test_data, 14)
        
        # 验证输出长度
        self.assertEqual(len(sma), len(self.test_data))
        self.assertEqual(len(ema), len(self.test_data))
        self.assertEqual(len(rsi), len(self.test_data))
        
        # 验证输出类型
        self.assertIsInstance(sma, list)
        self.assertIsInstance(ema, list)
        self.assertIsInstance(rsi, list)


def run_fixed_tests():
    """运行修正的测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🔧 运行修正的Backtrader兼容测试套件")
    print("=" * 60)
    
    # 创建测试套件
    test_classes = [
        TestSMA_Fixed,
        TestEMA_Fixed,
        TestRSI_Fixed,
        TestDataIntegrity_Fixed
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
    print("📊 修正测试套件总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("\n🎉 所有修正测试通过！")
        print("✅ Python绑定与原版backtrader算法一致")
        return True
    else:
        print("\n❌ 部分测试仍然失败，需要进一步调整。")
        return False


if __name__ == "__main__":
    success = run_fixed_tests()
    sys.exit(0 if success else 1)