#include "../../include/filters/renko.h"
#include <cmath>

namespace backtrader {
namespace filters {

Renko::Renko(const Params& params) : p(params) {
    if (p.size <= 0.0) {
        throw std::invalid_argument("Renko box size must be positive");
    }
}

void Renko::__call__(std::shared_ptr<DataSeries> data) {
    if (!data || data->lines.empty() || data->lines.size() < 5) {
        return;
    }
    
    auto& datetime_line = data->lines[0];
    auto& open_line = data->lines[1];
    auto& high_line = data->lines[2];
    auto& low_line = data->lines[3];
    auto& close_line = data->lines[4];
    
    if (datetime_line.empty()) {
        return;
    }
    
    // Create new Renko data
    std::vector<double> renko_datetime, renko_open, renko_high, renko_low, renko_close;
    
    // Initialize with first bar
    double current_renko_open = open_line[0];
    double current_renko_close = close_line[0];
    double current_datetime = datetime_line[0];
    
    // Process each bar
    for (size_t i = 1; i < datetime_line.size(); ++i) {
        double high = high_line[i];
        double low = low_line[i];
        double close = close_line[i];
        double datetime = datetime_line[i];
        
        // Check if we need to create new Renko boxes
        std::vector<RenkoBox> new_boxes = generateRenkoBoxes(
            current_renko_close, high, low, close, datetime
        );
        
        // Add new boxes to the data
        for (const auto& box : new_boxes) {
            renko_datetime.push_back(box.datetime);
            renko_open.push_back(box.open);
            renko_high.push_back(box.high);
            renko_low.push_back(box.low);
            renko_close.push_back(box.close);
            
            current_renko_close = box.close;
        }
        
        // Update current datetime
        current_datetime = datetime;
    }
    
    // If we have new Renko data, replace the original data
    if (!renko_datetime.empty()) {
        datetime_line = renko_datetime;
        open_line = renko_open;
        high_line = renko_high;
        low_line = renko_low;
        close_line = renko_close;
        
        // Adjust volume if present
        if (data->lines.size() > 5) {
            // Set volume to zero for Renko bars (common practice)
            auto& volume_line = data->lines[5];
            volume_line.assign(renko_datetime.size(), 0.0);
        }
    }
}

std::vector<Renko::RenkoBox> Renko::generateRenkoBoxes(
    double last_close, double high, double low, double close, double datetime) const {
    
    std::vector<RenkoBox> boxes;
    double current_level = last_close;
    
    // Check for upward movement
    while (high >= current_level + p.size) {
        RenkoBox box;
        box.datetime = datetime;
        box.open = current_level;
        box.close = current_level + p.size;
        box.high = box.close;
        box.low = box.open;
        
        boxes.push_back(box);
        current_level = box.close;
    }
    
    // Check for downward movement
    while (low <= current_level - p.size) {
        RenkoBox box;
        box.datetime = datetime;
        box.open = current_level;
        box.close = current_level - p.size;
        box.high = box.open;
        box.low = box.close;
        
        boxes.push_back(box);
        current_level = box.close;
    }
    
    return boxes;
}

} // namespace filters
} // namespace backtrader