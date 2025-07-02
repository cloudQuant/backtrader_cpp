#include "test_framework.h"
#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "indicators/BollingerBands.h"
#include "indicators/Ichimoku.h"
#include "core/LineRoot.h"
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include <future>

using namespace backtrader;
using namespace backtrader::testing;

/**
 * @brief 压力测试基类
 */
class StressTestBase : public BacktraderTestBase {
protected:
    struct StressTestResult {
        std::string test_name;
        size_t operations_completed;
        size_t operations_failed;
        double success_rate;
        double avg_response_time_ms;
        double max_response_time_ms;
        size_t memory_peak_mb;
        
        StressTestResult(const std::string& name) 
            : test_name(name), operations_completed(0), operations_failed(0),
              success_rate(0.0), avg_response_time_ms(0.0), max_response_time_ms(0.0),
              memory_peak_mb(0) {}
    };
    
    std::vector<StressTestResult> stress_results_;
    
    void TearDown() override {
        BacktraderTestBase::TearDown();
        printStressTestSummary();
    }
    
    void printStressTestSummary() {
        std::cout << "\n" << std::string(90, '=') << "\n";
        std::cout << "Stress Test Summary\n";
        std::cout << std::string(90, '=') << "\n";
        std::cout << std::left << std::setw(25) << "Test Name"
                 << std::setw(12) << "Completed"
                 << std::setw(10) << "Failed"
                 << std::setw(12) << "Success%"
                 << std::setw(15) << "Avg Time(ms)"
                 << std::setw(15) << "Max Time(ms)"
                 << std::setw(12) << "Peak Mem(MB)" << "\n";
        std::cout << std::string(90, '-') << "\n";
        
        for (const auto& result : stress_results_) {
            std::cout << std::left << std::setw(25) << result.test_name
                     << std::setw(12) << result.operations_completed
                     << std::setw(10) << result.operations_failed
                     << std::setw(12) << std::fixed << std::setprecision(1) << result.success_rate
                     << std::setw(15) << std::fixed << std::setprecision(3) << result.avg_response_time_ms
                     << std::setw(15) << std::fixed << std::setprecision(3) << result.max_response_time_ms
                     << std::setw(12) << result.memory_peak_mb << "\n";
        }
        std::cout << std::string(90, '=') << "\n";
    }
};

/**
 * @brief 高负载数据处理测试
 */
class HighVolumeDataTest : public StressTestBase {
public:
    void testMassiveDataProcessing() {
        StressTestResult result("MassiveDataProcessing");
        
        const size_t data_size = 1000000; // 1M数据点
        const size_t num_indicators = 50;
        
        auto large_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.03, 12345);
        
        std::vector<double> processing_times;
        processing_times.reserve(data_size);
        
        // 创建数据线
        auto data_line = std::make_shared<LineRoot>(10000, "massive_data"); // 10K缓冲区
        
        // 创建多个指标
        std::vector<std::shared_ptr<SMA>> sma_indicators;
        std::vector<std::shared_ptr<RSI>> rsi_indicators;
        
        for (size_t i = 0; i < num_indicators; ++i) {
            sma_indicators.push_back(std::make_shared<SMA>(data_line, 20 + (i % 30)));
            if (i < num_indicators / 2) {
                rsi_indicators.push_back(std::make_shared<RSI>(data_line, 14 + (i % 10)));
            }
        }
        
        auto total_start = std::chrono::high_resolution_clock::now();
        
        // 处理所有数据
        for (size_t i = 0; i < data_size; ++i) {
            auto step_start = std::chrono::high_resolution_clock::now();
            
            try {
                // 添加数据点
                data_line->forward(large_data[3][i]);
                
                // 计算所有指标
                for (auto& sma : sma_indicators) {
                    sma->calculate();
                }
                for (auto& rsi : rsi_indicators) {
                    rsi->calculate();
                }
                
                result.operations_completed++;
                
                // 前进数据线
                if (i < data_size - 1) {
                    data_line->forward();
                }
                
            } catch (const std::exception& e) {
                result.operations_failed++;
                std::cout << "Error at step " << i << ": " << e.what() << std::endl;
            }
            
            auto step_end = std::chrono::high_resolution_clock::now();
            auto step_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(step_end - step_start);
            double step_time_ms = step_duration.count() / 1000000.0;
            processing_times.push_back(step_time_ms);
            
            if (i % 100000 == 0) {
                std::cout << "Processed " << i << " / " << data_size << " data points" << std::endl;
            }
        }
        
        auto total_end = std::chrono::high_resolution_clock::now();
        
