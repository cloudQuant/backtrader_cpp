#include "store.h"
#include "dataseries.h"
#include "broker.h"
#include "cerebro.h"
#include <algorithm>

namespace backtrader {

// Static member definitions
std::shared_ptr<Store> Store::instance_ = nullptr;
std::mutex Store::instance_mutex_;
std::function<std::shared_ptr<Broker>(const std::vector<std::any>&, const std::map<std::string, std::any>&)> Store::BrokerCls;
std::function<std::shared_ptr<DataSeries>(const std::vector<std::any>&, const std::map<std::string, std::any>&)> Store::DataCls;

std::shared_ptr<Store> Store::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        // Create using shared_ptr constructor that can access private constructor
        instance_ = std::shared_ptr<Store>(new Store());
    }
    return instance_;
}

std::shared_ptr<DataSeries> Store::getdata(const std::vector<std::any>& args, 
                                          const std::map<std::string, std::any>& kwargs) {
    if (!DataCls) {
        return nullptr;
    }
    
    auto data = DataCls(args, kwargs);
    if (data) {
        // Set store reference if the DataSeries has such capability
        // This would need to be implemented in DataSeries
    }
    return data;
}

std::shared_ptr<Broker> Store::getbroker(const std::vector<std::any>& args, 
                                        const std::map<std::string, std::any>& kwargs) {
    if (!BrokerCls) {
        return nullptr;
    }
    
    auto broker = BrokerCls(args, kwargs);
    if (broker) {
        // Set store reference if the Broker has such capability
        // This would need to be implemented in Broker
    }
    return broker;
}

void Store::start(std::shared_ptr<DataSeries> data, std::shared_ptr<Broker> broker) {
    if (!started_) {
        started_ = true;
        std::lock_guard<std::mutex> lock(notifs_mutex_);
        notifs_.clear();
        datas_.clear();
        broker_ = nullptr;
    }
    
    if (data) {
        // cerebro_ = data->get_env();  // This would need to be implemented in DataSeries
        datas_.push_back(data);
        
        if (broker_ && broker_->has_method("data_started")) {
            // broker_->data_started(data);  // This would need to be implemented
        }
    } else if (broker) {
        broker_ = broker;
    }
}

void Store::stop() {
    started_ = false;
}

void Store::put_notification(const std::string& msg, 
                           const std::vector<std::any>& args, 
                           const std::map<std::string, std::any>& kwargs) {
    std::lock_guard<std::mutex> lock(notifs_mutex_);
    notifs_.emplace_back(msg, args, kwargs);
}

std::vector<Notification> Store::get_notifications() {
    std::lock_guard<std::mutex> lock(notifs_mutex_);
    
    std::vector<Notification> result;
    
    // Add a marker (empty notification) and collect all notifications until marker
    notifs_.emplace_back("", std::vector<std::any>{}, std::map<std::string, std::any>{});
    
    while (!notifs_.empty()) {
        auto notification = std::move(notifs_.front());
        notifs_.pop_front();
        
        if (notification.message.empty() && notification.args.empty() && notification.kwargs.empty()) {
            // Found the marker, stop collecting
            break;
        }
        
        result.push_back(std::move(notification));
    }
    
    return result;
}

} // namespace backtrader