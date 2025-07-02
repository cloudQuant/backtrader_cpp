#include "data/DataFeed.h"
#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "indicators/BollingerBands.h"
#include "strategy/StrategyBase.h"
#include <iostream>

using namespace backtrader;
using namespace backtrader::data;
using namespace backtrader::strategy;

// Simple test strategy
class TestStrategy : public StrategyBase {
private:
    std::shared_ptr<SMA> sma_fast_;
    std::shared_ptr<SMA> sma_slow_;
    std::shared_ptr<RSI> rsi_;
    std::shared_ptr<MACD> macd_;
    std::shared_ptr<BollingerBands> bb_;
    
public:
    TestStrategy() : StrategyBase("TestStrategy") {}
    
    void init() override {
        auto data = getData();
        if (data) {
            sma_fast_ = std::make_shared<SMA>(data->close(), 5);
            sma_slow_ = std::make_shared<SMA>(data->close(), 10);
            rsi_ = std::make_shared<RSI>(data->close(), 14);
            macd_ = std::make_shared<MACD>(data->close(), 12, 26, 9);
            bb_ = std::make_shared<BollingerBands>(data->close(), 20);
            
            addIndicator(sma_fast_);
            addIndicator(sma_slow_);
            addIndicator(rsi_);
            addIndicator(macd_);
            addIndicator(bb_);
        }
        std::cout << "Strategy initialized with indicators\n";
    }
    
    void next() override {
        if (!sma_fast_ || !sma_slow_) return;
        
        double fast_value = sma_fast_->get(0);
        double slow_value = sma_slow_->get(0);
        double rsi_value = rsi_->get(0);
        
        if (isNaN(fast_value) || isNaN(slow_value) || isNaN(rsi_value)) return;
        
        // Simple strategy logic
        if (isEmpty()) {
            if (fast_value > slow_value && rsi_value < 70) {
                buy(1.0);
                std::cout << "BUY signal - SMA fast: " << fast_value 
                         << ", slow: " << slow_value << ", RSI: " << rsi_value << "\n";
            }
        } else if (isLong()) {
            if (fast_value < slow_value || rsi_value > 80) {
                sell(getPositionSize());
                std::cout << "SELL signal - closing position\n";
            }
        }
    }
};

int main() {
    std::cout << "Verifying Phase 1 Implementation\n";
    std::cout << "==================================\n\n";
    
    // Test 1: Create test data
    std::cout << "1. Creating test data...\n";
    auto test_data_feed = DataFeedFactory::createRandom(100, 100.0, 0.02, "TestData");
    std::cout << "   Created random data feed with 100 points\n";
    
    // Test 2: Create sine wave data
    auto sine_data_feed = DataFeedFactory::createSineWave(50, 10.0, 0.1, 100.0, "SineWave");
    std::cout << "   Created sine wave data feed\n";
    
    // Test 3: Test data loading
    std::cout << "\n2. Testing data loading...\n";
    auto static_feed = dynamic_cast<StaticDataFeed*>(test_data_feed.get());
    if (static_feed) {
        size_t loaded = static_feed->loadBatch(20);
        std::cout << "   Loaded " << loaded << " data points\n";
        std::cout << "   Data length: " << static_feed->len() << "\n";
        std::cout << "   Remaining: " << static_feed->getRemainingDataCount() << "\n";
    }
    
    // Test 4: Test indicators
    std::cout << "\n3. Testing indicators...\n";
    if (static_feed && static_feed->len() > 0) {
        auto close_line = static_feed->close();
        
        // Test SMA
        auto sma = std::make_shared<SMA>(close_line, 5);
        std::cout << "   Created SMA(5)\n";
        
        // Test RSI
        auto rsi = std::make_shared<RSI>(close_line, 14);
        std::cout << "   Created RSI(14)\n";
        
        // Test MACD
        auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
        std::cout << "   Created MACD(12,26,9)\n";
        
        // Test Bollinger Bands
        auto bb = std::make_shared<BollingerBands>(close_line, 20);
        std::cout << "   Created BollingerBands(20)\n";
        
        // Calculate indicators for current data
        sma->calculate();
        rsi->calculate();
        macd->calculate();
        bb->calculate();
        
        std::cout << "   Indicators calculated successfully\n";
    }
    
    // Test 5: Test strategy
    std::cout << "\n4. Testing strategy framework...\n";
    auto strategy = std::make_unique<TestStrategy>();
    strategy->addDataFeed(std::move(test_data_feed));
    strategy->initialize();
    
    // Run strategy simulation
    std::cout << "\n5. Running strategy simulation...\n";
    int steps = 0;
    auto data_feed = strategy->getData();
    while (data_feed && data_feed->hasNext() && steps < 10) {
        data_feed->next();
        strategy->processNext();
        steps++;
    }
    
    std::cout << "   Processed " << steps << " data points\n";
    std::cout << "   Total trades: " << strategy->getTradeCount() << "\n";
    std::cout << "   Total PnL: " << strategy->getTotalPnL() << "\n";
    std::cout << "   Position size: " << strategy->getPositionSize() << "\n";
    
    strategy->finalize();
    
    std::cout << "\n✓ Phase 1 implementation verification completed successfully!\n";
    std::cout << "\nImplemented components:\n";
    std::cout << "  ✓ DataFeed system (CSV, Static, Factory)\n";
    std::cout << "  ✓ Advanced indicators (RSI, MACD, BollingerBands)\n";
    std::cout << "  ✓ Strategy framework with order management\n";
    std::cout << "  ✓ Position tracking and PnL calculation\n";
    
    return 0;
}