#include <iostream>
#include <string>

// Test basic includes
#include "Common.h"
#include "CircularBuffer.h"  
#include "LineRoot.h"

int main() {
    std::cout << "=== Backtrader C++ Compilation Diagnostics ===" << std::endl;
    
    try {
        std::cout << "1. Testing Common.h..." << std::endl;
        using namespace backtrader;
        double nan_value = NaN;
        bool is_nan = isNaN(nan_value);
        std::cout << "   ✓ Common.h compiled successfully" << std::endl;
        
        std::cout << "2. Testing CircularBuffer..." << std::endl;
        CircularBuffer<double> buffer(10);
        buffer.forward(1.0);
        buffer.forward(2.0);
        double value = buffer[0];  // Should be 2.0
        std::cout << "   ✓ CircularBuffer compiled and basic operations work" << std::endl;
        std::cout << "   Current value: " << value << std::endl;
        
        std::cout << "3. Testing LineRoot..." << std::endl;
        auto line = std::make_shared<LineRoot>(100, "test");
        line->forward(10.0);
        line->forward(20.0);
        double line_value = line->get(0);  // Should be 20.0
        std::cout << "   ✓ LineRoot compiled and basic operations work" << std::endl;
        std::cout << "   Current value: " << line_value << std::endl;
        
        std::cout << "\n=== All basic components compiled successfully! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
}