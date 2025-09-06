#include "indicators/crossover.h"
#include "dataseries.h"
#include "indicator.h"
#include <limits>
#include <iostream>

namespace backtrader {

// Helper function to get the default value from a LineSeries
// For DataSeries, returns close price; for indicators, returns default line
// When ago is an absolute index (in once mode), access array directly
static double getDefaultValue(std::shared_ptr<LineSeries> series, int ago = 0) {
    static int call_count = 0;
    call_count++;
    
    // Check if it's a DataSeries
    if (auto data_series = std::dynamic_pointer_cast<DataSeries>(series)) {
        // In once mode, we need to access close line directly at absolute index
        if (data_series->lines && data_series->lines->size() > 3) {
            auto close_line = data_series->lines->getline(3);  // Close line (correct index)
            if (auto buffer = std::dynamic_pointer_cast<LineBuffer>(close_line)) {
                const auto& array = buffer->array();
                if (ago >= 0 && ago < static_cast<int>(array.size())) {
                    return array[ago];
                }
            }
        }
        return data_series->close(ago);
    }
    // Check if it's an Indicator
    else if (auto indicator = std::dynamic_pointer_cast<Indicator>(series)) {
        // In once() mode (called from NonZeroDifference::once), ago is an absolute index
        // Access the indicator's buffer directly at that index
        if (series->lines && series->lines->size() > 0) {
            auto line = series->lines->getline(0);
            if (auto buffer = std::dynamic_pointer_cast<LineBuffer>(line)) {
                const auto& array = buffer->array();
                if (ago >= 0 && ago < static_cast<int>(array.size())) {
                    double val = array[ago];
                    if (call_count <= 20) {
                        std::cerr << "getDefaultValue #" << call_count << " - Indicator buffer[" << ago << "] = " << val 
                                  << ", indicator ptr=" << indicator.get() << std::endl;
                    }
                    return val;
                }
            }
        }
        // Don't fallback to get() method - that expects relative indexing
        // If buffer access failed, return NaN
        if (call_count <= 20) {
            std::cerr << "getDefaultValue #" << call_count << " - Indicator buffer access failed for index " << ago 
                      << ", indicator ptr=" << indicator.get() << std::endl;
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
    // Otherwise it's a generic LineSeries
    else if (series->lines && series->lines->size() > 0) {
        auto line = series->lines->getline(0);
        if (line) {
            // In once() mode, ago is actually an absolute index
            // Check if we have a LineBuffer and use direct array access
            if (auto buffer = std::dynamic_pointer_cast<LineBuffer>(line)) {
                const auto& array = buffer->array();
                if (ago >= 0 && ago < static_cast<int>(array.size())) {
                    return array[ago];
                }
            }
            // Fallback to operator[]
            return (*line)[ago];
        }
    }
    return 0.0;
}

// NonZeroDifference implementation
NonZeroDifference::NonZeroDifference() : Indicator(), last_nzd_(0.0) {
    setup_lines();
    _minperiod(2); // Base minimum period - will be updated when data sources are added
    _ltype = LineRoot::IndType::IndType;  // Ensure NonZeroDifference is marked as an indicator
}

void NonZeroDifference::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void NonZeroDifference::add_data(std::shared_ptr<LineSeries> data) {
    if (!data0_) {
        data0_ = data;
    } else if (!data1_) {
        data1_ = data;
        
        // CRITICAL: Update minimum period based on data sources
        // NonZeroDifference needs both data sources to be ready
        int data0_minperiod = 1;
        int data1_minperiod = 1;
        
        // Get minimum period from data0 (usually DataSeries)
        if (auto ind0 = std::dynamic_pointer_cast<Indicator>(data0_)) {
            data0_minperiod = ind0->getMinPeriod();
        }
        
        // Get minimum period from data1 (usually SMA indicator)
        if (auto ind1 = std::dynamic_pointer_cast<Indicator>(data1_)) {
            data1_minperiod = ind1->getMinPeriod();
        }
        
        // Set our minimum period to be the maximum of both sources
        int required_minperiod = std::max(data0_minperiod, data1_minperiod);
        _minperiod(required_minperiod);
    }
    // Don't add to datas vector since type mismatch (LineActions vs LineSeries)
}

void NonZeroDifference::prenext() {
    // During warm-up, we need to check if we can calculate a difference
    if (!data0_ || !data1_) return;
    
    auto nzd_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(nzd));
    if (nzd_line) {
        // Try to get values from both data sources
        double val0 = getDefaultValue(data0_);
        double val1 = std::numeric_limits<double>::quiet_NaN();
        
        // Special handling for indicator data sources
        if (auto ind = std::dynamic_pointer_cast<Indicator>(data1_)) {
            // Check if the indicator has produced a value for the current bar
            if (ind->size() > 0) {
                val1 = getDefaultValue(data1_);
            }
        } else {
            val1 = getDefaultValue(data1_);
        }
        
        // If both values are valid, calculate and store the difference
        if (!std::isnan(val0) && !std::isnan(val1)) {
            double diff = val0 - val1;
            if (diff != 0.0) {
                last_nzd_ = diff;
            }
            nzd_line->append(diff != 0.0 ? diff : last_nzd_);
        } else {
            // During warm-up, append NaN if we can't calculate
            nzd_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

void NonZeroDifference::nextstart() {
    if (!data0_ || !data1_) return;
    
    auto nzd_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(nzd));
    if (nzd_line) {
        double diff = getDefaultValue(data0_) - getDefaultValue(data1_);
        nzd_line->append(diff);
        // Always update last_nzd_ in nextstart (seed value)
        last_nzd_ = diff;
    }
}

void NonZeroDifference::next() {
    if (!data0_ || !data1_) return;
    
    auto nzd_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(nzd));
    if (nzd_line) {
        // In streaming mode, get values using appropriate methods
        double val0 = 0.0;
        double val1 = 0.0;
        
        // Get value from data0 (could be DataSeries or Indicator)
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            val0 = data_series->close(0);
        } else if (auto indicator = std::dynamic_pointer_cast<Indicator>(data0_)) {
            val0 = indicator->get(0);
        } else if (data0_->lines && data0_->lines->size() > 0) {
            auto line = data0_->lines->getline(0);
            if (line) {
                val0 = (*line)[0];
            }
        }
        
        // Get value from data1 (usually SMA indicator)
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data1_)) {
            val1 = data_series->close(0);
        } else if (auto indicator = std::dynamic_pointer_cast<Indicator>(data1_)) {
            val1 = indicator->get(0);
        } else if (data1_->lines && data1_->lines->size() > 0) {
            auto line = data1_->lines->getline(0);
            if (line) {
                val1 = (*line)[0];
            }
        }
        
        double diff = std::numeric_limits<double>::quiet_NaN();
        if (!std::isnan(val0) && !std::isnan(val1)) {
            diff = val0 - val1;
        }
        
        {
            std::cout << "NonZeroDifference::next()" 
                      << ": val0=" << val0 << ", val1=" << val1 
                      << ", diff=" << diff << ", last_nzd_=" << last_nzd_ << std::endl;
        }

        if (!std::isnan(diff)) {
            // Python behavior: only use last non-zero value if diff is EXACTLY 0.0
            // Python uses "d if d else self.l.nzd[-1]" which treats 0.0 as falsy
            if (diff != 0.0) {
                last_nzd_ = diff;
                nzd_line->append(diff);
            } else {
                // If diff is exactly 0, append the last non-zero value
                nzd_line->append(last_nzd_);
            }
        } else {
            // If we can't calculate diff (one of the values is NaN), append NaN
            nzd_line->append(std::numeric_limits<double>::quiet_NaN());
        }
    }
}

