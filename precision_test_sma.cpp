#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <limits>

// 简单的SMA实现（标准算法）
class SimpleSMA {
private:
    std::vector<double> buffer;
    int period;
    int count;
    
public:
    SimpleSMA(int period) : period(period), count(0) {
        buffer.reserve(period);
    }
    
    void add(double value) {
        if (buffer.size() < period) {
            buffer.push_back(value);
        } else {
            buffer[count % period] = value;
        }
        count++;
    }
    
    double get() const {
        if (buffer.size() == 0) return std::numeric_limits<double>::quiet_NaN();
        
        double sum = 0.0;
        int size = std::min(count, period);
        for (int i = 0; i < size; ++i) {
            sum += buffer[i];
        }
        return sum / size;
    }
    
    bool isValid() const {
        return count >= period;
    }
};

// Kahan求和SMA实现
class KahanSMA {
private:
    std::vector<double> buffer;
    int period;
    int count;
    double sum;
    double c; // Kahan求和补偿项
    
public:
    KahanSMA(int period) : period(period), count(0), sum(0.0), c(0.0) {
        buffer.reserve(period);
    }
    
    void add(double value) {
        if (buffer.size() < period) {
            // 添加新值到缓冲区
            buffer.push_back(value);
            
            // Kahan求和添加
            double y = value - c;
            double t = sum + y;
            c = (t - sum) - y;
            sum = t;
        } else {
            // 移除旧值，添加新值
            double old_value = buffer[count % period];
            buffer[count % period] = value;
            
            // Kahan求和移除旧值
            double y = -old_value - c;
            double t = sum + y;
            c = (t - sum) - y;
            sum = t;
            
            // Kahan求和添加新值
            y = value - c;
            t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }
        count++;
    }
    
    double get() const {
        if (buffer.size() == 0) return std::numeric_limits<double>::quiet_NaN();
        
        int size = std::min(count, period);
        return sum / size;
    }
    
    bool isValid() const {
        return count >= period;
    }
};

// 手动计算精确平均值（用于验证）
double calculateExactMean(const std::vector<double>& values, int start, int period) {
    if (start + period > values.size()) return std::numeric_limits<double>::quiet_NaN();
    
    long double sum = 0.0L;
    for (int i = start; i < start + period; ++i) {
        sum += values[i];
    }
    return static_cast<double>(sum / period);
}

int main() {
    std::cout << std::fixed << std::setprecision(15);
    
    // 创建测试数据 - 使用一些可能导致精度问题的值
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
    SimpleSMA simple_sma(period);
    KahanSMA kahan_sma(period);
    
    std::cout << "=== SMA精度对比测试 ===" << std::endl;
    std::cout << "数据点\t\t简单算法\t\tKahan算法\t\t精确值\t\t\t简单误差\t\tKahan误差" << std::endl;
    std::cout << std::string(120, '-') << std::endl;
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        simple_sma.add(test_data[i]);
        kahan_sma.add(test_data[i]);
        
        if (i >= period - 1) {  // 当有足够数据计算SMA时
            double simple_result = simple_sma.get();
            double kahan_result = kahan_sma.get();
            double exact_result = calculateExactMean(test_data, i - period + 1, period);
            
            double simple_error = std::abs(simple_result - exact_result);
            double kahan_error = std::abs(kahan_result - exact_result);
            
            std::cout << i + 1 << "\t\t"
                      << simple_result << "\t"
                      << kahan_result << "\t"
                      << exact_result << "\t"
                      << simple_error << "\t"
                      << kahan_error << std::endl;
        }
    }
    
    std::cout << std::endl;
    
    // 测试极端情况 - 大数和小数混合
    std::cout << "=== 极端精度测试（大数和小数混合） ===" << std::endl;
    
    std::vector<double> extreme_data = {
        1e15,           // 很大的数
        1e-15,          // 很小的数
        1e15 + 1.0,     // 略大于第一个数
        2e-15,          // 略大于第二个数
        1e15 - 1.0      // 略小于第一个数
    };
    
    SimpleSMA extreme_simple(5);
    KahanSMA extreme_kahan(5);
    
    for (size_t i = 0; i < extreme_data.size(); ++i) {
        extreme_simple.add(extreme_data[i]);
        extreme_kahan.add(extreme_data[i]);
    }
    
    double extreme_simple_result = extreme_simple.get();
    double extreme_kahan_result = extreme_kahan.get();
    double extreme_exact_result = calculateExactMean(extreme_data, 0, 5);
    
    std::cout << "简单算法结果: " << extreme_simple_result << std::endl;
    std::cout << "Kahan算法结果: " << extreme_kahan_result << std::endl;
    std::cout << "精确结果:     " << extreme_exact_result << std::endl;
    std::cout << "简单算法误差: " << std::abs(extreme_simple_result - extreme_exact_result) << std::endl;
    std::cout << "Kahan算法误差: " << std::abs(extreme_kahan_result - extreme_exact_result) << std::endl;
    
    // 判断哪个算法更精确
    double total_simple_error = 0.0;
    double total_kahan_error = 0.0;
    int comparison_count = 0;
    
    SimpleSMA comp_simple(period);
    KahanSMA comp_kahan(period);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        comp_simple.add(test_data[i]);
        comp_kahan.add(test_data[i]);
        
        if (i >= period - 1) {
            double simple_result = comp_simple.get();
            double kahan_result = comp_kahan.get();
            double exact_result = calculateExactMean(test_data, i - period + 1, period);
            
            total_simple_error += std::abs(simple_result - exact_result);
            total_kahan_error += std::abs(kahan_result - exact_result);
            comparison_count++;
        }
    }
    
    std::cout << std::endl << "=== 总结 ===" << std::endl;
    std::cout << "平均简单算法误差: " << (total_simple_error / comparison_count) << std::endl;
    std::cout << "平均Kahan算法误差: " << (total_kahan_error / comparison_count) << std::endl;
    
    if (total_kahan_error < total_simple_error) {
        std::cout << "✓ Kahan求和算法精度更高，误差减少了 " 
                  << ((total_simple_error - total_kahan_error) / total_simple_error * 100) 
                  << "%" << std::endl;
    } else if (total_simple_error == total_kahan_error) {
        std::cout << "两种算法精度相同" << std::endl;
    } else {
        std::cout << "简单算法精度更高（在这个测试案例中）" << std::endl;
    }
    
    return 0;
}