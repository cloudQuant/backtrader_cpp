#include <iostream>
#include <memory>
#include <iomanip>
#include "test_common_simple.h"
#include "indicators/MACD.h"

using namespace backtrader::tests::original;
using namespace backtrader;

int main() {
    // Load test data
    auto csv_data = getdata(0);
    std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
    
    // Create close line
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    std::cout << "Close line created with " << close_line->len() << " points" << std::endl;
    
    // Create MACD indicator
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    std::cout << "MACD created with min period: " << macd->getMinPeriod() << std::endl;
    
    // Check the EMA components
    std::cout << "Fast EMA period: " << macd->getFastPeriod() << std::endl;
    std::cout << "Slow EMA period: " << macd->getSlowPeriod() << std::endl;
    std::cout << "Signal period: " << macd->getSignalPeriod() << std::endl;
    
    // Calculate first few values to see what happens
    std::cout << "\nFirst 40 calculation steps:" << std::endl;
    std::cout << "Step  | Price      | MACD       | Signal     | Histogram" << std::endl;
    std::cout << "------|------------|------------|------------|----------" << std::endl;
    
    for (size_t i = 0; i < std::min(size_t(40), csv_data.size()); ++i) {
        macd->calculate();
        
        double price = csv_data[i].close;
        double macd_val = macd->getMACDLine(0);
        double signal_val = macd->getSignalLine(0);
        double hist_val = macd->getHistogram(0);
        
        std::cout << std::setw(5) << i 
                  << " | " << std::setw(10) << std::fixed << std::setprecision(2) << price
                  << " | " << std::setw(10) << std::fixed << std::setprecision(6) 
                  << (std::isnan(macd_val) ? "NaN" : std::to_string(macd_val))
                  << " | " << std::setw(10) << std::fixed << std::setprecision(6) 
                  << (std::isnan(signal_val) ? "NaN" : std::to_string(signal_val))
                  << " | " << std::setw(10) << std::fixed << std::setprecision(6) 
                  << (std::isnan(hist_val) ? "NaN" : std::to_string(hist_val))
                  << std::endl;
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "\nFinal values:" << std::endl;
    // Continue to the end
    for (size_t i = 40; i < csv_data.size(); ++i) {
        macd->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    double final_macd = macd->getMACDLine(0);
    double final_signal = macd->getSignalLine(0);
    double final_hist = macd->getHistogram(0);
    
    std::cout << "Final MACD: " << final_macd << std::endl;
    std::cout << "Final Signal: " << final_signal << std::endl;
    std::cout << "Final Histogram: " << final_hist << std::endl;
    
    return 0;
}