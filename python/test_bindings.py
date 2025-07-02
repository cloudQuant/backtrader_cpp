#!/usr/bin/env python3
"""
Phase 0 Python绑定测试脚本
用于验证C++模块的基本功能
"""

import sys
import numpy as np
import time

def test_basic_functionality():
    """测试基本功能"""
    print("=== Testing Basic Functionality ===")
    
    try:
        import backtrader_cpp as btcpp
        print("✓ Import successful")
    except ImportError as e:
        print(f"✗ Import failed: {e}")
        return False
    
    # 测试LineRoot
    try:
        line = btcpp.LineRoot(10, "test")
        line.forward(1.0)
        line.forward(2.0)
        line.forward(3.0)
        
        assert line.len() == 3
        assert line.get(0) == 3.0
        assert line.get(-1) == 2.0
        assert line.get(-2) == 1.0
        print("✓ LineRoot basic operations")
    except Exception as e:
        print(f"✗ LineRoot test failed: {e}")
        return False
    
    # 测试CircularBuffer
    try:
        buffer = btcpp.CircularBuffer(5)
        buffer.forward(1.0)
        buffer.forward(2.0)
        
        assert buffer.len() == 2
        assert buffer.get(0) == 2.0
        assert buffer.get(-1) == 1.0
        print("✓ CircularBuffer basic operations")
    except Exception as e:
        print(f"✗ CircularBuffer test failed: {e}")
        return False
    
    return True

def test_indicators():
    """测试指标"""
    print("\n=== Testing Indicators ===")
    
    try:
        import backtrader_cpp as btcpp
        
        # 创建测试数据
        data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        line = btcpp.create_line_from_list(data, "test_data")
        
        # 测试SMA
        sma = btcpp.SMA(line, 3)
        
        # 计算几个值
        sma.calculate()  # 应该是NaN
        line.forward()
        sma.calculate()  # 应该是NaN
        line.forward()
        sma.calculate()  # 应该是2.0 (1+2+3)/3
        
        assert btcpp.isNaN(sma.get(-2))  # 前两个值应该是NaN
        assert btcpp.isNaN(sma.get(-1))
        assert abs(sma.get(0) - 2.0) < 1e-10
        
        print("✓ SMA calculation")
        
        # 测试EMA
        line.home()
        for value in data:
            line.forward(value)
        
        ema = btcpp.EMA(line, 3)
        ema.calculate()  # 第一个值应该等于输入
        
        assert ema.get(0) == 1.0
        print("✓ EMA calculation")
        
    except Exception as e:
        print(f"✗ Indicator test failed: {e}")
        return False
    
    return True

def test_operations():
    """测试运算操作"""
    print("\n=== Testing Operations ===")
    
    try:
        import backtrader_cpp as btcpp
        
        # 创建两个数据线
        data1 = [10.0, 20.0, 30.0]
        data2 = [5.0, 4.0, 3.0]
        
        line1 = btcpp.create_line_from_list(data1, "line1")
        line2 = btcpp.create_line_from_list(data2, "line2")
        
        # 测试运算
        add_result = line1 + line2
        sub_result = line1 - line2
        mul_result = line1 * 2.0
        div_result = line1 / 2.0
        
        # 验证结果
        assert add_result.get(0) == 33.0  # 30 + 3
        assert sub_result.get(0) == 27.0  # 30 - 3
        assert mul_result.get(0) == 60.0  # 30 * 2
        assert div_result.get(0) == 15.0  # 30 / 2
        
        print("✓ Arithmetic operations")
        
        # 测试比较运算
        gt_result = line1 > 25.0
        lt_result = line1 < 25.0
        
        assert gt_result.get(0) == 1.0  # 30 > 25 = true = 1.0
        assert lt_result.get(0) == 0.0  # 30 < 25 = false = 0.0
        
        print("✓ Comparison operations")
        
    except Exception as e:
        print(f"✗ Operations test failed: {e}")
        return False
    
    return True

