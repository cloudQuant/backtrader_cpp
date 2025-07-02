#pragma once

#include "core/LineRoot.h"
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace backtrader {
namespace test {

/**
 * @brief OHLCV数据结构
 */
struct OHLCVData {
    double open;
    double high;
    double low;
    double close;
    double volume;
};

/**
 * @brief 测试数据提供器
 * 
 * 提供各种测试数据生成功能
 */
class TestDataProvider {
public:
    /**
     * @brief 生成随机数据
     * @param count 数据点数量
     * @param mean 均值
     * @param stddev 标准差
     * @param seed 随机种子
     * @return 随机数据向量
     */
    static std::vector<double> generateRandomData(size_t count, 
                                                 double mean = 0.0, 
                                                 double stddev = 1.0,
                                                 uint32_t seed = 12345);
    
    /**
     * @brief 生成趋势数据
     * @param count 数据点数量
     * @param start_value 起始值
     * @param trend_rate 趋势斜率
     * @param noise_level 噪声水平
     * @param seed 随机种子
     * @return 趋势数据向量
     */
    static std::vector<double> generateTrendingData(size_t count,
                                                   double start_value = 100.0,
                                                   double trend_rate = 0.1,
                                                   double noise_level = 1.0,
                                                   uint32_t seed = 12345);
    
    /**
     * @brief 生成正弦波数据
     * @param count 数据点数量
     * @param amplitude 振幅
     * @param frequency 频率（周期数）
     * @param phase 相位
     * @param offset 偏移量
     * @return 正弦波数据向量
     */
    static std::vector<double> generateSineWave(size_t count,
                                               double amplitude = 1.0,
                                               double frequency = 1.0,
                                               double phase = 0.0,
                                               double offset = 0.0);
    
    /**
     * @brief 生成阶跃函数数据
     * @param count 数据点数量
     * @param levels 各个水平值
     * @param step_size 每个水平持续的点数
     * @return 阶跃函数数据向量
     */
    static std::vector<double> generateStepFunction(size_t count,
                                                   const std::vector<double>& levels,
                                                   size_t step_size = 10);
    
    /**
     * @brief 从数据向量创建LineRoot对象
     * @param data 数据向量
     * @param name 数据线名称
     * @return LineRoot智能指针
     */
    static std::shared_ptr<LineRoot> createLineRootFromData(const std::vector<double>& data,
                                                           const std::string& name = "test_data");
    
    /**
     * @brief 比较两个double向量是否近似相等
     * @param a 向量A
     * @param b 向量B
     * @param tolerance 容差
     * @return true if approximately equal
     */
    static bool compareDoubleVectors(const std::vector<double>& a,
                                    const std::vector<double>& b,
                                    double tolerance = 1e-9);
    
    /**
     * @brief 生成OHLCV数据
     * @param count 数据点数量
     * @param initial_price 初始价格
     * @param volatility 波动率
     * @param trend 趋势
     * @param seed 随机种子
     * @return OHLCV数据向量
     */
    static std::vector<OHLCVData> generateOHLCVData(size_t count,
                                                    double initial_price = 100.0,
                                                    double volatility = 2.0,
                                                    double trend = 0.01,
                                                    uint32_t seed = 12345);
};

} // namespace test
} // namespace backtrader