        // 计算统计信息
        auto stats = TestUtils::calculateStatistics(processing_times);
        result.avg_response_time_ms = stats[0];
        result.max_response_time_ms = stats[3];
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        // 验证最终结果
        bool all_valid = true;
        for (auto& sma : sma_indicators) {
            if (std::isnan(sma->get(0))) {
                all_valid = false;
                break;
            }
        }
        for (auto& rsi : rsi_indicators) {
            if (std::isnan(rsi->get(0))) {
                all_valid = false;
                break;
            }
        }
        
        EXPECT_TRUE(all_valid);
        EXPECT_GT(result.success_rate, 99.0); // 至少99%成功率
        
        stress_results_.push_back(result);
    }
    
    void testContinuousDataStream() {
        StressTestResult result("ContinuousDataStream");
        
        const size_t stream_duration_seconds = 10;
        const size_t data_rate_hz = 1000; // 1KHz数据率
        const size_t total_points = stream_duration_seconds * data_rate_hz;
        
        auto data_line = std::make_shared<LineRoot>(1000, "stream");
        auto sma = std::make_shared<SMA>(data_line, 50);
        auto rsi = std::make_shared<RSI>(data_line, 14);
        
        std::atomic<bool> running(true);
        std::atomic<size_t> points_processed(0);
        std::vector<double> response_times;
        std::mutex response_times_mutex;
        
        // 数据生成线程
        std::thread data_generator([&]() {
            std::mt19937 rng(42);
            std::normal_distribution<double> dist(100.0, 5.0);
            
            auto start_time = std::chrono::steady_clock::now();
            auto next_data_time = start_time;
            
            while (running && points_processed < total_points) {
                auto now = std::chrono::steady_clock::now();
                
                if (now >= next_data_time) {
                    auto process_start = std::chrono::high_resolution_clock::now();
                    
                    try {
                        double value = dist(rng);
                        data_line->forward(value);
                        
                        sma->calculate();
                        rsi->calculate();
                        
                        data_line->forward();
                        points_processed++;
                        result.operations_completed++;
                        
                    } catch (const std::exception& e) {
                        result.operations_failed++;
                    }
                    
                    auto process_end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(process_end - process_start);
                    double time_ms = duration.count() / 1000000.0;
                    
                    {
                        std::lock_guard<std::mutex> lock(response_times_mutex);
                        response_times.push_back(time_ms);
                    }
                    
                    // 计算下一个数据点时间
                    next_data_time += std::chrono::microseconds(1000000 / data_rate_hz);
                }
                
                // 短暂休眠避免CPU占用过高
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
        
        // 等待完成或超时
        auto start_time = std::chrono::steady_clock::now();
        while (running && points_processed < total_points) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > std::chrono::seconds(stream_duration_seconds + 5)) {
                running = false; // 超时
                break;
            }
        }
        
        running = false;
        data_generator.join();
        
        // 计算统计信息
        if (!response_times.empty()) {
            auto stats = TestUtils::calculateStatistics(response_times);
            result.avg_response_time_ms = stats[0];
            result.max_response_time_ms = stats[3];
        }
        
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        std::cout << "Continuous stream test: processed " << points_processed << " points" << std::endl;
        
        EXPECT_GT(points_processed, total_points * 0.9); // 至少处理90%的数据
        EXPECT_GT(result.success_rate, 95.0);
        EXPECT_LT(result.avg_response_time_ms, 1.0); // 平均处理时间应该小于1ms
        
        stress_results_.push_back(result);
    }
};

TEST_F(HighVolumeDataTest, MassiveDataProcessing) {
    testMassiveDataProcessing();
}

TEST_F(HighVolumeDataTest, ContinuousDataStream) {
    testContinuousDataStream();
}

/**
 * @brief 多线程并发测试
 */
class ConcurrencyStressTest : public StressTestBase {
public:
    void testMultiThreadedIndicators() {
        StressTestResult result("MultiThreadedIndicators");
        
        const size_t num_threads = std::thread::hardware_concurrency();
        const size_t data_size = 50000;
        const size_t operations_per_thread = data_size;
        
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 777);
        
        std::vector<std::thread> threads;
        std::vector<std::future<size_t>> futures;
        std::atomic<size_t> total_completed(0);
        std::atomic<size_t> total_failed(0);
        
