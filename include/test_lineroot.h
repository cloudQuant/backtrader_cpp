#pragma once

#include "lineroot.h"
#include "linebuffer.h"
#include <memory>

namespace backtrader {

// Test implementation of LineRoot that wraps LineBuffer
// This is for backward compatibility with tests that incorrectly use LineRoot
class TestLineRoot : public LineRoot {
public:
    TestLineRoot(size_t size, const std::string& name = "") : LineRoot(size, name) {
        buffer_ = std::make_shared<LineBuffer>();
        buffer_->set_maxlen(size);
    }
    
    void forward(double value) override {
        if (buffer_->size() == 0) {
            buffer_->set(0, value);
        } else {
            buffer_->append(value);
        }
    }
    
    double operator[](int index) const {
        return (*buffer_)[index];
    }
    
    size_t size() const {
        return buffer_->size();
    }
    
    size_t buflen() const {
        return buffer_->size();
    }
    
    std::shared_ptr<LineBuffer> getBuffer() const {
        return buffer_;
    }
    
private:
    std::shared_ptr<LineBuffer> buffer_;
};

} // namespace backtrader