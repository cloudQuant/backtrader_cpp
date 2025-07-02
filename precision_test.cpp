/**
 * @file precision_test.cpp
 * @brief 精度测试程序 - 分析SMA计算的精确性
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>

#include "LineRoot.h"
#include "indicators/SMA.h"

using namespace backtrader;

// CSV读取器
struct OHLCVData {
    std::string date;
    double open, high, low, close, volume, openinterest;
};

std::vector<OHLCVData> loadTestData() {
    std::vector<OHLCVData> data;
    std::ifstream file("../tests/datas/2006-day-001.txt");
    
    if (!file.is_open()) {
        std::cerr << "Cannot open test data file" << std::endl;
        return data;
    }
    
    std::string line;
    if (std::getline(file, line)) {  // 跳过标题
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string item;
            OHLCVData bar;
            
            if (std::getline(ss, item, ',')) bar.date = item;
            if (std::getline(ss, item, ',')) bar.open = std::stod(item);
            if (std::getline(ss, item, ',')) bar.high = std::stod(item);
            if (std::getline(ss, item, ',')) bar.low = std::stod(item);
            if (std::getline(ss, item, ',')) bar.close = std::stod(item);
            if (std::getline(ss, item, ',')) bar.volume = std::stod(item);
            if (std::getline(ss, item, ',')) bar.openinterest = std::stod(item);
            
            data.push_back(bar);
        }
    }
    return data;
}

// Kahan求和算法
double kahanSum(const std::vector<double>& values) {
    double sum = 0.0;
    double compensation = 0.0;
    
    for (double value : values) {
        double y = value - compensation;
        double t = sum + y;
        compensation = (t - sum) - y;
        sum = t;
    }
    
    return sum;
}

int main() {
    std::cout << "=== Precision Analysis Test ===" << std::endl;
    std::cout << std::fixed << std::setprecision(8);
    
    auto csv_data = loadTestData();
    if (csv_data.empty()) {
        std::cerr << "No test data loaded" << std::endl;
        return 1;
    }
    
    std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
    
    // 创建数据线和SMA指标
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    auto sma30 = std::make_shared<SMA>(close_line, 30);
    
    // 存储所有价格数据
    std::vector<double> all_prices;
    
    // 逐步处理数据
    for (size_t i = 0; i < csv_data.size(); ++i) {
        all_prices.push_back(csv_data[i].close);
        close_line->forward(csv_data[i].close);
        sma30->calculate();
    }
    
    // 计算检查点
    int l = static_cast<int>(csv_data.size());
    int mp = 30;
    std::vector<int> chkpts = {0, -(l - mp), -(l - mp) / 2};
    
    std::cout << "\nData length: " << l << ", min period: " << mp << std::endl;
    std::cout << "Check points: ";
    for (int chkpt : chkpts) {
        std::cout << chkpt << " ";
    }
    std::cout << std::endl;
    
    // 手动验证每个检查点
    std::vector<std::string> expected = {"4063.463000", "3644.444667", "3554.693333"};
    
    for (size_t i = 0; i < chkpts.size(); ++i) {
        double sma_value = sma30->get(chkpts[i]);
        
        // 计算对应的数据窗口（倒推到原始数据位置）
        int data_pos = static_cast<int>(csv_data.size()) + chkpts[i] - 1;
        
        std::cout << "\n--- Check Point " << i << " ---" << std::endl;
        std::cout << "SMA offset: " << chkpts[i] << std::endl;
        std::cout << "Data position: " << data_pos << " (0-based)" << std::endl;
        std::cout << "Expected: " << expected[i] << std::endl;
        std::cout << "Actual:   " << sma_value << std::endl;
        
        // 手动计算这个位置的SMA
        if (data_pos >= 29) {  // 确保有足够数据
            std::vector<double> window;
            for (int j = 0; j < 30; ++j) {
                window.push_back(all_prices[data_pos - j]);
            }
            
            // 使用不同的求和方法
            double simple_sum = 0.0;
            for (double val : window) {
                simple_sum += val;
            }
            double simple_avg = simple_sum / 30.0;
            
            double kahan_avg = kahanSum(window) / 30.0;
            
            std::cout << "Manual (simple): " << simple_avg << std::endl;
            std::cout << "Manual (Kahan):  " << kahan_avg << std::endl;
            
            // 显示价格窗口的前几个和后几个值
            std::cout << "Price window [0-4]: ";
            for (int j = 0; j < 5 && j < 30; ++j) {
                std::cout << window[j] << " ";
            }
            std::cout << "... ";
            for (int j = 25; j < 30; ++j) {
                std::cout << window[j] << " ";
            }
            std::cout << std::endl;
        }
        
        double diff = sma_value - std::stod(expected[i]);
        double pct_diff = std::abs(diff / std::stod(expected[i])) * 100.0;
        std::cout << "Difference: " << diff << " (" << pct_diff << "%)" << std::endl;
    }
    
    return 0;
}