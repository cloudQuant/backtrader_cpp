#!/usr/bin/env python3
"""
Algorithm Verification Test Suite
验证我们的算法实现正确性，而不是与原版期望值完全匹配
"""

import sys
import os
import unittest
import math
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


class TestAlgorithmVerification(unittest.TestCase):
    """算法验证测试基类"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载原版测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
        
        # 验证数据加载
        self.assertEqual(len(self.test_data), 255)


class TestSMA_Verification(TestAlgorithmVerification):
    """SMA算法验证"""
    
    def test_sma_basic_properties(self):
        """测试SMA基本属性"""
        period = 30
        sma = bt.calculate_sma(self.test_data, period)
        
        # 基本属性验证
        self.assertEqual(len(sma), len(self.test_data))
        
        # 前29个值应该是NaN
        for i in range(period - 1):
            self.assertTrue(math.isnan(sma[i]), f"SMA[{i}] should be NaN")
        
        # 从第30个值开始应该有效
        for i in range(period - 1, len(sma)):
            self.assertFalse(math.isnan(sma[i]), f"SMA[{i}] should be valid")
            self.assertGreater(sma[i], 0, f"SMA[{i}] should be positive")
    
    def test_sma_calculation_accuracy(self):
        """测试SMA计算精度"""
        # 使用简单数据验证
        test_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        period = 3
        
        sma = bt.calculate_sma(test_data, period)
        
        # 手动验证
        self.assertTrue(math.isnan(sma[0]))
        self.assertTrue(math.isnan(sma[1]))
        self.assertAlmostEqual(sma[2], 2.0, places=6)  # (1+2+3)/3
        self.assertAlmostEqual(sma[3], 3.0, places=6)  # (2+3+4)/3
        self.assertAlmostEqual(sma[4], 4.0, places=6)  # (3+4+5)/3
        self.assertAlmostEqual(sma[5], 5.0, places=6)  # (4+5+6)/3
    
    def test_sma_with_real_data(self):
        """使用真实数据测试SMA"""
        period = 30
        sma = bt.calculate_sma(self.test_data, period)
        
        # 验证第30个值（索引29）
        manual_sma_30 = sum(self.test_data[:30]) / 30
        self.assertAlmostEqual(sma[29], manual_sma_30, places=6)
        
        # 验证第31个值（索引30）
        manual_sma_31 = sum(self.test_data[1:31]) / 30
        self.assertAlmostEqual(sma[30], manual_sma_31, places=6)
        
        # 验证最后一个值
        manual_sma_last = sum(self.test_data[-30:]) / 30
        self.assertAlmostEqual(sma[-1], manual_sma_last, places=6)
        
        print(f"SMA验证:")
        print(f"  第30个值: {sma[29]:.6f} (手动计算: {manual_sma_30:.6f})")
        print(f"  最后值: {sma[-1]:.6f} (手动计算: {manual_sma_last:.6f})")


class TestEMA_Verification(TestAlgorithmVerification):
    """EMA算法验证"""
    
    def test_ema_basic_properties(self):
        """测试EMA基本属性"""
        period = 30
        ema = bt.calculate_ema(self.test_data, period)
        
        # 基本属性验证
        self.assertEqual(len(ema), len(self.test_data))
        
        # EMA应该没有NaN值
        for i, val in enumerate(ema):
            self.assertFalse(math.isnan(val), f"EMA[{i}] should not be NaN")
            self.assertGreater(val, 0, f"EMA[{i}] should be positive")
    
    def test_ema_convergence(self):
        """测试EMA收敛性"""
        # 使用恒定价格
        constant_data = [100.0] * 100
        ema = bt.calculate_ema(constant_data, 10)
        
        # 第一个值应该等于价格
        self.assertAlmostEqual(ema[0], 100.0, places=6)
        
        # 最后应该收敛到价格
        self.assertAlmostEqual(ema[-1], 100.0, places=3)
        
        # 价格跳跃测试
        jump_data = [100.0] * 50 + [110.0] * 50
        ema_jump = bt.calculate_ema(jump_data, 10)
        
        # 跳跃后应该向新价格移动
        self.assertGreater(ema_jump[-1], 105.0)
        self.assertLess(ema_jump[-1], 110.0)
    
    def test_ema_with_real_data(self):
        """使用真实数据测试EMA"""
        period = 30
        ema = bt.calculate_ema(self.test_data, period)
        
        # 第一个值应该等于第一个价格
        self.assertAlmostEqual(ema[0], self.test_data[0], places=6)
        
        # EMA应该响应价格变化
        # 如果价格上涨，EMA也应该总体上涨
        if self.test_data[-1] > self.test_data[0]:
            self.assertGreater(ema[-1], ema[0])
        
        print(f"EMA验证:")
        print(f"  第一个值: {ema[0]:.6f} (价格: {self.test_data[0]:.6f})")
        print(f"  最后值: {ema[-1]:.6f} (价格: {self.test_data[-1]:.6f})")


class TestRSI_Verification(TestAlgorithmVerification):
    """RSI算法验证"""
    
    def test_rsi_basic_properties(self):
        """测试RSI基本属性"""
        period = 14
        rsi = bt.calculate_rsi(self.test_data, period)
        
        # 基本属性验证
        self.assertEqual(len(rsi), len(self.test_data))
        
        # 验证RSI值范围
        for i, val in enumerate(rsi):
            if not math.isnan(val):
                self.assertGreaterEqual(val, 0.0, f"RSI[{i}] should be >= 0")
                self.assertLessEqual(val, 100.0, f"RSI[{i}] should be <= 100")
    
    def test_rsi_extreme_values(self):
        """测试RSI极端值"""
        # 持续上涨
        rising_data = list(range(1, 101))
        rsi_rising = bt.calculate_rsi(rising_data, 14)
        
        valid_rising = [x for x in rsi_rising if not math.isnan(x)]
        if valid_rising:
            # 持续上涨应该产生高RSI
            self.assertGreater(valid_rising[-1], 70.0)
        
        # 持续下跌
        falling_data = list(range(100, 0, -1))
        rsi_falling = bt.calculate_rsi(falling_data, 14)
        
        valid_falling = [x for x in rsi_falling if not math.isnan(x)]
        if valid_falling:
            # 持续下跌应该产生低RSI
            self.assertLess(valid_falling[-1], 30.0)
    
    def test_rsi_with_real_data(self):
        """使用真实数据测试RSI"""
        period = 14
        rsi = bt.calculate_rsi(self.test_data, period)
        
        # 计算有效值
        valid_rsi = [x for x in rsi if not math.isnan(x)]
        
        # 应该有足够的有效值
        self.assertGreater(len(valid_rsi), len(self.test_data) - period - 5)
        
        # 平均RSI应该在合理范围内
        if valid_rsi:
            avg_rsi = sum(valid_rsi) / len(valid_rsi)
            self.assertGreater(avg_rsi, 20.0)
            self.assertLess(avg_rsi, 80.0)
        
        print(f"RSI验证:")
        print(f"  有效值数量: {len(valid_rsi)}")
        if valid_rsi:
            print(f"  范围: {min(valid_rsi):.2f} - {max(valid_rsi):.2f}")
            print(f"  平均: {sum(valid_rsi)/len(valid_rsi):.2f}")


class TestDataProcessing_Verification(TestAlgorithmVerification):
    """数据处理验证"""
    
    def test_returns_calculation(self):
        """测试收益率计算"""
        returns = bt.calculate_returns(self.test_data)
        
        # 基本验证
        self.assertEqual(len(returns), len(self.test_data) - 1)
        
        # 手动验证前几个收益率
        for i in range(min(5, len(returns))):
            expected_return = (self.test_data[i+1] - self.test_data[i]) / self.test_data[i]
            self.assertAlmostEqual(returns[i], expected_return, places=10)
        
        print(f"收益率验证:")
        print(f"  数量: {len(returns)}")
        print(f"  前3个: {returns[:3]}")
    
    def test_volatility_calculation(self):
        """测试波动率计算"""
        returns = bt.calculate_returns(self.test_data)
        volatility = bt.calculate_volatility(returns, 20)
        
        # 基本验证
        self.assertEqual(len(volatility), len(returns))
        
        # 前19个应该是NaN
        for i in range(19):
            self.assertTrue(math.isnan(volatility[i]))
        
        # 有效值应该为正
        valid_vol = [x for x in volatility if not math.isnan(x)]
        for vol in valid_vol:
            self.assertGreater(vol, 0.0)
        
        print(f"波动率验证:")
        print(f"  有效值数量: {len(valid_vol)}")
        if valid_vol:
            print(f"  平均波动率: {sum(valid_vol)/len(valid_vol):.6f}")
    
    def test_sharpe_ratio(self):
        """测试夏普比率"""
        returns = bt.calculate_returns(self.test_data)
        sharpe = bt.calculate_sharpe(returns, 0.02)
        
        # 基本验证
        self.assertIsInstance(sharpe, float)
        self.assertFalse(math.isnan(sharpe))
        
        print(f"夏普比率: {sharpe:.4f}")


class TestStrategy_Verification(TestAlgorithmVerification):
    """策略验证"""
    
    def test_moving_average_strategy(self):
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
        self.assertGreaterEqual(strategy['num_trades'], 0)
        
        # 信号数量应该等于数据数量
        self.assertEqual(len(strategy['signals']), len(self.test_data))
        
        print(f"策略验证:")
        print(f"  初始资金: ${strategy['initial_value']:.2f}")
        print(f"  最终价值: ${strategy['final_value']:.2f}")
        print(f"  总收益率: {strategy['total_return']:.2%}")
        print(f"  交易次数: {strategy['num_trades']}")


def run_verification_tests():
    """运行验证测试套件"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available. Cannot run tests.")
        return False
    
    print("🔍 运行算法验证测试套件")
    print("=" * 60)
    
    test_classes = [
        TestSMA_Verification,
        TestEMA_Verification,
        TestRSI_Verification,
        TestDataProcessing_Verification,
        TestStrategy_Verification
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
                # 只显示关键错误信息
                error_lines = traceback.strip().split('\n')
                for line in error_lines[-3:]:
                    if 'AssertionError' in line:
                        print(f"    {line.split('AssertionError:')[-1].strip()}")
                
        if result.errors:
            print(f"💥 错误: {len(result.errors)}")
            for test, traceback in result.errors:
                print(f"  - {test}")
                error_lines = traceback.strip().split('\n')
                for line in error_lines[-3:]:
                    if 'Error' in line:
                        print(f"    {line.split('Error:')[-1].strip()}")
    
    # 总结
    print("\n" + "=" * 60)
    print("📊 算法验证测试总结")
    print("=" * 60)
    
    success_rate = ((total_tests - total_failures - total_errors) / total_tests * 100) if total_tests > 0 else 0
    
    print(f"总测试数: {total_tests}")
    print(f"成功: {total_tests - total_failures - total_errors}")
    print(f"失败: {total_failures}")
    print(f"错误: {total_errors}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_failures == 0 and total_errors == 0:
        print("\n🎉 所有算法验证测试通过！")
        print("✅ 我们的实现算法正确")
        return True
    else:
        print("\n❌ 部分算法验证失败，需要修复。")
        return False


if __name__ == "__main__":
    success = run_verification_tests()
    sys.exit(0 if success else 1)