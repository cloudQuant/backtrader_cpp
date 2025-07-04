#pragma once

#include <memory>

namespace backtrader {

// Commission information class
class CommInfo {
public:
    // Commission parameters
    double commission = 0.0;      // Commission rate
    double margin = 0.0;          // Margin requirement
    double mult = 1.0;            // Contract multiplier
    bool percabs = false;         // Percentage (True) or absolute (False)
    bool stocklike = true;        // Stock-like behavior
    bool commtype = false;        // Commission type (False = percentage, True = absolute)
    
    // Interest parameters
    double interest = 0.0;        // Interest rate for margin
    double interest_long = 0.0;   // Interest rate for long positions
    double interest_short = 0.0;  // Interest rate for short positions
    
    // Leverage parameters
    double leverage = 1.0;        // Maximum leverage allowed
    
    CommInfo() = default;
    CommInfo(double commission, 
             double margin = 0.0, 
             double mult = 1.0, 
             bool percabs = false,
             bool stocklike = true);
    virtual ~CommInfo() = default;
    
    // Commission calculation
    virtual double getcommission(double size, double price) const;
    virtual double getcommissioninfo(double size, double price) const;
    
    // Margin calculation
    virtual double getmargin(double price) const;
    virtual double getoperationcost(double size, double price) const;
    
    // Size operations
    virtual double getsize(double price, double cash) const;
    virtual double getvalue(double size, double price) const;
    virtual double getvaluesize(double size, double price) const;
    
    // Profit and loss calculation
    virtual double profitandloss(double size, double price, double newprice) const;
    virtual double cashadjust(double size, double price, double newprice) const;
    
    // Position value
    virtual double get_credit_interest(double data, double pos, double dt) const;
    
    // Clone
    virtual std::shared_ptr<CommInfo> clone() const;
    
    // String representation
    virtual std::string to_string() const;
    
protected:
    // Internal calculation helpers
    double _getcommission(double size, double price, bool pseudoexec = false) const;
    double _profitandloss(double size, double price, double newprice) const;
};

// Stock commission info
class CommInfoStock : public CommInfo {
public:
    CommInfoStock(double commission = 0.0, 
                  bool percabs = false);
    virtual ~CommInfoStock() = default;
    
    double getcommission(double size, double price) const override;
};

// Futures commission info
class CommInfoFutures : public CommInfo {
public:
    CommInfoFutures(double commission = 0.0, 
                    double margin = 0.0, 
                    double mult = 1.0);
    virtual ~CommInfoFutures() = default;
    
    double getcommission(double size, double price) const override;
    double getmargin(double price) const override;
    double profitandloss(double size, double price, double newprice) const override;
};

// Forex commission info
class CommInfoForex : public CommInfo {
public:
    CommInfoForex(double commission = 0.0, 
                  double margin = 0.0, 
                  double mult = 1.0,
                  double leverage = 1.0);
    virtual ~CommInfoForex() = default;
    
    double getcommission(double size, double price) const override;
    double getmargin(double price) const override;
};

// Factory functions
std::shared_ptr<CommInfo> create_comminfo(double commission = 0.0,
                                          double margin = 0.0,
                                          double mult = 1.0,
                                          bool percabs = false,
                                          bool stocklike = true);

std::shared_ptr<CommInfoStock> create_stock_comminfo(double commission = 0.0,
                                                     bool percabs = false);

std::shared_ptr<CommInfoFutures> create_futures_comminfo(double commission = 0.0,
                                                         double margin = 0.0,
                                                         double mult = 1.0);

std::shared_ptr<CommInfoForex> create_forex_comminfo(double commission = 0.0,
                                                     double margin = 0.0,
                                                     double mult = 1.0,
                                                     double leverage = 1.0);

} // namespace backtrader