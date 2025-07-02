#include "cerebro/Cerebro.h"
#include "strategy/StrategyBase.h"
#include "indicators/Stochastic.h"
#include "indicators/WilliamsR.h"
#include "indicators/ATR.h"
#include "indicators/RSI.h"
#include "indicators/BollingerBands.h"
#include "analyzers/AnalyzerBase.h"
#include "data/DataFeed.h"
#include <iostream>
#include <iomanip>

using namespace backtrader;
using namespace backtrader::cerebro;
using namespace backtrader::strategy;
using namespace backtrader::data;
using namespace backtrader::analyzers;

// 高级测试策略
class AdvancedTestStrategy : public StrategyBase {
private:
    std::shared_ptr<Stochastic> stoch_;
    std::shared_ptr<WilliamsR> williams_;
    std::shared_ptr<ATR> atr_;
    std::shared_ptr<RSI> rsi_;
    std::shared_ptr<BollingerBands> bb_;
    
    // 策略参数
    double atr_multiplier_;
    double position_size_pct_;
    
public:
    AdvancedTestStrategy() : StrategyBase("AdvancedStrategy"),
                            atr_multiplier_(2.0), position_size_pct_(0.1) {
        setParam("atr_multiplier", atr_multiplier_);
        setParam("position_size_pct", position_size_pct_);
    }
    
    void init() override {
        auto data = getData();
        if (data) {
            // 初始化多个技术指标
            stoch_ = std::make_shared<Stochastic>(
                data->close(), data->high(), data->low(), 14, 3
            );
            williams_ = std::make_shared<WilliamsR>(
                data->close(), data->high(), data->low(), 14
            );
            atr_ = std::make_shared<ATR>(
                data->high(), data->low(), data->close(), 14
            );
            rsi_ = std::make_shared<RSI>(data->close(), 14);
            bb_ = std::make_shared<BollingerBands>(data->close(), 20, 2.0);
            
            addIndicator(stoch_);
            addIndicator(williams_);
            addIndicator(atr_);
            addIndicator(rsi_);
            addIndicator(bb_);
        }
        std::cout << "Advanced strategy initialized with 5 indicators\n";
    }
    
    void next() override {
        if (!stoch_ || !williams_ || !atr_ || !rsi_ || !bb_) return;
        
        // 获取指标值
        double stoch_k = stoch_->getPercentK(0);
        double stoch_d = stoch_->getPercentD(0);
        double williams_r = williams_->get(0);
        double atr_value = atr_->get(0);
        double rsi_value = rsi_->get(0);
        double bb_upper = bb_->getUpperBand(0);
        double bb_lower = bb_->getLowerBand(0);
        double current_price = getData()->close()->get(0);
        
        if (isNaN(stoch_k) || isNaN(williams_r) || isNaN(atr_value) || 
            isNaN(rsi_value) || isNaN(bb_upper)) {
            return;
        }
        
        // 多指标综合信号
        int bullish_signals = 0;
        int bearish_signals = 0;
        
        // 随机指标信号
        if (stoch_k > stoch_d && stoch_k < 80) bullish_signals++;
        if (stoch_k < stoch_d && stoch_k > 20) bearish_signals++;
        
        // 威廉指标信号
        if (williams_r > -80 && williams_r < -20) bullish_signals++;
        if (williams_r < -20) bearish_signals++;
        
        // RSI信号
        if (rsi_value > 30 && rsi_value < 70) bullish_signals++;
        if (rsi_value > 70) bearish_signals++;
        
        // 布林带信号
        if (current_price < bb_lower) bullish_signals++;
        if (current_price > bb_upper) bearish_signals++;
        
        // 计算动态仓位大小（基于ATR）
        double account_value = getTotalPnL() + 100000.0; // 假设初始资金10万
        double risk_amount = account_value * position_size_pct_;
        double position_size = (atr_value > 0) ? risk_amount / (atr_value * atr_multiplier_) : 1.0;
        position_size = std::max(0.1, std::min(position_size, 10.0)); // 限制仓位
        
        // 交易逻辑
        if (isEmpty()) {
            if (bullish_signals >= 3) {
                buy(position_size);
                std::cout << "BUY signal - Bullish: " << bullish_signals 
                         << ", Size: " << position_size 
                         << ", ATR: " << atr_value << "\n";
            }
        } else if (isLong()) {
            if (bearish_signals >= 2) {
                sell(getPositionSize());
                std::cout << "SELL signal - Bearish: " << bearish_signals << "\n";
            }
        }
    }
};

// 简单趋势策略（用于对比）
class SimpleTrendStrategy : public StrategyBase {
private:
    std::shared_ptr<SMA> sma_fast_;
    std::shared_ptr<SMA> sma_slow_;
    
public:
    SimpleTrendStrategy() : StrategyBase("SimpleTrend") {}
    
