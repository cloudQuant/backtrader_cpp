#!/usr/bin/env python3
"""
Python Bindings Test Runner
运行所有Python绑定测试
"""

import sys
import os
import unittest
import importlib
from typing import List

def find_test_files() -> List[str]:
    """查找所有测试文件"""
    test_dir = os.path.dirname(__file__)
    test_files = []
    
    for file in os.listdir(test_dir):
        if file.startswith('test_python_') and file.endswith('.py'):
            test_files.append(file[:-3])  # 去掉.py后缀
    
    return sorted(test_files)

def run_all_python_binding_tests():
    """运行所有Python绑定测试"""
    print("🚀 运行所有Python绑定测试")
    print("=" * 60)
    
    # 检查模块可用性
    try:
        import backtrader_cpp as bt
        print(f"✅ backtrader_cpp module loaded (version: {getattr(bt, '__version__', 'unknown')})")
    except ImportError as e:
        print(f"❌ Cannot import backtrader_cpp: {e}")
        return False
    
    test_files = find_test_files()
    print(f"📋 发现 {len(test_files)} 个测试文件")
    
    total_tests = 0
    total_failures = 0
    total_errors = 0
    successful_files = 0
    
    for test_file in test_files:
        print(f"\n📂 运行 {test_file}")
        print("-" * 40)
        
        try:
            # 动态导入测试模块
            module = importlib.import_module(test_file)
            
            if hasattr(module, 'run_' + test_file[12:] + '_tests'):
                # 如果有自定义运行函数，使用它
                runner_func = getattr(module, 'run_' + test_file[12:] + '_tests')
                success = runner_func()
                if success:
                    successful_files += 1
            else:
                # 否则使用标准unittest运行
                suite = unittest.TestLoader().loadTestsFromModule(module)
                runner = unittest.TextTestRunner(verbosity=1)
                result = runner.run(suite)
                
                total_tests += result.testsRun
                total_failures += len(result.failures)
                total_errors += len(result.errors)
                
                if len(result.failures) == 0 and len(result.errors) == 0:
                    successful_files += 1
                    
        except Exception as e:
            print(f"❌ 运行 {test_file} 时出错: {e}")
            total_errors += 1
    
    # 总结
    print("\n" + "=" * 60)
    print("📊 Python绑定测试总结")
    print("=" * 60)
    
    success_rate = (successful_files / len(test_files) * 100) if test_files else 0
    
    print(f"测试文件总数: {len(test_files)}")
    print(f"成功运行: {successful_files}")
    print(f"失败/错误: {len(test_files) - successful_files}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_tests > 0:
        print(f"\n总测试用例: {total_tests}")
        print(f"失败: {total_failures}")
        print(f"错误: {total_errors}")
    
    if successful_files == len(test_files):
        print("\n🎉 所有Python绑定测试都运行成功！")
        return True
    else:
        print(f"\n❌ {len(test_files) - successful_files} 个测试文件运行失败")
        return False

if __name__ == "__main__":
    success = run_all_python_binding_tests()
    sys.exit(0 if success else 1)
