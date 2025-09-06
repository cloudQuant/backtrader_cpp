/**
 * @file test_writer.cpp
 * @brief 数据写入器测试 - 对应Python test_writer.py
 * 
 * 原始Python测试:
 * - 测试CSV写入器功能
 * - 验证数据输出格式
 * - 测试输出行数（header + 256行数据）
 * - 验证分隔符和格式的正确性
 */

#include "test_common.h"
#include "strategy.h"
#include "cerebro.h"
#include "indicators/sma.h"
#include "writer.h"
#include "writers/WriterStringIO.h"
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <regex>
#include <algorithm>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;
using namespace backtrader;

// 测试策略类
class WriterTestStrategy : public backtrader::Strategy {
private:
    bool main_;
    std::shared_ptr<backtrader::indicators::SMA> sma_;

public:
    struct Params {
        bool main;
        Params() : main(false) {}
    };

    explicit WriterTestStrategy(const Params& params = Params()) : main_(params.main) {}

    void init() override {
        // 创建SMA指标（使用默认参数）
        sma_ = std::make_shared<backtrader::indicators::SMA>(data(0));
    }

    void next() override {
        // 策略逻辑（这里很简单，只是为了测试写入器）
    }

    // Getter方法
    bool isMain() const { return main_; }
};

// 运行写入器测试的辅助函数
std::unique_ptr<backtrader::Cerebro> runWriterTest(bool main = false, bool print_output = false) {
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    auto csv_data = getdata_feed(0);
    cerebro->adddata(csv_data);

    // 设置策略参数
    WriterTestStrategy::Params params;
    params.main = main;

    // 添加策略
    cerebro->addstrategy<WriterTestStrategy>(params);

    // 添加CSV写入器
    auto writer = std::make_shared<writers::WriterStringIO>();
    writer->setCSVFormat(true);
    cerebro->addwriter(writer);

    // 运行回测
    auto results = cerebro->run();

    return cerebro;
}

// 基本写入器测试
TEST(OriginalTests, Writer_BasicCSVOutput) {
    auto cerebro = runWriterTest(false, false);

    // 获取写入器
    const auto& writers = cerebro->getWriters();
    ASSERT_EQ(writers.size(), 1) << "Should have exactly 1 writer";

    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    ASSERT_NE(string_writer, nullptr) << "Writer should be WriterStringIO";

    // 获取输出内容
    const auto& output_lines = string_writer->getOutput();
    
    // Debug: Print the number of lines captured
    std::cout << "Number of output lines captured: " << output_lines.size() << std::endl;
    for (size_t i = 0; i < std::min(size_t(10), output_lines.size()); ++i) {
        std::cout << "Line " << i << ": " << output_lines[i] << std::endl;
    }
    
    EXPECT_GT(output_lines.size(), 0) << "Should have output lines";

    // 验证输出格式
    bool found_header = false;
    int data_line_count = 0;
    bool in_data_section = false;
    
    for (const auto& line : output_lines) {
        std::string trimmed_line = line;
        // 去除换行符
        if (!trimmed_line.empty() && (trimmed_line.back() == '\n' || trimmed_line.back() == '\r')) {
            trimmed_line.pop_back();
        }
        if (!trimmed_line.empty() && (trimmed_line.back() == '\n' || trimmed_line.back() == '\r')) {
            trimmed_line.pop_back();
        }

        if (trimmed_line.length() >= 79 && trimmed_line.substr(0, 79) == std::string(79, '=')) {
            if (!found_header) {
                found_header = true;
                in_data_section = true;
            } else {
                // 结束标记
                in_data_section = false;
                break;
            }
        } else if (in_data_section && !trimmed_line.empty()) {
            data_line_count++;
        }
    }

    EXPECT_TRUE(found_header) << "Should find header separator";
    EXPECT_EQ(data_line_count, 256) << "Should have exactly 256 data lines (header + data)";
}

// 测试写入器输出内容
TEST(OriginalTests, Writer_OutputContent) {
    auto cerebro = runWriterTest(false, false);

    const auto& writers = cerebro->getWriters();
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output_lines = string_writer->getOutput();

    // 检查输出格式
    bool found_data = false;
    
    for (const auto& line : output_lines) {
        if (line.find("Date") != std::string::npos || 
            line.find("Open") != std::string::npos ||
            line.find("High") != std::string::npos) {
            found_data = true;
            break;
        }
    }

    EXPECT_TRUE(found_data) << "Should find data column headers";
}