def test_performance():
    """测试性能"""
    print("\n=== Testing Performance ===")
    
    try:
        import backtrader_cpp as btcpp
        
        # 运行性能测试
        results = btcpp.test_performance(data_size=10000, period=20, iterations=1)
        
        if 'error' in results:
            print(f"✗ Performance test failed: {results['error']}")
            return False
        
        print(f"✓ SMA rate: {results['sma_rate']:,.0f} calc/sec")
        print(f"✓ EMA rate: {results['ema_rate']:,.0f} calc/sec")
        print(f"✓ LineRoot rate: {results['lineroot_rate']:,.0f} ops/sec")
        
        # 基本的性能要求检查
        if results['sma_rate'] < 100000:  # 至少10万次/秒
            print(f"⚠ SMA performance might be low: {results['sma_rate']:,.0f} calc/sec")
        
        return True
        
    except Exception as e:
        print(f"✗ Performance test failed: {e}")
        return False

def test_accuracy():
    """测试计算准确性"""
    print("\n=== Testing Calculation Accuracy ===")
    
    try:
        import backtrader_cpp as btcpp
        
        # 生成测试数据
        test_data = [float(i) for i in range(1, 21)]  # 1 to 20
        
        # 测试SMA准确性
        line = btcpp.create_line_from_list(test_data)
        sma = btcpp.SMA(line, 5)
        
        # 移动到有效位置
        for _ in range(4):  # 前4次移动
            line.forward()
        
        sma.calculate()
        expected_sma = sum(test_data[:5]) / 5  # (1+2+3+4+5)/5 = 3.0
        actual_sma = sma.get(0)
        
        assert abs(actual_sma - expected_sma) < 1e-10, f"SMA mismatch: {actual_sma} vs {expected_sma}"
        print(f"✓ SMA accuracy: expected={expected_sma}, actual={actual_sma}")
        
        # 测试EMA准确性
        line = btcpp.create_line_from_list(test_data)
        ema = btcpp.EMA(line, 5)
        
        ema.calculate()
        expected_first = test_data[0]  # 第一个值应该等于输入
        actual_first = ema.get(0)
        
        assert abs(actual_first - expected_first) < 1e-10
        print(f"✓ EMA first value: expected={expected_first}, actual={actual_first}")
        
        return True
        
    except Exception as e:
        print(f"✗ Accuracy test failed: {e}")
        return False

def test_error_handling():
    """测试错误处理"""
    print("\n=== Testing Error Handling ===")
    
    try:
        import backtrader_cpp as btcpp
        
        # 测试无效参数
        line = btcpp.LineRoot(10)
        
        try:
            sma = btcpp.SMA(line, 0)  # 应该抛出异常
            print("✗ Should have thrown exception for period=0")
            return False
        except Exception:
            print("✓ SMA period validation")
        
        try:
            ema = btcpp.EMA(line, 0)  # 应该抛出异常
            print("✗ Should have thrown exception for period=0")
            return False
        except Exception:
            print("✓ EMA period validation")
        
        # 测试空缓冲区访问
        empty_buffer = btcpp.CircularBuffer(5)
        try:
            value = empty_buffer.get(0)  # 应该抛出异常
            print("✗ Should have thrown exception for empty buffer access")
            return False
        except Exception:
            print("✓ Empty buffer access validation")
        
        return True
        
    except Exception as e:
        print(f"✗ Error handling test failed: {e}")
        return False

def main():
    """主测试函数"""
    print("Backtrader C++ Phase 0 - Python Bindings Test")
    print("=" * 50)
    
    tests = [
        test_basic_functionality,
        test_indicators,
        test_operations,
        test_performance,
        test_accuracy,
        test_error_handling
    ]
    
    passed = 0
    total = len(tests)
    
    for test in tests:
        if test():
            passed += 1
        else:
            print(f"Test failed, stopping here.")
            break
    
    print(f"\n=== Test Results ===")
    print(f"Passed: {passed}/{total}")
    
    if passed == total:
        print("🎉 All tests passed! Phase 0 prototype is working correctly.")
        return 0
    else:
        print("❌ Some tests failed. Please check the implementation.")
        return 1

if __name__ == "__main__":
    sys.exit(main())