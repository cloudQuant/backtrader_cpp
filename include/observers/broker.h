#pragma once

#include "../observer.h"
#include <vector>

namespace backtrader {

// Cash Observer - tracks current amount of cash in the broker
class Cash : public Observer {
public:
    // Lines
    enum Lines {
        CASH = 0
    };
    
    Cash();
    virtual ~Cash() = default;
    
    // Observer interface
    void next() override;
    
    // Get cash value
    double get_cash() const;
    
private:
    std::vector<double> cash_line_;
};

// Value Observer - tracks current portfolio value including cash
class Value : public Observer {
public:
    struct Params {
        bool fund = false;              // Use fund mode
        bool auto_fund = true;          // Auto-detect fund mode
    } params;
    
    // Lines
    enum Lines {
        VALUE = 0
    };
    
    Value();
    virtual ~Value() = default;
    
    // Observer interface
    void start() override;
    void next() override;
    
    // Get portfolio value
    double get_value() const;
    
private:
    std::vector<double> value_line_;
    bool fundmode_;
};

// Broker Observer - tracks both cash and portfolio value
class Broker : public Observer {
public:
    struct Params {
        bool fund = false;              // Use fund mode
        bool auto_fund = true;          // Auto-detect fund mode
    } params;
    
    // Lines
    enum Lines {
        CASH = 0,
        VALUE = 1
    };
    
    Broker();
    virtual ~Broker() = default;
    
    // Observer interface
    void start() override;
    void next() override;
    
    // Get values
    double get_cash() const;
    double get_value() const;
    
private:
    std::vector<double> cash_line_;
    std::vector<double> value_line_;
    bool fundmode_;
};

// Alias for Broker
using CashValue = Broker;

// FundValue Observer - tracks fund-like value
class FundValue : public Observer {
public:
    // Lines
    enum Lines {
        FUNDVAL = 0
    };
    
    FundValue();
    virtual ~FundValue() = default;
    
    // Observer interface
    void next() override;
    
    // Get fund value
    double get_fund_value() const;
    
private:
    std::vector<double> fundval_line_;
};

// Aliases for FundValue
using FundShareValue = FundValue;
using FundVal = FundValue;

// FundShares Observer - tracks fund-like shares
class FundShares : public Observer {
public:
    // Lines
    enum Lines {
        FUNDSHARES = 0
    };
    
    FundShares();
    virtual ~FundShares() = default;
    
    // Observer interface
    void next() override;
    
    // Get fund shares
    double get_fund_shares() const;
    
private:
    std::vector<double> fundshares_line_;
};

} // namespace backtrader