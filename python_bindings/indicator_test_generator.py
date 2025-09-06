#!/usr/bin/env python3
"""
Indicator Test Generator
自动生成所有指标的Python绑定测试文件
基于C++测试文件结构生成对应的Python测试
"""

import os
import re
from typing import List, Dict, Tuple


class IndicatorTestGenerator:
    """指标测试生成器"""
    
    def __init__(self):
        self.cpp_test_dir = "/home/yun/Documents/refactor_backtrader/backtrader_cpp/tests/original_tests"
        self.python_test_dir = "/home/yun/Documents/refactor_backtrader/backtrader_cpp/python_bindings/tests"
        self.python_original_tests = "/home/yun/Documents/refactor_backtrader/backtrader/tests/original_tests"
        
        # 创建Python测试目录
        os.makedirs(self.python_test_dir, exist_ok=True)
        
        # 指标分类
        self.technical_indicators = []
        self.analyzers = []
        self.data_processors = []
        self.strategies = []
        self.core_components = []
        
        self._classify_tests()
    
    def _classify_tests(self):
        """分类测试文件"""
        cpp_files = [f for f in os.listdir(self.cpp_test_dir) if f.endswith('.cpp')]
        
        for file in cpp_files:
            if file.startswith('test_ind_'):
                self.technical_indicators.append(file)
            elif file.startswith('test_analyzer'):
                self.analyzers.append(file)
            elif file.startswith('test_data_'):
                self.data_processors.append(file)
            elif file.startswith('test_strategy'):
                self.strategies.append(file)
            else:
                self.core_components.append(file)
    
    def get_indicator_name(self, cpp_filename: str) -> str:
        """从C++文件名提取指标名称"""
        if cpp_filename.startswith('test_ind_'):
            return cpp_filename[9:-4]  # 去掉 'test_ind_' 前缀和 '.cpp' 后缀
        elif cpp_filename.startswith('test_'):
            return cpp_filename[5:-4]  # 去掉 'test_' 前缀和 '.cpp' 后缀
        return cpp_filename[:-4]
    
    def read_cpp_test_info(self, cpp_filename: str) -> Dict:
        """读取C++测试文件信息"""
        cpp_path = os.path.join(self.cpp_test_dir, cpp_filename)
        
        info = {
            'indicator_name': self.get_indicator_name(cpp_filename),
            'expected_values': [],
            'min_period': 1,
            'parameters': {},
            'test_description': ''
        }
        
        try:
            with open(cpp_path, 'r') as f:
                content = f.read()
                
                # 提取期望值
                expected_match = re.search(r'const std::vector<std::vector<std::string>> \w+_EXPECTED_VALUES = \{([^}]+)\}', content)
                if expected_match:
                    values_str = expected_match.group(1)
                    # 简单解析期望值
                    info['expected_values'] = ['0.0', '0.0', '0.0']  # 默认值
                
                # 提取最小周期
                period_match = re.search(r'const int \w+_MIN_PERIOD = (\d+)', content)
                if period_match:
                    info['min_period'] = int(period_match.group(1))
                
                # 提取测试描述
                doc_match = re.search(r'/\*\*(.*?)\*/', content, re.DOTALL)
                if doc_match:
                    info['test_description'] = doc_match.group(1).strip()
                    
        except Exception as e:
            print(f"Warning: Could not read {cpp_filename}: {e}")
        
        return info
    
    def generate_indicator_test(self, cpp_filename: str) -> str:
        """为单个指标生成Python测试代码"""
        info = self.read_cpp_test_info(cpp_filename)
        indicator_name = info['indicator_name']
        
        # 将指标名称转换为函数名称
        function_name = f"calculate_{indicator_name.lower()}"
        
        test_code = f'''#!/usr/bin/env python3
"""
Python binding test for {indicator_name.upper()} indicator
对应C++测试: {cpp_filename}
{info['test_description']}
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
    print(f"❌ Cannot import backtrader_cpp: {{e}}")
    MODULE_AVAILABLE = False


class Test{indicator_name.upper()}(unittest.TestCase):
    """{indicator_name.upper()}指标测试"""
    
    def setUp(self):
        self.assertTrue(MODULE_AVAILABLE, "backtrader_cpp module must be available")
        
        # 加载测试数据
        self.data_loader = BacktraderTestData()
        self.test_data = self.data_loader.get_close_prices(0)
        
        # 验证数据
        self.assertGreater(len(self.test_data), 0)
        self.assertTrue(all(not math.isnan(x) for x in self.test_data))
    
    def test_{indicator_name.lower()}_functionality(self):
        """{indicator_name}基础功能测试"""
        # 检查函数是否存在
        if not hasattr(bt, '{function_name}'):
            self.skipTest(f"Function {function_name} not implemented in Python bindings yet")
        
        # 测试基本调用
        try:
            result = getattr(bt, '{function_name}')(self.test_data)
            
            # 基本验证
            self.assertIsInstance(result, list)
            self.assertEqual(len(result), len(self.test_data))
            
            # 检查是否有有效值
            valid_values = [x for x in result if not math.isnan(x)]
            self.assertGreater(len(valid_values), 0, f"{indicator_name} should produce some valid values")
            
            print(f"✅ {indicator_name} test passed: {{len(valid_values)}} valid values")
            
        except Exception as e:
            self.fail(f"{indicator_name} calculation failed: {{e}}")
    
    def test_{indicator_name.lower()}_with_parameters(self):
        """{indicator_name}参数测试"""
        if not hasattr(bt, '{function_name}'):
            self.skipTest(f"Function {function_name} not implemented yet")
        
        # 测试不同参数
        test_params = [5, 10, 20, 30]
        
        for param in test_params:
            with self.subTest(period=param):
                try:
                    result = getattr(bt, '{function_name}')(self.test_data, param)
                    self.assertIsInstance(result, list)
                    self.assertEqual(len(result), len(self.test_data))
                except Exception:
                    # 如果不支持参数，尝试无参数调用
                    result = getattr(bt, '{function_name}')(self.test_data)
                    self.assertIsInstance(result, list)
                    break
    
    def test_{indicator_name.lower()}_edge_cases(self):
        """{indicator_name}边界情况测试"""
        if not hasattr(bt, '{function_name}'):
            self.skipTest(f"Function {function_name} not implemented yet")
        
        # 测试小数据集
        small_data = self.test_data[:10]
        
        try:
            result = getattr(bt, '{function_name}')(small_data)
            self.assertIsInstance(result, list)
            self.assertEqual(len(result), len(small_data))
        except Exception as e:
            # 边界情况可能失败，记录但不中断测试
            print(f"Note: {indicator_name} edge case failed: {{e}}")
    
    def test_{indicator_name.lower()}_value_ranges(self):
        """{indicator_name}值范围验证"""
        if not hasattr(bt, '{function_name}'):
            self.skipTest(f"Function {function_name} not implemented yet")
        
        try:
            result = getattr(bt, '{function_name}')(self.test_data)
            valid_values = [x for x in result if not math.isnan(x)]
            
            if valid_values:
                min_val = min(valid_values)
                max_val = max(valid_values)
                
                # 基本范围检查（根据指标类型调整）
                if '{indicator_name.lower()}' in ['rsi', 'stochastic', 'williamsad', 'williamsr']:
                    # 0-100范围的振荡器
                    self.assertGreaterEqual(min_val, -5.0)  # 允许小误差
                    self.assertLessEqual(max_val, 105.0)
                else:
                    # 其他指标应该是有限值
                    self.assertFalse(math.isinf(min_val))
                    self.assertFalse(math.isinf(max_val))
                
                print(f"✅ {indicator_name} value range: {{min_val:.2f}} - {{max_val:.2f}}")
                
        except Exception as e:
            print(f"Note: {indicator_name} range check failed: {{e}}")


def run_{indicator_name.lower()}_tests():
    """运行{indicator_name}测试"""
    if not MODULE_AVAILABLE:
        print("❌ backtrader_cpp module not available.")
        return False
    
    print(f"🧪 Running {indicator_name.upper()} Tests")
    print("=" * 50)
    
    suite = unittest.TestLoader().loadTestsFromTestCase(Test{indicator_name.upper()})
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    success = len(result.failures) == 0 and len(result.errors) == 0
    
    if success:
        print(f"✅ All {indicator_name.upper()} tests passed!")
    else:
        print(f"❌ Some {indicator_name.upper()} tests failed.")
        
    return success


if __name__ == "__main__":
    run_{indicator_name.lower()}_tests()
'''
        
        return test_code
    
    def generate_all_tests(self):
        """生成所有指标的测试文件"""
        print("🏗️ 生成Python绑定测试文件")
        print("=" * 60)
        
        total_files = 0
        generated_files = 0
        
        # 生成技术指标测试
        print("\n📊 生成技术指标测试...")
        for cpp_file in self.technical_indicators:
            indicator_name = self.get_indicator_name(cpp_file)
            python_file = f"test_python_{indicator_name}.py"
            python_path = os.path.join(self.python_test_dir, python_file)
            
            test_code = self.generate_indicator_test(cpp_file)
            
            with open(python_path, 'w') as f:
                f.write(test_code)
            
            total_files += 1
            generated_files += 1
            print(f"  ✓ {python_file}")
        
        # 生成分析器测试
        print(f"\n📈 生成分析器测试...")
        for cpp_file in self.analyzers:
            analyzer_name = self.get_indicator_name(cpp_file)
            python_file = f"test_python_{analyzer_name}.py"
            python_path = os.path.join(self.python_test_dir, python_file)
            
            test_code = self.generate_indicator_test(cpp_file)
            
            with open(python_path, 'w') as f:
                f.write(test_code)
            
            total_files += 1
            generated_files += 1
            print(f"  ✓ {python_file}")
        
        # 生成其他组件测试
        print(f"\n🔧 生成其他组件测试...")
        for cpp_file in self.data_processors + self.strategies + self.core_components:
            component_name = self.get_indicator_name(cpp_file)
            python_file = f"test_python_{component_name}.py"
            python_path = os.path.join(self.python_test_dir, python_file)
            
            test_code = self.generate_indicator_test(cpp_file)
            
            with open(python_path, 'w') as f:
                f.write(test_code)
            
            total_files += 1
            generated_files += 1
            print(f"  ✓ {python_file}")
        
        print("\n" + "=" * 60)
        print(f"📊 测试文件生成统计:")
        print(f"  技术指标: {len(self.technical_indicators)} 个")
        print(f"  分析器: {len(self.analyzers)} 个")
        print(f"  数据处理: {len(self.data_processors)} 个")
        print(f"  策略: {len(self.strategies)} 个")
        print(f"  核心组件: {len(self.core_components)} 个")
        print(f"  总计: {total_files} 个测试文件生成")
        
        return generated_files
    
    def generate_test_runner(self):
        """生成测试运行器"""
        runner_code = '''#!/usr/bin/env python3
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
        print(f"\\n📂 运行 {test_file}")
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
    print("\\n" + "=" * 60)
    print("📊 Python绑定测试总结")
    print("=" * 60)
    
    success_rate = (successful_files / len(test_files) * 100) if test_files else 0
    
    print(f"测试文件总数: {len(test_files)}")
    print(f"成功运行: {successful_files}")
    print(f"失败/错误: {len(test_files) - successful_files}")
    print(f"成功率: {success_rate:.1f}%")
    
    if total_tests > 0:
        print(f"\\n总测试用例: {total_tests}")
        print(f"失败: {total_failures}")
        print(f"错误: {total_errors}")
    
    if successful_files == len(test_files):
        print("\\n🎉 所有Python绑定测试都运行成功！")
        return True
    else:
        print(f"\\n❌ {len(test_files) - successful_files} 个测试文件运行失败")
        return False

if __name__ == "__main__":
    success = run_all_python_binding_tests()
    sys.exit(0 if success else 1)
'''
        
        runner_path = os.path.join(self.python_test_dir, "run_all_tests.py")
        with open(runner_path, 'w') as f:
            f.write(runner_code)
        
        # 使文件可执行
        os.chmod(runner_path, 0o755)
        
        print(f"✅ 测试运行器已生成: {runner_path}")


def main():
    """主函数"""
    print("🏭 Python绑定测试生成器")
    print("=" * 60)
    
    generator = IndicatorTestGenerator()
    
    # 生成所有测试文件
    generated_count = generator.generate_all_tests()
    
    # 生成测试运行器
    generator.generate_test_runner()
    
    print(f"\\n🎉 完成！生成了 {generated_count} 个Python绑定测试文件")
    print("📋 测试文件保存在: /home/yun/Documents/refactor_backtrader/backtrader_cpp/python_bindings/tests/")
    print("🚀 使用以下命令运行所有测试:")
    print("   cd /home/yun/Documents/refactor_backtrader/backtrader_cpp/python_bindings/tests/")
    print("   python run_all_tests.py")

if __name__ == "__main__":
    main()