#include "test_common.h"
#include "lineseries.h"
#include "linebuffer.h"
#include "indicators/macd.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;
using namespace backtrader;

int main() {
    // Test MACD histogram using common framework
    std::vector<std::vector<std::string>> expected_vals = {
        {"3.843516", "5.999669", "4.618090"}  // histogram values
    };
    
    // Load data
    auto csv_data = getdata(0);
    std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
    
    // Create data series using SimpleTestDataSeries
    auto data_series = std::make_shared<SimpleTestDataSeries>(csv_data);
    
    // Create MACDHisto with DataSeries (since we're testing histogram values)
    auto macd = std::make_shared<MACDHisto>(std::static_pointer_cast<DataSeries>(data_series), 12, 26, 9);
    
    // Calculate MACD once for all data
    macd->calculate();
    
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
        std::cout << "  MACD[" << i << "] (ago=" << chkpts[i] << ") = " << std::fixed << std::setprecision(6) << macd_val << std::endl;
    }
    
    // Debug: Check specific positions
    std::cout << "\nDebug MACD at specific positions:" << std::endl;
    std::cout << "  Position 34 (first valid): MACD=" << macd->getMACDLine(-221) << std::endl;
    std::cout << "  Position 144 (middle): MACD=" << macd->getMACDLine(-111) << std::endl;
    std::cout << "  Position 254 (last): MACD=" << macd->getMACDLine(0) << std::endl;
    
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