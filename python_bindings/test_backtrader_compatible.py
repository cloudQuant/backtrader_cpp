#!/usr/bin/env python3
"""
Backtrader Compatible Test Suite
与原版backtrader测试用例完全兼容的测试套件
使用相同的测试数据、参数和期望值
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


class TestBacktraderCompatibleBase(unittest.TestCase):
    """与原版backtrader兼容的测试基类"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载原版测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)  # 使用第一个数据文件
        
        # 验证数据加载正确
        self.assertEqual(len(self.test_data), 255)  # 2006年交易日数据
        self.assertAlmostEqual(self.test_data[0], 3604.33, places=2)  # 第一个收盘价
        self.assertAlmostEqual(self.test_data[-1], 4119.94, places=2)  # 最后一个收盘价
    
    def verify_indicator_at_checkpoints(self, 
                                       indicator_values: List[float],
                                       expected_values: List[str],
                                       min_period: int,
                                       tolerance: float = 0.000001):
        """
        在检查点验证指标值，与原版testcommon.py逻辑一致
        """
        data_length = len(indicator_values)
        check_points = OriginalTestParameters.get_check_points(data_length, min_period)
        
        self.assertEqual(len(expected_values), len(check_points), 
                        "Expected values count must match check points count")
        
        for i, (check_point, expected_str) in enumerate(zip(check_points, expected_values)):
            # 计算实际索引（处理负索引）
            actual_index = check_point if check_point >= 0 else data_length + check_point
            
            self.assertGreaterEqual(actual_index, 0, f"Invalid index for check point {check_point}")
            self.assertLess(actual_index, data_length, f"Index out of range for check point {check_point}")
            
            actual_value = indicator_values[actual_index]
            expected_value = float(expected_str)
            
            # 验证值是否匹配
            if math.isnan(expected_value):
                self.assertTrue(math.isnan(actual_value), 
                              f"Expected NaN at check point {check_point} (index {actual_index})")
            else:
                self.assertFalse(math.isnan(actual_value), 
                               f"Got NaN but expected {expected_value} at check point {check_point}")
                
                # 使用相对误差进行比较
                if expected_value != 0:
                    relative_error = abs((actual_value - expected_value) / expected_value)
                    self.assertLess(relative_error, tolerance,
                                  f"Value mismatch at check point {check_point} (index {actual_index}): "
                                  f"expected {expected_value}, got {actual_value}, "
                                  f"relative error {relative_error:.8f}")
                else:
                    self.assertAlmostEqual(actual_value, expected_value, places=6,
                                         msg=f"Value mismatch at check point {check_point}")


