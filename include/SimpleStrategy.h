#pragma once

#include "strategy/StrategyBase.h"
#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "indicators/ATR.h"
#include <memory>
#include <map>

namespace backtrader {

/**
 * @brief Simple moving average crossover strategy
 * 
 * Implements a basic dual moving average crossover strategy with
 * additional filters using RSI and ATR for risk management.
 */
class SMACrossoverStrategy : public StrategyBase {
private:
    // Strategy parameters
    size_t fast_period_;
    size_t slow_period_;
    size_t rsi_period_;
    size_t atr_period_;
    double rsi_overbought_;
    double rsi_oversold_;
    double atr_multiplier_;
    
    // Indicators
    std::unique_ptr<SMA> fast_sma_;
    std::unique_ptr<SMA> slow_sma_;
    std::unique_ptr<RSI> rsi_;
    std::unique_ptr<ATR> atr_;
    
    // Position tracking
    bool in_position_;
    double entry_price_;
    double stop_loss_;
    double take_profit_;
    
public:
    explicit SMACrossoverStrategy(size_t fast_period = 10,
                                 size_t slow_period = 30,
                                 size_t rsi_period = 14,
                                 size_t atr_period = 14,
                                 double rsi_overbought = 70.0,
                                 double rsi_oversold = 30.0,
                                 double atr_multiplier = 2.0)
        : StrategyBase("SMACrossover"),
          fast_period_(fast_period),
          slow_period_(slow_period),
          rsi_period_(rsi_period),
          atr_period_(atr_period),
          rsi_overbought_(rsi_overbought),
          rsi_oversold_(rsi_oversold),
          atr_multiplier_(atr_multiplier),
          in_position_(false),
          entry_price_(0.0),
          stop_loss_(0.0),
          take_profit_(0.0) {}
    
    void init() override {
        // Initialize indicators
        auto close_line = getDataLine("close");
        auto high_line = getDataLine("high");
        auto low_line = getDataLine("low");
        
        fast_sma_ = std::make_unique<SMA>(close_line, fast_period_);
        slow_sma_ = std::make_unique<SMA>(close_line, slow_period_);
        rsi_ = std::make_unique<RSI>(close_line, rsi_period_);
        atr_ = std::make_unique<ATR>(high_line, low_line, close_line, atr_period_);
        
        in_position_ = false;
    }
    
    void next() override {
        // Calculate indicators
        fast_sma_->calculate();
        slow_sma_->calculate();
        rsi_->calculate();
        atr_->calculate();
        
        // Get current values
        double fast_sma = fast_sma_->get(0);
        double slow_sma = slow_sma_->get(0);
        double fast_sma_prev = fast_sma_->get(-1);
        double slow_sma_prev = slow_sma_->get(-1);
        double rsi_value = rsi_->get(0);
        double atr_value = atr_->get(0);
        double close_price = getCurrentPrice("close");
        
        // Check if indicators are valid
        if (isNaN(fast_sma) || isNaN(slow_sma) || isNaN(rsi_value) || isNaN(atr_value)) {
            return;
        }
        
        // Position management
        if (in_position_) {
            managePosition(close_price);
        } else {
            checkEntry(fast_sma, slow_sma, fast_sma_prev, slow_sma_prev, rsi_value, atr_value, close_price);
        }
    }
    
private:
    void checkEntry(double fast_sma, double slow_sma, double fast_sma_prev, double slow_sma_prev,
                   double rsi_value, double atr_value, double close_price) {
        
        // Long entry: fast SMA crosses above slow SMA and RSI is not overbought
        if (fast_sma_prev <= slow_sma_prev && fast_sma > slow_sma && rsi_value < rsi_overbought_) {
            buy();
            in_position_ = true;
            entry_price_ = close_price;
            stop_loss_ = close_price - (atr_value * atr_multiplier_);
            take_profit_ = close_price + (atr_value * atr_multiplier_ * 2.0);
            
            logMessage("Long entry: Price=" + std::to_string(close_price) + 
                      ", RSI=" + std::to_string(rsi_value) +
                      ", Stop=" + std::to_string(stop_loss_) +
                      ", Target=" + std::to_string(take_profit_));
        }
        
        // Short entry: fast SMA crosses below slow SMA and RSI is not oversold
        else if (fast_sma_prev >= slow_sma_prev && fast_sma < slow_sma && rsi_value > rsi_oversold_) {
            sell();
            in_position_ = true;
            entry_price_ = close_price;
            stop_loss_ = close_price + (atr_value * atr_multiplier_);
            take_profit_ = close_price - (atr_value * atr_multiplier_ * 2.0);
            
            logMessage("Short entry: Price=" + std::to_string(close_price) + 
                      ", RSI=" + std::to_string(rsi_value) +
                      ", Stop=" + std::to_string(stop_loss_) +
                      ", Target=" + std::to_string(take_profit_));
        }
    }
    