void NonZeroDifference::oncestart(int start, int end) {
    if (!data0_ || !data1_ || start >= end) return;
    
    auto nzd_line = lines->getline(nzd);
    if (nzd_line) {
        double diff = getDefaultValue(data0_, start) - getDefaultValue(data1_, start);
        nzd_line->set(start, diff);
    }
}

void NonZeroDifference::once(int start, int end) {
    if (!data0_ || !data1_) {
        std::cerr << "NonZeroDifference::once() - Missing data: data0_=" << data0_.get() 
                  << ", data1_=" << data1_.get() << std::endl;
        return;
    }
    
    static int call_count = 0;
    if (call_count++ < 3) {
        std::cerr << "NonZeroDifference::once(" << start << ", " << end << ") called" << std::endl;
    }
    
    auto nzd_line = lines->getline(nzd);
    if (!nzd_line) return;
    
    // Ensure the line has the correct size - not more, not less
    auto nzd_buffer = std::dynamic_pointer_cast<LineBuffer>(nzd_line);
    if (nzd_buffer) {
        // Clear and resize to exact size needed
        nzd_buffer->array().clear();
        nzd_buffer->array().resize(end, 0.0);
    }
    
    double prev = 0.0;
    int valid_count = 0;
    
    for (int i = start; i < end; ++i) {
        double val0 = getDefaultValue(data0_, i);
        double val1 = getDefaultValue(data1_, i);
        
        // Skip if either value is NaN
        if (std::isnan(val0) || std::isnan(val1)) {
            nzd_line->set(i, std::numeric_limits<double>::quiet_NaN());
            continue;
        }
        
        double diff = val0 - val1;
        valid_count++;
        
        // Python behavior: if diff is exactly zero, use the last non-zero difference
        if (diff != 0.0) {
            prev = diff;
        }
        double value_to_set = diff != 0.0 ? diff : prev;
        nzd_line->set(i, value_to_set);
    }
    
    // Removed debug output
}

