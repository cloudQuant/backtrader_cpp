# Backtrader C++ 重构详细计划

## 📋 重构总体策略

### 核心原则
1. **测试驱动开发**: 先用gtest重写测试用例，确保接口和行为一致性
2. **渐进式重构**: 从底层基础组件开始，逐步向上层应用扩展
3. **性能优化**: 保持Python版本的灵活性，提升C++的执行效率
4. **接口兼容**: 保持核心API的使用习惯，降低学习成本

## 🎯 Phase 1: 测试框架和基础设施 (4-6周)

### 1.1 测试框架搭建 (1周)
- [ ] 配置CMake构建系统
- [ ] 集成Google Test框架
- [ ] 创建测试数据管理系统
- [ ] 实现Python兼容的浮点数格式化
- [ ] 建立CI/CD测试流水线

### 1.2 核心测试用例移植 (3-5周)

#### 第1批 - 核心基础 (1周)
- [ ] `test_ind_sma.py` → `SMATest.cpp`
- [ ] `test_position.py` → `PositionTest.cpp`
- [ ] `test_comminfo.py` → `CommissionTest.cpp`

#### 第2批 - 重要指标 (2周)
- [ ] `test_ind_ema.py` → `EMATest.cpp`
- [ ] `test_ind_rsi.py` → `RSITest.cpp`
- [ ] `test_ind_atr.py` → `ATRTest.cpp`
- [ ] `test_ind_bbands.py` → `BollingerBandsTest.cpp`

#### 第3批 - 核心功能 (2周)
- [ ] `test_trade.py` → `TradeTest.cpp`
- [ ] `test_order.py` → `OrderTest.cpp`
- [ ] `test_data_resample.py` → `DataResampleTest.cpp`

## 🏗️ Phase 2: 核心数据结构 (6-8周)

### 2.1 元类系统重构 (2周)
```cpp
// 设计目标：实现类似Python的元类功能
class MetaBase {
public:
    virtual void DoPreNew() {}
    virtual void DoNew() {}
    virtual void DoPreInit() {}
    virtual void DoInit() {}
    virtual void DoPostInit() {}
};

template<typename T>
class MetaParams : public MetaBase {
private:
    std::map<std::string, std::any> params_;
public:
    template<typename U>
    void SetParam(const std::string& name, U&& value);
    
    template<typename U>
    U GetParam(const std::string& name) const;
};
```

### 2.2 数据线系统 (3-4周)
```cpp
// LineRoot基类
class LineRoot {
protected:
    std::vector<double> data_;
    size_t current_index_ = 0;
    
public:
    // 运算符重载
    LineRoot operator+(const LineRoot& other) const;
    LineRoot operator-(const LineRoot& other) const;
    LineRoot operator*(const LineRoot& other) const;
    LineRoot operator/(const LineRoot& other) const;
    
    // 索引访问
    double operator[](int index) const;
    double operator()(int index) const; // 负索引支持
};

// LineBuffer环形缓冲区
class LineBuffer : public LineRoot {
private:
    std::deque<double> buffer_;
    size_t max_size_;
    
public:
    void Forward(size_t size = 1);
    void Home();
    void Extend(size_t size);
    
    // 内存管理模式
    enum BufferMode { UnBounded, QBuffer };
    void SetBufferMode(BufferMode mode, size_t max_len = 0);
};
```

### 2.3 LineIterator和LineSeries (1-2周)
```cpp
class LineIterator {
protected:
    std::vector<std::unique_ptr<LineBuffer>> lines_;
    size_t min_period_ = 1;
    
public:
    virtual void PreNext() {}
    virtual void NextStart() {}
    virtual void Next() = 0;
    virtual void PreOnce(size_t start, size_t end) {}
    virtual void OnceStart(size_t start, size_t end) {}
    virtual void Once(size_t start, size_t end) {}
    
    // 执行控制
    void _Next();
    void _Once();
};
```

## 🔧 Phase 3: 指标系统实现 (8-10周)

### 3.1 基础指标框架 (2周)
```cpp
class IndicatorBase : public LineIterator {
protected:
    std::vector<LineRoot*> data_sources_;
    
public:
    IndicatorBase(const std::vector<LineRoot*>& data);
    
    // 缓存系统
    static std::unordered_map<std::string, std::shared_ptr<IndicatorBase>> cache_;
    
    // 工厂方法
    template<typename T, typename... Args>
    static std::shared_ptr<T> Create(Args&&... args);
};
```

