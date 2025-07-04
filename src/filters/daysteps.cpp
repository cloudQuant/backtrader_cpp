#include "../../include/filters/daysteps.h"
#include <cmath>
#include <chrono>
#include <ctime>

namespace backtrader {
namespace filters {

DaySteps::DaySteps(const Params& params) : p(params) {
    if (p.days <= 0) {
        throw std::invalid_argument("Days step must be positive");
    }
}

void DaySteps::__call__(std::shared_ptr<DataSeries> data) {
    if (!data || data->lines.empty()) {
        return;
    }
    
    auto& datetime_line = data->lines[0];
    
    if (datetime_line.empty()) {
        return;
    }
    
    // Find indices to keep based on day steps
    std::vector<size_t> indices_to_keep;
    findIndicesToKeep(datetime_line, indices_to_keep);
    
    // Filter all data lines
    for (auto& line : data->lines) {
        std::vector<double> filtered_line;
        filtered_line.reserve(indices_to_keep.size());
        
        for (size_t idx : indices_to_keep) {
            if (idx < line.size()) {
                filtered_line.push_back(line[idx]);
            }
        }
        
        line = std::move(filtered_line);
    }
}

void DaySteps::findIndicesToKeep(const std::vector<double>& datetime_line, 
                                std::vector<size_t>& indices_to_keep) const {
    if (datetime_line.empty()) {
        return;
    }
    
    indices_to_keep.clear();
    
    // Always keep the first data point
    indices_to_keep.push_back(0);
    
    if (datetime_line.size() == 1) {
        return;
    }
    
    double last_kept_date = datetime_line[0];
    
    for (size_t i = 1; i < datetime_line.size(); ++i) {
        double current_date = datetime_line[i];
        
        if (shouldKeepDataPoint(last_kept_date, current_date)) {
            indices_to_keep.push_back(i);
            last_kept_date = current_date;
        }
    }
}

bool DaySteps::shouldKeepDataPoint(double last_kept_date, double current_date) const {
    // Convert timestamps to days since epoch
    int last_day = static_cast<int>(last_kept_date / 86400.0);  // 86400 seconds per day
    int current_day = static_cast<int>(current_date / 86400.0);
    
    // Check if enough days have passed
    return (current_day - last_day) >= p.days;
}

int DaySteps::getDayOfYear(double timestamp) const {
    auto time_t_val = static_cast<time_t>(timestamp);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_yday + 1;  // tm_yday is 0-based
}

int DaySteps::getYear(double timestamp) const {
    auto time_t_val = static_cast<time_t>(timestamp);
    auto tm = *std::localtime(&time_t_val);
    return tm.tm_year + 1900;
}

} // namespace filters
} // namespace backtrader