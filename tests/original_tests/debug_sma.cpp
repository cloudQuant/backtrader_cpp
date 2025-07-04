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
    std::cout << "First few values in buffer: ";
    for (int i = 0; i < std::min(5, static_cast<int>(close_buffer->size())); ++i) {
        std::cout << (*close_buffer)[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Creating SMA..." << std::endl;
    auto sma = std::make_shared<SMA>(close_line_series, 30);
    
    std::cout << "Calculating SMA..." << std::endl;
    try {
        sma->calculate();
        std::cout << "SMA calculation completed." << std::endl;
        
        auto sma_line = sma->lines->getline(0);
        std::cout << "SMA line size: " << sma_line->size() << std::endl;
        std::cout << "SMA value at index 0: " << sma->get(0) << std::endl;
        std::cout << "SMA value at index -1: " << sma->get(-1) << std::endl;
        std::cout << "SMA line values directly: ";
        for (int i = 0; i < std::min(10, static_cast<int>(sma_line->size())); ++i) {
            std::cout << (*sma_line)[i] << " ";
        }
        std::cout << std::endl;
        
        // 检查Python期望的检查点
        int data_length = static_cast<int>(csv_data.size());
        int min_period = 30;
        std::vector<int> check_points = {
            0,                                    
            -(data_length - min_period),         
            static_cast<int>(std::floor(static_cast<double>(-(data_length - min_period)) / 2.0))
        };
        
        std::cout << "Python check points: ";
        for (int cp : check_points) {
            std::cout << cp << " ";
        }
        std::cout << std::endl;
        
        std::cout << "SMA values at Python check points: ";
        for (int cp : check_points) {
            double val = sma->get(cp);
            std::cout << val << " ";
        }
        std::cout << std::endl;
        
        // 找到第一个有效的SMA值
        std::cout << "Finding first valid SMA value..." << std::endl;
        for (int i = 0; i < static_cast<int>(sma_line->size()); ++i) {
            double val = (*sma_line)[i];
            if (!std::isnan(val)) {
                std::cout << "First valid SMA at index " << i << ": " << val << std::endl;
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during SMA calculation: " << e.what() << std::endl;
    }
    
    return 0;
}
