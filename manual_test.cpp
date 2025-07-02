/**
 * @file manual_test.cpp
 * @brief 手动测试核心功能
 */

#include <iostream>
#include <vector>
#include <iomanip>

// 核心头文件
#include "LineRoot.h"
#include "indicators/SMA.h"
#include "indicators/EMA.h"

int main() {
    std::cout << "=== Backtrader C++ Manual Test ===" << std::endl;
    
    try {
        // 测试1: LineRoot基本功能
        std::cout << "\n1. Testing LineRoot..." << std::endl;
        auto line = std::make_shared<backtrader::LineRoot>(10, "test_line");
        
        std::vector<double> test_data = {1.0, 2.0, 3.0, 4.0, 5.0};
        for (double value : test_data) {
            line->forward(value);
        }
        
        std::cout << "   Current value: " << line->get(0) << std::endl;
        std::cout << "   Previous value: " << line->get(-1) << std::endl;
        std::cout << "   Size: " << line->size() << std::endl;
        
        // 测试2: SMA指标
        std::cout << "\n2. Testing SMA..." << std::endl;
        auto close_line = std::make_shared<backtrader::LineRoot>(100, "close");
        
        // 添加更多测试数据
        std::vector<double> prices = {
            100.0, 101.0, 102.0, 103.0, 104.0,
            105.0, 106.0, 107.0, 108.0, 109.0,
            110.0, 111.0, 112.0, 113.0, 114.0
        };
        
        for (double price : prices) {
            close_line->forward(price);
        }
        
        auto sma5 = std::make_shared<backtrader::indicators::SMA>(close_line, 5);
        
        // 计算SMA值
        for (size_t i = 0; i < prices.size(); ++i) {
            sma5->calculate();
            if (i < prices.size() - 1) {
                close_line->forward();
            }
        }
        
        std::cout << "   SMA(5) current value: " << sma5->get(0) << std::endl;
        std::cout << "   SMA(5) min period: " << sma5->getMinPeriod() << std::endl;
        
        // 测试3: EMA指标
        std::cout << "\n3. Testing EMA..." << std::endl;
        close_line->home(); // 重置到开始位置
        
        auto ema5 = std::make_shared<backtrader::indicators::EMA>(close_line, 5);
        
        // 计算EMA值
        for (size_t i = 0; i < prices.size(); ++i) {
            ema5->calculate();
            if (i < prices.size() - 1) {
                close_line->forward();
            }
        }
        
        std::cout << "   EMA(5) current value: " << ema5->get(0) << std::endl;
        std::cout << "   EMA(5) min period: " << ema5->getMinPeriod() << std::endl;
        
        std::cout << "\n=== All tests completed successfully! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}