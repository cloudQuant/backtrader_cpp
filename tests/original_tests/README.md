# Backtrader Original Tests - C++ Port

这个目录包含了从原始Python backtrader测试套件移植的C++测试用例。这些测试确保C++实现与原始Python实现产生完全相同的计算结果。

## 测试理念

### Test-Driven Development (TDD) 方法
按照用户要求，我们采用了TDD方法：
1. **首先移植测试用例**：保持测试的输入、输出和接口与原始Python版本完全一致
2. **然后实现C++代码**：确保通过所有移植的测试
3. **不修改测试用例**：测试用例作为"黄金标准"，C++实现必须适应测试

### 精确性要求
- **完全一致的计算结果**：C++计算结果必须与Python版本在6位小数精度内完全匹配
- **相同的最小周期**：每个指标的最小周期必须与Python版本相同
- **相同的NaN处理**：数据不足时的NaN处理必须一致
- **相同的参数默认值**：所有指标的默认参数必须与Python版本匹配

## 已移植的测试

### 1. Simple Moving Average (SMA)
**文件**: `test_ind_sma.cpp`

**原始Python测试**:
```python
chkvals = [['4063.463000', '3644.444667', '3554.693333']]
chkmin = 30
```

**测试内容**:
- 默认30周期SMA计算验证
- 不同周期的参数化测试
- 边界条件测试
- 精度验证测试

### 2. Relative Strength Index (RSI)
**文件**: `test_ind_rsi.cpp`

**原始Python测试**:
```python
chkvals = [['57.644284', '41.630968', '53.352553']]
chkmin = 15
```

**测试内容**:
- 默认14周期RSI计算验证
- 0-100范围验证
- 超买超卖状态测试
- 不同周期的参数化测试
- 计算逻辑验证

### 3. Bollinger Bands (BBands)
**文件**: `test_ind_bbands.cpp`

**原始Python测试**:
```python
chkvals = [
    ['4065.884000', '3621.185000', '3582.895500'],  # middle
    ['4190.782310', '3712.008864', '3709.453081'],  # upper
    ['3940.985690', '3530.361136', '3456.337919'],  # lower
]
chkmin = 20
```

**测试内容**:
- 三条线（上轨、中轨、下轨）计算验证
- 带宽和百分比B计算
- 价格位置统计
- 标准差对称性验证

### 4. Average True Range (ATR)
**文件**: `test_ind_atr.cpp`

**原始Python测试**:
```python
chkvals = [['35.866308', '34.264286', '54.329064']]
chkmin = 15
```

**测试内容**:
- 默认14周期ATR计算验证
- True Range计算逻辑验证
- 非负值验证
- 波动性度量测试

### 5. Stochastic Oscillator
**文件**: `test_ind_stochastic.cpp`

**原始Python测试**:
```python
chkvals = [
    ['88.667626', '21.409626', '63.796187'],  # %K
    ['82.845850', '15.710059', '77.642219'],  # %D
]
chkmin = 18
```

**测试内容**:
- %K和%D线计算验证
- 0-100范围验证
- 超买超卖检测
- 平滑性验证（%D应比%K平滑）

## 测试数据

### 数据来源
所有测试使用与原始Python测试相同的数据文件：
- **文件**: `2006-day-001.txt`
- **格式**: CSV格式，包含Date, Open, High, Low, Close, Volume, OpenInterest
- **期间**: 2006年全年日线数据
- **价格范围**: 约3500-4200

### 数据格式
```csv
Date,Open,High,Low,Close,Volume,OpenInterest
2006-01-02,3578.73,3605.95,3578.73,3604.33,0,0
2006-01-03,3604.08,3638.42,3601.84,3614.34,0,0
...
```

## 验证方法

### 检查点策略
每个测试验证三个关键点的计算结果：
1. **第一个有效值** (index 0)
2. **中间值** (index = -(data_length - min_period) / 2)
3. **最后一个值** (index = -(data_length - min_period))

这与原始Python测试的`chkpts = [0, -l + mp, (-l + mp) // 2]`完全一致。

### 精度匹配
- 使用`std::fixed << std::setprecision(6)`格式化数值
- 字符串完全匹配，确保与Python的`'%f'`格式一致
- 特殊处理NaN值的情况

### 最小周期验证
每个指标必须报告正确的最小周期：
- SMA: `period` 
- RSI: `period + 1`
- BBands: `period`
- ATR: `period + 1`
- Stochastic: `period + period_dfast + 1`

## 构建和运行

### 构建测试
```bash
cd backtrader_cpp
mkdir build && cd build
cmake .. -DENABLE_TESTING=ON
make -j4

# 构建原始测试
cd tests/original_tests
cmake .
make -j4
```