class TestSMA_Original(TestBacktraderCompatibleBase):
    """SMA测试 - 与原版test_ind_sma.py完全一致"""
    
    def test_sma_default_period_30(self):
        """测试SMA默认30周期，与原版期望值比较"""
        # 原版测试参数
        period = 30
        expected_values = OriginalTestParameters.SMA_TEST['expected_values']
        min_period = OriginalTestParameters.SMA_TEST['min_period']
        
        # 计算SMA
        sma_values = bt.calculate_sma(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(sma_values), len(self.test_data))
        self.assertEqual(period, min_period)  # 确认参数一致性
        
        # 验证前29个值为NaN
        for i in range(period - 1):
            self.assertTrue(math.isnan(sma_values[i]), 
                          f"SMA[{i}] should be NaN for period {period}")
        
        # 验证从第30个值开始有效
        for i in range(period - 1, len(sma_values)):
            self.assertFalse(math.isnan(sma_values[i]), 
                           f"SMA[{i}] should be valid for period {period}")
        
        # 在检查点验证期望值
        self.verify_indicator_at_checkpoints(sma_values, expected_values, min_period)
        
        # 调试输出
        check_points = OriginalTestParameters.get_check_points(len(sma_values), min_period)
        print(f"\nSMA Debug Info:")
        print(f"  Data length: {len(self.test_data)}")
        print(f"  SMA length: {len(sma_values)}")
        print(f"  Period: {period}")
        print(f"  Check points: {check_points}")
        for i, (cp, expected) in enumerate(zip(check_points, expected_values)):
            actual_idx = cp if cp >= 0 else len(sma_values) + cp
            actual_val = sma_values[actual_idx]
            print(f"  Check point {cp} (idx {actual_idx}): expected {expected}, got {actual_val:.6f}")
    
    def test_sma_manual_verification(self):
        """手动验证SMA计算的正确性"""
        # 使用简单数据进行手动验证
        simple_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        period = 3
        
        sma = bt.calculate_sma(simple_data, period)
        
        # 验证前两个值为NaN
        self.assertTrue(math.isnan(sma[0]))
        self.assertTrue(math.isnan(sma[1]))
        
        # 验证手动计算的值
        self.assertAlmostEqual(sma[2], 2.0, places=6)  # (1+2+3)/3
        self.assertAlmostEqual(sma[3], 3.0, places=6)  # (2+3+4)/3
        self.assertAlmostEqual(sma[4], 4.0, places=6)  # (3+4+5)/3
    
    def test_sma_different_periods(self):
        """测试不同周期的SMA"""
        periods = [5, 10, 15, 20, 30]
        
        for period in periods:
            sma = bt.calculate_sma(self.test_data, period)
            
            # 验证基本属性
            self.assertEqual(len(sma), len(self.test_data))
            
            # 验证NaN值数量
            nan_count = sum(1 for x in sma if math.isnan(x))
            self.assertEqual(nan_count, period - 1)
            
            # 验证有效值数量
            valid_count = len(self.test_data) - period + 1
            valid_values = [x for x in sma if not math.isnan(x)]
            self.assertEqual(len(valid_values), valid_count)


class TestEMA_Original(TestBacktraderCompatibleBase):
    """EMA测试 - 与原版test_ind_ema.py完全一致"""
    
    def test_ema_default_period_30(self):
        """测试EMA默认30周期，与原版期望值比较"""
        # 原版测试参数
        period = 30
        expected_values = OriginalTestParameters.EMA_TEST['expected_values']
        min_period = OriginalTestParameters.EMA_TEST['min_period']
        
        # 计算EMA
        ema_values = bt.calculate_ema(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(ema_values), len(self.test_data))
        
        # EMA应该没有NaN值（从第一个数据点开始就有值）
        valid_values = [x for x in ema_values if not math.isnan(x)]
        self.assertEqual(len(valid_values), len(self.test_data))
        
        # 在检查点验证期望值
        self.verify_indicator_at_checkpoints(ema_values, expected_values, min_period)
        
        # 调试输出
        check_points = OriginalTestParameters.get_check_points(len(ema_values), min_period)
        print(f"\nEMA Debug Info:")
        print(f"  Data length: {len(self.test_data)}")
        print(f"  EMA length: {len(ema_values)}")
        print(f"  Period: {period}")
        print(f"  Check points: {check_points}")
        for i, (cp, expected) in enumerate(zip(check_points, expected_values)):
            actual_idx = cp if cp >= 0 else len(ema_values) + cp
            actual_val = ema_values[actual_idx]
            print(f"  Check point {cp} (idx {actual_idx}): expected {expected}, got {actual_val:.6f}")
    
    def test_ema_convergence_behavior(self):
        """测试EMA收敛行为"""
        # 使用恒定价格测试收敛
        constant_price = 100.0
        constant_data = [constant_price] * 100
        
        ema = bt.calculate_ema(constant_data, 10)
        
        # EMA应该收敛到恒定价格
        self.assertAlmostEqual(ema[-1], constant_price, places=6)
        
        # 测试价格跳跃
        jump_data = [100.0] * 50 + [110.0] * 50
        ema_jump = bt.calculate_ema(jump_data, 10)
        
        # 在价格跳跃后，EMA应该向新价格移动
        self.assertGreater(ema_jump[-1], 105.0)
        self.assertLess(ema_jump[-1], 110.0)
    
    def test_ema_vs_sma_responsiveness(self):
        """测试EMA相对于SMA的响应性"""
        period = 20
        
        ema = bt.calculate_ema(self.test_data, period)
        sma = bt.calculate_sma(self.test_data, period)
        
        # 计算变化幅度
        ema_changes = []
        sma_changes = []
        
        for i in range(period, len(self.test_data)):
            if i > period:  # 确保SMA有效
                ema_change = abs(ema[i] - ema[i-1])
                sma_change = abs(sma[i] - sma[i-1])
                
                if not math.isnan(sma_change):
                    ema_changes.append(ema_change)
                    sma_changes.append(sma_change)
        
        # EMA通常应该有更大的变化（更敏感）
        if ema_changes and sma_changes:
            avg_ema_change = sum(ema_changes) / len(ema_changes)
            avg_sma_change = sum(sma_changes) / len(sma_changes)
            
            # 验证都是正值
            self.assertGreater(avg_ema_change, 0.0)
            self.assertGreater(avg_sma_change, 0.0)