void NonZeroDifference::_once() {
    
    // Get data's buflen
    size_t data_buflen = 0;
    if (data0_) {
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            data_buflen = data_series->buflen();
        } else if (auto line_series = std::dynamic_pointer_cast<LineSeries>(data0_)) {
            data_buflen = line_series->buflen();
        }
    }
    
    // std::cerr << "NonZeroDifference::_once() - data_buflen=" << data_buflen << std::endl;
    
    // Calculate all values from the beginning
    if (data_buflen > 0) {
        once(0, data_buflen);
    }
    
    // Position at the last valid index
    auto nzd_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(nzd));
    if (nzd_line && nzd_line->array().size() > 0) {
        nzd_line->set_idx(nzd_line->array().size() - 1);
    }
    
    // Execute binding synchronization if any
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

double NonZeroDifference::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto nzd_line = lines->getline(nzd);
    if (!nzd_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*nzd_line)[ago];
}

// CrossBase implementation
CrossBase::CrossBase(bool crossup) : Indicator(), crossup_(crossup) {
    setup_lines();
    _minperiod(2); // Needs previous value to detect crossover
    _ltype = LineRoot::IndType::IndType;  // Ensure CrossBase is marked as an indicator
}

void CrossBase::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void CrossBase::add_data(std::shared_ptr<LineSeries> data) {
    std::cout << "DEBUG CrossBase::add_data() called with data=" << data.get() << std::endl;
    if (!data0_) {
        data0_ = data;
        std::cout << "DEBUG CrossBase: Set data0_=" << data0_.get() << std::endl;
    } else if (!data1_) {
        data1_ = data;
        std::cout << "DEBUG CrossBase: Set data1_=" << data1_.get() << std::endl;
        // Create NZD indicator once we have both data sources
        nzd_ = std::make_shared<NonZeroDifference>();
        nzd_->add_data(data0_);
        nzd_->add_data(data1_);
        
        // CRITICAL: Set the clock for nzd_ before adding it
        // Find a suitable clock - prefer data0 if it's a DataSeries
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            nzd_->_clock = data0_;
            std::cout << "DEBUG CrossBase: Set nzd_ clock to data0_ (DataSeries)" << std::endl;
        } else if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data1_)) {
            nzd_->_clock = data1_;
            std::cout << "DEBUG CrossBase: Set nzd_ clock to data1_ (DataSeries)" << std::endl;
        } else if (_clock) {
            nzd_->_clock = _clock;
            std::cout << "DEBUG CrossBase: Set nzd_ clock to parent clock" << std::endl;
        } else {
            std::cout << "WARNING: CrossBase could not find a suitable clock for nzd_!" << std::endl;
        }
        
        addindicator(nzd_);
        std::cout << "DEBUG CrossBase: Created and added nzd_=" << nzd_.get() << std::endl;
        
    }
    // Base class add_data for datas vector
    // Don't add to datas vector since type mismatch (LineActions vs LineSeries)
}

void CrossBase::prenext() {
    static int prenext_count = 0;
    prenext_count++;
    if (prenext_count <= 5) {
        std::cout << "DEBUG CrossBase::prenext() called #" << prenext_count 
                  << ", crossup_=" << crossup_ << ", this=" << this << std::endl;
    }
    
    // DON'T manually call child indicators - LineIterator handles this automatically
    // Manually calling _next() creates infinite recursion!
    
    // Append 0.0 during warm-up period to keep line buffer in sync
    auto cross_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(cross));
    if (cross_line) {
        cross_line->append(0.0);
    }
}

void CrossBase::nextstart() {
    // Call next() for the first calculation
    next();
}