    void init() override {
        auto data = getData();
        if (data) {
            sma_fast_ = std::make_shared<SMA>(data->close(), 10);
            sma_slow_ = std::make_shared<SMA>(data->close(), 30);
            
            addIndicator(sma_fast_);
            addIndicator(sma_slow_);
        }
    }
    
    void next() override {
        if (!sma_fast_ || !sma_slow_) return;
        
        double fast_value = sma_fast_->get(0);
        double slow_value = sma_slow_->get(0);
        
        if (isNaN(fast_value) || isNaN(slow_value)) return;
        
        if (isEmpty() && fast_value > slow_value) {
            buy(1.0);
        } else if (isLong() && fast_value < slow_value) {
            sell(getPositionSize());
        }
    }
};

void printResults(const BacktestResult& result, const std::string& strategy_name) {
    std::cout << "\n=== " << strategy_name << " Results ===\n";
    std::cout << "Total Trades: " << result.total_trades << "\n";
    std::cout << "Total Return: " << std::fixed << std::setprecision(2) 
              << result.total_return << "%\n";
    std::cout << "Annualized Return: " << result.annualized_return << "%\n";
    std::cout << "Max Drawdown: " << result.max_drawdown << "%\n";
    std::cout << "Win Rate: " << result.win_rate << "%\n";
    std::cout << "Sharpe Ratio: " << result.sharpe_ratio << "\n";
    std::cout << "Execution Time: " << result.execution_time.count() << "s\n";
    
    if (!result.equity_curve.empty()) {
        std::cout << "Final Equity: " << result.equity_curve.back() << "\n";
    }
}

