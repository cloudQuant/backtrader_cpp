#pragma once

#include <vector>
#include <numeric>
#include <cmath>

namespace backtrader {

// Calculate average (mean) of a sequence
// bessel: if true, use Bessel's correction (divide by n-1 instead of n)
double average(const std::vector<double>& x, bool bessel = false);

// Calculate variance of a sequence
// avgx: if provided, use this as the mean, otherwise calculate it
std::vector<double> variance(const std::vector<double>& x, double avgx = -1.0);

// Calculate standard deviation of a sequence
// avgx: if provided, use this as the mean, otherwise calculate it
// bessel: if true, use Bessel's correction for the average calculation
double standarddev(const std::vector<double>& x, double avgx = -1.0, bool bessel = false);

} // namespace backtrader