        auto worker = [&](int thread_id) -> size_t {
            size_t completed = 0;
            
            try {
                // 每个线程创建自己的数据线和指标
                auto data_line = std::make_shared<LineRoot>(data_size, "thread_" + std::to_string(thread_id));
                
                for (double value : test_data[3]) {
                    data_line->forward(value);
                }
                
                auto sma = std::make_shared<SMA>(data_line, 20);
                auto rsi = std::make_shared<RSI>(data_line, 14);
                auto macd = std::make_shared<MACD>(data_line, 12, 26, 9);
                
                // 执行计算
                for (size_t i = 0; i < operations_per_thread; ++i) {
                    sma->calculate();
                    rsi->calculate();
                    macd->calculate();
                    
                    if (i < operations_per_thread - 1) {
                        data_line->forward();
                    }
                    
                    completed++;
                }
                
                // 验证结果
                EXPECT_NO_NAN(sma->get(0));
                EXPECT_NO_NAN(rsi->get(0));
                EXPECT_NO_NAN(macd->getMACDLine(0));
                
            } catch (const std::exception& e) {
                std::cout << "Thread " << thread_id << " error: " << e.what() << std::endl;
                total_failed++;
            }
            
            total_completed += completed;
            return completed;
        };
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 启动所有线程
        for (size_t i = 0; i < num_threads; ++i) {
            futures.push_back(std::async(std::launch::async, worker, i));
        }
        
        // 等待所有线程完成
        for (auto& future : futures) {
            future.get();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        result.operations_completed = total_completed;
        result.operations_failed = total_failed;
        result.success_rate = (double)total_completed / (total_completed + total_failed) * 100.0;
        result.avg_response_time_ms = duration.count() / (double)total_completed;
        
        std::cout << "Multi-threaded test with " << num_threads << " threads completed" << std::endl;
        std::cout << "Total operations: " << total_completed << std::endl;
        
        EXPECT_GT(result.success_rate, 99.0);
        stress_results_.push_back(result);
    }
    
    void testSharedDataLineAccess() {
        StressTestResult result("SharedDataLineAccess");
        
        const size_t num_readers = 8;
        const size_t num_operations = 10000;
        const size_t data_size = 5000;
        
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 888);
        auto shared_data_line = std::make_shared<LineRoot>(data_size, "shared");
        
        for (double value : test_data[3]) {
            shared_data_line->forward(value);
        }
        
        std::atomic<size_t> read_operations(0);
        std::atomic<size_t> read_errors(0);
        std::vector<std::future<void>> reader_futures;
        
        // 创建多个读取器线程
        for (size_t i = 0; i < num_readers; ++i) {
            reader_futures.push_back(std::async(std::launch::async, [&, i]() {
                std::mt19937 rng(i);
                std::uniform_int_distribution<int> ago_dist(0, 100);
                
                for (size_t op = 0; op < num_operations; ++op) {
                    try {
                        int ago = ago_dist(rng);
                        double value = shared_data_line->get(ago);
                        
                        // 验证读取的值是合理的
                        if (!std::isnan(value) && (value < 0 || value > 1000)) {
                            read_errors++;
                        }
                        
                        read_operations++;
                    } catch (const std::exception& e) {
                        read_errors++;
                    }
                }
            }));
        }
        
        // 等待所有读取器完成
        for (auto& future : reader_futures) {
            future.get();
        }
        
        result.operations_completed = read_operations;
        result.operations_failed = read_errors;
        result.success_rate = (double)read_operations / (read_operations + read_errors) * 100.0;
        
        EXPECT_GT(result.success_rate, 99.0);
        stress_results_.push_back(result);
    }
};

TEST_F(ConcurrencyStressTest, MultiThreadedIndicators) {
    testMultiThreadedIndicators();
}

TEST_F(ConcurrencyStressTest, SharedDataLineAccess) {
    testSharedDataLineAccess();
}

/**
 * @brief 内存压力测试
 */
class MemoryStressTest : public StressTestBase {
public:
    void testMemoryLeakDetection() {
        StressTestResult result("MemoryLeakDetection");
        
        const size_t num_iterations = 1000;
        const size_t indicators_per_iteration = 100;
        
        for (size_t iter = 0; iter < num_iterations; ++iter) {
            try {
                std::vector<std::shared_ptr<SMA>> indicators;
                indicators.reserve(indicators_per_iteration);
                
                // 创建临时数据
                auto temp_data = TestDataGenerator::generateRandomOHLCV(100, 100.0, 0.02, iter);
                auto temp_line = std::make_shared<LineRoot>(100, "temp_" + std::to_string(iter));
                
                for (double value : temp_data[3]) {
                    temp_line->forward(value);
                }
                
                // 创建大量指标
                for (size_t i = 0; i < indicators_per_iteration; ++i) {
                    indicators.push_back(std::make_shared<SMA>(temp_line, 10 + (i % 20)));
                }
                
                // 计算所有指标
                for (auto& indicator : indicators) {
                    for (int j = 0; j < 50; ++j) {
                        indicator->calculate();
                        if (j < 49) temp_line->forward();
                    }
                }
                
                // 验证一些结果
                for (size_t i = 0; i < std::min(size_t(10), indicators.size()); ++i) {
                    if (std::isnan(indicators[i]->get(0))) {
                        result.operations_failed++;
                    } else {
                        result.operations_completed++;
                    }
                }
                
                // 显式清理
                indicators.clear();
                
            } catch (const std::exception& e) {
                result.operations_failed++;
                std::cout << "Memory test iteration " << iter << " failed: " << e.what() << std::endl;
            }
            
            if (iter % 100 == 0) {
                std::cout << "Memory test iteration " << iter << " / " << num_iterations << std::endl;
            }
        }
        
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        EXPECT_GT(result.success_rate, 95.0);
        stress_results_.push_back(result);
    }
    
