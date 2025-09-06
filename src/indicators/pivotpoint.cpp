#include "indicators/pivotpoint.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

// PivotPoint implementation
PivotPoint::PivotPoint() : Indicator() {
    setup_lines();
    _minperiod(1);
}

void PivotPoint::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void PivotPoint::prenext() {
    Indicator::prenext();
}

void PivotPoint::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Get OHLC data
    double open = (*data_lines->getline(0))[0];   // Open
    double high = (*data_lines->getline(1))[0];   // High
    double low = (*data_lines->getline(2))[0];    // Low
    double close = (*data_lines->getline(4))[0];  // Close
    
    // Calculate pivot point
    double pivot;
    if (params.close) {
        pivot = (high + low + 2.0 * close) / 4.0;
    } else if (params.open) {
        pivot = (high + low + close + open) / 4.0;
    } else {
        pivot = (high + low + close) / 3.0;
    }
    
    // Calculate support and resistance levels
    double s1 = 2.0 * pivot - high;
    double s2 = pivot - (high - low);
    double r1 = 2.0 * pivot - low;
    double r2 = pivot + (high - low);
    
    // Set line values
    (*lines->getline(p))[0] = pivot;
    (*lines->getline(s1))[0] = s1;
    (*lines->getline(s2))[0] = s2;
    (*lines->getline(r1))[0] = r1;
    (*lines->getline(r2))[0] = r2;
}

void PivotPoint::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    for (int i = start; i < end; ++i) {
        // Get OHLC data
        double open = (*data_lines->getline(0))[i];   // Open
        double high = (*data_lines->getline(1))[i];   // High
        double low = (*data_lines->getline(2))[i];    // Low
        double close = (*data_lines->getline(4))[i];  // Close
        
        // Calculate pivot point
        double pivot;
        if (params.close) {
            pivot = (high + low + 2.0 * close) / 4.0;
        } else if (params.open) {
            pivot = (high + low + close + open) / 4.0;
        } else {
            pivot = (high + low + close) / 3.0;
        }
        
        // Calculate support and resistance levels
        double s1 = 2.0 * pivot - high;
        double s2 = pivot - (high - low);
        double r1 = 2.0 * pivot - low;
        double r2 = pivot + (high - low);
        
        // Set line values
        (*lines->getline(p))[i] = pivot;
        (*lines->getline(s1))[i] = s1;
        (*lines->getline(s2))[i] = s2;
        (*lines->getline(r1))[i] = r1;
        (*lines->getline(r2))[i] = r2;
    }
}

// FibonacciPivotPoint implementation
FibonacciPivotPoint::FibonacciPivotPoint() : Indicator() {
    setup_lines();
    _minperiod(1);
}

void FibonacciPivotPoint::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void FibonacciPivotPoint::prenext() {
    Indicator::prenext();
}

void FibonacciPivotPoint::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Get OHLC data
    double open = (*data_lines->getline(0))[0];   // Open
    double high = (*data_lines->getline(1))[0];   // High
    double low = (*data_lines->getline(2))[0];    // Low
    double close = (*data_lines->getline(4))[0];  // Close
    
    // Calculate pivot point
    double pivot;
    if (params.close) {
        pivot = (high + low + 2.0 * close) / 4.0;
    } else if (params.open) {
        pivot = (high + low + close + open) / 4.0;
    } else {
        pivot = (high + low + close) / 3.0;
    }
    
    // Calculate range
    double range = high - low;
    
    // Calculate Fibonacci levels
    double s1 = pivot - params.level1 * range;
    double s2 = pivot - params.level2 * range;
    double s3 = pivot - params.level3 * range;
    double r1 = pivot + params.level1 * range;
    double r2 = pivot + params.level2 * range;
    double r3 = pivot + params.level3 * range;
    
    // Set line values
    (*lines->getline(p))[0] = pivot;
    (*lines->getline(s1))[0] = s1;
    (*lines->getline(s2))[0] = s2;
    (*lines->getline(s3))[0] = s3;
    (*lines->getline(r1))[0] = r1;
    (*lines->getline(r2))[0] = r2;
    (*lines->getline(r3))[0] = r3;
}