### 3.2 核心指标实现 (6-8周)

#### 移动平均类 (2周)
- [ ] SMA (Simple Moving Average)
- [ ] EMA (Exponential Moving Average)  
- [ ] WMA (Weighted Moving Average)
- [ ] HMA (Hull Moving Average)
- [ ] ZLEMA (Zero Lag EMA)

#### 振荡器指标 (2周)
- [ ] RSI (Relative Strength Index)
- [ ] LRSI (Laguerre RSI)
- [ ] CCI (Commodity Channel Index)
- [ ] Williams %R

#### 趋势指标 (2周)
- [ ] MACD (Moving Average Convergence Divergence)
- [ ] ADX (Average Directional Movement Index)
- [ ] Aroon Oscillator
- [ ] TRIX

#### 波动率指标 (2周)
- [ ] ATR (Average True Range)
- [ ] Bollinger Bands
- [ ] Standard Deviation
- [ ] Variance

## 📊 Phase 4: 数据管理系统 (4-6周)

### 4.1 数据源抽象 (2周)
```cpp
class DataFeed {
public:
    enum DataState {
        LIVE, DELAYED, CONNBROKEN, NOTSUBSCRIBED,
        DISCONNECTED, UNKNOWN, RECONNECTING, REPLAY
    };
    
protected:
    std::vector<OHLCV> data_;
    DataState state_ = LIVE;
    
public:
    virtual bool LoadData(const std::string& source) = 0;
    virtual OHLCV GetNext() = 0;
    virtual bool HasNext() const = 0;
    
    // 时间框架处理
    void SetTimeFrame(TimeFrame tf, int compression = 1);
    
    // 过滤器支持
    void AddFilter(std::unique_ptr<DataFilter> filter);
};

// CSV数据源实现
class CSVDataFeed : public DataFeed {
public:
    bool LoadData(const std::string& filename) override;
    OHLCV GetNext() override;
};
```

### 4.2 数据处理功能 (2-4周)
- [ ] 数据重采样 (Resample)
- [ ] 数据回放 (Replay)
- [ ] 多时间框架同步
- [ ] 数据过滤器系统

## 💰 Phase 5: 交易系统核心 (6-8周)

### 5.1 订单管理 (2周)
```cpp
class Order {
public:
    enum Type { Market, Limit, Stop, StopLimit };
    enum Status { Created, Submitted, Accepted, Partial, Completed, Canceled, Expired, Margin, Rejected };
    
private:
    Type type_;
    Status status_;
    double size_;
    double price_;
    double exec_price_;
    DateTime created_time_;
    
public:
    void UpdateStatus(Status new_status, double exec_price = 0.0);
    bool IsActive() const;
    bool IsCompleted() const;
};

class OrderManager {
private:
    std::vector<std::unique_ptr<Order>> orders_;
    std::queue<std::unique_ptr<Order>> pending_orders_;
    
public:
    OrderId SubmitOrder(std::unique_ptr<Order> order);
    void CancelOrder(OrderId id);
    void ProcessPendingOrders();
};
```

### 5.2 持仓管理 (2周)
```cpp
class Position {
private:
    double size_ = 0.0;
    double price_ = 0.0;
    double uprice_ = 0.0;  // 未实现价格
    DateTime datetime_;
    
public:
    void Update(double size, double price, const DateTime& dt);
    double GetUnrealizedPnL(double current_price) const;
    double GetRealizedPnL() const;
    bool IsLong() const { return size_ > 0; }
    bool IsShort() const { return size_ < 0; }
};
```

### 5.3 交易记录 (1周)
```cpp
class Trade {
private:
    std::vector<std::unique_ptr<Order>> orders_;
    double pnl_ = 0.0;
    double pnl_commission_ = 0.0;
    DateTime open_datetime_;
    DateTime close_datetime_;
    
public:
    void AddOrder(std::unique_ptr<Order> order);
    void Close();
    double GetPnL() const { return pnl_; }
    double GetPnLNet() const { return pnl_commission_; }
    bool IsOpen() const;
    bool IsClosed() const;
};
```