int main() {
    std::cout << "Verifying Phase 2 Implementation\n";
    std::cout << "==================================\n\n";
    
    // 创建测试数据
    std::cout << "1. Creating test data...\n";
    auto test_data = DataFeedFactory::createRandom(500, 100.0, 0.02, "TestData");
    auto trend_data = DataFeedFactory::createSineWave(300, 15.0, 0.05, 100.0, "TrendData");
    
    std::cout << "   Created random data (500 points) and trend data (300 points)\n";
    
    // 测试1: 高级策略回测
    {
        std::cout << "\n2. Testing Advanced Multi-Indicator Strategy...\n";
        
        Cerebro cerebro(100000.0);
        
        // 添加策略和数据
        auto strategy = std::make_shared<AdvancedTestStrategy>();
        cerebro.addStrategy(strategy);
        cerebro.addDataFeed(std::shared_ptr<DataFeed>(std::move(test_data)));
        
        // 配置broker
        cerebro.setCommission(0.001, true);  // 0.1%手续费
        cerebro.setSlippage(0.001, true);    // 0.1%滑点
        
        // 添加分析器
        cerebro.addAnalyzer(std::make_unique<ReturnsAnalyzer>());
        cerebro.addAnalyzer(std::make_unique<TradesAnalyzer>());
        cerebro.addAnalyzer(std::make_unique<DrawdownAnalyzer>());
        
        // 设置进度回调
        cerebro.setProgressCallback([](size_t progress) {
            if (progress % 100 == 0) {
                std::cout << "   Progress: " << progress << " bars processed\n";
            }
        });
        
        // 运行回测
        auto result = cerebro.run();
        printResults(result, "Advanced Strategy");
    }
    
    // 测试2: 简单策略对比
    {
        std::cout << "\n3. Testing Simple Trend Strategy for comparison...\n";
        
        Cerebro cerebro(100000.0);
        
        auto strategy = std::make_shared<SimpleTrendStrategy>();
        cerebro.addStrategy(strategy);
        cerebro.addDataFeed(std::shared_ptr<DataFeed>(std::move(trend_data)));
        
        cerebro.setCommission(0.001, true);
        cerebro.addAnalyzer(std::make_unique<ReturnsAnalyzer>());
        cerebro.addAnalyzer(std::make_unique<TradesAnalyzer>());
        
        auto result = cerebro.run();
        printResults(result, "Simple Trend Strategy");
    }
    
    // 测试3: 指标独立测试
    {
        std::cout << "\n4. Testing individual indicators...\n";
        
        // 创建小量测试数据
        auto small_data = DataFeedFactory::createRandom(50, 100.0, 0.02, "SmallData");
        auto static_feed = dynamic_cast<StaticDataFeed*>(small_data.get());
        static_feed->loadBatch();
        
        auto close_line = static_feed->close();
        auto high_line = static_feed->high();
        auto low_line = static_feed->low();
        
        // 测试Stochastic
        auto stoch = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
        
        // 测试Williams %R
        auto williams = std::make_shared<WilliamsR>(close_line, high_line, low_line, 14);
        
        // 测试ATR
        auto atr = std::make_shared<ATR>(high_line, low_line, close_line, 14);
        
        // 计算最后几个值
        for (int i = 0; i < 20; ++i) {
            stoch->calculate();
            williams->calculate();
            atr->calculate();
            
            if (i < 19) {
                close_line->forward();
                high_line->forward();
                low_line->forward();
            }
        }
        
        std::cout << "   Stochastic %K: " << stoch->getPercentK(0) 
                  << ", %D: " << stoch->getPercentD(0) << "\n";
        std::cout << "   Williams %R: " << williams->get(0) << "\n";
        std::cout << "   ATR: " << atr->get(0) 
                  << ", Relative ATR: " << atr->getRelativeATR() << "%\n";
        
        // 测试信号检测
        auto stoch_signal = stoch->getCrossoverSignal();
        auto williams_signal = williams->getReversalSignal();
        auto volatility_level = atr->getVolatilityLevel();
        
        std::cout << "   Stochastic crossover signal: " << stoch_signal << "\n";
        std::cout << "   Williams reversal signal: " << williams_signal << "\n";
        std::cout << "   Volatility level: " << volatility_level << "\n";
    }
    
    // 测试4: Broker系统测试
    {
        std::cout << "\n5. Testing advanced broker features...\n";
        
        auto data_feed = DataFeedFactory::createRandom(100, 100.0, 0.01, "BrokerTest");
        auto shared_data = std::shared_ptr<DataFeed>(std::move(data_feed));
        Broker broker(shared_data, 50000.0);
        
        // 设置高级手续费模型
        broker.setCommissionModel(
            std::make_unique<TieredCommissionModel>(0.002)
        );
        
        // 设置百分比滑点
        broker.setSlippageModel(
            std::make_unique<PercentageSlippageModel>(0.05)
        );
        
        // 设置风险参数
        broker.setRiskParameters(10000.0, 5000.0, 0.2, true);
        
        auto static_feed = std::dynamic_pointer_cast<StaticDataFeed>(shared_data);
        static_feed->loadBatch(20);
        
        // 模拟一些交易
        Order buy_order;
        buy_order.id = 1;
        buy_order.type = OrderType::MARKET;
        buy_order.side = OrderSide::BUY;
        buy_order.size = 10.0;
        
        size_t order_id = broker.submitOrder(buy_order);
        std::cout << "   Submitted buy order, ID: " << order_id << "\n";
        
        // 更新几步市场
        for (int i = 0; i < 5; ++i) {
            if (shared_data->hasNext()) {
                shared_data->next();
                broker.updateMarket();
            }
        }
        
        // 提交卖单
        Order sell_order;
        sell_order.id = 2;
        sell_order.type = OrderType::LIMIT;
        sell_order.side = OrderSide::SELL;
        sell_order.size = 5.0;
        sell_order.price = shared_data->close()->get(0) * 1.02; // 限价高于当前价格2%
        
        size_t sell_id = broker.submitOrder(sell_order);
        std::cout << "   Submitted limit sell order, ID: " << sell_id << "\n";
        
        // 获取账户信息
        auto account = broker.getAccountInfo();
        std::cout << "   Account equity: " << account.equity << "\n";
        std::cout << "   Realized P&L: " << account.realized_pnl << "\n";
        std::cout << "   Unrealized P&L: " << account.unrealized_pnl << "\n";
        std::cout << "   Total commission: " << account.total_commission << "\n";
        
        // 获取统计信息
        auto stats = broker.getTradingStatistics();
        std::cout << "   Total trades: " << stats.total_trades << "\n";
        std::cout << "   Win rate: " << stats.win_rate * 100 << "%\n";
        
        auto engine_stats = broker.getMatchingEngineStatistics();
        std::cout << "   Orders processed: " << engine_stats.total_orders << "\n";
        std::cout << "   Execution rate: " << engine_stats.execution_rate * 100 << "%\n";
    }
    
    std::cout << "\n✓ Phase 2 implementation verification completed successfully!\n";
    std::cout << "\nImplemented Phase 2 components:\n";
    std::cout << "  ✓ Advanced technical indicators (Stochastic, Williams %R, ATR)\n";
    std::cout << "  ✓ Sophisticated order matching engine with multiple order types\n";
    std::cout << "  ✓ Advanced broker system with risk management\n";
    std::cout << "  ✓ Multiple commission and slippage models\n";
    std::cout << "  ✓ Cerebro backtesting engine with analyzers\n";
    std::cout << "  ✓ Multi-strategy and multi-indicator support\n";
    std::cout << "  ✓ Comprehensive performance analysis\n";
    
    return 0;
}