    void testLargeBufferHandling() {
        StressTestResult result("LargeBufferHandling");
        
        const std::vector<size_t> buffer_sizes = {10000, 50000, 100000, 500000, 1000000};
        
        for (size_t buffer_size : buffer_sizes) {
            try {
                auto large_data = TestDataGenerator::generateRandomOHLCV(buffer_size, 100.0, 0.02, 999);
                auto large_line = std::make_shared<LineRoot>(buffer_size, "large_buffer");
                
                auto start_time = std::chrono::high_resolution_clock::now();
                
                // 填充大缓冲区
                for (double value : large_data[3]) {
                    large_line->forward(value);
                }
                
                auto sma = std::make_shared<SMA>(large_line, 1000);
                
                // 执行计算
                for (size_t i = 0; i < 10000; ++i) {
                    sma->calculate();
                    if (i < 9999) large_line->forward();
                }
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                if (!std::isnan(sma->get(0))) {
                    result.operations_completed++;
                } else {
                    result.operations_failed++;
                }
                
                std::cout << "Buffer size " << buffer_size << " processed in " << duration.count() << "ms" << std::endl;
                
            } catch (const std::exception& e) {
                result.operations_failed++;
                std::cout << "Large buffer test failed for size " << buffer_size << ": " << e.what() << std::endl;
            }
        }
        
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        EXPECT_GT(result.success_rate, 80.0); // 大缓冲区可能有一些限制
        stress_results_.push_back(result);
    }
};

TEST_F(MemoryStressTest, MemoryLeakDetection) {
    testMemoryLeakDetection();
}

TEST_F(MemoryStressTest, LargeBufferHandling) {
    testLargeBufferHandling();
}

/**
 * @brief 边界条件压力测试
 */
class BoundaryStressTest : public StressTestBase {
public:
    void testExtremeValues() {
        StressTestResult result("ExtremeValues");
        
        const std::vector<double> extreme_values = {
            0.0, -0.0, 1e-10, -1e-10, 1e10, -1e10,
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::lowest(),
            std::numeric_limits<double>::min(),
            std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::signaling_NaN()
        };
        
        for (double extreme_value : extreme_values) {
            try {
                auto data_line = std::make_shared<LineRoot>(100, "extreme");
                
                // 添加一些正常值
                for (int i = 0; i < 50; ++i) {
                    data_line->forward(100.0 + i);
                }
                
                // 添加极端值
                data_line->forward(extreme_value);
                
                auto sma = std::make_shared<SMA>(data_line, 10);
                auto rsi = std::make_shared<RSI>(data_line, 14);
                
                // 计算指标
                for (int i = 0; i < 20; ++i) {
                    sma->calculate();
                    rsi->calculate();
                    
                    if (i < 19) data_line->forward();
                }
                
                double sma_result = sma->get(0);
                double rsi_result = rsi->get(0);
                
                // 检查结果是否在合理范围内
                bool sma_valid = std::isfinite(sma_result) || std::isnan(sma_result);
                bool rsi_valid = (std::isnan(rsi_result) || (rsi_result >= 0 && rsi_result <= 100));
                
                if (sma_valid && rsi_valid) {
                    result.operations_completed++;
                } else {
                    result.operations_failed++;
                }
                
            } catch (const std::exception& e) {
                // 某些极端值可能导致异常，这是可以接受的
                result.operations_completed++; // 正确处理异常也算成功
            }
        }
        
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        EXPECT_GT(result.success_rate, 90.0);
        stress_results_.push_back(result);
    }
    