### 5.4 经纪商模拟 (1-3周)
```cpp
class Broker {
private:
    double cash_ = 100000.0;
    double value_ = 100000.0;
    std::unique_ptr<CommissionInfo> commission_info_;
    std::unique_ptr<OrderManager> order_manager_;
    std::map<std::string, Position> positions_;
    
public:
    OrderId Buy(const std::string& symbol, double size, double price = 0.0);
    OrderId Sell(const std::string& symbol, double size, double price = 0.0);
    
    double GetCash() const { return cash_; }
    double GetValue() const { return value_; }
    
    void SetCommission(std::unique_ptr<CommissionInfo> commission);
    void ProcessOrders(const std::map<std::string, OHLCV>& market_data);
};
```

## 📈 Phase 6: 策略框架 (4-6周)

### 6.1 策略基类 (2-3周)
```cpp
class Strategy : public LineIterator {
protected:
    std::unique_ptr<Broker> broker_;
    std::vector<std::unique_ptr<DataFeed>> data_feeds_;
    std::vector<std::unique_ptr<IndicatorBase>> indicators_;
    std::vector<std::unique_ptr<AnalyzerBase>> analyzers_;
    
public:
    // 生命周期钩子
    virtual void Start() {}
    virtual void PreNext() override {}
    virtual void NextStart() override {}
    virtual void Next() override = 0;
    virtual void Stop() {}
    
    // 通知机制
    virtual void NotifyOrder(const Order& order) {}
    virtual void NotifyTrade(const Trade& trade) {}
    virtual void NotifyCashValue(double cash, double value) {}
    virtual void NotifyData(const DataFeed& data, DataState state) {}
    
    // 交易接口
    OrderId Buy(double size = 0, double price = 0);
    OrderId Sell(double size = 0, double price = 0);
    void Close(const std::string& symbol = "");
    
    // 数据访问
    template<int N = 0>
    const DataFeed& Data() const;
    
    template<typename T>
    const T& GetIndicator(const std::string& name) const;
};
```

### 6.2 参数优化系统 (2-3周)
```cpp
class ParameterOptimizer {
public:
    struct Parameter {
        std::string name;
        std::any min_value;
        std::any max_value;
        std::any step;
    };
    
private:
    std::vector<Parameter> parameters_;
    std::function<double(const std::map<std::string, std::any>&)> objective_function_;
    
public:
    void AddParameter(const Parameter& param);
    void SetObjectiveFunction(std::function<double(const std::map<std::string, std::any>&)> func);
    
    std::map<std::string, std::any> Optimize();
    
    // 支持多进程优化
    std::map<std::string, std::any> OptimizeParallel(int num_threads = 0);
};
```

## 🧠 Phase 7: 主引擎 Cerebro (3-4周)

### 7.1 执行引擎 (2-3周)
```cpp
class Cerebro {
private:
    std::vector<std::unique_ptr<Strategy>> strategies_;
    std::vector<std::unique_ptr<DataFeed>> data_feeds_;
    std::unique_ptr<Broker> broker_;
    std::vector<std::unique_ptr<AnalyzerBase>> analyzers_;
    
    // 执行控制
    bool run_once_ = true;
    bool preload_ = true;
    bool live_ = false;
    
public:
    // 策略管理
    void AddStrategy(std::unique_ptr<Strategy> strategy);
    void OptimizeStrategy(std::unique_ptr<Strategy> strategy, 
                         const std::vector<ParameterOptimizer::Parameter>& params);
    
    // 数据管理
    void AddData(std::unique_ptr<DataFeed> data);
    void ResampleData(TimeFrame timeframe, int compression = 1);
    void ReplayData();
    
    // 分析器
    void AddAnalyzer(std::unique_ptr<AnalyzerBase> analyzer);
    
    // 执行
    std::vector<StrategyResult> Run();
    
    // 配置
    void SetBroker(std::unique_ptr<Broker> broker);
    void SetRunOnce(bool run_once) { run_once_ = run_once; }
    void SetPreload(bool preload) { preload_ = preload; }
};
```

### 7.2 多进程优化支持 (1周)
```cpp
class MultiProcessOptimizer {
private:
    std::unique_ptr<ThreadPool> thread_pool_;
    
public:
    MultiProcessOptimizer(int num_workers = 0);
    
    std::vector<StrategyResult> OptimizeStrategy(
        const Strategy& strategy_template,
        const std::vector<ParameterSet>& parameter_sets,
        const std::vector<std::unique_ptr<DataFeed>>& data_feeds);
};
```

## 📊 Phase 8: 分析器系统 (3-4周)

