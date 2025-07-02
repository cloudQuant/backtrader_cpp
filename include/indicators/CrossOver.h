#pragma once

#include "IndicatorBase.h"
#include "Common.h"

namespace backtrader {

/**
 * @brief 交叉指标（CrossOver）
 * 
 * 检测两条线的交叉情况：
 * - 返回 1.0：line1上穿line2（金叉）
 * - 返回 -1.0：line1下穿line2（死叉）
 * - 返回 0.0：无交叉
 * 
 * 这是策略中常用的信号指标
 */
class CrossOver : public IndicatorBase {
private:
    std::shared_ptr<LineRoot> line2_;  // 第二条输入线
    bool has_previous_;                 // 是否有前一个值
    double prev_line1_;                // 前一个line1值
    double prev_line2_;                // 前一个line2值
    
public:
    /**
     * @brief 构造函数
     * @param line1 第一条输入线
     * @param line2 第二条输入线
     */
    explicit CrossOver(std::shared_ptr<LineRoot> line1, 
                       std::shared_ptr<LineRoot> line2)
        : IndicatorBase(line1, "CrossOver"), 
          line2_(line2),
          has_previous_(false),
          prev_line1_(NaN),
          prev_line2_(NaN) {
        
        if (!line2) {
            throw std::invalid_argument("CrossOver line2 cannot be null");
        }
        
        // 添加第二条输入线
        addInput(line2);
        
        // 最小周期为2（需要前一个值进行比较）
        setMinPeriod(2);
    }
    
    /**
     * @brief 获取第二条线
     * @return 第二条输入线
     */
    std::shared_ptr<LineRoot> getLine2() const { return line2_; }
    
    /**
     * @brief 重置指标状态
     */
    void reset() override {
        IndicatorBase::reset();
        has_previous_ = false;
        prev_line1_ = NaN;
        prev_line2_ = NaN;
    }
    
    /**
     * @brief 单步计算
     */
    void calculate() override {
        if (!hasValidInput() || inputs_.size() < 2) {
            setOutput(0, 0.0);  // 无交叉
            return;
        }
        
        auto line1 = getInput(0);
        auto line2 = getInput(1);
        
        double current_line1 = line1->get(0);
        double current_line2 = line2->get(0);
        
        if (isNaN(current_line1) || isNaN(current_line2)) {
            setOutput(0, 0.0);
            return;
        }
        
        double crossover_value = 0.0;
        
        if (has_previous_ && !isNaN(prev_line1_) && !isNaN(prev_line2_)) {
            // 检测交叉
            // 金叉：line1从下方上穿line2
            if (prev_line1_ <= prev_line2_ && current_line1 > current_line2) {
                crossover_value = 1.0;
            }
            // 死叉：line1从上方下穿line2
            else if (prev_line1_ >= prev_line2_ && current_line1 < current_line2) {
                crossover_value = -1.0;
            }
        }
        
        // 更新前一个值
        prev_line1_ = current_line1;
        prev_line2_ = current_line2;
        has_previous_ = true;
        
        setOutput(0, crossover_value);
    }
    
    /**
     * @brief 批量计算（向量化版本）
     * @param start 起始位置
     * @param end 结束位置
     */
    void calculateBatch(size_t start, size_t end) override {
        if (!hasValidInput() || inputs_.size() < 2) {
            return;
        }
        
        auto line1 = getInput(0);
        auto line2 = getInput(1);
        
        for (size_t i = start; i < end; ++i) {
            calculate();
            
            if (i < end - 1) {
                line1->forward();
                line2->forward();
            }
        }
    }
    
    /**
     * @brief 检查是否为金叉
     * @param ago 偏移量，默认0
     * @return true if golden cross
     */
    bool isGoldenCross(int ago = 0) const {
        double value = get(ago);
        return !isNaN(value) && value > 0.0;
    }
    
    /**
     * @brief 检查是否为死叉
     * @param ago 偏移量，默认0
     * @return true if death cross
     */
    bool isDeathCross(int ago = 0) const {
        double value = get(ago);
        return !isNaN(value) && value < 0.0;
    }
    
    /**
     * @brief 检查是否有交叉
     * @param ago 偏移量，默认0
     * @return true if any cross
     */
    bool hasCross(int ago = 0) const {
        double value = get(ago);
        return !isNaN(value) && value != 0.0;
    }
    
    /**
     * @brief 获取交叉方向
     * @param ago 偏移量，默认0
     * @return 1.0=金叉, -1.0=死叉, 0.0=无交叉
     */
    double getCrossDirection(int ago = 0) const {
        return get(ago);
    }
    
    /**
     * @brief 创建便捷的CrossOver指标
     * @param line1 第一条线（通常是快线）
     * @param line2 第二条线（通常是慢线）
     * @return CrossOver指标实例
     */
    static std::shared_ptr<CrossOver> create(std::shared_ptr<LineRoot> line1,
                                           std::shared_ptr<LineRoot> line2) {
        return std::make_shared<CrossOver>(line1, line2);
    }
    
    /**
     * @brief 从两个指标创建CrossOver
     * @param indicator1 第一个指标
     * @param indicator2 第二个指标
     * @return CrossOver指标实例
     */
    static std::shared_ptr<CrossOver> fromIndicators(
            std::shared_ptr<IndicatorBase> indicator1,
            std::shared_ptr<IndicatorBase> indicator2) {
        return std::make_shared<CrossOver>(
            std::static_pointer_cast<LineRoot>(indicator1),
            std::static_pointer_cast<LineRoot>(indicator2)
        );
    }
};

/**
 * @brief CrossUp指标 - 专门检测上穿
 */
class CrossUp : public CrossOver {
public:
    explicit CrossUp(std::shared_ptr<LineRoot> line1, 
                     std::shared_ptr<LineRoot> line2)
        : CrossOver(line1, line2) {
        setName("CrossUp");
    }
    
    void calculate() override {
        CrossOver::calculate();
        double value = get(0);
        // 只保留上穿信号
        setOutput(0, value > 0.0 ? 1.0 : 0.0);
    }
};

/**
 * @brief CrossDown指标 - 专门检测下穿
 */
class CrossDown : public CrossOver {
public:
    explicit CrossDown(std::shared_ptr<LineRoot> line1, 
                       std::shared_ptr<LineRoot> line2)
        : CrossOver(line1, line2) {
        setName("CrossDown");
    }
    
    void calculate() override {
        CrossOver::calculate();
        double value = get(0);
        // 只保留下穿信号
        setOutput(0, value < 0.0 ? 1.0 : 0.0);
    }
};

} // namespace backtrader