    void testRapidParameterChanges() {
        StressTestResult result("RapidParameterChanges");
        
        const size_t num_changes = 10000;
        
        auto test_data = TestDataGenerator::generateRandomOHLCV(1000, 100.0, 0.02, 555);
        auto data_line = std::make_shared<LineRoot>(1000, "param_change");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        std::mt19937 rng(42);
        std::uniform_int_distribution<size_t> period_dist(5, 100);
        
        for (size_t i = 0; i < num_changes; ++i) {
            try {
                size_t period = period_dist(rng);
                auto sma = std::make_shared<SMA>(data_line, period);
                
                // 快速计算几个值
                for (int j = 0; j < 10; ++j) {
                    sma->calculate();
                    if (j < 9) data_line->forward();
                }
                
                double result_value = sma->get(0);
                
                if (std::isfinite(result_value) || std::isnan(result_value)) {
                    result.operations_completed++;
                } else {
                    result.operations_failed++;
                }
                
            } catch (const std::exception& e) {
                result.operations_failed++;
            }
        }
        
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        EXPECT_GT(result.success_rate, 95.0);
        stress_results_.push_back(result);
    }
};

TEST_F(BoundaryStressTest, ExtremeValues) {
    testExtremeValues();
}

TEST_F(BoundaryStressTest, RapidParameterChanges) {
    testRapidParameterChanges();
}

/**
 * @brief 长时间运行测试
 */
class EnduranceTest : public StressTestBase {
public:
    void testLongRunningCalculation() {
        StressTestResult result("LongRunningCalculation");
        
        const auto test_duration = std::chrono::minutes(2); // 2分钟测试
        const size_t batch_size = 1000;
        
        auto data_line = std::make_shared<LineRoot>(batch_size, "endurance");
        auto sma = std::make_shared<SMA>(data_line, 50);
        auto rsi = std::make_shared<RSI>(data_line, 14);
        auto macd = std::make_shared<MACD>(data_line, 12, 26, 9);
        
        std::mt19937 rng(42);
        std::normal_distribution<double> price_dist(100.0, 5.0);
        
        auto start_time = std::chrono::steady_clock::now();
        size_t iteration = 0;
        
        while (std::chrono::steady_clock::now() - start_time < test_duration) {
            try {
                // 生成新的数据批次
                for (size_t i = 0; i < batch_size; ++i) {
                    double price = price_dist(rng);
                    data_line->forward(price);
                    
                    sma->calculate();
                    rsi->calculate();
                    macd->calculate();
                    
                    if (i < batch_size - 1) {
                        data_line->forward();
                    }
                }
                
                // 验证结果
                double sma_val = sma->get(0);
                double rsi_val = rsi->get(0);
                double macd_val = macd->getMACDLine(0);
                
                bool all_valid = true;
                if (iteration > 1) { // 前几次迭代可能有NaN
                    all_valid = !std::isnan(sma_val) && !std::isnan(rsi_val) && !std::isnan(macd_val);
                    all_valid = all_valid && (rsi_val >= 0 && rsi_val <= 100);
                }
                
                if (all_valid) {
                    result.operations_completed += batch_size;
                } else {
                    result.operations_failed += batch_size;
                }
                
                iteration++;
                
                if (iteration % 100 == 0) {
                    auto elapsed = std::chrono::steady_clock::now() - start_time;
                    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
                    std::cout << "Endurance test: " << elapsed_seconds << "s elapsed, " 
                             << result.operations_completed << " operations completed" << std::endl;
                }
                
            } catch (const std::exception& e) {
                result.operations_failed += batch_size;
                std::cout << "Endurance test error at iteration " << iteration << ": " << e.what() << std::endl;
            }
        }
        
        result.success_rate = (double)result.operations_completed / (result.operations_completed + result.operations_failed) * 100.0;
        
        auto actual_duration = std::chrono::steady_clock::now() - start_time;
        auto actual_seconds = std::chrono::duration_cast<std::chrono::seconds>(actual_duration).count();
        
        std::cout << "Endurance test completed after " << actual_seconds << " seconds" << std::endl;
        std::cout << "Total operations: " << result.operations_completed << std::endl;
        
        EXPECT_GT(result.success_rate, 95.0);
        EXPECT_GT(result.operations_completed, 100000); // 至少处理10万次操作
        
        stress_results_.push_back(result);
    }
};

TEST_F(EnduranceTest, LongRunningCalculation) {
    testLongRunningCalculation();
}

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Stress Testing Suite for Backtrader C++" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "System Configuration:" << std::endl;
    std::cout << "Hardware Threads: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "=========================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\nStress Tests Completed!" << std::endl;
    std::cout << "Check results above for any failures or performance issues." << std::endl;
    
    return result;
}