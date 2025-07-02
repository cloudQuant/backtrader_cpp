#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "indicators/BollingerBands.h"
#include "LineRoot.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>

using namespace backtrader;

// 测试数据 - 模拟股价
std::vector<double> createTestData() {
    return {
        150.0, 151.5, 149.8, 152.3, 150.7, 153.1, 151.9, 154.2, 152.6, 155.0,
        153.4, 156.8, 154.1, 157.5, 155.9, 158.3, 156.7, 159.1, 157.4, 160.0,
        158.2, 161.5, 159.8, 162.3, 160.7, 163.1, 161.9, 164.2, 162.6, 165.0,
        163.4, 166.8, 164.1, 167.5, 165.9, 168.3, 166.7, 169.1, 167.4, 170.0
    };
}

void testSMA() {
    std::cout << "=== SMA测试 ===" << std::endl;
    
    auto test_data = createTestData();
    auto input_line = std::make_shared<LineRoot>(test_data.size(), "test_input");
    auto sma = std::make_shared<SMA>(input_line, 10);  // 10期SMA
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        input_line->forward(test_data[i]);
        sma->calculate();
        
        double result = sma->get(0);
        if (!std::isnan(result)) {
            std::cout << "SMA[" << i << "] = " << std::fixed << std::setprecision(6) << result << std::endl;
        }
    }
    std::cout << std::endl;
}

void testRSI() {
    std::cout << "=== RSI测试 ===" << std::endl;
    
    auto test_data = createTestData();
    auto input_line = std::make_shared<LineRoot>(test_data.size(), "test_input");
    auto rsi = std::make_shared<RSI>(input_line, 14);  // 14期RSI
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        input_line->forward(test_data[i]);
        rsi->calculate();
        
        double result = rsi->get(0);
        if (!std::isnan(result)) {
            std::cout << "RSI[" << i << "] = " << std::fixed << std::setprecision(6) << result << std::endl;
        }
    }
    std::cout << std::endl;
}

void testMACD() {
    std::cout << "=== MACD测试 ===" << std::endl;
    
    auto test_data = createTestData();
    auto input_line = std::make_shared<LineRoot>(test_data.size(), "test_input");
    auto macd = std::make_shared<MACD>(input_line, 12, 26, 9);  // 标准MACD参数
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        input_line->forward(test_data[i]);
        macd->calculate();
        
        double macd_line = macd->get(0);
        double signal_line = macd->get(1);
        double histogram = macd->get(2);
        
        if (!std::isnan(macd_line)) {
            std::cout << "MACD[" << i << "] = " 
                      << std::fixed << std::setprecision(6) 
                      << "MACD: " << macd_line 
                      << ", Signal: " << signal_line 
                      << ", Histogram: " << histogram << std::endl;
        }
    }
    std::cout << std::endl;
}

void testBollingerBands() {
    std::cout << "=== BollingerBands测试 ===" << std::endl;
    
    auto test_data = createTestData();
    auto input_line = std::make_shared<LineRoot>(test_data.size(), "test_input");
    auto bb = std::make_shared<BollingerBands>(input_line, 20, 2.0);  // 20期, 2倍标准差
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        input_line->forward(test_data[i]);
        bb->calculate();
        
        double middle = bb->get(0);
        double upper = bb->get(1);
        double lower = bb->get(2);
        
        if (!std::isnan(middle)) {
            std::cout << "BB[" << i << "] = " 
                      << std::fixed << std::setprecision(6) 
                      << "Upper: " << upper 
                      << ", Middle: " << middle 
                      << ", Lower: " << lower << std::endl;
        }
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "测试所有指标实现" << std::endl;
    std::cout << "=================" << std::endl << std::endl;
    
    try {
        testSMA();
        testRSI();
        testMACD();
        testBollingerBands();
        
        std::cout << "✓ 所有指标测试完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}