/**
 * @file test_sma_debug.cpp
 * @brief SMA调试测试程序
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "LineRoot.h"
#include "indicators/SMA.h"

using namespace backtrader;

// 简化的CSV读取器
struct OHLCVData {
    std::string date;
    double open, high, low, close, volume, openinterest;
};

std::vector<OHLCVData> loadTestData() {
    std::vector<OHLCVData> data;
    std::ifstream file("tests/datas/2006-day-001.txt");
    
    if (!file.is_open()) {
        std::cerr << "Cannot open test data file" << std::endl;
        return data;
    }
    
    std::string line;
    // 跳过标题行
    if (std::getline(file, line)) {
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

int main() {
    std::cout << "=== SMA Debug Test ===" << std::endl;
    
    try {
        // 加载测试数据
        auto csv_data = loadTestData();
        if (csv_data.empty()) {
            std::cerr << "No test data loaded" << std::endl;
            return 1;
        }
        
        std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
        
        // 创建数据线和SMA指标
        auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
        auto sma30 = std::make_shared<SMA>(close_line, 30);
        
        std::cout << "SMA minimum period: " << sma30->getMinPeriod() << std::endl;
        
        // 逐步处理数据
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_line->forward(csv_data[i].close);
            sma30->calculate();
            
            // 输出前几个和最后几个值
            if (i < 35 || i >= csv_data.size() - 5) {
                std::cout << "Step " << std::setw(3) << i+1 
                          << ": price=" << std::fixed << std::setprecision(2) << csv_data[i].close
                          << ", data_len=" << close_line->len()
                          << ", sma=" << std::setprecision(6) << sma30->get(0) << std::endl;
            } else if (i == 35) {
                std::cout << "..." << std::endl;
            }
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
        
        // 输出检查点的值
        std::cout << "SMA values at check points:" << std::endl;
        for (size_t i = 0; i < chkpts.size(); ++i) {
            double val = sma30->get(chkpts[i]);
            std::cout << "  chkpts[" << i << "] = " << chkpts[i] 
                      << " -> " << std::fixed << std::setprecision(6) << val << std::endl;
        }
        
        // 期望值
        std::vector<std::string> expected = {"4063.463000", "3644.444667", "3554.693333"};
        std::cout << "\nExpected values:" << std::endl;
        for (size_t i = 0; i < expected.size(); ++i) {
            std::cout << "  expected[" << i << "] = " << expected[i] << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}