// 测试多个数据源的写入器
TEST(OriginalTests, Writer_MultipleDataFeeds) {
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 添加单个数据源（对应chkdatas = 1）
    auto csv_data = getdata_feed(0);
    cerebro->adddata(csv_data);

    WriterTestStrategy::Params params;
    params.main = false;
    cerebro->addstrategy<WriterTestStrategy>(params);

    // 添加写入器
    auto writer = std::make_shared<writers::WriterStringIO>();
    writer->setCSVFormat(true);
    cerebro->addwriter(writer);

    auto results = cerebro->run();

    // 验证写入器正常工作
    const auto& writers = cerebro->getWriters();
    EXPECT_EQ(writers.size(), 1) << "Should have exactly 1 writer";
    
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output = string_writer->getOutput();
    EXPECT_GT(output.size(), 0) << "Should have output content";
}

// 测试写入器格式验证
TEST(OriginalTests, Writer_FormatValidation) {
    auto cerebro = runWriterTest(false, false);

    const auto& writers = cerebro->getWriters();
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output_lines = string_writer->getOutput();

    // 验证CSV格式
    int csv_line_count = 0;
    
    for (const auto& line : output_lines) {
        // 跳过分隔符行
        if (line.find('=') != std::string::npos) {
            continue;
        }
        
        // 检查是否包含逗号（CSV格式）
        if (line.find(',') != std::string::npos) {
            csv_line_count++;
        }
    }

    EXPECT_GT(csv_line_count, 0) << "Should have CSV formatted lines";
}

// 测试写入器的数据行数验证
TEST(OriginalTests, Writer_LineCountValidation) {
    auto cerebro = runWriterTest(false, false);

    const auto& writers = cerebro->getWriters();
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output_lines = string_writer->getOutput();

    // 使用迭代器验证行数（模拟Python代码的逻辑）
    auto lines_iter = output_lines.begin();
    
    // 查找第一个分隔符
    bool found_first_separator = false;
    while (lines_iter != output_lines.end()) {
        std::string line = *lines_iter;
        // 去除换行符
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        
        if (line.length() >= 79 && line == std::string(79, '=')) {
            found_first_separator = true;
            ++lines_iter;
            break;
        }
        ++lines_iter;
    }

    ASSERT_TRUE(found_first_separator) << "Should find first separator";

    // 计算数据行数
    int count = 0;
    while (lines_iter != output_lines.end()) {
        std::string line = *lines_iter;
        // 去除换行符
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        
        if (!line.empty() && line[0] == '=') {
            break;  // 遇到结束分隔符
        }
        count++;
        ++lines_iter;
    }

    EXPECT_EQ(count, 256) << "Should have exactly 256 lines (header + data)";
}

// 测试写入器配置
TEST(OriginalTests, Writer_Configuration) {
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    auto csv_data = getdata_feed(0);
    cerebro->adddata(csv_data);

    WriterTestStrategy::Params params;
    params.main = false;
    cerebro->addstrategy<WriterTestStrategy>(params);

    // 创建带特定配置的写入器
    auto writer = std::make_shared<writers::WriterStringIO>();
    writer->setCSVFormat(true);
    
    // 可以设置其他配置选项
    writer->setIncludeTimestamp(true);
    writer->setIncludeIndicators(true);

    cerebro->addwriter(writer);

    auto results = cerebro->run();

    // 验证配置生效
    const auto& writers = cerebro->getWriters();
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    
    EXPECT_TRUE(string_writer->isCSVFormat()) << "Writer should be in CSV format";
    EXPECT_TRUE(string_writer->getIncludeTimestamp()) << "Writer should include timestamp";
    EXPECT_TRUE(string_writer->getIncludeIndicators()) << "Writer should include indicators";
}

// 测试写入器数据完整性
TEST(OriginalTests, Writer_DataIntegrity) {
    auto cerebro = runWriterTest(false, false);

    const auto& writers = cerebro->getWriters();
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output_lines = string_writer->getOutput();

    // 验证输出包含预期的数据字段
    std::vector<std::string> expected_fields = {
        "Date", "Open", "High", "Low", "Close", "Volume"
    };

    bool all_fields_found = true;
    
    for (const auto& field : expected_fields) {
        bool field_found = false;
    
    for (const auto& line : output_lines) {
            if (line.find(field) != std::string::npos) {
                field_found = true;
                break;
            }
        }
        if (!field_found) {
            all_fields_found = false;
            std::cout << "Missing field: " << field << std::endl;
        }
    }

    EXPECT_TRUE(all_fields_found) << "All expected fields should be present in output";
}

