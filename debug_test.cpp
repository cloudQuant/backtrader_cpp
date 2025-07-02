/**
 * @file debug_test.cpp
 * @brief 调试测试程序
 */

#include <iostream>
#include <vector>
#include <iomanip>

#include "LineRoot.h"
#include "indicators/SMA.h"

using namespace backtrader;

int main() {
    std::cout << "=== Debug Test ===" << std::endl;
    
    try {
        // 测试CircularBuffer负索引
        std::cout << "\n1. Testing CircularBuffer negative indexing..." << std::endl;
        auto line = std::make_shared<LineRoot>(10, "test");
        
        std::vector<double> test_data = {1.0, 2.0, 3.0, 4.0, 5.0};
        for (double value : test_data) {
            line->forward(value);
            std::cout << "   Added: " << value << ", size: " << line->len() << std::endl;
        }
        
        std::cout << "   Current (0): " << line->get(0) << std::endl;
        std::cout << "   Previous (-1): " << line->get(-1) << std::endl;
        std::cout << "   Before that (-2): " << line->get(-2) << std::endl;
        
        // 测试SMA
        std::cout << "\n2. Testing SMA calculation..." << std::endl;
        auto close_line = std::make_shared<LineRoot>(100, "close");
        auto sma5 = std::make_shared<SMA>(close_line, 5);
        
        std::vector<double> prices = {10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0};
        
        for (size_t i = 0; i < prices.size(); ++i) {
            close_line->forward(prices[i]);
            sma5->calculate();
            
            std::cout << "   Step " << i+1 << ": price=" << prices[i] 
                      << ", data_len=" << close_line->len() 
                      << ", sma=" << sma5->get(0) << std::endl;
        }
        
        // 计算期望的最后5个值的平均值
        double expected = (16.0 + 17.0 + 18.0 + 19.0 + 20.0) / 5.0;
        std::cout << "   Expected SMA(5): " << expected << std::endl;
        std::cout << "   Actual SMA(5): " << sma5->get(0) << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}