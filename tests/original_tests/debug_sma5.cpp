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
    sma->calculate();
    
    auto sma_line = sma->lines->getline(0);
    std::cout << "SMA line size: " << sma_line->size() << std::endl;
    
    // Check the last few values
    std::cout << "Last 10 values in SMA line:" << std::endl;
    int size = static_cast<int>(sma_line->size());
    for (int i = std::max(0, size - 10); i < size; ++i) {
        std::cout << "  [" << i << "] = " << (*sma_line)[i] << std::endl;
    }
    
    // Check if there's an empty element at the end
    std::cout << "\nChecking line buffer structure..." << std::endl;
    
    // Expected checkpoints from Python
    // Data length: 255, Min period: 30
    // chkpts = [0, -(255-30), (-(255-30))//2] = [0, -225, -112]
    // But the test says -113, which suggests Python uses floor division
    // Let's calculate like Python: (-225) // 2 = -113 (not -112)
    
    std::cout << "\nPython checkpoint calculation:" << std::endl;
    int data_length = 255;
    int min_period = 30;
    int checkpoint_2 = static_cast<int>(std::floor(static_cast<double>(-(data_length - min_period)) / 2.0));
    std::cout << "checkpoint_2 = floor(-(255-30)/2) = floor(-225/2) = " << checkpoint_2 << std::endl;
    
    // Expected SMA length should be data_length - min_period + 1 = 255 - 30 + 1 = 226
    int expected_sma_length = data_length - min_period + 1;
    std::cout << "Expected SMA length: " << expected_sma_length << std::endl;
    std::cout << "Actual SMA length: " << size << std::endl;
    
    return 0;
}