    void managePosition(double close_price) {
        bool should_exit = false;
        std::string exit_reason;
        
        // Check stop loss and take profit
        if (getPosition() > 0) {  // Long position
            if (close_price <= stop_loss_) {
                should_exit = true;
                exit_reason = "Stop loss hit";
            } else if (close_price >= take_profit_) {
                should_exit = true;
                exit_reason = "Take profit hit";
            }
        } else if (getPosition() < 0) {  // Short position
            if (close_price >= stop_loss_) {
                should_exit = true;
                exit_reason = "Stop loss hit";
            } else if (close_price <= take_profit_) {
                should_exit = true;
                exit_reason = "Take profit hit";
            }
        }
        
        // Additional exit condition: RSI extreme values
        double rsi_value = rsi_->get(0);
        if (!isNaN(rsi_value)) {
            if (getPosition() > 0 && rsi_value >= rsi_overbought_) {
                should_exit = true;
                exit_reason = "RSI overbought";
            } else if (getPosition() < 0 && rsi_value <= rsi_oversold_) {
                should_exit = true;
                exit_reason = "RSI oversold";
            }
        }
        
        if (should_exit) {
            close();
            in_position_ = false;
            
            double pnl = (getPosition() > 0) ? (close_price - entry_price_) : (entry_price_ - close_price);
            logMessage("Position closed: " + exit_reason + 
                      ", Price=" + std::to_string(close_price) + 
                      ", PnL=" + std::to_string(pnl));
        }
    }
};

/**
 * @brief RSI momentum strategy
 * 
 * Trades based on RSI overbought/oversold conditions with
 * MACD confirmation and ATR-based position sizing.
 */
class RSIMomentumStrategy : public StrategyBase {
private:
    size_t rsi_period_;
    double rsi_overbought_;
    double rsi_oversold_;
    double macd_threshold_;
    
    std::unique_ptr<RSI> rsi_;
    std::unique_ptr<MACD> macd_;
    std::unique_ptr<ATR> atr_;
    
    bool in_position_;

public:
    explicit RSIMomentumStrategy(size_t rsi_period = 14,
                                double rsi_overbought = 70.0,
                                double rsi_oversold = 30.0,
                                double macd_threshold = 0.0)
        : StrategyBase("RSIMomentum"),
          rsi_period_(rsi_period),
          rsi_overbought_(rsi_overbought),
          rsi_oversold_(rsi_oversold),
          macd_threshold_(macd_threshold),
          in_position_(false) {}
    
    void init() override {
        auto close_line = getDataLine("close");
        auto high_line = getDataLine("high");
        auto low_line = getDataLine("low");
        
        rsi_ = std::make_unique<RSI>(close_line, rsi_period_);
        macd_ = std::make_unique<MACD>(close_line, 12, 26, 9);
        atr_ = std::make_unique<ATR>(high_line, low_line, close_line, 14);
        
        in_position_ = false;
    }
    
    void next() override {
        rsi_->calculate();
        macd_->calculate();
        atr_->calculate();
        
        double rsi_value = rsi_->get(0);
        double macd_line = macd_->getMACDLine(0);
        double close_price = getCurrentPrice("close");
        
        if (isNaN(rsi_value) || isNaN(macd_line)) {
            return;
        }
        
        if (!in_position_) {
            // Long entry: RSI oversold and MACD above threshold
            if (rsi_value <= rsi_oversold_ && macd_line > macd_threshold_) {
                buy();
                in_position_ = true;
                logMessage("Long entry: RSI=" + std::to_string(rsi_value) + 
                          ", MACD=" + std::to_string(macd_line));
            }
            // Short entry: RSI overbought and MACD below threshold  
            else if (rsi_value >= rsi_overbought_ && macd_line < macd_threshold_) {
                sell();
                in_position_ = true;
                logMessage("Short entry: RSI=" + std::to_string(rsi_value) + 
                          ", MACD=" + std::to_string(macd_line));
            }
        } else {
            // Exit conditions
            bool should_exit = false;
            
            if (getPosition() > 0 && rsi_value >= rsi_overbought_) {
                should_exit = true;
            } else if (getPosition() < 0 && rsi_value <= rsi_oversold_) {
                should_exit = true;
            }
            
            if (should_exit) {
                close();
                in_position_ = false;
                logMessage("Position closed: RSI=" + std::to_string(rsi_value));
            }
        }
    }
};

/**
 * @brief Mean reversion strategy using multiple indicators
 */
class MeanReversionStrategy : public StrategyBase {
private:
    std::unique_ptr<SMA> sma_;
    std::unique_ptr<RSI> rsi_;
    std::unique_ptr<ATR> atr_;
    
    size_t sma_period_;
    double std_dev_multiplier_;
    double mean_threshold_;
    bool in_position_;

public:
    explicit MeanReversionStrategy(size_t sma_period = 20,
                                  double std_dev_multiplier = 2.0,
                                  double mean_threshold = 0.02)
        : StrategyBase("MeanReversion"),
          sma_period_(sma_period),
          std_dev_multiplier_(std_dev_multiplier),
          mean_threshold_(mean_threshold),
          in_position_(false) {}
    
    void init() override {
        auto close_line = getDataLine("close");
        auto high_line = getDataLine("high");
        auto low_line = getDataLine("low");
        
        sma_ = std::make_unique<SMA>(close_line, sma_period_);
        rsi_ = std::make_unique<RSI>(close_line, 14);
        atr_ = std::make_unique<ATR>(high_line, low_line, close_line, 14);
        
        in_position_ = false;
    }
    
    void next() override {
        sma_->calculate();
        rsi_->calculate();
        atr_->calculate();
        
        double sma_value = sma_->get(0);
        double rsi_value = rsi_->get(0);
        double close_price = getCurrentPrice("close");
        
        if (isNaN(sma_value) || isNaN(rsi_value)) {
            return;
        }
        
        double deviation = (close_price - sma_value) / sma_value;
        
        if (!in_position_) {
            // Mean reversion entry conditions
            if (deviation > mean_threshold_ && rsi_value > 70.0) {
                sell();  // Price too high, expect reversion down
                in_position_ = true;
            } else if (deviation < -mean_threshold_ && rsi_value < 30.0) {
                buy();   // Price too low, expect reversion up
                in_position_ = true;
            }
        } else {
            // Exit when price returns to mean
            if (std::abs(deviation) < mean_threshold_ / 2.0) {
                close();
                in_position_ = false;
            }
        }
    }
};

} // namespace backtrader