#pragma once

#include <string>
#include <memory>

namespace backtrader {

class Position {
public:
    // Position attributes
    double size = 0.0;           // Current position size
    double price = 0.0;          // Current average price
    double price_orig = 0.0;     // Original price when position was opened
    
    // Update tracking
    double upopened = 0.0;       // Size opened in last update
    double upclosed = 0.0;       // Size closed in last update
    
    // Adjustment base (for corporate actions)
    std::shared_ptr<void> adjbase = nullptr;
    
    // Update datetime (placeholder)
    std::string updt = "";
    
    Position() = default;
    Position(double size, double price = 0.0);
    virtual ~Position() = default;
    
    // Position operations
    void update(double size, double price);
    bool fix(double size, double price);
    void set(double size, double price);
    
    // Position queries
    bool islong() const { return size > 0.0; }
    bool isshort() const { return size < 0.0; }
    bool isclosed() const { return size == 0.0; }
    
    // Size operations
    double get_size() const { return size; }
    double get_price() const { return price; }
    
    // Clone operation
    std::shared_ptr<Position> clone() const;
    
    // String representation
    std::string to_string() const;
    
    // Length operator (for testing if position exists)
    operator bool() const { return size != 0.0; }
    
    // Addition operator for updating position
    Position& operator+=(const Position& other);
    
private:
    void _calculate_update_values(double new_size, double new_price);
};

// Position factory function
std::shared_ptr<Position> create_position(double size = 0.0, double price = 0.0);

} // namespace backtrader