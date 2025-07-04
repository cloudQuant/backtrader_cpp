#include "observers/broker.h"

namespace backtrader {

// Cash Observer implementation
Cash::Cash() : Observer() {
    // Initialize cash line
    cash_line_.clear();
}

void Cash::next() {
    if (!strategy_) {
        return;
    }
    
    // Get current cash amount from broker
    double cash = strategy_->broker->get_cash();
    cash_line_.push_back(cash);
}

double Cash::get_cash() const {
    if (cash_line_.empty()) {
        return 0.0;
    }
    return cash_line_.back();
}

// Value Observer implementation
Value::Value() : Observer(), fundmode_(false) {
    // Initialize value line
    value_line_.clear();
}

void Value::start() {
    Observer::start();
    
    // Auto-detect fund mode if needed
    if (params.auto_fund && strategy_) {
        fundmode_ = strategy_->broker->get_fundmode();
    } else {
        fundmode_ = params.fund;
    }
    
    value_line_.clear();
}

void Value::next() {
    if (!strategy_) {
        return;
    }
    
    double value;
    if (!fundmode_) {
        value = strategy_->broker->get_value();
    } else {
        value = strategy_->broker->get_fundvalue();
    }
    
    value_line_.push_back(value);
}

double Value::get_value() const {
    if (value_line_.empty()) {
        return 0.0;
    }
    return value_line_.back();
}

// Broker Observer implementation
Broker::Broker() : Observer(), fundmode_(false) {
    // Initialize lines
    cash_line_.clear();
    value_line_.clear();
}

void Broker::start() {
    Observer::start();
    
    // Auto-detect fund mode if needed
    if (params.auto_fund && strategy_) {
        fundmode_ = strategy_->broker->get_fundmode();
    } else {
        fundmode_ = params.fund;
    }
    
    cash_line_.clear();
    value_line_.clear();
}

void Broker::next() {
    if (!strategy_) {
        return;
    }
    
    if (!fundmode_) {
        // Normal mode - track both cash and total value
        double value = strategy_->broker->get_value();
        double cash = strategy_->broker->get_cash();
        
        value_line_.push_back(value);
        cash_line_.push_back(cash);
    } else {
        // Fund mode - only track fund value
        double fund_value = strategy_->broker->get_fundvalue();
        value_line_.push_back(fund_value);
        
        // Cash line not used in fund mode, but maintain same size
        cash_line_.push_back(0.0);
    }
}

double Broker::get_cash() const {
    if (cash_line_.empty()) {
        return 0.0;
    }
    return cash_line_.back();
}

double Broker::get_value() const {
    if (value_line_.empty()) {
        return 0.0;
    }
    return value_line_.back();
}

// FundValue Observer implementation
FundValue::FundValue() : Observer() {
    // Initialize fund value line
    fundval_line_.clear();
}

void FundValue::next() {
    if (!strategy_) {
        return;
    }
    
    // Get current fund value from broker
    double fund_value = strategy_->broker->get_fundvalue();
    fundval_line_.push_back(fund_value);
}

double FundValue::get_fund_value() const {
    if (fundval_line_.empty()) {
        return 0.0;
    }
    return fundval_line_.back();
}

// FundShares Observer implementation
FundShares::FundShares() : Observer() {
    // Initialize fund shares line
    fundshares_line_.clear();
}

void FundShares::next() {
    if (!strategy_) {
        return;
    }
    
    // Get current fund shares from broker
    double fund_shares = strategy_->broker->get_fundshares();
    fundshares_line_.push_back(fund_shares);
}

double FundShares::get_fund_shares() const {
    if (fundshares_line_.empty()) {
        return 0.0;
    }
    return fundshares_line_.back();
}

} // namespace backtrader