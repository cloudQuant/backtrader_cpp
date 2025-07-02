#include "test_framework.h"
#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "indicators/BollingerBands.h"
#include "indicators/Stochastic.h"
#include "indicators/ATR.h"
#include "indicators/Ichimoku.h"
#include "indicators/CCI.h"
#include "core/LineRoot.h"
#include <thread>
#include <vector>
#include <chrono>
#include <iomanip>
#include <map>

using namespace backtrader;
using namespace backtrader::testing;

/**
 * @brief 性能基准测试类
 */
class PerformanceBenchmark : public BacktraderTestBase {
protected:
    struct BenchmarkResult {
        std::string test_name;
        size_t data_points;
        double execution_time_ms;
        double throughput_per_second;
        size_t memory_usage_mb;
        
        BenchmarkResult(const std::string& name, size_t points, double time_ms, double throughput, size_t memory = 0)
            : test_name(name), data_points(points), execution_time_ms(time_ms), 
              throughput_per_second(throughput), memory_usage_mb(memory) {}
    };
    
    std::vector<BenchmarkResult> results_;
    
    void SetUp() override {
        BacktraderTestBase::SetUp();
        results_.clear();
    }
    
    void TearDown() override {
        BacktraderTestBase::TearDown();
        printBenchmarkSummary();
    }
    
    void printBenchmarkSummary() {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "Performance Benchmark Summary\n";
        std::cout << std::string(80, '=') << "\n";
        std::cout << std::left << std::setw(25) << "Test Name" 
                 << std::setw(12) << "Data Points"
                 << std::setw(12) << "Time (ms)"
                 << std::setw(15) << "Throughput/s"
                 << std::setw(12) << "Memory (MB)" << "\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (const auto& result : results_) {
            std::cout << std::left << std::setw(25) << result.test_name
                     << std::setw(12) << result.data_points
                     << std::setw(12) << std::fixed << std::setprecision(2) << result.execution_time_ms
                     << std::setw(15) << std::fixed << std::setprecision(0) << result.throughput_per_second
                     << std::setw(12) << result.memory_usage_mb << "\n";
        }
        std::cout << std::string(80, '=') << "\n";
    }
    
    void recordBenchmark(const std::string& name, size_t data_points, double time_ms, size_t memory_mb = 0) {
        double throughput = (time_ms > 0) ? (data_points * 1000.0 / time_ms) : 0.0;
        results_.emplace_back(name, data_points, time_ms, throughput, memory_mb);
    }
};

/**
 * @brief SMA性能测试
 */
class SMAPerformanceTest : public PerformanceBenchmark {
public:
    void benchmarkSMACalculation(size_t data_size, size_t period, const std::string& test_name) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 42);
        auto data_line = std::make_shared<LineRoot>(data_size, "sma_perf");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        auto sma = std::make_shared<SMA>(data_line, period);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            sma->calculate();
            if (i < data_size - 1) {
                data_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        // 验证结果有效性
        EXPECT_NO_NAN(sma->get(0));
        
        recordBenchmark(test_name, data_size, time_ms);
    }
};

TEST_F(SMAPerformanceTest, SmallDataset) {
    benchmarkSMACalculation(1000, 20, "SMA_Small_1K");
}

TEST_F(SMAPerformanceTest, MediumDataset) {
    benchmarkSMACalculation(10000, 20, "SMA_Medium_10K");
}

TEST_F(SMAPerformanceTest, LargeDataset) {
    benchmarkSMACalculation(100000, 20, "SMA_Large_100K");
}

TEST_F(SMAPerformanceTest, VaryingPeriods) {
    const size_t data_size = 50000;
    std::vector<size_t> periods = {5, 10, 20, 50, 100, 200};
    
    for (size_t period : periods) {
        std::string test_name = "SMA_Period_" + std::to_string(period);
        benchmarkSMACalculation(data_size, period, test_name);
    }
}

/**
 * @brief 复杂指标性能测试
 */
