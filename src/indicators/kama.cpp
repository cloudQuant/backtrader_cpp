#include "indicators/kama.h"
#include "indicators/sma.h"
#include "dataseries.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>
#include <iostream>
#include <iomanip>
#include <fstream>

namespace backtrader {

// AdaptiveMovingAverage implementation
AdaptiveMovingAverage::AdaptiveMovingAverage() 
    : Indicator(), data_source_(nullptr), current_index_(0), prev_kama_(0.0), initialized_(false) {
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    // Note: SMA params setup would be done differently based on actual SMA implementation
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
}

AdaptiveMovingAverage::AdaptiveMovingAverage(std::shared_ptr<LineSeries> data_source)
    : Indicator(), data_source_(data_source), current_index_(0), prev_kama_(0.0), initialized_(false) {
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

AdaptiveMovingAverage::AdaptiveMovingAverage(std::shared_ptr<LineSeries> data_source, int period, int fast, int slow)
    : Indicator(), data_source_(data_source), current_index_(0), prev_kama_(0.0), initialized_(false) {
    params.period = period;
    params.fast = fast;
    params.slow = slow;
    
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

AdaptiveMovingAverage::AdaptiveMovingAverage(std::shared_ptr<DataSeries> data_source)
    : Indicator(), data_source_(nullptr), current_index_(0), prev_kama_(0.0), initialized_(false) {
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

AdaptiveMovingAverage::AdaptiveMovingAverage(std::shared_ptr<DataSeries> data_source, int period, int fast, int slow)
    : Indicator(), data_source_(nullptr), current_index_(0), prev_kama_(0.0), initialized_(false) {
    params.period = period;
    params.fast = fast;
    params.slow = slow;
    
    setup_lines();
    
    // Create SMA for initial seed value
    sma_seed_ = std::make_shared<indicators::SMA>();
    
    // Calculate smoothing constants
    fast_sc_ = 2.0 / (params.fast + 1.0);   // Fast EMA smoothing factor
    slow_sc_ = 2.0 / (params.slow + 1.0);   // Slow EMA smoothing factor
    
    _minperiod(params.period + 1); // Need period + 1 for direction calculation
    
    // Set data for test framework compatibility
    this->data = data_source;
    this->datas.push_back(data_source);
}

double AdaptiveMovingAverage::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto line = lines->getline(kama);
    if (!line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Delegate to the line's operator[] which handles ago indexing properly
    return (*line)[ago];
}

int AdaptiveMovingAverage::getMinPeriod() const {
    return params.period + 1;
}

void AdaptiveMovingAverage::setup_lines() {
    if (lines->size() == 0) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
}

void AdaptiveMovingAverage::prenext() {
    Indicator::prenext();
    
    // SMA will be calculated separately - avoid calling protected methods
}

void AdaptiveMovingAverage::nextstart() {
    if (datas.empty() || !datas[0]->lines) return;
    
    // Connect data to SMA seed if not already done
    if (sma_seed_->datas.empty() && !datas.empty()) {
        sma_seed_->datas = datas;
    }
    
    // Use manual SMA calculation instead of calling protected methods
    
    auto kama_line = lines->getline(kama);
    
    // Calculate simple moving average manually for seed value
    if (!datas.empty() && datas[0]->lines) {
        // For simple line series (like from tests), use first line (index 0)
    // For OHLCV data (DataSeries), use close price line (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = DataSeries::Close;  // Close price for DataSeries
    }
    auto close_line = datas[0]->lines->getline(line_index);
        if (close_line && close_line->size() >= static_cast<size_t>(params.period)) {
            double sum = 0.0;
            for (int i = 0; i < params.period; ++i) {
                double value = (*close_line)[i];
                sum += value;
            }
            double sma_value = sum / params.period;
            
            if (kama_line) {
                // Initialize KAMA with SMA value
                prev_kama_ = sma_value;
                kama_line->set(0, prev_kama_);
                initialized_ = true;
            }
        }
    }
}

void AdaptiveMovingAverage::next() {
    if (!initialized_) {
        nextstart();
        return;
    }
    
    if (datas.empty() || !datas[0]->lines) return;
    
    auto kama_line = lines->getline(kama);
    // For simple line series (like from tests), use first line (index 0)
    // For OHLCV data (DataSeries), use close price line (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = DataSeries::Close;  // Close price for DataSeries
    }
    auto close_line = datas[0]->lines->getline(line_index);
    
    if (!kama_line || !close_line) return;
    
    // Calculate efficiency ratio
    double efficiency_ratio = calculate_efficiency_ratio();
    
    // Calculate smoothing constant
    double smoothing_constant = calculate_smoothing_constant(efficiency_ratio);
    
    // Calculate KAMA: KAMA = KAMA_prev + SC * (Price - KAMA_prev)
    double current_price = (*close_line)[0];
    double kama_value = prev_kama_ + smoothing_constant * (current_price - prev_kama_);
    
    kama_line->set(0, kama_value);
    prev_kama_ = kama_value;
}

void AdaptiveMovingAverage::once(int start, int end) {
    // std::cout << "KAMA::once ENTRY: start=" << start << ", end=" << end << ", params.period=" << params.period << std::endl;
    
    if (datas.empty() || !datas[0]->lines) {
        // std::cout << "KAMA::once: Early return - no data" << std::endl;
        return;
    }
    
    // Debug output to understand calling pattern
    // std::cout << "KAMA::once start=" << start << ", end=" << end << ", params.period=" << params.period << std::endl;
    
    auto kama_line = lines->getline(kama);
    // For simple line series (like from tests), use first line (index 0)
    // For OHLCV data (DataSeries), use close price line (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = DataSeries::Close;  // Close price for DataSeries
    }
    auto close_line = datas[0]->lines->getline(line_index);
    
    if (!kama_line || !close_line) {
        return;
    }
    
    auto kama_buffer = std::dynamic_pointer_cast<LineBuffer>(kama_line);
    if (!kama_buffer) return;
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (!close_buffer) return;
    
    // Get the actual data array from the buffer
    std::vector<double> prices = close_buffer->array();
    // std::cout << "KAMA::once prices.size()=" << prices.size();
    
    // Debug: find first non-NaN value
    int first_non_nan = -1;
    for (int i = 0; i < static_cast<int>(prices.size()); ++i) {
        if (!std::isnan(prices[i])) {
            first_non_nan = i;
            break;
        }
    }
    // std::cout << ", first_non_nan_at=" << first_non_nan;
    
    if (!prices.empty()) {
        // std::cout << ", First 5 prices: ";
        for (int i = 0; i < 5 && i < static_cast<int>(prices.size()); ++i) {
            // std::cout << prices[i] << " ";
        }
    }
    // std::cout << std::endl;
    
    // Skip initial NaN if present
    int data_offset = 0;
    if (!prices.empty() && std::isnan(prices[0])) {
        data_offset = 1;
    }
    
    // 关键修复：无论框架传入什么范围，我们都在第一次有效调用时计算全部数据
    // 这是因为Python版本使用ExponentialSmoothingDynamic，期望一次性计算所有值
    // std::cout << "KAMA::once: Checking condition start=" << start << " >= params.period=" << params.period << std::endl;
    if (start >= params.period) {
        // 这是第一次有效调用 - 计算从最小周期到数据结尾的所有值
        // std::cout << "KAMA::once: Computing ALL values from " << params.period << " to " << (prices.size() - data_offset) << std::endl;
        
        // 重置并计算所有KAMA值
        kama_buffer->reset();
        
        // 先填充前面的NaN值 (reset已经添加了一个，所以少添加一个)
        for (int i = 1; i < params.period; ++i) {
            kama_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
        
        double kama_prev = std::numeric_limits<double>::quiet_NaN();
        
        // 计算所有有效的KAMA值
        for (int i = params.period; i < static_cast<int>(prices.size()) - data_offset; ++i) {
            if (i == params.period) {
                // 第一个KAMA值：使用SMA作为种子
                double sum = 0.0;
                for (int j = 0; j < params.period; ++j) {
                    sum += prices[j + data_offset];
                }
                double sma_value = sum / params.period;
                
                // 计算效率比率和平滑常数
                double current_price = prices[i + data_offset];
                double period_ago_price = prices[i - params.period + data_offset];
                double direction = current_price - period_ago_price;  // Don't take abs here
                
                double volatility = 0.0;
                for (int j = 1; j <= params.period; ++j) {
                    double price_change = std::abs(prices[i - j + 1 + data_offset] - prices[i - j + data_offset]);
                    volatility += price_change;
                }
                
                double efficiency_ratio = (volatility != 0.0) ? std::abs(direction / volatility) : 0.0;
                double sc_range = fast_sc_ - slow_sc_;
                double smoothing_constant = std::pow(efficiency_ratio * sc_range + slow_sc_, 2.0);
                
                // KAMA = 前值 + SC * (当前价格 - 前值)
                kama_prev = sma_value + smoothing_constant * (current_price - sma_value);
                kama_buffer->append(kama_prev);
                
                // std::cout << "*** MODIFIED KAMA CALCULATION *** i=" << i << ": sma=" << sma_value << ", kama=" << kama_prev << std::endl;
            } else {
                // 后续KAMA值：使用前一个KAMA值
                double current_price = prices[i + data_offset];
                double period_ago_price = prices[i - params.period + data_offset];
                double direction = current_price - period_ago_price;  // Don't take abs here
                
                double volatility = 0.0;
                for (int j = 1; j <= params.period; ++j) {
                    double price_change = std::abs(prices[i - j + 1 + data_offset] - prices[i - j + data_offset]);
                    volatility += price_change;
                }
                
                double efficiency_ratio = (volatility != 0.0) ? std::abs(direction / volatility) : 0.0;
                double sc_range = fast_sc_ - slow_sc_;
                double smoothing_constant = std::pow(efficiency_ratio * sc_range + slow_sc_, 2.0);
                
                kama_prev = kama_prev + smoothing_constant * (current_price - kama_prev);
                kama_buffer->append(kama_prev);
            }
        }
        
        // 设置缓冲区索引到末尾以便正确的ago索引
        if (kama_buffer->size() > 0) {
            kama_buffer->set_idx(kama_buffer->size() - 1);
        }
        
        // std::cout << "KAMA::once: Calculated " << kama_buffer->size() << " total values" << std::endl;
    } else {
        // 忽略无效的调用
        // std::cout << "KAMA::once: Ignoring call with start=" << start << " < period=" << params.period << std::endl;
    }
}

double AdaptiveMovingAverage::calculate_efficiency_ratio() {
    if (datas.empty() || !datas[0]->lines) return 0.0;
    
    // For simple line series (like from tests), use first line (index 0)
    // For OHLCV data (DataSeries), use close price line (index 4)
    int line_index = 0;
    auto dataseries = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    if (dataseries) {
        line_index = DataSeries::Close;  // Close price for DataSeries
    }
    auto close_line = datas[0]->lines->getline(line_index);
    if (!close_line || close_line->size() <= params.period) return 0.0;
    
    // Direction: absolute change over period
    double current_price = (*close_line)[0];
    double period_ago_price = (*close_line)[params.period];
    double direction = std::abs(current_price - period_ago_price);
    
    // Volatility: sum of absolute daily changes
    double volatility = 0.0;
    for (int i = 1; i <= params.period; ++i) {
        double price_change = std::abs((*close_line)[i-1] - (*close_line)[i]);
        volatility += price_change;
    }
    
    // Efficiency Ratio
    return (volatility != 0.0) ? direction / volatility : 0.0;
}

double AdaptiveMovingAverage::calculate_smoothing_constant(double efficiency_ratio) {
    // SC = [ER * (fast SC - slow SC) + slow SC]^2
    double sc_range = fast_sc_ - slow_sc_;
    return std::pow(efficiency_ratio * sc_range + slow_sc_, 2.0);
}

void AdaptiveMovingAverage::calculate() {
    // std::cout << "KAMA::calculate() called, datas.size()=" << datas.size() << std::endl;
    if (datas.empty()) {
        // std::cout << "KAMA::calculate() early return - datas.empty()" << std::endl;
        return;
    }
    
    // Debug: Check what type of data we have
    if (!datas.empty() && datas[0]) {
        auto ds = std::dynamic_pointer_cast<DataSeries>(datas[0]);
        auto ls = std::dynamic_pointer_cast<LineSeries>(datas[0]);
        if (ds) {
            // std::cout << "KAMA::calculate() - datas[0] is DataSeries" << std::endl;
        } else if (ls) {
            // std::cout << "KAMA::calculate() - datas[0] is LineSeries with " << ls->lines->size() << " lines" << std::endl;
        }
    }
    
    // Similar pattern to EMA: calculate all values at once, ignoring framework's once() calls
    auto kama_line = lines->getline(kama);
    auto data_series = std::dynamic_pointer_cast<DataSeries>(datas[0]);
    
    // For DataSeries, use Close price (index 4)
    // For LineSeries, use first line (index 0)
    int line_index = 0;
    if (data_series) {
        line_index = DataSeries::Close;  // Use Close enum for clarity
    }
    
    auto close_line = datas[0]->lines->getline(line_index);
    
    if (!kama_line || !close_line) return;
    
    auto kama_buffer = std::dynamic_pointer_cast<LineBuffer>(kama_line);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line);
    if (!kama_buffer || !close_buffer) return;
    
    // Get actual data array size
    std::vector<double> prices = close_buffer->array();
    // std::cout << "KAMA::calculate() prices.size()=" << prices.size() << std::endl;
    if (prices.empty()) {
        // std::cout << "KAMA::calculate() early return - prices.empty()" << std::endl;
        return;
    }
    
    // Debug: print first few prices
    // std::cout << "KAMA::calculate() First 5 prices: ";
    for (int i = 0; i < 5 && i < static_cast<int>(prices.size()); ++i) {
        // std::cout << prices[i] << " ";
    }
    // std::cout << std::endl;
    
    // Count initial NaN values
    int nan_count = 0;
    for (size_t i = 0; i < prices.size(); ++i) {
        if (std::isnan(prices[i])) {
            nan_count++;
        } else {
            break;
        }
    }
    
    // Reset and calculate all KAMA values using the Python approach:
    // 1. Pre-calculate all efficiency ratios and smoothing constants
    // 2. Apply ExponentialSmoothingDynamic with varying alpha
    kama_buffer->reset();
    
    // First, add the initial NaN values to maintain alignment
    for (int i = 0; i < nan_count; ++i) {
        kama_buffer->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    int effective_size = static_cast<int>(prices.size()) - nan_count;
    std::vector<double> alphas(effective_size, std::numeric_limits<double>::quiet_NaN());
    
    // Step 1: Calculate dynamic smoothing constants (alphas) for each position
    for (int i = params.period; i < effective_size; ++i) {
        // Python: direction = self.data - self.data(-self.p.period)
        double current_price = prices[i + nan_count];
        double period_ago_price = prices[i - params.period + nan_count];
        double direction = current_price - period_ago_price;  // Don't take abs here
        
        // Python: volatility = SumN(abs(self.data - self.data(-1)), period=self.p.period)
        double volatility = 0.0;
        for (int j = 1; j <= params.period; ++j) {
            double price_change = std::abs(prices[i - j + 1 + nan_count] - prices[i - j + nan_count]);
            volatility += price_change;
        }
        
        // Python: er = abs(direction / volatility)
        double efficiency_ratio = (volatility > 0.0) ? std::abs(direction / volatility) : 0.0;
        
        // Python: sc = pow((er * (fast - slow)) + slow, 2)
        double sc_range = fast_sc_ - slow_sc_;
        double smoothing_constant = std::pow(efficiency_ratio * sc_range + slow_sc_, 2.0);
        
        alphas[i] = smoothing_constant;
        
        // Debug output for first few values only
        // if (i - params.period < 3) {
        //     std::cout << "DEBUG Alpha[" << i << "]: price[" << i << "]=" << current_price << ", price[" << (i-params.period) << "]=" << period_ago_price << ", direction=" << direction << ", volatility=" << volatility << ", ER=" << efficiency_ratio << ", alpha=" << smoothing_constant << std::endl;
        // }
    }
    
    // std::cout << "DEBUG About to start SMA calculation step" << std::endl;
    
    // Step 2: Apply ExponentialSmoothingDynamic logic
    // First, calculate seed value (SMA of first period)
    double sma_seed = 0.0;
    for (int j = 0; j < params.period; ++j) {
        sma_seed += prices[j + nan_count];
    }
    sma_seed /= params.period;
    
    // std::cout << "DEBUG SMA calculation: nan_count=" << nan_count << ", sma_seed=" << sma_seed << std::endl;
    
    // Fill NaN values up to the minimum period (reset已经添加了一个，所以少添加一个)
    for (int i = 1; i < params.period; ++i) {
        kama_buffer->append(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Apply dynamic exponential smoothing: av = prev * (1 - alpha) + data * alpha
    // Start from the position where we have first alpha value
    double prev_kama = sma_seed;
    
    // KAMA seed calculated
    
    int calculated_count = 0;
    for (int i = params.period; i < effective_size; ++i) {
        double current_price = prices[i + nan_count];
        double alpha = alphas[i];
        
        if (!std::isnan(alpha)) {
            double kama_value = prev_kama * (1.0 - alpha) + current_price * alpha;
            kama_buffer->append(kama_value);
            prev_kama = kama_value;
            
            // if (calculated_count < 3) {
            //     std::cout << "DEBUG KAMA[" << calculated_count << "]: prev=" << (calculated_count == 0 ? sma_seed : prev_kama) << ", current=" << current_price << ", alpha=" << alpha << ", KAMA=" << kama_value << std::endl;
            // }
            calculated_count++;
        } else {
            kama_buffer->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    // std::cout << "KAMA::calculate() - Calculated " << calculated_count << " valid KAMA values out of " << kama_buffer->size() << " total values" << std::endl;
    
    // Set the buffer index to the end for proper ago indexing
    if (kama_buffer->size() > 0) {
        kama_buffer->set_idx(kama_buffer->size() - 1);
    }
}

size_t AdaptiveMovingAverage::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    
    auto kama_line = lines->getline(kama);
    if (!kama_line) {
        return 0;
    }
    
    auto kama_buffer = std::dynamic_pointer_cast<const LineBuffer>(kama_line);
    if (!kama_buffer) {
        return kama_line->size();
    }
    
    // Return the actual number of values in the array
    return kama_buffer->array().size();
}

} // namespace backtrader