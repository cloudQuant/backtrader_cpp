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
    
    std::cout << "SMA line size after construction: " << sma->lines->getline(0)->size() << std::endl;
    
    std::cout << "Calculating SMA..." << std::endl;
    sma->calculate();
    
    auto sma_line = sma->lines->getline(0);
    std::cout << "SMA line size after calculation: " << sma_line->size() << std::endl;
    
    // Count valid values
    int valid_count = 0;
    for (int i = 0; i < static_cast<int>(sma_line->size()); ++i) {
        if (!std::isnan((*sma_line)[i])) {
            valid_count++;
        }
    }
    std::cout << "Valid SMA values: " << valid_count << std::endl;
    
    // Show valid values
    std::cout << "First 10 valid SMA values:" << std::endl;
    int count = 0;
    for (int i = 0; i < static_cast<int>(sma_line->size()) && count < 10; ++i) {
        if (!std::isnan((*sma_line)[i])) {
            std::cout << "  [" << i << "] = " << (*sma_line)[i] << std::endl;
            count++;
        }
    }
    
    // Show last 10 valid values
    std::cout << "Last 10 valid SMA values:" << std::endl;
    std::vector<std::pair<int, double>> last_valid;
    for (int i = 0; i < static_cast<int>(sma_line->size()); ++i) {
        if (!std::isnan((*sma_line)[i])) {
            last_valid.push_back({i, (*sma_line)[i]});
        }
    }
    
    int start_idx = std::max(0, static_cast<int>(last_valid.size()) - 10);
    for (int i = start_idx; i < static_cast<int>(last_valid.size()); ++i) {
        std::cout << "  [" << last_valid[i].first << "] = " << last_valid[i].second << std::endl;
    }
    
    return 0;
}