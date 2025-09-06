#include "../include/strategy.h"
#include "../include/cerebro.h"
#include "../include/indicators/sma.h"
#include "../include/feeds/csvfeed.h"
#include "../include/analyzers/sharpe.h"
#include "../include/analyzers/drawdown.h"
#include "../include/analyzers/timereturn.h"
#include <iostream>
#include <memory>

/**
 * Simple Moving Average Crossover Strategy
 * 
 * This example demonstrates a classic SMA crossover strategy:
 * - Buy when short SMA crosses above long SMA
 * - Sell when short SMA crosses below long SMA
 * 
 * Features demonstrated:
 * - Custom strategy implementation
 * - Technical indicator usage
 * - Order management
 * - backtrader::Position tracking
 * - Performance analysis
 */
class SMAStrategy : public backtrader::Strategy {
public:
    // Strategy parameters
    struct Params {
        int short_period = 10;    // Short SMA period
        int long_period = 30;     // Long SMA period
        double stake = 100;       // Number of shares per trade
        bool print_log = true;    // Print trade logs
    };
    
    SMAStrategy(const Params& params = Params{}) : params_(params) {}
    
    void start() override {
        backtrader::Strategy::start();
        
        std::cout << "Starting SMA Strategy:" << std::endl;
        std::cout << "  Short SMA: " << params_.short_period << " periods" << std::endl;
        std::cout << "  Long SMA: " << params_.long_period << " periods" << std::endl;
        std::cout << "  backtrader::Position size: " << params_.stake << " shares" << std::endl;
        std::cout << "  Initial cash: $" << broker->get_cash() << std::endl;
        std::cout << std::endl;
    }
    
    void next() override {
        // Calculate current indicator values
        double short_sma = short_sma_->get_value();
        double long_sma = long_sma_->get_value();
        double prev_short_sma = short_sma_->get_value(-1);
        double prev_long_sma = long_sma_->get_value(-1);
        
        // Current price and date
        double current_price = data()->close[0];
        auto current_date = data()->datetime[0];
        
        // Check for crossover signals
        bool golden_cross = (prev_short_sma <= prev_long_sma) && (short_sma > long_sma);
        bool death_cross = (prev_short_sma >= prev_long_sma) && (short_sma < long_sma);
        
        // Get current position
        auto position = broker->get_position(data());
        bool has_position = position->size != 0;
        
        // Trading logic
        if (golden_cross && !has_position) {
            // Buy signal: Short SMA crosses above Long SMA
            auto order = buy(params_.stake);
            
            if (params_.print_log && order) {
                std::cout << format_date(current_date) << " BUY CREATE: " 
                         << params_.stake << " @ $" << std::fixed << std::setprecision(2) 
                         << current_price << std::endl;
            }
        }
        else if (death_cross && has_position) {
            // Sell signal: Short SMA crosses below Long SMA
            auto order = sell(params_.stake);
            
            if (params_.print_log && order) {
                std::cout << format_date(current_date) << " SELL CREATE: " 
                         << params_.stake << " @ $" << std::fixed << std::setprecision(2) 
                         << current_price << std::endl;
            }
        }
    }
    
    void notify_order(std::shared_ptr<backtrader::Order> order) override {
        if (order->status == backtrader::Order::Status::COMPLETED) {
            if (params_.print_log) {
                std::string action = order->is_buy() ? "BUY" : "SELL";
                std::cout << format_date(order->executed.dt) << " " << action 
                         << " EXECUTED: " << order->executed.size 
                         << " @ $" << std::fixed << std::setprecision(2) 
                         << order->executed.price << std::endl;
            }
        }
        else if (order->status == backtrader::Order::Status::CANCELED ||
                 order->status == backtrader::Order::Status::MARGIN ||
                 order->status == backtrader::Order::Status::REJECTED) {
            if (params_.print_log) {
                std::cout << "Order " << order->get_status_name() << std::endl;
            }
        }
    }
    
    void notify_trade(std::shared_ptr<backtrader::Trade> trade) override {
        if (trade->is_closed() && params_.print_log) {
            std::cout << format_date(trade->close_datetime) << " TRADE CLOSED: "
                     << "P&L: $" << std::fixed << std::setprecision(2) << trade->pnl
                     << " (Net: $" << trade->pnl_net << ")" << std::endl;
        }
    }
    
    void stop() override {
        double final_value = broker->get_value();
        double total_return = (final_value / broker->get_cash()) - 1.0;
        
        std::cout << std::endl << "Strategy Results:" << std::endl;
        std::cout << "  Final Portfolio Value: $" << std::fixed << std::setprecision(2) 
                 << final_value << std::endl;
        std::cout << "  Total Return: " << std::fixed << std::setprecision(2) 
                 << (total_return * 100.0) << "%" << std::endl;
        std::cout << std::endl;
    }
    
private:
    Params params_;
    std::shared_ptr<backtrader::indicators::SMA> short_sma_;
    std::shared_ptr<backtrader::indicators::SMA> long_sma_;
    