void CrossBase::next() {
    if (!data0_ || !data1_ || !nzd_) {
        if (bar_count_ < 5) {
            std::cout << "DEBUG CrossBase::next() - Missing data: data0_=" << (data0_ ? "OK" : "NULL") 
                      << ", data1_=" << (data1_ ? "OK" : "NULL") 
                      << ", nzd_=" << (nzd_ ? "OK" : "NULL") << std::endl;
        }
        return;
    }
    
    // DON'T manually call _next() - this creates infinite recursion!
    // LineIterator already handles the execution order properly
    
    auto cross_line = lines->getline(cross);
    auto nzd_line = nzd_->lines->getline(NonZeroDifference::nzd);
    
    if (bar_count_ < 5) {
        std::cout << "DEBUG CrossBase::next() - cross_line=" << (cross_line ? "OK" : "NULL") 
                  << ", nzd_line=" << (nzd_line ? "OK" : "NULL") 
                  << ", nzd_ptr=" << nzd_.get() << std::endl;
    }
    
    if (cross_line && nzd_line) {
        if (bar_count_ < 5) {
            std::cout << "DEBUG CrossBase::next() about to call getDefaultValue - data0_=" << data0_.get() 
                      << ", data1_=" << data1_.get() << std::endl;
        }
        
        // Get values directly without getDefaultValue to debug
        double current_data0 = 0.0;
        double current_data1 = 0.0;
        
        // For data0 (could be DataSeries or Indicator like CloseLine)
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            current_data0 = data_series->close(0);
            if (bar_count_ < 5) {
                std::cout << "DEBUG CrossBase::next() - data0 is DataSeries, close(0)=" << current_data0 << std::endl;
            }
        } else if (auto indicator = std::dynamic_pointer_cast<Indicator>(data0_)) {
            // In streaming mode, use get(0) for indicators
            current_data0 = indicator->get(0);
            if (bar_count_ < 5) {
                std::cout << "DEBUG CrossBase::next() - data0 is Indicator, get(0)=" << current_data0 << std::endl;
            }
        } else {
            // Generic LineSeries - try to get the first line
            if (data0_->lines && data0_->lines->size() > 0) {
                auto line = data0_->lines->getline(0);
                if (line) {
                    current_data0 = (*line)[0];
                }
            }
            if (bar_count_ < 5) {
                std::cout << "DEBUG CrossBase::next() - data0 is generic LineSeries, value=" << current_data0 << std::endl;
            }
        }
        
        // For data1 (could be SMA indicator or DataSeries)
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data1_)) {
            current_data1 = data_series->close(0);
            if (bar_count_ < 5) {
                std::cout << "DEBUG CrossBase::next() - data1 is DataSeries, close(0)=" << current_data1 << std::endl;
            }
        } else if (auto indicator = std::dynamic_pointer_cast<Indicator>(data1_)) {
            // In streaming mode, use get(0) for indicators
            current_data1 = indicator->get(0);
            if (bar_count_ < 10) {
                std::cout << "DEBUG CrossBase::next() #" << bar_count_ 
                          << " - data1 is Indicator, get(0)=" << current_data1 << std::endl;
            }
        } else {
            // Generic LineSeries - try to get the first line
            if (data1_->lines && data1_->lines->size() > 0) {
                auto line = data1_->lines->getline(0);
                if (line) {
                    current_data1 = (*line)[0];
                }
            }
            if (bar_count_ < 5) {
                std::cout << "DEBUG CrossBase::next() - data1 is generic LineSeries, value=" << current_data1 << std::endl;
            }
        }
        
        // if (bar_count_ < 5) {
        //     std::cout << "DEBUG CrossBase::next() final values - data0=" << current_data0 
        //               << ", data1=" << current_data1 << std::endl;
        // }
        
        // Get the NZD value from the previous bar
        // When CrossBase::next() runs, NZD has already appended for this bar
        // So we need index [1] to get the previous bar's value
        double prev_nzd = std::numeric_limits<double>::quiet_NaN();
        
        // CRITICAL FIX: We need the previous bar's NZD value
        // Since CrossBase is called AFTER NZD has already executed next() for this bar,
        // the current value is at index [0] and previous is at index [-1]
        // But we need to ensure NZD has been executed first
        if (nzd_ && nzd_->size() < data0_->size()) {
            // NZD hasn't executed for this bar yet, call it manually
            nzd_->next();
        }
        
        if (nzd_line->size() > 1) {
            prev_nzd = (*nzd_line)[-1];
            
            // Debug: check multiple indices to understand the buffer
            if (bar_count_ == 30) {  // Check at the specific bar we've been debugging
                std::cout << "NZD buffer debug bar_count_=" << bar_count_ << ", size=" << nzd_line->size() << std::endl;
                std::cout << "  nzd[0]=" << (*nzd_line)[0] << " (current)" << std::endl;
                if (nzd_line->size() > 1) {
                    std::cout << "  nzd[-1]=" << (*nzd_line)[-1] << " (previous)" << std::endl;
                }
                if (nzd_line->size() > 2) {
                    std::cout << "  nzd[-2]=" << (*nzd_line)[-2] << " (two bars ago)" << std::endl;
                }
                // Also check if the buffer is a LineBuffer with array access
                auto nzd_buffer = std::dynamic_pointer_cast<LineBuffer>(nzd_line);
                if (nzd_buffer) {
                    auto array = nzd_buffer->array();
                    std::cout << "  raw array size=" << array.size() << std::endl;
                    if (array.size() >= 3) {
                        std::cout << "  raw array[-3]=" << array[array.size()-3] 
                                  << ", [-2]=" << array[array.size()-2] 
                                  << ", [-1]=" << array[array.size()-1] << std::endl;
                    }
                }
            }
        }
        
        // Count bars for this instance
        bar_count_++;
        
        // Debug for more bars to see crossover activity
        if (bar_count_ <= 50) {
            std::cout << "**DISTINCTIVE** DEBUG CrossBase::next() bar_count_=" << bar_count_ 
                      << ", data0=" << current_data0 << ", data1=" << current_data1
                      << ", current_nzd=" << (nzd_line->size() > 0 ? (*nzd_line)[0] : 0.0)
                      << ", prev_nzd=" << prev_nzd << ", nzd_size=" << nzd_line->size()
                      << ", crossup_=" << crossup_ << std::endl;
        }

        bool cross_detected = false;
        
        if (crossup_) {
            // CrossUp: previous diff was negative and current data0 > data1
            cross_detected = (prev_nzd < 0.0) && (current_data0 > current_data1);
            if (bar_count_ <= 50 || cross_detected) {
                std::cout << "DEBUG CrossUp bar_count_=" << bar_count_ << ": prev_nzd < 0.0 = " << (prev_nzd < 0.0) 
                          << ", data0 > data1 = " << (current_data0 > current_data1)
                          << ", cross_detected = " << cross_detected << std::endl;
            }
        } else {
            // CrossDown: previous diff was positive and current data0 < data1
            cross_detected = (prev_nzd > 0.0) && (current_data0 < current_data1);
            if (bar_count_ <= 50 || cross_detected) {
                std::cout << "DEBUG CrossDown bar_count_=" << bar_count_ << ": prev_nzd > 0.0 = " << (prev_nzd > 0.0) 
                          << ", data0 < data1 = " << (current_data0 < current_data1)
                          << ", cross_detected = " << cross_detected << std::endl;
            }
        }
        
        auto cross_buffer = std::dynamic_pointer_cast<LineBuffer>(cross_line);
        if (cross_buffer) {
            cross_buffer->append(cross_detected ? 1.0 : 0.0);
        }
    } else {
        if (bar_count_ < 5) {
            std::cout << "DEBUG CrossBase::next() - Missing lines: cross_line=" << (cross_line ? "OK" : "NULL") 
                      << ", nzd_line=" << (nzd_line ? "OK" : "NULL") << std::endl;
        }
    }
}