class ComplexIndicatorPerformanceTest : public PerformanceBenchmark {
public:
    void benchmarkRSI(size_t data_size) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 123);
        auto data_line = std::make_shared<LineRoot>(data_size, "rsi_perf");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        auto rsi = std::make_shared<RSI>(data_line, 14);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            rsi->calculate();
            if (i < data_size - 1) {
                data_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        EXPECT_NO_NAN(rsi->get(0));
        recordBenchmark("RSI_" + std::to_string(data_size), data_size, time_ms);
    }
    
    void benchmarkMACD(size_t data_size) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 456);
        auto data_line = std::make_shared<LineRoot>(data_size, "macd_perf");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        auto macd = std::make_shared<MACD>(data_line, 12, 26, 9);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            macd->calculate();
            if (i < data_size - 1) {
                data_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        EXPECT_NO_NAN(macd->getMACDLine(0));
        recordBenchmark("MACD_" + std::to_string(data_size), data_size, time_ms);
    }
    
    void benchmarkBollingerBands(size_t data_size) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 789);
        auto data_line = std::make_shared<LineRoot>(data_size, "bb_perf");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        auto bb = std::make_shared<BollingerBands>(data_line, 20, 2.0);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            bb->calculate();
            if (i < data_size - 1) {
                data_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        EXPECT_NO_NAN(bb->getMiddleBand(0));
        recordBenchmark("BollingerBands_" + std::to_string(data_size), data_size, time_ms);
    }
    
    void benchmarkIchimoku(size_t data_size) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 321);
        
        auto close_line = std::make_shared<LineRoot>(data_size, "ich_close");
        auto high_line = std::make_shared<LineRoot>(data_size, "ich_high");
        auto low_line = std::make_shared<LineRoot>(data_size, "ich_low");
        
        for (size_t i = 0; i < test_data[0].size(); ++i) {
            high_line->forward(test_data[1][i]);
            low_line->forward(test_data[2][i]);
            close_line->forward(test_data[3][i]);
        }
        
        auto ichimoku = std::make_shared<Ichimoku>(close_line, high_line, low_line, 9, 26, 52, 26);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            ichimoku->calculate();
            if (i < data_size - 1) {
                close_line->forward();
                high_line->forward();
                low_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        EXPECT_NO_NAN(ichimoku->getTenkanSen(0));
        recordBenchmark("Ichimoku_" + std::to_string(data_size), data_size, time_ms);
    }
};

TEST_F(ComplexIndicatorPerformanceTest, RSIPerformance) {
    benchmarkRSI(50000);
}

TEST_F(ComplexIndicatorPerformanceTest, MACDPerformance) {
    benchmarkMACD(50000);
}

TEST_F(ComplexIndicatorPerformanceTest, BollingerBandsPerformance) {
    benchmarkBollingerBands(50000);
}

TEST_F(ComplexIndicatorPerformanceTest, IchimokuPerformance) {
    benchmarkIchimoku(30000); // Ichimoku更复杂，使用较少数据
}

/**
 * @brief 多指标并行性能测试
 */
