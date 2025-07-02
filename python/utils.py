"""
Python工具函数和便利方法
"""

import time
import numpy as np
from typing import List, Union

try:
    from .backtrader_cpp_native.core import LineRoot
    from .backtrader_cpp_native.indicators import SMA, EMA
    NATIVE_AVAILABLE = True
except ImportError:
    NATIVE_AVAILABLE = False

def create_line_from_list(data: List[float], name: str = "data") -> 'LineRoot':
    """
    从Python列表创建LineRoot对象
    
    Args:
        data: 数据列表
        name: 数据线名称
        
    Returns:
        LineRoot对象
    """
    if not NATIVE_AVAILABLE:
        raise RuntimeError("Native C++ modules not available")
    
    line = LineRoot(len(data) + 100, name)
    for value in data:
        line.forward(value)
    
    return line

def create_line_from_numpy(data: np.ndarray, name: str = "numpy_data") -> 'LineRoot':
    """
    从numpy数组创建LineRoot对象
    
    Args:
        data: numpy数组
        name: 数据线名称
        
    Returns:
        LineRoot对象
    """
    if not NATIVE_AVAILABLE:
        raise RuntimeError("Native C++ modules not available")
    
    return create_line_from_list(data.tolist(), name)

def test_performance(data_size: int = 10000, period: int = 20, iterations: int = 1):
    """
    性能测试函数
    
    Args:
        data_size: 数据大小
        period: 指标周期
        iterations: 迭代次数
        
    Returns:
        性能测试结果字典
    """
    if not NATIVE_AVAILABLE:
        return {"error": "Native C++ modules not available"}
    
    # 生成测试数据
    data = np.random.randn(data_size) * 10 + 100
    
    results = {}
    
    # 测试SMA性能
    start_time = time.time()
    for _ in range(iterations):
        line = create_line_from_numpy(data, "perf_test")
        sma = SMA(line, period)
        
        # 执行计算
        for i in range(data_size):
            sma.calculate()
            if i < data_size - 1:
                line.forward()
    
    sma_time = (time.time() - start_time) / iterations
    results['sma_time'] = sma_time
    results['sma_rate'] = data_size / sma_time  # calculations per second
    
    # 测试EMA性能
    start_time = time.time()
    for _ in range(iterations):
        line = create_line_from_numpy(data, "perf_test")
        ema = EMA(line, period)
        
        # 执行计算
        for i in range(data_size):
            ema.calculate()
            if i < data_size - 1:
                line.forward()
    
    ema_time = (time.time() - start_time) / iterations
    results['ema_time'] = ema_time
    results['ema_rate'] = data_size / ema_time  # calculations per second
    
    # 测试LineRoot运算性能
    start_time = time.time()
    for _ in range(iterations):
        line1 = create_line_from_numpy(data[:data_size//2], "line1")
        line2 = create_line_from_numpy(data[data_size//2:], "line2")
        
        # 执行运算
        result = line1 + line2
        result = result * 2.0
        result = result / 3.0
        
        # 访问结果
        for i in range(min(len(data)//2, 1000)):  # 限制访问次数
            _ = result.get(-i)
    
    lineroot_time = (time.time() - start_time) / iterations
    results['lineroot_time'] = lineroot_time
    results['lineroot_rate'] = (data_size // 2) / lineroot_time
    
    results['data_size'] = data_size
    results['period'] = period
    results['iterations'] = iterations
    
    return results

def print_performance_report(results: dict):
    """
    打印性能测试报告
    
    Args:
        results: test_performance返回的结果字典
    """
    if 'error' in results:
        print(f"Performance test failed: {results['error']}")
        return
    
    print("=== Backtrader C++ Performance Report ===")
    print(f"Data size: {results['data_size']:,}")
    print(f"Period: {results['period']}")
    print(f"Iterations: {results['iterations']}")
    print()
    
    print(f"SMA Performance:")
    print(f"  Time: {results['sma_time']:.6f} seconds")
    print(f"  Rate: {results['sma_rate']:,.0f} calculations/second")
    print()
    
    print(f"EMA Performance:")
    print(f"  Time: {results['ema_time']:.6f} seconds")
    print(f"  Rate: {results['ema_rate']:,.0f} calculations/second")
    print()
    
    print(f"LineRoot Operations:")
    print(f"  Time: {results['lineroot_time']:.6f} seconds")
    print(f"  Rate: {results['lineroot_rate']:,.0f} operations/second")
    print()
    
    print(f"EMA vs SMA ratio: {results['ema_rate'] / results['sma_rate']:.2f}x")

def compare_with_python_sma(data: List[float], period: int) -> dict:
    """
    与纯Python SMA实现进行比较
    
    Args:
        data: 测试数据
        period: SMA周期
        
    Returns:
        比较结果
    """
    if not NATIVE_AVAILABLE:
        return {"error": "Native C++ modules not available"}
    
    # C++ SMA
    start_time = time.time()
    cpp_line = create_line_from_list(data)
    cpp_sma = SMA(cpp_line, period)
    
    cpp_results = []
    for i in range(len(data)):
        cpp_sma.calculate()
        cpp_results.append(cpp_sma.get(0))
        if i < len(data) - 1:
            cpp_line.forward()
    
    cpp_time = time.time() - start_time
    
    # 纯Python SMA
    start_time = time.time()
    python_results = []
    
    for i in range(len(data)):
        if i < period - 1:
            python_results.append(float('nan'))
        else:
            window = data[i - period + 1:i + 1]
            avg = sum(window) / len(window)
            python_results.append(avg)
    
    python_time = time.time() - start_time
    
    # 验证结果一致性（跳过NaN值）
    max_diff = 0.0
    valid_count = 0
    
    for i in range(period - 1, len(data)):
        if not (np.isnan(cpp_results[i]) and np.isnan(python_results[i])):
            diff = abs(cpp_results[i] - python_results[i])
            max_diff = max(max_diff, diff)
            valid_count += 1
    
    return {
        'cpp_time': cpp_time,
        'python_time': python_time,
        'speedup': python_time / cpp_time if cpp_time > 0 else float('inf'),
        'max_difference': max_diff,
        'valid_comparisons': valid_count,
        'data_size': len(data),
        'period': period
    }

def validate_indicator_accuracy(test_data: List[float], period: int = 20) -> dict:
    """
    验证指标计算的准确性
    
    Args:
        test_data: 测试数据
        period: 指标周期
        
    Returns:
        验证结果
    """
    if not NATIVE_AVAILABLE:
        return {"error": "Native C++ modules not available"}
    
    results = {}
    
    # 测试SMA准确性
    line = create_line_from_list(test_data)
    sma = SMA(line, period)
    
    sma_values = []
    for i in range(len(test_data)):
        sma.calculate()
        sma_values.append(sma.get(0))
        if i < len(test_data) - 1:
            line.forward()
    
    # 验证SMA值
    sma_errors = []
    for i in range(period - 1, min(len(test_data), period + 10)):  # 检查前几个有效值
        if i >= period - 1:
            expected = sum(test_data[i - period + 1:i + 1]) / period
            actual = sma_values[i]
            if not np.isnan(actual):
                error = abs(actual - expected)
                sma_errors.append(error)
    
    results['sma_max_error'] = max(sma_errors) if sma_errors else 0.0
    results['sma_avg_error'] = sum(sma_errors) / len(sma_errors) if sma_errors else 0.0
    
    # 测试EMA准确性（与手工计算比较前几个值）
    line = create_line_from_list(test_data)
    ema = EMA(line, period)
    
    alpha = 2.0 / (period + 1)
    expected_ema = test_data[0]  # 第一个值
    
    ema.calculate()
    actual_ema = ema.get(0)
    first_error = abs(actual_ema - expected_ema)
    
    # 计算第二个值
    line.forward()
    ema.calculate()
    expected_ema = alpha * test_data[1] + (1 - alpha) * expected_ema
    actual_ema = ema.get(0)
    second_error = abs(actual_ema - expected_ema)
    
    results['ema_first_error'] = first_error
    results['ema_second_error'] = second_error
    results['ema_alpha'] = alpha
    
    return results