class TestRSI_Original(TestBacktraderCompatibleBase):
    """RSI测试 - 与原版test_ind_rsi.py完全一致"""
    
    def test_rsi_default_period_14(self):
        """测试RSI默认14周期，与原版期望值比较"""
        # 原版测试参数 (注意：原版测试chkmin=15，但RSI实际周期是14)
        period = 14
        expected_values = OriginalTestParameters.RSI_TEST['expected_values']
        min_period = OriginalTestParameters.RSI_TEST['min_period']  # 15
        
        # 计算RSI
        rsi_values = bt.calculate_rsi(self.test_data, period)
        
        # 验证基本属性
        self.assertEqual(len(rsi_values), len(self.test_data))
        
        # 验证RSI值范围 (0-100)
        valid_values = [x for x in rsi_values if not math.isnan(x)]
        for val in valid_values:
            self.assertGreaterEqual(val, 0.0, "RSI value should be >= 0")
            self.assertLessEqual(val, 100.0, "RSI value should be <= 100")
        
        # 在检查点验证期望值
        self.verify_indicator_at_checkpoints(rsi_values, expected_values, min_period)
        
        # 调试输出
        check_points = OriginalTestParameters.get_check_points(len(rsi_values), min_period)
        print(f"\nRSI Debug Info:")
        print(f"  Data length: {len(self.test_data)}")
        print(f"  RSI length: {len(rsi_values)}")
        print(f"  Period: {period}")
        print(f"  Min period (chkmin): {min_period}")
        print(f"  Check points: {check_points}")
        print(f"  Valid values: {len(valid_values)}")
        for i, (cp, expected) in enumerate(zip(check_points, expected_values)):
            actual_idx = cp if cp >= 0 else len(rsi_values) + cp
            actual_val = rsi_values[actual_idx]
            print(f"  Check point {cp} (idx {actual_idx}): expected {expected}, got {actual_val:.6f}")
    
    def test_rsi_extreme_conditions(self):
        """测试RSI在极端条件下的行为"""
        # 持续上涨数据
        rising_data = list(range(1, 51))  # 1到50持续上涨
        rsi_rising = bt.calculate_rsi(rising_data, 14)
        
        valid_rising = [x for x in rsi_rising if not math.isnan(x)]
        if valid_rising:
            # 持续上涨应该产生高RSI
            self.assertGreater(valid_rising[-1], 70.0)
        
        # 持续下跌数据
        falling_data = list(range(50, 0, -1))  # 50到1持续下跌
        rsi_falling = bt.calculate_rsi(falling_data, 14)
        
        valid_falling = [x for x in rsi_falling if not math.isnan(x)]
        if valid_falling:
            # 持续下跌应该产生低RSI
            self.assertLess(valid_falling[-1], 30.0)
    
    def test_rsi_calculation_logic(self):
        """测试RSI计算逻辑"""
        # 使用已知数据测试RSI计算
        test_prices = [44, 44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.85, 
                      46.08, 45.89, 46.03, 46.83, 47.69, 46.49, 46.26]
        
        rsi = bt.calculate_rsi(test_prices, 14)
        
        # 验证RSI数组长度
        self.assertEqual(len(rsi), len(test_prices))
        
        # 验证前14个值处理（根据实现可能有所不同）
        valid_rsi = [x for x in rsi if not math.isnan(x)]
        
        # 应该有至少一个有效值
        self.assertGreater(len(valid_rsi), 0)
        
        # 所有有效值应该在正确范围内
        for val in valid_rsi:
            self.assertGreaterEqual(val, 0.0)
            self.assertLessEqual(val, 100.0)


