#include "test_common_simple.h"
#include "indicators/sma.h"
#include <iostream>

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

int main() {
    auto csv_data = getdata(0);
    if (csv_data.empty()) {
        std::cout << "Failed to load test data" << std::endl;
        return 1;
    }
    
    std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
    
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto sma = std::make_shared<SMA>(close_line_series, 30);
    sma->calculate();
    
    auto sma_line = sma->lines->getline(0);
    std::cout << "SMA line size: " << sma_line->size() << std::endl;
    
    // Test the get() method
    std::cout << "Testing get() method:" << std::endl;
    std::cout << "get(0) = " << sma->get(0) << std::endl;
    std::cout << "get(-1) = " << sma->get(-1) << std::endl;
    std::cout << "get(-225) = " << sma->get(-225) << std::endl;
    std::cout << "get(-113) = " << sma->get(-113) << std::endl;
    
    // Manual index calculation
    std::cout << "\nManual index calculation:" << std::endl;
    int size = static_cast<int>(sma_line->size());
    std::cout << "Size: " << size << std::endl;
    
    int index_0 = size - 1 + 0;
    int index_neg225 = size - 1 + (-225);
    int index_neg113 = size - 1 + (-113);
    
    std::cout << "ago=0 -> index=" << index_0 << " -> value=" << (index_0 >= 0 && index_0 < size ? (*sma_line)[index_0] : std::numeric_limits<double>::quiet_NaN()) << std::endl;
    std::cout << "ago=-225 -> index=" << index_neg225 << " -> value=" << (index_neg225 >= 0 && index_neg225 < size ? (*sma_line)[index_neg225] : std::numeric_limits<double>::quiet_NaN()) << std::endl;
    std::cout << "ago=-113 -> index=" << index_neg113 << " -> value=" << (index_neg113 >= 0 && index_neg113 < size ? (*sma_line)[index_neg113] : std::numeric_limits<double>::quiet_NaN()) << std::endl;
    
    // Expected values
    std::cout << "\nExpected values:" << std::endl;
    std::cout << "Point 0: 4063.463000" << std::endl;
    std::cout << "Point 1: 3644.444667" << std::endl;
    std::cout << "Point 2: 3554.693333" << std::endl;
    
    return 0;
}