#include "../../include/filters/datafiller.h"
#include <stdexcept>

namespace backtrader {
namespace filters {

DataFiller::DataFiller(const Params& params) : p(params) {
    if (p.fillprice < 0 || p.fillprice > 6) {
        throw std::invalid_argument("fillprice must be between 0 and 6 (inclusive)");
    }
    if (p.fillvol < 0 || p.fillvol > 1) {
        throw std::invalid_argument("fillvol must be 0 or 1");
    }
}

void DataFiller::__call__(std::shared_ptr<DataSeries> data) {
    if (!data || data->lines.empty()) {
        return;
    }
    
    auto& lines = data->lines;
    size_t num_lines = lines.size();
    
    // Ensure we have at least OHLCV data
    if (num_lines < 6) {
        throw std::invalid_argument("Data must have at least 6 lines (datetime, open, high, low, close, volume)");
    }
    
    // Get references to the data lines
    auto& datetime_line = lines[0];  // datetime
    auto& open_line = lines[1];      // open
    auto& high_line = lines[2];      // high
    auto& low_line = lines[3];       // low
    auto& close_line = lines[4];     // close
    auto& volume_line = lines[5];    // volume
    
    // Find gaps in the data and fill them
    for (size_t i = 1; i < datetime_line.size(); ++i) {
        double current_time = datetime_line[i];
        double prev_time = datetime_line[i-1];
        
        // Check if there's a gap based on the timeframe
        if (isGap(prev_time, current_time)) {
            // Fill the gap
            fillGap(data, i-1, i);
        }
    }
    
    // Fill missing price values
    fillMissingPrices(data);
    
    // Fill missing volume values
    if (p.fillvol == 1) {
        fillMissingVolume(data);
    }
}

bool DataFiller::isGap(double prev_time, double current_time) const {
    // Convert to time difference based on session timeframe
    double expected_interval = getExpectedInterval();
    double actual_interval = current_time - prev_time;
    
    // Consider it a gap if the interval is significantly larger than expected
    return actual_interval > expected_interval * 1.5;
}

void DataFiller::fillGap(std::shared_ptr<DataSeries> data, size_t prev_idx, size_t curr_idx) {
    auto& lines = data->lines;
    
    double prev_time = lines[0][prev_idx];
    double curr_time = lines[0][curr_idx];
    double interval = getExpectedInterval();
    
    // Calculate number of missing bars
    int missing_bars = static_cast<int>((curr_time - prev_time) / interval) - 1;
    
    if (missing_bars <= 0) {
        return;
    }
    
    // Get the fill price based on the previous bar
    double fill_price = getFillPrice(data, prev_idx);
    double fill_volume = getFillVolume(data, prev_idx);
    
    // Insert missing bars
    for (int i = 1; i <= missing_bars; ++i) {
        double new_time = prev_time + (interval * i);
        
        // Insert into each line
        for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
            if (line_idx == 0) {
                // DateTime line
                lines[line_idx].insert(lines[line_idx].begin() + curr_idx, new_time);
            } else if (line_idx >= 1 && line_idx <= 4) {
                // OHLC lines
                lines[line_idx].insert(lines[line_idx].begin() + curr_idx, fill_price);
            } else if (line_idx == 5) {
                // Volume line
                lines[line_idx].insert(lines[line_idx].begin() + curr_idx, fill_volume);
            } else {
                // Other lines - use previous value
                double prev_value = lines[line_idx][prev_idx];
                lines[line_idx].insert(lines[line_idx].begin() + curr_idx, prev_value);
            }
        }
        
        // Update current index since we inserted a new bar
        curr_idx++;
    }
}

void DataFiller::fillMissingPrices(std::shared_ptr<DataSeries> data) {
    auto& lines = data->lines;
    
    for (size_t i = 0; i < lines[1].size(); ++i) {
        // Check if any OHLC value is missing (NaN or 0)
        bool has_missing = false;
        for (size_t j = 1; j <= 4; ++j) {
            if (std::isnan(lines[j][i]) || lines[j][i] == 0.0) {
                has_missing = true;
                break;
            }
        }
        
        if (has_missing) {
            double fill_price = getFillPrice(data, i > 0 ? i-1 : 0);
            
            // Fill missing OHLC values
            for (size_t j = 1; j <= 4; ++j) {
                if (std::isnan(lines[j][i]) || lines[j][i] == 0.0) {
                    lines[j][i] = fill_price;
                }
            }
        }
    }
}

void DataFiller::fillMissingVolume(std::shared_ptr<DataSeries> data) {
    auto& lines = data->lines;
    
    if (lines.size() < 6) {
        return;
    }
    
    auto& volume_line = lines[5];
    
    for (size_t i = 0; i < volume_line.size(); ++i) {
        if (std::isnan(volume_line[i]) || volume_line[i] < 0) {
            volume_line[i] = getFillVolume(data, i > 0 ? i-1 : 0);
        }
    }
}

double DataFiller::getFillPrice(std::shared_ptr<DataSeries> data, size_t idx) const {
    auto& lines = data->lines;
    
    if (idx >= lines[1].size()) {
        idx = lines[1].size() - 1;
    }
    
    switch (p.fillprice) {
        case 0:  // Use previous close
            return lines[4][idx];  // close
        case 1:  // Use previous open
            return lines[1][idx];  // open
        case 2:  // Use previous high
            return lines[2][idx];  // high
        case 3:  // Use previous low
            return lines[3][idx];  // low
        case 4:  // Use previous close (same as 0)
            return lines[4][idx];  // close
        case 5:  // Use average of OHLC
            return (lines[1][idx] + lines[2][idx] + lines[3][idx] + lines[4][idx]) / 4.0;
        case 6:  // Use median of OHLC
            {
                std::vector<double> ohlc = {lines[1][idx], lines[2][idx], lines[3][idx], lines[4][idx]};
                std::sort(ohlc.begin(), ohlc.end());
                return (ohlc[1] + ohlc[2]) / 2.0;
            }
        default:
            return lines[4][idx];  // default to close
    }
}

double DataFiller::getFillVolume(std::shared_ptr<DataSeries> data, size_t idx) const {
    auto& lines = data->lines;
    
    if (lines.size() < 6 || idx >= lines[5].size()) {
        return 0.0;
    }
    
    switch (p.fillvol) {
        case 0:  // Use zero volume
            return 0.0;
        case 1:  // Use previous volume
            return lines[5][idx];  // volume
        default:
            return 0.0;
    }
}

double DataFiller::getExpectedInterval() const {
    // This is a simplified implementation
    // In reality, this would need to be calculated based on the actual timeframe
    // For now, assume 1 minute intervals
    return 60.0;  // 60 seconds
}

} // namespace filters
} // namespace backtrader