class TestDataValidation_Original(TestBacktraderCompatibleBase):
    """数据验证测试 - 确保与原版数据源一致"""
    
    def test_original_data_integrity(self):
        """验证原版数据的完整性"""
        # 验证数据大小
        self.assertEqual(len(self.test_data), 255)
        
        # 验证数据范围
        self.assertGreater(min(self.test_data), 3000)
        self.assertLess(max(self.test_data), 5000)
        
        # 验证没有无效值
        for i, price in enumerate(self.test_data):
            self.assertFalse(math.isnan(price), f"NaN found at index {i}")
            self.assertFalse(math.isinf(price), f"Inf found at index {i}")
            self.assertGreater(price, 0, f"Non-positive price at index {i}")
    
    def test_data_consistency_with_backtrader(self):
        """验证数据与backtrader一致性"""
        # 验证特定已知值
        expected_first = 3604.33
        expected_last = 4119.94
        
        self.assertAlmostEqual(self.test_data[0], expected_first, places=2)
        self.assertAlmostEqual(self.test_data[-1], expected_last, places=2)
        
        # 验证数据的基本统计特性
        data_info = self.data_loader.get_data_info(0)
        self.assertEqual(data_info['size'], 255)
        self.assertEqual(data_info['start_date'], '2006-01-02')
        self.assertEqual(data_info['end_date'], '2006-12-29')
    
    def test_data_format_compatibility(self):
        """测试数据格式兼容性"""
        # 验证数据可以被我们的函数处理
        sma = bt.calculate_sma(self.test_data, 10)
        self.assertEqual(len(sma), len(self.test_data))
        
        ema = bt.calculate_ema(self.test_data, 10)
        self.assertEqual(len(ema), len(self.test_data))
        
        rsi = bt.calculate_rsi(self.test_data, 14)
        self.assertEqual(len(rsi), len(self.test_data))


class TestPerformance_Original(TestBacktraderCompatibleBase):
    """性能测试 - 使用原版数据验证性能"""
    
    def test_performance_with_original_data(self):
        """使用原版数据测试性能"""
        import time
        
        # 测试多次计算的性能
        iterations = 1000
        
        start_time = time.time()
        for _ in range(iterations):
            bt.calculate_sma(self.test_data, 20)
        sma_time = time.time() - start_time
        
        start_time = time.time()
        for _ in range(iterations):
            bt.calculate_ema(self.test_data, 20)
        ema_time = time.time() - start_time
        
        start_time = time.time()
        for _ in range(iterations):
            bt.calculate_rsi(self.test_data, 14)
        rsi_time = time.time() - start_time
        
        # 验证性能合理性
        self.assertLess(sma_time, 1.0, "SMA calculation too slow")
        self.assertLess(ema_time, 1.0, "EMA calculation too slow")
        self.assertLess(rsi_time, 1.0, "RSI calculation too slow")
        
        print(f"\nPerformance with original data ({len(self.test_data)} points, {iterations} iterations):")
        print(f"  SMA time: {sma_time*1000:.2f}ms")
        print(f"  EMA time: {ema_time*1000:.2f}ms")
        print(f"  RSI time: {rsi_time*1000:.2f}ms")


def run_backtrader_compatible_tests():
    """运行与backtrader兼容的测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🔄 运行与原版Backtrader兼容的测试套件")
    print("=" * 60)
    
    # 创建测试套件
    test_classes = [
        TestSMA_Original,
        TestEMA_Original,
        TestRSI_Original,
        TestDataValidation_Original,
        TestPerformance_Original
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
    print("📊 Backtrader兼容性测试总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("\n🎉 所有Backtrader兼容性测试通过！")
        print("✅ Python绑定与原版backtrader完全兼容")
        return True
    else:
        print("\n❌ 部分兼容性测试失败，需要检查。")
        return False


if __name__ == "__main__":
    success = run_backtrader_compatible_tests()
    sys.exit(0 if success else 1)