    void create_indicators() override {
        // Create moving averages
        short_sma_ = add_indicator<backtrader::indicators::SMA>(params_.short_period);
        long_sma_ = add_indicator<backtrader::indicators::SMA>(params_.long_period);
    }
    
    std::string format_date(const std::chrono::system_clock::time_point& dt) const {
        std::time_t time_t = std::chrono::system_clock::to_time_t(dt);
        std::tm* tm_ptr = std::localtime(&time_t);
        
        std::ostringstream oss;
        oss << std::put_time(tm_ptr, "%Y-%m-%d");
        return oss.str();
    }
};

/**
 * Main function demonstrating the SMA strategy
 */
int main() {
    try {
        std::cout << "=== Backtrader C++ SMA Strategy Example ===" << std::endl << std::endl;
        
        // Create Cerebro engine
        auto cerebro = std::make_unique<backtrader::Cerebro>();
        
        // Set initial cash
        cerebro->broker()->set_cash(10000.0);
        
        // Set commission (0.1% per trade)
        cerebro->broker()->set_commission(0.001);
        
        // Add strategy with custom parameters
        SMAStrategy::Params strategy_params;
        strategy_params.short_period = 10;
        strategy_params.long_period = 30;
        strategy_params.stake = 100;
        strategy_params.print_log = true;
        
        cerebro->add_strategy<SMAStrategy>(strategy_params);
        
        // Add data feed (you would replace this with actual data)
        // For this example, we'll create synthetic data
        auto data_feed = create_sample_data_feed();
        cerebro->add_data(data_feed);
        
        // Add analyzers for performance analysis
        cerebro->add_analyzer<backtrader::analyzers::Sharpe>();
        cerebro->add_analyzer<backtrader::analyzers::DrawDown>();
        cerebro->add_analyzer<backtrader::analyzers::TimeReturn>();
        
        // Run the backtest
        std::cout << "Running backtest..." << std::endl << std::endl;
        auto results = cerebro->run();
        
        // Print analyzer results
        std::cout << "=== Performance Analysis ===" << std::endl;
        
        if (!results.empty()) {
            auto strategy_result = results[0];
            
            // Print Sharpe ratio
            auto sharpe_analyzer = strategy_result->get_analyzer<backtrader::analyzers::Sharpe>();
            if (sharpe_analyzer) {
                std::cout << "Sharpe Ratio: " << std::fixed << std::setprecision(3) 
                         << sharpe_analyzer->get_sharpe_ratio() << std::endl;
            }
            
            // Print maximum drawdown
            auto drawdown_analyzer = strategy_result->get_analyzer<backtrader::analyzers::DrawDown>();
            if (drawdown_analyzer) {
                std::cout << "Max Drawdown: " << std::fixed << std::setprecision(2) 
                         << (drawdown_analyzer->get_max_drawdown() * 100.0) << "%" << std::endl;
            }
            
            // Print total return
            auto return_analyzer = strategy_result->get_analyzer<backtrader::analyzers::TimeReturn>();
            if (return_analyzer) {
                auto returns = return_analyzer->get_returns();
                if (!returns.empty()) {
                    double total_return = 1.0;
                    for (auto ret : returns) {
                        total_return *= (1.0 + ret.second);
                    }
                    total_return -= 1.0;
                    
                    std::cout << "Total Return: " << std::fixed << std::setprecision(2) 
                             << (total_return * 100.0) << "%" << std::endl;
                }
            }
        }
        
        // Optional: Plot results (if plotting is enabled)
        // cerebro->plot();
        
        std::cout << std::endl << "Backtest completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

/**
 * Helper function to create sample data for demonstration
 * In a real application, you would load data from CSV, database, or API
 */
std::shared_ptr<backtrader::feeds::DataFeed> create_sample_data_feed() {
    // This would be replaced with actual data loading
    // For now, return a placeholder
    
    // Example: Load from CSV file
    // return std::make_shared<backtrader::feeds::CSVFeed>("data/AAPL.csv");
    
    // For this example, create synthetic data
    auto feed = std::make_shared<backtrader::feeds::GenericDataFeed>();
    
    // Generate 252 days of sample OHLCV data (1 trading year)
    auto start_date = std::chrono::system_clock::now() - std::chrono::hours(24 * 365);
    double price = 100.0;
    
    for (int i = 0; i < 252; ++i) {
        auto date = start_date + std::chrono::hours(24 * i);
        
        // Random walk with slight upward bias
        double change = ((std::rand() % 200) - 95) / 1000.0; // -9.5% to +10.5%
        double open = price;
        price *= (1.0 + change);
        double close = price;
        double high = std::max(open, close) * (1.0 + (std::rand() % 50) / 10000.0);
        double low = std::min(open, close) * (1.0 - (std::rand() % 50) / 10000.0);
        double volume = 1000000 + (std::rand() % 2000000);
        
        feed->add_bar(date, open, high, low, close, volume, 0.0);
    }
    
    return feed;
}