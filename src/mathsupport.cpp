#include "mathsupport.h"
#include <stdexcept>

namespace backtrader {

double average(const std::vector<double>& x, bool bessel) {
    if (x.empty()) {
        throw std::invalid_argument("Cannot calculate average of empty sequence");
    }
    
    double sum = std::accumulate(x.begin(), x.end(), 0.0);
    size_t denominator = x.size() - (bessel ? 1 : 0);
    
    if (denominator == 0) {
        throw std::invalid_argument("Cannot calculate average: denominator is zero");
    }
    
    return sum / denominator;
}

std::vector<double> variance(const std::vector<double>& x, double avgx) {
    if (x.empty()) {
        return {};
    }
    
    double mean = (avgx == -1.0) ? average(x) : avgx;
    
    std::vector<double> result;
    result.reserve(x.size());
    
    for (double value : x) {
        double diff = value - mean;
        result.push_back(diff * diff);
    }
    
    return result;
}

double standarddev(const std::vector<double>& x, double avgx, bool bessel) {
    if (x.empty()) {
        throw std::invalid_argument("Cannot calculate standard deviation of empty sequence");
    }
    
    double mean = (avgx == -1.0) ? average(x) : avgx;
    std::vector<double> var = variance(x, mean);
    
    double avg_variance = average(var, bessel);
    return std::sqrt(avg_variance);
}

} // namespace backtrader