void CrossBase::once(int start, int end) {
    if (!data0_ || !data1_ || !nzd_) return;
    
    // Don't recalculate NZD here - it's already calculated by _once()
    
    auto cross_line = lines->getline(cross);
    auto nzd_line = nzd_->lines->getline(NonZeroDifference::nzd);
    
    if (!cross_line || !nzd_line) return;
    
    // Ensure the line has the correct size - not more, not less
    auto cross_buffer = std::dynamic_pointer_cast<LineBuffer>(cross_line);
    if (cross_buffer) {
        // Clear and resize to exact size needed (same fix as NZD)
        cross_buffer->array().clear();
        cross_buffer->array().resize(end, 0.0);
    }
    
    auto nzd_buffer = std::dynamic_pointer_cast<LineBuffer>(nzd_line);
    
    int cross_count = 0;
    for (int i = start; i < end; ++i) {
        double current_data0 = getDefaultValue(data0_, i);
        double current_data1 = getDefaultValue(data1_, i);
        
        // Skip if either value is NaN
        if (std::isnan(current_data0) || std::isnan(current_data1)) {
            cross_line->set(i, 0.0);
            continue;
        }
        
        // Use NZD buffer directly for previous value
        auto nzd_buffer = std::dynamic_pointer_cast<LineBuffer>(nzd_line);
        double prev_nzd = 0.0;
        double curr_nzd = 0.0;
        if (nzd_buffer && i > 0) {
            const auto& nzd_array = nzd_buffer->array();
            if (i < static_cast<int>(nzd_array.size())) {
                curr_nzd = nzd_array[i];
            }
            if (i - 1 >= 0 && i - 1 < static_cast<int>(nzd_array.size())) {
                prev_nzd = nzd_array[i - 1];
                // Skip if previous NZD is NaN
                if (std::isnan(prev_nzd)) {
                    cross_line->set(i, 0.0);
                    continue;
                }
            }
        }
        
        
        bool cross_detected = false;
        
        if (crossup_) {
            cross_detected = (prev_nzd < 0.0) && (current_data0 > current_data1);
        } else {
            cross_detected = (prev_nzd > 0.0) && (current_data0 < current_data1);
        }
        
        if (cross_detected) {
            cross_count++;
        }
        
        cross_line->set(i, cross_detected ? 1.0 : 0.0);
    }
    
    // Debug output only if crosses found
    if (cross_count > 0) {
        static int debug_count = 0;
        if (debug_count++ < 5) {
            std::cerr << "CrossBase::once() " << (crossup_ ? "UP" : "DOWN") 
                      << " - Found " << cross_count << " crosses in range [" 
                      << start << ", " << end << ")" << std::endl;
        }
    }
    
}

