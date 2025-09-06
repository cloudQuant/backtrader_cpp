#include "position.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

namespace backtrader {

Position::Position(double size_val, double price_val) {
    size = size_val;
    if (size != 0.0) {
        price = price_orig = price_val;
    } else {
        price = 0.0;
        price_orig = 0.0;
    }
    
    adjbase = nullptr;
    upopened = size;
    upclosed = 0.0;
    set(size, price_val);
    updt = "";
}

void Position::update(double new_size, double new_price) {
    std::cerr << "Position::update called - current size=" << size 
              << ", new_size=" << new_size << ", new_price=" << new_price << std::endl;
    
    // Calculate new average price if adding to position
    if (size != 0.0 && new_size != 0.0) {
        // Same direction - calculate weighted average
        if ((size > 0.0 && new_size > 0.0) || (size < 0.0 && new_size < 0.0)) {
            double total_value = size * price + new_size * new_price;
            double total_size = size + new_size;
            if (total_size != 0.0) {
                price = total_value / total_size;
            }
            size = total_size;
            std::cerr << "Position::update - Same direction, new size=" << size << std::endl;
        } else {
            // Opposite direction - closing or reversing position
            double remaining_size = size + new_size;
            std::cerr << "Position::update - Opposite direction, remaining_size=" << remaining_size << std::endl;
            if (remaining_size == 0.0) {
                // Complete close
                size = 0.0;
                price = 0.0;
            } else if ((size > 0.0 && remaining_size < 0.0) || (size < 0.0 && remaining_size > 0.0)) {
                // Position reversal
                size = remaining_size;
                price = new_price;
            } else {
                // Partial close, keep original price
                size = remaining_size;
            }
        }
    } else if (size == 0.0 && new_size != 0.0) {
        // Opening new position
        size = new_size;
        price = price_orig = new_price;
        std::cerr << "Position::update - Opening new position, size=" << size << std::endl;
    }
    
    set(size, price);
}

bool Position::fix(double new_size, double new_price) {
    double old_size = size;
    size = new_size;
    price = new_price;
    return size == old_size;
}

void Position::set(double new_size, double new_price) {
    _calculate_update_values(new_size, new_price);
}

std::shared_ptr<Position> Position::clone() const {
    auto cloned = std::make_shared<Position>();
    cloned->size = size;
    cloned->price = price;
    cloned->price_orig = price_orig;
    cloned->upopened = upopened;
    cloned->upclosed = upclosed;
    cloned->adjbase = adjbase;
    cloned->updt = updt;
    return cloned;
}

std::string Position::to_string() const {
    std::ostringstream oss;
    oss << "--- Position Begin\n";
    oss << "- Size: " << std::fixed << std::setprecision(2) << size << "\n";
    oss << "- Price: " << std::fixed << std::setprecision(4) << price << "\n";
    oss << "- Price orig: " << std::fixed << std::setprecision(4) << price_orig << "\n";
    oss << "- Closed: " << std::fixed << std::setprecision(2) << upclosed << "\n";
    oss << "- Opened: " << std::fixed << std::setprecision(2) << upopened << "\n";
    oss << "- Adjbase: " << (adjbase ? "Set" : "None") << "\n";
    oss << "--- Position End";
    return oss.str();
}

Position& Position::operator+=(const Position& other) {
    update(other.size, other.price);
    return *this;
}

void Position::_calculate_update_values(double new_size, double new_price) {
    // Calculate what was opened and closed in this update
    if (size > 0.0) {
        if (new_size > size) {
            // Increasing long position
            upopened = new_size - size;
            upclosed = 0.0;
        } else {
            // Decreasing long position or reversing
            upopened = std::min(0.0, new_size);
            upclosed = std::min(size, size - new_size);
        }
    } else if (size < 0.0) {
        if (new_size < size) {
            // Increasing short position
            upopened = new_size - size;
            upclosed = 0.0;
        } else {
            // Decreasing short position or reversing
            upopened = std::max(0.0, new_size);
            upclosed = std::min(-size, new_size - size);
        }
    } else {
        // Position was flat
        if (new_size != 0.0) {
            upopened = new_size;
            upclosed = 0.0;
        } else {
            upopened = 0.0;
            upclosed = 0.0;
        }
    }
}

std::shared_ptr<Position> create_position(double size, double price) {
    return std::make_shared<Position>(size, price);
}

} // namespace backtrader