### 运行测试
```bash
# 运行所有原始测试
make run_original_tests

# 运行特定指标测试
make run_sma_test
make run_rsi_test
make run_bbands_test
make run_atr_test
make run_stochastic_test

# 运行调试模式
make run_original_tests_debug

# 直接运行可执行文件
./original_tests/test_ind_sma
./all_original_tests --sma    # 只运行SMA测试
./all_original_tests --debug  # 运行调试测试
```

### 测试输出示例
```
Running: OriginalTests.SMA_Manual
[  PASSED  ] OriginalTests.SMA_Manual
Running: OriginalTests.RSI_Manual
[  PASSED  ] OriginalTests.RSI_Manual

========================================
Original Backtrader Test Results Summary
========================================
Total Tests Run: 25
Passed: 25
Failed: 0

Success Rate: 100.0%
========================================
🎉 All original tests passed! C++ implementation matches Python behavior.
```

## 测试类型

### 1. 手动验证测试 (`*_Manual`)
- 直接对应原始Python测试
- 验证特定的三个检查点值
- 验证最小周期
- 作为主要的合规性测试

### 2. 参数化测试 (`*ParameterizedTest`)
- 测试不同参数组合
- 验证指标在各种配置下的行为
- 确保参数变化不会破坏基本属性

### 3. 范围验证测试 (`*_RangeValidation`)
- 验证指标值在预期范围内
- RSI: 0-100
- Stochastic: 0-100
- ATR: >= 0

### 4. 边界条件测试 (`*_EdgeCases`)
- 测试数据不足情况
- 测试极端数值
- 测试特殊情况（如价格平坦）

### 5. 计算逻辑测试 (`*_Calculation*`)
- 验证特定计算步骤
- 与手动计算结果比较
- 确保算法正确性

## 添加新测试

### 移植新的Python测试
1. **分析Python测试文件**:
   ```python
   chkdatas = 1
   chkvals = [['val1', 'val2', 'val3']]
   chkmin = N
   chkind = btind.IndicatorName
   ```

2. **创建C++测试文件**:
   ```cpp
   #include "test_common.h"
   #include "indicators/IndicatorName.h"
   
   const std::vector<std::vector<std::string>> EXPECTED_VALUES = {
       {"val1", "val2", "val3"}
   };
   const int MIN_PERIOD = N;
   
   TEST(OriginalTests, IndicatorName_Manual) {
       // 实现测试逻辑
   }
   ```

3. **验证计算结果**:
   - 确保使用相同的测试数据
   - 验证相同的检查点
   - 确保精度匹配

### 测试命名规范
- `test_ind_*.cpp`: 指标测试文件
- `*_Manual`: 对应原始Python测试的手动测试
- `*_Debug`: 带调试输出的测试版本
- `*ParameterizedTest`: 参数化测试类
- `*_EdgeCases`: 边界条件测试

## 故障排除

### 常见问题

1. **计算结果不匹配**
   - 检查算法实现
   - 验证参数默认值
   - 确认数据读取正确

2. **最小周期不正确**
   - 检查指标的warmup逻辑
   - 确认期间计算方式

3. **NaN处理不一致**
   - 检查数据不足时的处理
   - 确认初始值设置

4. **测试数据找不到**
   - 确保测试数据文件路径正确
   - 检查CSV读取器实现

### 调试技巧

1. **使用调试测试**:
   ```cpp
   TEST(OriginalTests, SMA_Debug) {
       runtest<SMA>(expected_vals, min_period, true);  // true = debug mode
   }
   ```

2. **打印中间值**:
   ```cpp
   for (size_t i = 0; i < data.size(); ++i) {
       indicator->calculate();
       std::cout << "Step " << i << ": " << indicator->get(0) << std::endl;
   }
   ```

3. **手动验证特定点**:
   ```cpp
   double manual_calc = (data[n-1] + data[n-2] + ... + data[n-period]) / period;
   EXPECT_NEAR(indicator->get(0), manual_calc, 1e-10);
   ```

## 下一步计划

### 待移植的Python测试
1. **基础指标**:
   - EMA (Exponential Moving Average)
   - WMA (Weighted Moving Average)
   - DEMA, TEMA, KAMA (高级移动平均)

2. **震荡指标**:
   - Williams %R
   - CCI (Commodity Channel Index)
   - Ultimate Oscillator
   - Momentum

3. **趋势指标**:
   - MACD (包含直方图)
   - PPO (Price Percentage Oscillator)
   - DM (Directional Movement)
   - Aroon

4. **复杂指标**:
   - Ichimoku (一目均衡表)
   - Trix
   - Vortex
   - Heikin-Ashi

5. **统计指标**:
   - Highest/Lowest
   - Percentile Rank
   - Linear Regression Slope
   - Standard Deviation

### 测试增强
1. **多数据源测试**
2. **多时间框架测试**
3. **性能基准测试**
4. **内存泄漏测试**
5. **并发安全测试**

这个测试套件确保了C++重构版本的正确性，为整个项目提供了可靠的质量保证基础。