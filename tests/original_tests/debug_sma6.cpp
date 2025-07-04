#include "test_common_simple.h"
#include "indicators/sma.h"
#include <iostream>

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

int main() {
    auto csv_data = getdata(0);
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto sma = std::make_shared<SMA>(close_line_series, 30);
    
    auto sma_line = sma->lines->getline(0);
    std::cout << "Initial SMA line size: " << sma_line->size() << std::endl;
    std::cout << "Initial value at index 0: " << (*sma_line)[0] << std::endl;
    
    sma->calculate();
    
    std::cout << "Final SMA line size: " << sma_line->size() << std::endl;
    
    std::cout << "First 5 values:" << std::endl;
    for (int i = 0; i < std::min(5, static_cast<int>(sma_line->size())); ++i) {
        std::cout << "  [" << i << "] = " << (*sma_line)[i] << std::endl;
    }
    
    std::cout << "Last 5 values:" << std::endl;
    int size = static_cast<int>(sma_line->size());
    for (int i = std::max(0, size - 5); i < size; ++i) {
        std::cout << "  [" << i << "] = " << (*sma_line)[i] << std::endl;
    }
    
    // Expected: 226 valid SMA values
    // Index 0: First SMA value (3644.44...)
    // Index 225: Last SMA value (4063.46...)
    
    return 0;
}