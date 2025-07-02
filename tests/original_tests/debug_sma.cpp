#include "test_common_simple.h"
#include "indicators/SMA.h"
#include <gtest/gtest.h>
#include <iomanip>

using namespace backtrader::tests::original;
using namespace backtrader;

int main() {
    // 加载测试数据
    auto csv_data = getdata(0);
    std::cout << "Loaded " << csv_data.size() << " data points" << std::endl;
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 创建SMA指标（30周期）
    auto sma = std::make_shared<SMA>(close_line, 30, false);  // force direct mode
    
    // 逐步添加数据并计算
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        sma->calculate();
        
        // 在关键点显示计算详情
        if (i == 255-1 || i == 255-225-1 || i == 255-112-1) {
            std::cout << "\n=== Data point " << i << " ===" << std::endl;
            std::cout << "Close: " << std::fixed << std::setprecision(6) << csv_data[i].close << std::endl;
            std::cout << "SMA Value: " << std::fixed << std::setprecision(6) << sma->get(0) << std::endl;
            
            // 显示最近30个输入值
            if (i >= 29) {
                std::cout << "Last 30 values for SMA calculation:" << std::endl;
                double manual_sum = 0.0;
                for (int j = 0; j < 30; ++j) {
                    double val = close_line->get(-j);
                    manual_sum += val;
                    std::cout << "  [" << j << "] = " << std::fixed << std::setprecision(6) << val << std::endl;
                }
                std::cout << "Manual sum: " << std::fixed << std::setprecision(6) << manual_sum << std::endl;
                std::cout << "Manual average: " << std::fixed << std::setprecision(6) << manual_sum / 30.0 << std::endl;
            }
        }
    }
    
    // 计算检查点，对应Python的chkpts
    int l = static_cast<int>(csv_data.size());
    int mp = 30;
    std::vector<int> chkpts = {0, -(l - mp), -(l - mp) / 2};
    
    std::cout << "\n=== Final Results ===" << std::endl;
    std::cout << "Data length: " << l << std::endl;
    std::cout << "Minimum period: " << mp << std::endl;
    std::cout << "Check points: ";
    for (int chkpt : chkpts) {
        std::cout << chkpt << " ";
    }
    std::cout << std::endl;
    
    // 输出实际值
    std::cout << "Actual values:" << std::endl;
    for (size_t i = 0; i < chkpts.size(); ++i) {
        double value = sma->get(chkpts[i]);
        std::cout << "  [" << i << "] = " << std::fixed << std::setprecision(6) << value << std::endl;
    }
    
    std::vector<std::string> expected = {"4063.463000", "3644.444667", "3554.693333"};
    std::cout << "Expected values:" << std::endl;
    for (size_t i = 0; i < expected.size(); ++i) {
        std::cout << "  [" << i << "] = " << expected[i] << std::endl;
    }
    
    return 0;
}