void CrossBase::_once() {
    
    // Get data's buflen
    size_t data_buflen = 0;
    if (data0_) {
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            data_buflen = data_series->buflen();
        } else if (auto line_series = std::dynamic_pointer_cast<LineSeries>(data0_)) {
            data_buflen = line_series->buflen();
        }
    }
    
    
    // Process child indicators first
    if (nzd_) {
        nzd_->_once();
    }
    
    // Calculate all values from the beginning
    if (data_buflen > 0) {
        once(0, data_buflen);
    }
    
    // Position at the last valid index
    auto cross_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(cross));
    if (cross_line && cross_line->array().size() > 0) {
        cross_line->set_idx(cross_line->array().size() - 1);
    }
    
    // IMPORTANT: Reset buffer index to 0 for array access compatibility
    // This ensures operator[] works correctly when CrossOver reads the values
    if (cross_line) {
        cross_line->set_idx(0);
    }
    
    // Execute binding synchronization if any
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

double CrossBase::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto cross_line = lines->getline(cross);
    if (!cross_line) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return (*cross_line)[ago];
}

// CrossUp implementation
CrossUp::CrossUp() : CrossBase(true) {
    std::cout << "DEBUG CrossUp constructor called, this=" << this << std::endl;
}

// CrossDown implementation
CrossDown::CrossDown() : CrossBase(false) {
    std::cout << "DEBUG CrossDown constructor called, this=" << this << std::endl;
}

// CrossOver implementation
CrossOver::CrossOver() : Indicator(), data0_(nullptr), data1_(nullptr), upcross_(nullptr), downcross_(nullptr) {
    setup_lines();
    _minperiod(2); // Needs previous value to detect crossover
    _ltype = LineRoot::IndType::IndType;  // Ensure CrossOver is marked as an indicator
}

CrossOver::CrossOver(std::shared_ptr<LineActions> data0, std::shared_ptr<LineActions> data1) : Indicator(), data0_(nullptr), data1_(nullptr), upcross_(nullptr), downcross_(nullptr) {
    setup_lines();
    _minperiod(2); // Needs previous value to detect crossover
    _ltype = LineRoot::IndType::IndType;  // Ensure CrossOver is marked as an indicator
    // Note: LineActions constructor is deprecated - use template constructor or add_data() instead
}

void CrossOver::setup_lines() {
    if (lines->size() == 0) {
            lines->add_line(std::make_shared<LineBuffer>());
        }
}

void CrossOver::add_data(std::shared_ptr<LineSeries> data) {
    std::cout << "DEBUG CrossOver::add_data() called with data=" << data.get() << std::endl;
    if (!data0_) {
        data0_ = data;
        std::cout << "DEBUG CrossOver: Set data0_=" << data0_.get() << std::endl;
    } else if (!data1_) {
        data1_ = data;
        std::cout << "DEBUG CrossOver: Set data1_=" << data1_.get() << std::endl;
        // Create CrossUp and CrossDown indicators
        upcross_ = std::make_shared<CrossUp>();
        upcross_->add_data(data0_);
        upcross_->add_data(data1_);
        
        // Set clock for upcross_
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            upcross_->_clock = data0_;
        } else if (_clock) {
            upcross_->_clock = _clock;
        }
        
        addindicator(upcross_);
        std::cout << "DEBUG CrossOver: Created and added upcross_=" << upcross_.get() << std::endl;
        
        downcross_ = std::make_shared<CrossDown>();
        downcross_->add_data(data0_);
        downcross_->add_data(data1_);
        
        // Set clock for downcross_
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            downcross_->_clock = data0_;
        } else if (_clock) {
            downcross_->_clock = _clock;
        }
        
        addindicator(downcross_);
        std::cout << "DEBUG CrossOver: Created and added downcross_=" << downcross_.get() << std::endl;
        
        // Update minimum period based on child indicators
        _periodrecalc();
        
        // Also consider the minimum period of data sources
        int data0_minperiod = 1;
        int data1_minperiod = 1;
        
        if (auto ind0 = std::dynamic_pointer_cast<Indicator>(data0_)) {
            data0_minperiod = ind0->getMinPeriod();
        }
        if (auto ind1 = std::dynamic_pointer_cast<Indicator>(data1_)) {
            data1_minperiod = ind1->getMinPeriod();
        }
        
        int data_minperiod = std::max(data0_minperiod, data1_minperiod);
        if (data_minperiod > minperiod_) {
            _minperiod(data_minperiod);
        }
        
        std::cout << "DEBUG CrossOver: Final minperiod=" << minperiod_ << std::endl;
    }
    // Base class add_data for datas vector
    // Don't add to datas vector since type mismatch (LineActions vs LineSeries)
}

