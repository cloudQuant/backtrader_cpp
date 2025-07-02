#include "indicators/SMA.h"
#include "LineRoot.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <limits>

using namespace backtrader;

int main() {
    std::cout << std::fixed << std::setprecision(15);
    
    // 创建测试数据 - 包含极端值以测试精度
    std::vector<double> test_data = {
        1.23456789012345,
        2.34567890123456,
        3.45678901234567,
        0.00000000000001,
        1000000.00000001,
        0.99999999999999,
        4.56789012345678,
        5.67890123456789,
        6.78901234567890,
        7.89012345678901,
        8.90123456789012,
        9.01234567890123,
        10.1234567890123,
        11.2345678901234,
        12.3456789012345
    };
    
    const int period = 5;
    
    // 创建输入数据线
    auto input_line = std::make_shared<LineRoot>(test_data.size(), "test_input");
    
    // 创建SMA指标 - 测试两种模式
    auto sma_incremental = std::make_shared<SMA>(input_line, period, true);   // 增量计算
    auto sma_direct = std::make_shared<SMA>(input_line, period, false);       // 直接计算（使用Kahan求和）
    
    std::cout << "=== 测试实际SMA实现 ===" << std::endl;
    std::cout << "周期: " << period << std::endl;
    std::cout << std::endl;
    
    std::cout << "数据点\t\t输入值\t\t\t增量模式\t\t直接模式(Kahan)" << std::endl;
    std::cout << std::string(100, '-') << std::endl;
    
    // 逐步添加数据并计算
    for (size_t i = 0; i < test_data.size(); ++i) {
        input_line->forward(test_data[i]);
        
        sma_incremental->calculate();
        sma_direct->calculate();
        
        double incremental_result = sma_incremental->get(0);
        double direct_result = sma_direct->get(0);
        
        std::cout << i + 1 << "\t\t" 
                  << test_data[i] << "\t";
        
        if (std::isnan(incremental_result)) {
            std::cout << "NaN\t\t\t";
        } else {
            std::cout << incremental_result << "\t";
        }
        
        if (std::isnan(direct_result)) {
            std::cout << "NaN";
        } else {
            std::cout << direct_result;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << std::endl;
    
    // 验证两种方法在有效数据上的一致性
    std::cout << "=== 精度对比分析 ===" << std::endl;
    
    // 重新计算最后几个值进行精度比较
    auto input_line2 = std::make_shared<LineRoot>(test_data.size(), "test_input2");
    auto sma_inc2 = std::make_shared<SMA>(input_line2, period, true);
    auto sma_dir2 = std::make_shared<SMA>(input_line2, period, false);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        input_line2->forward(test_data[i]);
        sma_inc2->calculate();
        sma_dir2->calculate();
        
        if (i >= period - 1) {  // 有足够数据计算SMA
            double inc_result = sma_inc2->get(0);
            double dir_result = sma_dir2->get(0);
            
            // 手动计算精确值用于对比
            long double exact_sum = 0.0L;
            for (size_t j = i - period + 1; j <= i; ++j) {
                exact_sum += test_data[j];
            }
            double exact_result = static_cast<double>(exact_sum / period);
            
            double inc_error = std::abs(inc_result - exact_result);
            double dir_error = std::abs(dir_result - exact_result);
            
            std::cout << "位置 " << (i + 1) << ":" << std::endl;
            std::cout << "  增量模式: " << inc_result << " (误差: " << inc_error << ")" << std::endl;
            std::cout << "  直接模式: " << dir_result << " (误差: " << dir_error << ")" << std::endl;
            std::cout << "  精确值:   " << exact_result << std::endl;
            std::cout << std::endl;
        }
    }
    
    // 测试极端数值的精度
    std::cout << "=== 极端数值精度测试 ===" << std::endl;
    
    std::vector<double> extreme_data = {
        1e15,           // 很大的数
        1e-15,          // 很小的数
        1e15 + 1.0,     // 略大于第一个数
        2e-15,          // 略大于第二个数
        1e15 - 1.0,     // 略小于第一个数
        3.141592653589793,  // π
        2.718281828459045,  // e
        1.414213562373095   // √2
    };
    
    auto extreme_input = std::make_shared<LineRoot>(extreme_data.size(), "extreme_test");
    auto extreme_sma_inc = std::make_shared<SMA>(extreme_input, 5, true);
    auto extreme_sma_dir = std::make_shared<SMA>(extreme_input, 5, false);
    
    for (double value : extreme_data) {
        extreme_input->forward(value);
        extreme_sma_inc->calculate();
        extreme_sma_dir->calculate();
    }
    
    double extreme_inc_result = extreme_sma_inc->get(0);
    double extreme_dir_result = extreme_sma_dir->get(0);
    
    // 手工计算精确值
    long double extreme_exact_sum = 0.0L;
    for (size_t i = extreme_data.size() - 5; i < extreme_data.size(); ++i) {
        extreme_exact_sum += extreme_data[i];
    }
    double extreme_exact_result = static_cast<double>(extreme_exact_sum / 5.0);
    
    std::cout << "极端数据的最后5个值:" << std::endl;
    for (size_t i = extreme_data.size() - 5; i < extreme_data.size(); ++i) {
        std::cout << "  " << extreme_data[i] << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "增量模式结果: " << extreme_inc_result << std::endl;
    std::cout << "直接模式结果: " << extreme_dir_result << std::endl;
    std::cout << "精确结果:     " << extreme_exact_result << std::endl;
    std::cout << std::endl;
    
    std::cout << "增量模式误差: " << std::abs(extreme_inc_result - extreme_exact_result) << std::endl;
    std::cout << "直接模式误差: " << std::abs(extreme_dir_result - extreme_exact_result) << std::endl;
    
    if (std::abs(extreme_dir_result - extreme_exact_result) < 
        std::abs(extreme_inc_result - extreme_exact_result)) {
        std::cout << "✓ 直接模式（Kahan求和）在极端数值测试中精度更高！" << std::endl;
    } else if (std::abs(extreme_inc_result - extreme_exact_result) < 
               std::abs(extreme_dir_result - extreme_exact_result)) {
        std::cout << "增量模式在极端数值测试中精度更高" << std::endl;
    } else {
        std::cout << "两种模式精度相同" << std::endl;
    }
    
    return 0;
}