#include <iostream>
#include <memory>
#include <iomanip>
#include "test_common_simple.h"
#include "indicators/EMA.h"

using namespace backtrader::tests::original;
using namespace backtrader;

int main() {
    // Create simple test data
    std::vector<double> prices = {100, 101, 102, 103, 104, 105};
    
    auto line = std::make_shared<LineRoot>(prices.size(), "test");
    for (double price : prices) {
        line->forward(price);
    }
    
    std::cout << "Line created with " << line->len() << " points" << std::endl;
    
    // Reset line to beginning
    line->home();
    
    // Create EMA
    auto ema = std::make_shared<EMA>(line, 3);
    std::cout << "EMA created with period: " << ema->getPeriod() << std::endl;
    
    // Test calculation
    std::cout << "\nBefore calculation, line len=" << line->len() << std::endl;
    std::cout << "Line values: ";
    for (int i = -5; i <= 0; ++i) {
        std::cout << "[" << i << "]=" << line->get(i) << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\nEMA calculation:" << std::endl;
    for (size_t i = 0; i < prices.size(); ++i) {
        std::cout << "Before calc " << i << ": line[0]=" << line->get(0) << " len=" << line->len() << std::endl;
        ema->calculate();
        
        double current_price = line->get(0);
        double ema_value = ema->get(0);
        
        std::cout << "Step " << i << ": Price=" << current_price 
                  << " EMA=" << ema_value << std::endl;
        
        if (i < prices.size() - 1) {
            line->forward();
        }
    }
    
    return 0;
}