### 8.1 分析器基类 (1周)
```cpp
class AnalyzerBase {
protected:
    std::map<std::string, std::any> results_;
    Strategy* strategy_ = nullptr;
    
public:
    virtual void Start() {}
    virtual void Next() {}
    virtual void Stop() {}
    
    virtual void NotifyOrder(const Order& order) {}
    virtual void NotifyTrade(const Trade& trade) {}
    
    const std::map<std::string, std::any>& GetResults() const { return results_; }
    
    template<typename T>
    T GetResult(const std::string& key) const;
};
```

### 8.2 核心分析器实现 (2-3周)
- [ ] SQN (System Quality Number)
- [ ] Sharpe Ratio
- [ ] Drawdown分析
- [ ] 年化收益率
- [ ] 交易分析器
- [ ] 时间收益分析

## 🎨 Phase 9: 可视化和输出 (2-3周)

### 9.1 数据输出 (1周)
```cpp
class Writer {
public:
    virtual void WriteHeader() = 0;
    virtual void WriteData(const std::map<std::string, std::any>& data) = 0;
    virtual void WriteFooter() = 0;
};

class CSVWriter : public Writer {
    // CSV格式输出实现
};

class JSONWriter : public Writer {
    // JSON格式输出实现
};
```

### 9.2 绘图接口 (1-2周) - 可选
```cpp
class PlotInterface {
public:
    virtual void PlotCandles(const std::vector<OHLCV>& data) = 0;
    virtual void PlotLine(const std::vector<double>& data, const std::string& name) = 0;
    virtual void PlotIndicator(const IndicatorBase& indicator) = 0;
    virtual void Show() = 0;
};
```

## 🔧 Phase 10: 性能优化和完善 (4-6周)

### 10.1 性能优化 (2-3周)
- [ ] 内存池管理
- [ ] SIMD指令优化关键计算
- [ ] 缓存优化数据访问模式
- [ ] 编译时优化（模板特化）

### 10.2 并发优化 (1-2周)
- [ ] 无锁数据结构
- [ ] 线程池管理
- [ ] 异步I/O处理

### 10.3 质量保证 (1-2周)
- [ ] 内存泄漏检测
- [ ] 单元测试覆盖率100%
- [ ] 性能基准测试
- [ ] 文档完善

## 📝 关键技术决策

### 编程语言标准
- **C++17**: 提供现代C++特性，保持广泛兼容性

### 核心依赖库
- **STL**: 标准模板库，数据结构和算法
- **Google Test**: 单元测试框架
- **CMake**: 构建系统
- **可选**: Boost (仅在必要时使用)

### 内存管理策略
- **智能指针**: 使用std::unique_ptr和std::shared_ptr
- **RAII**: 资源获取即初始化
- **内存池**: 对频繁分配的小对象使用内存池

### 并发模型
- **数据不可变性**: 尽量使用不可变数据结构
- **消息传递**: 线程间通信使用消息队列
- **任务并行**: 使用std::async和std::future

## 📊 预期成果

### 性能目标
- **执行速度**: 比Python版本快5-10倍
- **内存使用**: 减少50%以上内存占用
- **并发性能**: 支持多核并行优化

### 兼容性目标
- **API兼容**: 保持90%以上的Python API兼容性
- **测试兼容**: 所有原始测试用例通过
- **数据兼容**: 支持相同的数据格式和来源

### 质量目标
- **测试覆盖率**: 95%以上
- **文档覆盖率**: 100%核心API文档
- **代码质量**: 通过静态分析工具检查

## 🚀 实施建议

### 团队配置
- **项目负责人**: 1名 (架构设计和进度管理)
- **核心开发**: 2-3名 (C++高级工程师)
- **测试工程师**: 1名 (测试用例移植和验证)
- **DevOps工程师**: 1名 (构建系统和CI/CD)

### 里程碑管理
- **每2周**: 阶段性成果汇报
- **每4周**: 详细的进度评审和调整
- **每阶段结束**: 代码审查和质量检查

### 风险控制
- **技术风险**: 提前进行关键技术的验证和原型开发
- **进度风险**: 采用敏捷开发方式，及时调整优先级
- **质量风险**: 严格的代码审查和测试覆盖率要求

这个重构计划总计需要40-55周的开发时间（约10-14个月），严格遵循测试驱动开发的原则，确保每个阶段的成果都有可靠的测试验证，最终实现一个高性能、高质量的C++量化交易框架。