void CrossOver::prenext() {
    static int prenext_count = 0;
    prenext_count++;
    if (prenext_count <= 50) {
        std::cout << "**WARNING** CrossOver::prenext() #" << prenext_count 
                  << " - Still in prenext mode!" << std::endl;
    }
    
    // DON'T manually call child indicators - LineIterator handles this automatically
    // Manually calling _next() creates infinite recursion!
    
    // Append 0.0 during warm-up period to keep line buffer in sync
    auto cross_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(crossover));
    if (cross_line) {
        cross_line->append(0.0);
    }
}

void CrossOver::nextstart() {
    next();
}

void CrossOver::next() {
    if (!upcross_ || !downcross_) {
        return;
    }
    
    // CRITICAL: Manually call child indicators' next() methods
    // In streaming mode, we need to ensure child indicators are updated
    static int next_count = 0;
    next_count++;
    if (next_count <= 50) {
        std::cout << "**CRITICAL** CrossOver::next() #" << next_count 
                  << " - This method is being called! upcross_=" << upcross_.get() 
                  << ", downcross_=" << downcross_.get() << std::endl;
    }
    
    // WORKAROUND: Manually execute child indicators to ensure they're updated
    // This is necessary in streaming mode where nested indicators may not be automatically executed
    bool manually_called = false;
    if (upcross_ && data0_ && upcross_->size() < data0_->size()) {
        upcross_->next();
        manually_called = true;
    }
    if (downcross_ && data0_ && downcross_->size() < data0_->size()) {
        downcross_->next();
        manually_called = true;
    }
    
    if (manually_called && next_count <= 100) {
        std::cout << "DEBUG CrossOver::next() #" << next_count 
                  << " - Manually called child indicators, data0 size=" << (data0_ ? data0_->size() : 0)
                  << ", upcross size=" << (upcross_ ? upcross_->size() : 0)
                  << ", downcross size=" << (downcross_ ? downcross_->size() : 0) << std::endl;
    }
    
    auto crossover_line = lines->getline(crossover);
    auto upcross_line = upcross_->lines->getline(CrossUp::cross);
    auto downcross_line = downcross_->lines->getline(CrossDown::cross);
    
    if (crossover_line && upcross_line && downcross_line) {
        // Get the current size of child indicators to check proper indexing
        size_t up_size = upcross_line->size();
        size_t down_size = downcross_line->size();
        
        // Get signals from child indicators
        double up_signal = 0.0;
        double down_signal = 0.0;
        
        // Since we just called the child indicators, access their current values (index 0)
        if (up_size > 0) {
            up_signal = (*upcross_line)[0];
        }
        if (down_size > 0) {
            down_signal = (*downcross_line)[0];
        }
        
        if (next_count <= 50 || (up_signal != 0.0 || down_signal != 0.0)) {
            std::cout << "DEBUG CrossOver::next() #" << next_count << ": up_signal=" << up_signal 
                      << ", down_signal=" << down_signal << ", result=" << (up_signal - down_signal) << std::endl;
        }
        
        // +1 for up cross, -1 for down cross, 0 for no cross
        double result = up_signal - down_signal;
        auto cross_buffer = std::dynamic_pointer_cast<LineBuffer>(crossover_line);
        if (cross_buffer) {
            // Always append in next() - we're in streaming mode
            cross_buffer->append(result);
        }
    }
}

