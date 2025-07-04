#include "observer.h"
#include "analyzer.h"

namespace backtrader {

Observer::Observer() : ObserverBase() {
    _ltype = LineRoot::IndType::ObsType;
}

void Observer::_start() {
    start();
}

void Observer::start() {
    // Default implementation - can be overridden by derived classes
}

void Observer::prenext() {
    // Observers always observe, so prenext calls next
    next();
}

void Observer::_register_analyzer(std::shared_ptr<Analyzer> analyzer) {
    _analyzers.push_back(analyzer);
}

std::vector<std::shared_ptr<Analyzer>> Observer::get_analyzers() const {
    return _analyzers;
}

} // namespace backtrader