class MultiIndicatorPerformanceTest : public PerformanceBenchmark {
public:
    void benchmarkMultipleIndicators(size_t data_size) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 555);
        
        auto close_line = std::make_shared<LineRoot>(data_size, "multi_close");
        auto high_line = std::make_shared<LineRoot>(data_size, "multi_high");
        auto low_line = std::make_shared<LineRoot>(data_size, "multi_low");
        
        for (size_t i = 0; i < test_data[0].size(); ++i) {
            high_line->forward(test_data[1][i]);
            low_line->forward(test_data[2][i]);
            close_line->forward(test_data[3][i]);
        }
        
        // 创建多个指标
        auto sma_fast = std::make_shared<SMA>(close_line, 10);
        auto sma_slow = std::make_shared<SMA>(close_line, 30);
        auto rsi = std::make_shared<RSI>(close_line, 14);
        auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
        auto bb = std::make_shared<BollingerBands>(close_line, 20, 2.0);
        auto stoch = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
        auto atr = std::make_shared<ATR>(high_line, low_line, close_line, 14);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            sma_fast->calculate();
            sma_slow->calculate();
            rsi->calculate();
            macd->calculate();
            bb->calculate();
            stoch->calculate();
            atr->calculate();
            
            if (i < data_size - 1) {
                close_line->forward();
                high_line->forward();
                low_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        // 验证所有指标都有有效结果
        EXPECT_NO_NAN(sma_fast->get(0));
        EXPECT_NO_NAN(sma_slow->get(0));
        EXPECT_NO_NAN(rsi->get(0));
        EXPECT_NO_NAN(macd->getMACDLine(0));
        EXPECT_NO_NAN(bb->getMiddleBand(0));
        EXPECT_NO_NAN(stoch->getPercentK(0));
        EXPECT_NO_NAN(atr->get(0));
        
        recordBenchmark("MultiIndicator_" + std::to_string(data_size), data_size * 7, time_ms);
    }
    
    void benchmarkParallelCalculation(size_t data_size) {
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 777);
        
        // 创建多个独立的数据线（模拟并行处理）
        std::vector<std::shared_ptr<LineRoot>> data_lines;
        std::vector<std::shared_ptr<SMA>> sma_indicators;
        
        const int num_parallel = 4;
        
        for (int i = 0; i < num_parallel; ++i) {
            auto line = std::make_shared<LineRoot>(data_size, "parallel_" + std::to_string(i));
            for (double value : test_data[3]) {
                line->forward(value);
            }
            data_lines.push_back(line);
            sma_indicators.push_back(std::make_shared<SMA>(line, 20));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 模拟并行计算
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_parallel; ++t) {
            threads.emplace_back([&, t]() {
                for (size_t i = 0; i < data_size; ++i) {
                    sma_indicators[t]->calculate();
                    if (i < data_size - 1) {
                        data_lines[t]->forward();
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        // 验证结果
        for (auto& sma : sma_indicators) {
            EXPECT_NO_NAN(sma->get(0));
        }
        
        recordBenchmark("Parallel_" + std::to_string(num_parallel) + "x" + std::to_string(data_size), 
                       data_size * num_parallel, time_ms);
    }
};

TEST_F(MultiIndicatorPerformanceTest, MultipleIndicatorsSequential) {
    benchmarkMultipleIndicators(20000);
}

TEST_F(MultiIndicatorPerformanceTest, ParallelCalculation) {
    benchmarkParallelCalculation(20000);
}

/**
 * @brief 内存性能测试
 */
class MemoryPerformanceTest : public PerformanceBenchmark {
public:
    void benchmarkMemoryUsage() {
        const size_t data_size = 100000;
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 888);
        
        // 测试不同缓冲区大小的影响
        std::vector<size_t> buffer_sizes = {1000, 5000, 10000, 50000};
        
        for (size_t buffer_size : buffer_sizes) {
            auto data_line = std::make_shared<LineRoot>(buffer_size, "memory_test");
            
            for (double value : test_data[3]) {
                data_line->forward(value);
            }
            
            auto sma = std::make_shared<SMA>(data_line, 50);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < data_size; ++i) {
                sma->calculate();
                if (i < data_size - 1) {
                    data_line->forward();
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            double time_ms = duration.count() / 1000.0;
            
            EXPECT_NO_NAN(sma->get(0));
            
            std::string test_name = "MemBuf_" + std::to_string(buffer_size);
            recordBenchmark(test_name, data_size, time_ms, buffer_size / 1000); // 估算内存使用
        }
    }
    
    void benchmarkLargeScaleMemory() {
        // 创建大量指标实例测试内存管理
        const size_t num_indicators = 1000;
        const size_t data_size = 10000;
        
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 999);
        auto data_line = std::make_shared<LineRoot>(data_size, "large_scale");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        std::vector<std::shared_ptr<SMA>> indicators;
        indicators.reserve(num_indicators);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 创建大量指标
        for (size_t i = 0; i < num_indicators; ++i) {
            indicators.push_back(std::make_shared<SMA>(data_line, 20 + (i % 30)));
        }
        
        // 计算所有指标
        for (size_t step = 0; step < 100; ++step) {
            for (auto& indicator : indicators) {
                indicator->calculate();
            }
            if (step < 99) {
                data_line->forward();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;
        
        // 验证一些指标
        for (size_t i = 0; i < std::min(size_t(10), indicators.size()); ++i) {
            EXPECT_NO_NAN(indicators[i]->get(0));
        }
        
        recordBenchmark("LargeScale_" + std::to_string(num_indicators), 
                       num_indicators * 100, time_ms, num_indicators / 10); // 估算内存
        
        indicators.clear();
    }
};

TEST_F(MemoryPerformanceTest, BufferSizeImpact) {
    benchmarkMemoryUsage();
}

TEST_F(MemoryPerformanceTest, LargeScaleMemoryManagement) {
    benchmarkLargeScaleMemory();
}

/**
 * @brief 实时性能测试
 */
class RealTimePerformanceTest : public PerformanceBenchmark {
public:
    void benchmarkRealTimeSimulation() {
        const size_t data_size = 10000;
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 111);
        
        auto data_line = std::make_shared<LineRoot>(100, "realtime"); // 小缓冲区模拟实时
        auto sma_fast = std::make_shared<SMA>(data_line, 10);
        auto sma_slow = std::make_shared<SMA>(data_line, 30);
        auto rsi = std::make_shared<RSI>(data_line, 14);
        
        std::vector<double> processing_times;
        processing_times.reserve(data_size);
        
        auto total_start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            auto step_start = std::chrono::high_resolution_clock::now();
            
            // 添加新数据点
            data_line->forward(test_data[3][i]);
            
            // 计算指标
            sma_fast->calculate();
            sma_slow->calculate();
            rsi->calculate();
            
            auto step_end = std::chrono::high_resolution_clock::now();
            auto step_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(step_end - step_start);
            processing_times.push_back(step_duration.count() / 1000.0); // 微秒
            
            // 前进数据线
            if (i < data_size - 1) {
                data_line->forward();
            }
        }
        
        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start);
        double total_time_ms = total_duration.count() / 1000.0;
        
        // 计算统计信息
        auto stats = TestUtils::calculateStatistics(processing_times);
        double avg_time_us = stats[0];
        double max_time_us = stats[3];
        
        EXPECT_NO_NAN(sma_fast->get(0));
        EXPECT_NO_NAN(sma_slow->get(0));
        EXPECT_NO_NAN(rsi->get(0));
        
        recordBenchmark("RealTime_Avg", data_size, total_time_ms);
        
        std::cout << "\nReal-time Performance Details:\n";
        std::cout << "Average per-tick time: " << std::fixed << std::setprecision(2) << avg_time_us << " μs\n";
        std::cout << "Maximum per-tick time: " << std::fixed << std::setprecision(2) << max_time_us << " μs\n";
        std::cout << "Theoretical max frequency: " << std::fixed << std::setprecision(0) 
                 << (1000000.0 / avg_time_us) << " ticks/second\n";
    }
};

TEST_F(RealTimePerformanceTest, SimulatedRealTime) {
    benchmarkRealTimeSimulation();
}

/**
 * @brief 比较测试（与理论基准比较）
 */
class ComparisonTest : public PerformanceBenchmark {
public:
    void compareWithNaiveImplementation() {
        const size_t data_size = 20000;
        const size_t sma_period = 20;
        
        auto test_data = TestDataGenerator::generateRandomOHLCV(data_size, 100.0, 0.02, 222);
        
        // 测试我们的实现
        auto data_line = std::make_shared<LineRoot>(data_size, "optimized");
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        auto optimized_sma = std::make_shared<SMA>(data_line, sma_period);
        
        auto start_optimized = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < data_size; ++i) {
            optimized_sma->calculate();
            if (i < data_size - 1) {
                data_line->forward();
            }
        }
        
        auto end_optimized = std::chrono::high_resolution_clock::now();
        auto duration_optimized = std::chrono::duration_cast<std::chrono::microseconds>(end_optimized - start_optimized);
        double time_optimized_ms = duration_optimized.count() / 1000.0;
        
        // 朴素实现比较
        auto start_naive = std::chrono::high_resolution_clock::now();
        
        std::vector<double> naive_results;
        naive_results.reserve(data_size);
        
        for (size_t i = 0; i < data_size; ++i) {
            if (i >= sma_period - 1) {
                double sum = 0.0;
                for (size_t j = i - sma_period + 1; j <= i; ++j) {
                    sum += test_data[3][j];
                }
                naive_results.push_back(sum / sma_period);
            } else {
                naive_results.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        }
        
        auto end_naive = std::chrono::high_resolution_clock::now();
        auto duration_naive = std::chrono::duration_cast<std::chrono::microseconds>(end_naive - start_naive);
        double time_naive_ms = duration_naive.count() / 1000.0;
        
        // 验证结果一致性
        double optimized_result = optimized_sma->get(0);
        double naive_result = naive_results.back();
        
        EXPECT_DOUBLE_NEAR(optimized_result, naive_result, 1e-10);
        
        recordBenchmark("Optimized_SMA", data_size, time_optimized_ms);
        recordBenchmark("Naive_SMA", data_size, time_naive_ms);
        
        double speedup = time_naive_ms / time_optimized_ms;
        std::cout << "\nPerformance Comparison:\n";
        std::cout << "Optimized SMA: " << std::fixed << std::setprecision(2) << time_optimized_ms << " ms\n";
        std::cout << "Naive SMA: " << std::fixed << std::setprecision(2) << time_naive_ms << " ms\n";
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
        
        // 期望至少有一些性能提升
        EXPECT_GT(speedup, 0.8); // 至少不要慢于朴素实现的80%
    }
};

TEST_F(ComparisonTest, OptimizedVsNaive) {
    compareWithNaiveImplementation();
}

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Performance Benchmark Suite for Backtrader C++" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "System Information:" << std::endl;
    std::cout << "Hardware Concurrency: " << std::thread::hardware_concurrency() << " threads" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\nPerformance Benchmarks Completed!" << std::endl;
    return result;
}