void CrossOver::once(int start, int end) {
    if (!upcross_ || !downcross_) {
        return;
    }
    
    static int call_count = 0;
    if (call_count++ < 3) {
        std::cerr << "CrossOver::once(" << start << ", " << end << ") called" << std::endl;
    }
    
    // Don't recalculate sub-indicators - they're already calculated by _once()
    // upcross_->once(start, end);
    // downcross_->once(start, end);
    
    auto crossover_line = lines->getline(crossover);
    auto upcross_line = upcross_->lines->getline(CrossUp::cross);
    auto downcross_line = downcross_->lines->getline(CrossDown::cross);
    
    if (!crossover_line || !upcross_line || !downcross_line) return;
    
    // Debug: check child buffer sizes
    auto upcross_buffer = std::dynamic_pointer_cast<LineBuffer>(upcross_line);
    auto downcross_buffer = std::dynamic_pointer_cast<LineBuffer>(downcross_line);
    if (call_count <= 3) {
        std::cerr << "CrossOver::once() - Child buffer sizes: up=" 
                  << (upcross_buffer ? upcross_buffer->array().size() : 0)
                  << ", down=" << (downcross_buffer ? downcross_buffer->array().size() : 0)
                  << ", need range [" << start << ", " << end << ")" << std::endl;
        
        // Check actual values in child buffers
        if (upcross_buffer && upcross_buffer->array().size() > 10) {
            std::cerr << "  UP buffer values [0-10]: ";
            for (int idx = 0; idx <= 10 && idx < static_cast<int>(upcross_buffer->array().size()); idx++) {
                std::cerr << "[" << idx << "]=" << upcross_buffer->array()[idx] << " ";
            }
            std::cerr << std::endl;
        }
    }
    
    // Ensure the line has the correct size - apply same fix as NZD and CrossBase  
    auto crossover_buffer = std::dynamic_pointer_cast<LineBuffer>(crossover_line);
    if (crossover_buffer) {
        // Clear and resize to exact size needed
        crossover_buffer->array().clear();
        crossover_buffer->array().resize(end, 0.0);
        
        if (call_count <= 3) {
            std::cerr << "CrossOver::once() - Reset buffer to size " << end << std::endl;
        }
    }
    
    // Count crossovers detected
    int up_count = 0, down_count = 0;
    
    for (int i = start; i < end; ++i) {
        double up_signal = (*upcross_line)[i];
        double down_signal = (*downcross_line)[i];
        
        if (up_signal != 0.0) up_count++;
        if (down_signal != 0.0) down_count++;
        
        // CrossOver returns: +1 for up cross, -1 for down cross, 0 for no cross
        double crossover_value = up_signal - down_signal;
        crossover_line->set(i, crossover_value);
        
        // Debug all values in early range to see the pattern
        if (i <= 10 || (i >= 7 && i <= 8) || i == 18) {
            std::cerr << "CrossOver[" << i << "]: up=" << up_signal 
                      << ", down=" << down_signal << ", result=" << crossover_value << std::endl;
        }
    }
    
    if (up_count > 0 || down_count > 0) {
        static int debug_count = 0;
        if (debug_count++ < 5) {
            std::cerr << "CrossOver::once() - " << up_count << " up crosses, " 
                      << down_count << " down crosses detected" << std::endl;
        }
    }
    
}

void CrossOver::_once() {
    
    // Get data's buflen since indicators use data's buflen in runonce mode
    size_t data_buflen = 0;
    if (data0_) {
        if (auto data_series = std::dynamic_pointer_cast<DataSeries>(data0_)) {
            data_buflen = data_series->buflen();
        } else if (auto line_series = std::dynamic_pointer_cast<LineSeries>(data0_)) {
            data_buflen = line_series->buflen();
        }
    }
    
    
    // DON'T forward - this moves the index past calculated values
    // Just calculate all values at the current position
    
    // Process child indicators first
    if (upcross_) {
        upcross_->_once();
    }
    if (downcross_) {
        downcross_->_once();
    }
    
    // Calculate CrossOver values from the beginning
    if (data_buflen > 0) {
        once(0, data_buflen);
    }
    
    // Position at the last valid index
    auto crossover_line = std::dynamic_pointer_cast<LineBuffer>(lines->getline(crossover));
    if (crossover_line && crossover_line->array().size() > 0) {
        crossover_line->set_idx(crossover_line->array().size() - 1);
        
        // Debug: verify some values were set
        static int debug_count = 0;
        if (debug_count++ < 3) {
            std::cerr << "CrossOver::_once() complete - buffer size=" << crossover_line->array().size() << std::endl;
            if (crossover_line->array().size() > 20) {
                std::cerr << "  Values at key positions: ";
                for (int idx : {7, 8, 10, 18, 24}) {
                    if (idx < static_cast<int>(crossover_line->array().size())) {
                        std::cerr << "[" << idx << "]=" << crossover_line->array()[idx] << " ";
                    }
                }
                std::cerr << std::endl;
            }
        }
    }
    
    // Execute binding synchronization if any
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

double CrossOver::get(int ago) const {
    if (!lines || lines->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    auto crossover_line = lines->getline(crossover);
    if (!crossover_line || crossover_line->size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    // Just use operator[] which handles ago correctly
    return (*crossover_line)[ago];
}

size_t NonZeroDifference::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto nzd_line = lines->getline(nzd);
    if (!nzd_line) {
        return 0;
    }
    return nzd_line->size();
}

size_t CrossBase::size() const {
    if (!lines || lines->size() == 0) {
        return 0;
    }
    auto cross_line = lines->getline(cross);
    if (!cross_line) {
        return 0;
    }
    return cross_line->size();
}

size_t CrossOver::size() const {
    if (!lines || lines->size() == 0) {
        std::cout << "DEBUG CrossOver::size() - no lines, returning 0" << std::endl;
        return 0;
    }
    auto crossover_line = lines->getline(crossover);
    if (!crossover_line) {
        std::cout << "DEBUG CrossOver::size() - no crossover line, returning 0" << std::endl;
        return 0;
    }
    size_t sz = crossover_line->size();
    static int count = 0;
    if (count++ < 5) {
        std::cout << "DEBUG CrossOver::size() returning " << sz << std::endl;
    }
    return sz;
}

} // namespace backtrader