void FibonacciPivotPoint::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    for (int i = start; i < end; ++i) {
        // Get OHLC data
        double open = (*data_lines->getline(0))[i];   // Open
        double high = (*data_lines->getline(1))[i];   // High
        double low = (*data_lines->getline(2))[i];    // Low
        double close = (*data_lines->getline(4))[i];  // Close
        
        // Calculate pivot point
        double pivot;
        if (params.close) {
            pivot = (high + low + 2.0 * close) / 4.0;
        } else if (params.open) {
            pivot = (high + low + close + open) / 4.0;
        } else {
            pivot = (high + low + close) / 3.0;
        }
        
        // Calculate range
        double range = high - low;
        
        // Calculate Fibonacci levels
        double s1 = pivot - params.level1 * range;
        double s2 = pivot - params.level2 * range;
        double s3 = pivot - params.level3 * range;
        double r1 = pivot + params.level1 * range;
        double r2 = pivot + params.level2 * range;
        double r3 = pivot + params.level3 * range;
        
        // Set line values
        (*lines->getline(p))[i] = pivot;
        (*lines->getline(s1))[i] = s1;
        (*lines->getline(s2))[i] = s2;
        (*lines->getline(s3))[i] = s3;
        (*lines->getline(r1))[i] = r1;
        (*lines->getline(r2))[i] = r2;
        (*lines->getline(r3))[i] = r3;
    }
}

// DemarkPivotPoint implementation
DemarkPivotPoint::DemarkPivotPoint() : Indicator() {
    setup_lines();
    _minperiod(1);
}

void DemarkPivotPoint::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void DemarkPivotPoint::prenext() {
    Indicator::prenext();
}

void DemarkPivotPoint::next() {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    // Get OHLC data
    double open = (*data_lines->getline(0))[0];   // Open
    double high = (*data_lines->getline(1))[0];   // High
    double low = (*data_lines->getline(2))[0];    // Low
    double close = (*data_lines->getline(4))[0];  // Close
    
    // Calculate X value using Demark method
    double x = calculate_x_value(open, high, low, close);
    
    // Calculate pivot point and levels
    double pivot = x / 4.0;
    double s1 = x / 2.0 - high;
    double r1 = x / 2.0 - low;
    
    // Set line values
    (*lines->getline(p))[0] = pivot;
    (*lines->getline(s1))[0] = s1;
    (*lines->getline(r1))[0] = r1;
}

void DemarkPivotPoint::once(int start, int end) {
    if (datas.empty() || !datas[0]->lines) return;
    
    auto data_lines = datas[0]->lines;
    
    for (int i = start; i < end; ++i) {
        // Get OHLC data
        double open = (*data_lines->getline(0))[i];   // Open
        double high = (*data_lines->getline(1))[i];   // High
        double low = (*data_lines->getline(2))[i];    // Low
        double close = (*data_lines->getline(4))[i];  // Close
        
        // Calculate X value using Demark method
        double x = calculate_x_value(open, high, low, close);
        
        // Calculate pivot point and levels
        double pivot = x / 4.0;
        double s1 = x / 2.0 - high;
        double r1 = x / 2.0 - low;
        
        // Set line values
        (*lines->getline(p))[i] = pivot;
        (*lines->getline(s1))[i] = s1;
        (*lines->getline(r1))[i] = r1;
    }
}

double DemarkPivotPoint::calculate_x_value(double open, double high, double low, double close) {
    if (close < open) {
        // x = high + (2 * low) + close
        return high + (2.0 * low) + close;
    } else if (close > open) {
        // x = (2 * high) + low + close
        return (2.0 * high) + low + close;
    } else {
        // x = high + low + (2 * close)
        return high + low + (2.0 * close);
    }
}

} // namespace backtrader