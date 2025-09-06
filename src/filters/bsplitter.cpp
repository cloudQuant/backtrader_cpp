#include "../../include/filters/bsplitter.h"
#include <cmath>

namespace backtrader {
namespace filters {

BSplitter::BSplitter(const Params& params) : p(params) {
    if (p.splits.empty()) {
        throw std::invalid_argument("Split ratios cannot be empty");
    }
    
    for (const auto& split : p.splits) {
        if (split.ratio <= 0.0) {
            throw std::invalid_argument("Split ratio must be positive");
        }
    }
}

void BSplitter::__call__(std::shared_ptr<DataSeries> data) {
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
    
    // Apply splits to the data
    for (size_t i = 0; i < datetime_line.size(); ++i) {
        double datetime = datetime_line[i];
        
        // Find applicable split
        SplitInfo applicable_split = findApplicableSplit(datetime);
        
        if (applicable_split.ratio != 1.0) {
            // Apply split adjustment
            open_line[i] = adjustPriceForSplit(open_line[i], applicable_split.ratio);
            high_line[i] = adjustPriceForSplit(high_line[i], applicable_split.ratio);
            low_line[i] = adjustPriceForSplit(low_line[i], applicable_split.ratio);
            close_line[i] = adjustPriceForSplit(close_line[i], applicable_split.ratio);
            
            // Adjust volume if present
            if (data->lines.size() > 5) {
                auto& volume_line = data->lines[5];
                volume_line[i] = adjustVolumeForSplit(volume_line[i], applicable_split.ratio);
            }
        }
    }
}

BSplitter::SplitInfo BSplitter::findApplicableSplit(double datetime) const {
    SplitInfo applicable_split;
    applicable_split.datetime = 0.0;
    applicable_split.ratio = 1.0;
    
    // Find the most recent split that occurred before or at this datetime
    for (const auto& split : p.splits) {
        if (split.datetime <= datetime && split.datetime > applicable_split.datetime) {
            applicable_split = split;
        }
    }
    
    return applicable_split;
}

double BSplitter::adjustPriceForSplit(double price, double split_ratio) const {
    if (split_ratio == 1.0) {
        return price;
    }
    
    // For a 2:1 split (split_ratio = 2.0), prices are halved
    // For a 1:2 reverse split (split_ratio = 0.5), prices are doubled
    return price / split_ratio;
}

double BSplitter::adjustVolumeForSplit(double volume, double split_ratio) const {
    if (split_ratio == 1.0) {
        return volume;
    }
    
    // For a 2:1 split (split_ratio = 2.0), volume is doubled
    // For a 1:2 reverse split (split_ratio = 0.5), volume is halved
    return volume * split_ratio;
}

} // namespace filters
} // namespace backtrader