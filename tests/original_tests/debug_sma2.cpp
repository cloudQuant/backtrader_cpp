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
    
    std::cout << "Line buffer size: " << close_buffer->size() << std::endl;
    
    auto sma = std::make_shared<SMA>(close_line_series, 30);
    
    std::cout << "Calculating SMA..." << std::endl;
    sma->calculate();
    
    auto sma_line = sma->lines->getline(0);
    std::cout << "SMA line size: " << sma_line->size() << std::endl;
    
    // Show all values in the SMA line
    std::cout << "SMA values (first 40 and last 10):" << std::endl;
    for (int i = 0; i < std::min(40, static_cast<int>(sma_line->size())); ++i) {
        std::cout << "  [" << i << "] = " << (*sma_line)[i] << std::endl;
    }
    
    std::cout << "..." << std::endl;
    for (int i = std::max(0, static_cast<int>(sma_line->size()) - 10); i < static_cast<int>(sma_line->size()); ++i) {
        std::cout << "  [" << i << "] = " << (*sma_line)[i] << std::endl;
    }
    
    // Check the get method
    std::cout << "\nTesting get() method:" << std::endl;
    std::cout << "get(0) = " << sma->get(0) << std::endl;
    std::cout << "get(-1) = " << sma->get(-1) << std::endl;
    std::cout << "get(-225) = " << sma->get(-225) << std::endl;
    std::cout << "get(-113) = " << sma->get(-113) << std::endl;
    
    // Check indices
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    std::vector<int> check_points = {
        0,                                    
        -(data_length - min_period),         
        static_cast<int>(std::floor(static_cast<double>(-(data_length - min_period)) / 2.0))
    };
    
    std::cout << "\nPython check points: ";
    for (int cp : check_points) {
        std::cout << cp << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Converted to indices (size=" << sma_line->size() << "):" << std::endl;
    for (int cp : check_points) {
        int index = static_cast<int>(sma_line->size()) - 1 + cp;
        std::cout << "  ago=" << cp << " -> index=" << index;
        if (index >= 0 && index < static_cast<int>(sma_line->size())) {
            std::cout << " -> value=" << (*sma_line)[index];
        } else {
            std::cout << " -> OUT OF BOUNDS";
        }
        std::cout << std::endl;
    }
    
    return 0;
}