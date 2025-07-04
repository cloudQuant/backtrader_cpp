#pragma once

#include "../feed.h"
#include <functional>
#include <memory>

namespace backtrader {
namespace filters {

/**
 * DataFilter - filters out bars from a given data source
 * 
 * This class filters out bars from a given data source. In addition to the
 * standard parameters of a DataBase it takes a funcfilter parameter which
 * can be any callable
 * 
 * Logic:
 *   - funcfilter will be called with the underlying data source
 *   - Return value true: current data source bar values will be used
 *   - Return value false: current data source bar values will be discarded
 */
class DataFilter : public AbstractDataBase {
public:
    // Parameters structure
    struct Params {
        std::function<bool(std::shared_ptr<AbstractDataBase>)> funcfilter = nullptr;
    };
    
    DataFilter(std::shared_ptr<AbstractDataBase> dataname, const Params& params = Params{});
    virtual ~DataFilter() = default;
    
    // Override data feed interface
    void preload() override;
    bool next() override;
    void start() override;
    void stop() override;
    
    // Data access
    size_t size() const override;
    void home() override;
    
protected:
    Params p;  // Parameters
    std::shared_ptr<AbstractDataBase> dataname_;  // Underlying data source
    
private:
    bool _load();
    void copy_timeframe_info();
};

} // namespace filters
} // namespace backtrader