// 测试写入器主模式
TEST(OriginalTests, Writer_MainMode) {
    // 测试主模式（打印输出）
    auto cerebro = runWriterTest(true, true);

    const auto& writers = cerebro->getWriters();
    EXPECT_EQ(writers.size(), 1) << "Should have exactly 1 writer in main mode";

    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output_lines = string_writer->getOutput();
    
    // 主模式下应该有输出
    EXPECT_GT(output_lines.size(), 0) << "Main mode should produce output";

    // 可以选择打印输出（用于调试）
    if (true) {  // 设置为false以禁用打印
        std::cout << "Writer output (first 10 lines):" << std::endl;
    for (size_t i = 0; i < std::min(size_t(10), output_lines.size()); ++i) {
            std::cout << output_lines[i];
            if (output_lines[i].back() != '\n') {
                std::cout << std::endl;
            }
        }
    }
}

// 测试写入器性能
TEST(OriginalTests, Writer_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // 运行多次写入器测试
    const int num_runs = 5;
    for (int i = 0; i < num_runs; ++i) {
        auto cerebro = runWriterTest(false, false);
        
        // 验证每次运行都产生输出
        const auto& writers = cerebro->getWriters();
        EXPECT_EQ(writers.size(), 1) << "Run " << i << " should have writer";
        
        auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
        const auto& output = string_writer->getOutput();
        EXPECT_GT(output.size(), 0) << "Run " << i << " should have output";
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Writer performance test: " << num_runs 
              << " runs in " << duration.count() << " ms" << std::endl;

    // 性能要求
    EXPECT_LT(duration.count(), 2000) 
        << "Performance test should complete within 2 seconds";
}

// 测试写入器边界条件
TEST(OriginalTests, Writer_EdgeCases) {
    // 测试无数据情况
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 不添加数据源，只添加策略和写入器
    WriterTestStrategy::Params params;
    cerebro->addstrategy<WriterTestStrategy>(params);

    auto writer = std::make_shared<writers::WriterStringIO>();
    writer->setCSVFormat(true);
    cerebro->addwriter(writer);

    // 这种情况下运行可能会产生空输出或错误
    // 具体行为取决于实现
    EXPECT_NO_THROW({
        auto results = cerebro->run();
    }) << "Should handle no data case gracefully";
}

// 测试多个写入器
TEST(OriginalTests, Writer_MultipleWriters) {
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    auto csv_data = getdata_feed(0);
    cerebro->adddata(csv_data);

    WriterTestStrategy::Params params;
    cerebro->addstrategy<WriterTestStrategy>(params);

    // 添加多个写入器
    auto writer1 = std::make_shared<writers::WriterStringIO>();
    writer1->setCSVFormat(true);
    cerebro->addwriter(writer1);

    auto writer2 = std::make_shared<writers::WriterStringIO>();
    writer2->setCSVFormat(true);
    cerebro->addwriter(writer2);

    auto results = cerebro->run();

    // 验证多个写入器都正常工作
    const auto& writers = cerebro->getWriters();
    EXPECT_EQ(writers.size(), 2) << "Should have exactly 2 writers";
    for (size_t i = 0; i < writers.size(); ++i) {
        auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[i]);
        ASSERT_NE(string_writer, nullptr) << "Writer " << i << " should be WriterStringIO";
        
        const auto& output = string_writer->getOutput();
        EXPECT_GT(output.size(), 0) << "Writer " << i << " should have output";
    }
}

// 测试写入器内容验证
TEST(OriginalTests, Writer_ContentValidation) {
    auto cerebro = runWriterTest(false, false);

    const auto& writers = cerebro->getWriters();
    auto string_writer = std::dynamic_pointer_cast<writers::WriterStringIO>(writers[0]);
    const auto& output_lines = string_writer->getOutput();

    // 验证输出内容的基本结构
    bool has_numeric_data = false;
    
    for (const auto& line : output_lines) {
        // 检查是否包含数字（价格、成交量等）
        if (std::regex_search(line, std::regex(R"(\d+\.\d+)"))) {
            has_numeric_data = true;
            break;
        }
    }

    EXPECT_TRUE(has_numeric_data) << "Output should contain numeric data";

    // 验证行结构合理
    int non_empty_lines = 0;
    
    for (const auto& line : output_lines) {
        if (!line.empty() && line.find_first_not_of(" \t\r\n") != std::string::npos) {
            non_empty_lines++;
        }
    }

    EXPECT_GT(non_empty_lines, 250) << "Should have substantial content";
}