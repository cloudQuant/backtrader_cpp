#include "test_common_simple.h"
#include "indicators/macd.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

int main() {
    // Test MACD histogram using common framework
    std::vector<std::vector<std::string>> expected_vals = {
        {"3.843516", "5.999669", "4.618090"}  // histogram values
    };
    
    // Load data
    auto csv_data = getdata(0);
    std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
    
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // Create MACD
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // Calculate using framework approach
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        macd->calculate();
    }
    
    // Calculate checkpoints like common framework
    int l = static_cast<int>(csv_data.size());
    int mp = 34;  // MACD min period
    int middle_checkpoint = static_cast<int>(std::floor(static_cast<double>(-(l - mp)) / 2.0));
    std::vector<int> chkpts = {0, -(l - mp), middle_checkpoint};
    
    std::cout << "Data length: " << l << std::endl;
    std::cout << "Min period: " << mp << std::endl;
    std::cout << "Checkpoints: ";
    for (int cp : chkpts) {
        std::cout << cp << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\nActual MACD values:" << std::endl;
    for (size_t i = 0; i < chkpts.size(); ++i) {
        double macd_val = macd->getMACDLine(chkpts[i]);
        std::cout << "  MACD[" << i << "] = " << std::fixed << std::setprecision(6) << macd_val << std::endl;
    }
    
    std::cout << "\nActual Signal values:" << std::endl;
    for (size_t i = 0; i < chkpts.size(); ++i) {
        double signal_val = macd->getSignalLine(chkpts[i]);
        std::cout << "  Signal[" << i << "] = " << std::fixed << std::setprecision(6) << signal_val << std::endl;
    }
    
    std::cout << "\nActual Histogram values:" << std::endl;
    for (size_t i = 0; i < chkpts.size(); ++i) {
        double hist_val = macd->getHistogram(chkpts[i]);
        std::cout << "  Histogram[" << i << "] = " << std::fixed << std::setprecision(6) << hist_val << std::endl;
    }
    
    std::cout << "\nExpected Histogram values:" << std::endl;
    for (size_t i = 0; i < expected_vals[0].size(); ++i) {
        std::cout << "  Expected[" << i << "] = " << expected_vals[0][i] << std::endl;
    }
    
    return 0;
}