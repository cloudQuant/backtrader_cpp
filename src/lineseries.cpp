#include "lineseries.h"
#include <algorithm>
#include <stdexcept>

namespace backtrader {

// Lines implementation
Lines::Lines() {
}

std::shared_ptr<LineSingle> Lines::operator[](size_t idx) const {
    return getline(idx);
}

std::shared_ptr<LineSingle> Lines::getline(size_t idx) const {
    if (idx >= lines_.size()) {
        throw std::out_of_range("Line index out of range");
    }
    return lines_[idx];
}

size_t Lines::size() const {
    return lines_.size();
}

bool Lines::empty() const {
    return lines_.empty();
}

void Lines::forward(size_t size) {
    for (auto& line : lines_) {
        line->forward(size);
    }
}

void Lines::backward(size_t size) {
    for (auto& line : lines_) {
        line->backward(size);
    }
}

void Lines::rewind(size_t size) {
    for (auto& line : lines_) {
        line->rewind(size);
    }
}

void Lines::extend(size_t size) {
    for (auto& line : lines_) {
        line->extend(size);
    }
}

void Lines::reset() {
    for (auto& line : lines_) {
        line->reset();
    }
}

void Lines::home() {
    for (auto& line : lines_) {
        line->home();
    }
}

void Lines::advance(size_t size) {
    for (auto& line : lines_) {
        line->advance(size);
    }
}

void Lines::addbinding(std::shared_ptr<LineSingle> binding) {
    if (!lines_.empty()) {
        lines_[0]->addbinding(binding);
    }
}

void Lines::oncebinding() {
    for (auto& line : lines_) {
        line->oncebinding();
    }
}

void Lines::qbuffer(size_t savemem) {
    for (auto& line : lines_) {
        if (auto buffer = dynamic_cast<LineBuffer*>(line.get())) {
            buffer->qbuffer(savemem);
        }
    }
}

void Lines::minbuffer(size_t size) {
    for (auto& line : lines_) {
        if (auto buffer = dynamic_cast<LineBuffer*>(line.get())) {
            buffer->minbuffer(size);
        }
    }
}

size_t Lines::buflen() const {
    if (lines_.empty()) {
        return 0;
    }
    return lines_[0]->buflen();
}

void Lines::add_line(std::shared_ptr<LineSingle> line) {
    lines_.push_back(line);
}

void Lines::set_line(size_t idx, std::shared_ptr<LineSingle> line) {
    if (idx >= lines_.size()) {
        lines_.resize(idx + 1);
    }
    lines_[idx] = line;
}

void Lines::add_alias(const std::string& name, size_t idx) {
    aliases_[name] = idx;
}

size_t Lines::get_alias_idx(const std::string& name) const {
    auto it = aliases_.find(name);
    if (it != aliases_.end()) {
        return it->second;
    }
    throw std::runtime_error("Alias not found: " + name);
}

bool Lines::has_alias(const std::string& name) const {
    return aliases_.find(name) != aliases_.end();
}

std::vector<std::string> Lines::get_aliases() const {
    std::vector<std::string> result;
    for (const auto& pair : aliases_) {
        result.push_back(pair.first);
    }
    return result;
}

std::shared_ptr<Lines> Lines::derive(const std::string& name, 
                                    const std::vector<std::string>& line_names,
                                    size_t extra_lines) {
    auto lines = std::make_shared<Lines>();
    
    // Create line buffers for each named line
    for (size_t i = 0; i < line_names.size(); ++i) {
        auto line_buffer = std::make_shared<LineBuffer>();
        lines->add_line(line_buffer);
        lines->add_alias(line_names[i], i);
    }
    
    // Add extra unnamed lines
    for (size_t i = 0; i < extra_lines; ++i) {
        lines->add_line(std::make_shared<LineBuffer>());
    }
    
    return lines;
}

// LineSeries implementation
LineSeries::LineSeries() : LineMultiple(), _owner(nullptr) {
    _init_lines();
}

std::shared_ptr<LineSingle> LineSeries::getline(size_t idx) {
    if (!lines || idx >= lines->size()) {
        throw std::out_of_range("Line index out of range");
    }
    return lines->getline(idx);
}

size_t LineSeries::getlinealiases() const {
    if (!lines) {
        return 0;
    }
    return lines->get_aliases().size();
}

std::string LineSeries::getlinealias(size_t idx) const {
    if (!lines) {
        return "";
    }
    
    auto aliases = lines->get_aliases();
    if (idx >= aliases.size()) {
        return "";
    }
    
    return aliases[idx];
}

size_t LineSeries::fullsize() const {
    if (!lines) {
        return 0;
    }
    return lines->size();
}

size_t LineSeries::size() const {
    if (!lines) {
        return 0;
    }
    return lines->size();
}

bool LineSeries::empty() const {
    if (!lines) {
        return true;
    }
    return lines->empty();
}

void LineSeries::_init_lines() {
    auto line_names = _get_line_names();
    lines = Lines::derive("LineSeries", line_names);
    
    // Update LineMultiple's lines_ vector
    lines_.clear();
    for (size_t i = 0; i < lines->size(); ++i) {
        lines_.push_back(lines->getline(i));
    }
}

std::vector<std::string> LineSeries::_get_line_names() const {
    return {}; // Base class has no default lines
}

// LineSeriesMaker implementation
std::shared_ptr<LineSeries> LineSeriesMaker::make(std::shared_ptr<LineRoot> source) {
    if (auto series = std::dynamic_pointer_cast<LineSeries>(source)) {
        return series;
    }
    
    if (auto single = std::dynamic_pointer_cast<LineSingle>(source)) {
        auto series = std::make_shared<LineSeries>();
        series->lines->add_line(single);
        return series;
    }
    
    // Create a new LineSeries and copy the source
    auto series = std::make_shared<LineSeries>();
    // This is a simplified implementation
    // In a full implementation, we'd need to properly copy the source
    return series;
}

std::shared_ptr<LineSeries> LineSeriesMaker::make(double value) {
    auto series = std::make_shared<LineSeries>();
    auto line_num = std::make_shared<LineNum>(value);
    series->lines->add_line(line_num);
    return series;
}

std::shared_ptr<LineSeries> LineSeriesMaker::make(const std::vector<double>& values) {
    auto series = std::make_shared<LineSeries>();
    auto line_buffer = std::make_shared<LineBuffer>();
    
    // Fill the buffer with values
    for (double value : values) {
        line_buffer->append(value);
    }
    
    series->lines->add_line(line_buffer);
    return series;
}

} // namespace backtrader