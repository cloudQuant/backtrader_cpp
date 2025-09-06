#!/usr/bin/env python3
"""
Test script for object-oriented indicators API in backtrader-cpp Python bindings
测试面向对象指标API
"""

import sys
import os
import unittest
import math

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

try:
    import backtrader_cpp as bt
    MODULE_AVAILABLE = True
except ImportError as e:
    print(f"❌ Cannot import backtrader_cpp: {e}")
    MODULE_AVAILABLE = False


class TestObjectOrientedIndicators(unittest.TestCase):
    """面向对象指标API测试"""

    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")

        # 创建测试数据
        self.test_data = [float(i + 100) for i in range(100)]

    def test_sma_indicator_creation(self):
        """测试SMA指标创建"""
        sma = bt.indicators.SMA(period=20)
        self.assertIsNotNone(sma)
        print("✅ SMA indicator created successfully")

    def test_sma_indicator_line_access(self):
        """测试SMA指标线访问"""
        sma = bt.indicators.SMA(period=20)
        line = sma.line()
        self.assertIsNotNone(line)
        print("✅ SMA indicator line access successful")

    def test_sma_indicator_with_different_periods(self):
        """测试SMA指标不同周期"""
        for period in [5, 10, 20, 50]:
            sma = bt.indicators.SMA(period=period)
            self.assertIsNotNone(sma)
            self.assertEqual(sma.period, period)
        print("✅ SMA indicator different periods test passed")

    def test_rsi_indicator_creation(self):
        """测试RSI指标创建"""
        rsi = bt.indicators.RSI(period=14)
        self.assertIsNotNone(rsi)
        self.assertEqual(rsi.period, 14)
        print("✅ RSI indicator created successfully")

    def test_macd_indicator_creation(self):
        """测试MACD指标创建"""
        macd = bt.indicators.MACD()
        self.assertIsNotNone(macd)
        print("✅ MACD indicator created successfully")

    def test_bollinger_bands_creation(self):
        """测试布林带指标创建"""
        bb = bt.indicators.BollingerBands()
        self.assertIsNotNone(bb)
        print("✅ Bollinger Bands indicator created successfully")

    def test_stochastic_indicator_creation(self):
        """测试随机指标创建"""
        stoch = bt.indicators.Stochastic()
        self.assertIsNotNone(stoch)
        print("✅ Stochastic indicator created successfully")

    def test_multiple_indicators_creation(self):
        """测试多个指标同时创建"""
        indicators = [
            bt.indicators.SMA(period=20),
            bt.indicators.EMA(period=20),
            bt.indicators.RSI(period=14),
            bt.indicators.MACD(),
            bt.indicators.BollingerBands(),
            bt.indicators.Stochastic(),
            bt.indicators.WilliamsR(period=14),
            bt.indicators.CCI(period=14),
            bt.indicators.ATR(period=14),
            bt.indicators.ADX(period=14),
        ]

        for i, indicator in enumerate(indicators):
            self.assertIsNotNone(indicator)
            print(f"✅ Indicator {i+1}/{len(indicators)} created successfully")

    def test_indicator_parameter_validation(self):
        """测试指标参数验证"""
        # 测试无效周期
        with self.assertRaises(bt.InvalidParameterError):
            bt.indicators.SMA(period=-1)

        with self.assertRaises(bt.InvalidParameterError):
            bt.indicators.RSI(period=0)

        print("✅ Indicator parameter validation working")

    def test_indicator_line_access(self):
        """测试指标线访问"""
        # MACD有多行
        macd = bt.indicators.MACD()
        macd_line = macd.line()
        self.assertIsNotNone(macd_line)

        # Stochastic有多行
        stoch = bt.indicators.Stochastic()
        k_line = stoch.k()
        d_line = stoch.d()
        self.assertIsNotNone(k_line)
        self.assertIsNotNone(d_line)

        print("✅ Indicator line access working")

    def test_advanced_indicators_creation(self):
        """测试高级指标创建"""
        advanced_indicators = [
            bt.indicators.SuperTrend(),
            bt.indicators.KeltnerChannel(),
            bt.indicators.DonchianChannel(),
            bt.indicators.WMAExponential(),
            bt.indicators.HullSuite(),
        ]

        for indicator in advanced_indicators:
            self.assertIsNotNone(indicator)

        print("✅ Advanced indicators creation successful")

    def test_data_series_creation(self):
        """测试数据系列创建"""
        data_series = bt.DataSeries()
        self.assertIsNotNone(data_series)

        line_buffer = bt.LineBuffer()
        self.assertIsNotNone(line_buffer)

        print("✅ Data series creation successful")


if __name__ == "__main__":
    print("Running Object-Oriented Indicators Tests")
    print("=" * 50)

    if MODULE_AVAILABLE:
        unittest.main(verbosity=2)
    else:
        print("❌ backtrader_cpp module not available")
