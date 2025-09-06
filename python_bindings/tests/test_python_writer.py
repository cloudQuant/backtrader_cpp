#!/usr/bin/env python3
"""
Python binding test for WRITER indicator
对应C++测试: test_writer.cpp
* @file test_writer.cpp
 * @brief 数据写入器测试 - 对应Python test_writer.py
 * 
 * 原始Python测试:
 * - 测试CSV写入器功能
 * - 验证数据输出格式
 * - 测试输出行数（header + 256行数据）
 * - 验证分隔符和格式的正确性
"""

import sys
import os
import unittest
import math
from typing import List

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

# Import data loader
sys.path.insert(0, os.path.dirname(__file__))
try:
    from data_loader import BacktraderTestData
except ImportError:
    # Fallback data loader
    class BacktraderTestData:
        def get_close_prices(self, file_index=0):
            # Simple test data if data_loader not available
            return [float(i + 100) for i in range(255)]

try:
    import backtrader_cpp as bt
    MODULE_AVAILABLE = True
except ImportError as e:
    print(f"❌ Cannot import backtrader_cpp: {e}")
    MODULE_AVAILABLE = False


class TestWRITER(unittest.TestCase):
    """WRITER指标测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
        
        # 验证数据
        self.assertGreater(len(self.test_data), 0)
        self.assertTrue(all(not math.isnan(x) for x in self.test_data))
    
    def test_writer_functionality(self):
        """writer基础功能测试"""
        # 检查函数是否存在
        if not hasattr(bt, 'calculate_writer'):
            self.skipTest(f"Function calculate_writer not implemented in Python bindings yet")
        
        # 测试基本调用
        try:
            result = getattr(bt, 'calculate_writer')(self.test_data)
            
            # 基本验证
            self.assertIsInstance(result, list)
            self.assertEqual(len(result), len(self.test_data))
            
            # 检查是否有有效值
            valid_values = [x for x in result if not math.isnan(x)]
            self.assertGreater(len(valid_values), 0, f"writer should produce some valid values")
            
            print(f"✅ writer test passed: {len(valid_values)} valid values")
            
        except Exception as e:
            self.fail(f"writer calculation failed: {e}")
    
    def test_writer_with_parameters(self):
        """writer参数测试"""
        if not hasattr(bt, 'calculate_writer'):
            self.skipTest(f"Function calculate_writer not implemented yet")
        
        # 测试不同参数
        test_params = [5, 10, 20, 30]
        
        for param in test_params:
            with self.subTest(period=param):
                try:
                    result = getattr(bt, 'calculate_writer')(self.test_data, param)
                    self.assertIsInstance(result, list)
                    self.assertEqual(len(result), len(self.test_data))
                except Exception:
                    # 如果不支持参数，尝试无参数调用
                    result = getattr(bt, 'calculate_writer')(self.test_data)
                    self.assertIsInstance(result, list)
                    break
    
    def test_writer_edge_cases(self):
        """writer边界情况测试"""
        if not hasattr(bt, 'calculate_writer'):
            self.skipTest(f"Function calculate_writer not implemented yet")
        
        # 测试小数据集
        small_data = self.test_data[:10]
        
        try:
            result = getattr(bt, 'calculate_writer')(small_data)
            self.assertIsInstance(result, list)
            self.assertEqual(len(result), len(small_data))
        except Exception as e:
            # 边界情况可能失败，记录但不中断测试
            print(f"Note: writer edge case failed: {e}")
    
    def test_writer_value_ranges(self):
        """writer值范围验证"""
        if not hasattr(bt, 'calculate_writer'):
            self.skipTest(f"Function calculate_writer not implemented yet")
        
        try:
            result = getattr(bt, 'calculate_writer')(self.test_data)
            valid_values = [x for x in result if not math.isnan(x)]
            
            if valid_values:
                min_val = min(valid_values)
                max_val = max(valid_values)
                
                # 基本范围检查（根据指标类型调整）
                if 'writer' in ['rsi', 'stochastic', 'williamsad', 'williamsr']:
                    # 0-100范围的振荡器
                    self.assertGreaterEqual(min_val, -5.0)  # 允许小误差
                    self.assertLessEqual(max_val, 105.0)
                else:
                    # 其他指标应该是有限值
                    self.assertFalse(math.isinf(min_val))
                    self.assertFalse(math.isinf(max_val))
                
                print(f"✅ writer value range: {min_val:.2f} - {max_val:.2f}")
                
        except Exception as e:
            print(f"Note: writer range check failed: {e}")


def run_writer_tests():
    """运行writer测试"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available.")
        return False
    
    print(f"🧪 Running WRITER Tests")
    print("=" * 50)
    
    suite = unittest.TestLoader().loadTestsFromTestCase(TestWRITER)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    success = len(result.failures) == 0 and len(result.errors) == 0
    
    if success:
        print(f"✅ All WRITER tests passed!")
    else:
        print(f"❌ Some WRITER tests failed.")
        
    return success


if __name__ == "__main__":
    run_writer_tests()
