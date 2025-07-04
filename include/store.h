#pragma once

#include "metabase.h"
#include <memory>
#include <vector>
#include <deque>
#include <tuple>
#include <any>
#include <mutex>

namespace backtrader {

// Forward declarations
class DataSeries;
class Broker;
class Cerebro;

// Notification structure
struct Notification {
    std::string message;
    std::vector<std::any> args;
    std::map<std::string, std::any> kwargs;
    
    Notification(const std::string& msg, 
                 const std::vector<std::any>& args = {}, 
                 const std::map<std::string, std::any>& kwargs = {})
        : message(msg), args(args), kwargs(kwargs) {}
};

// Singleton store base class
class Store {
public:
    static std::shared_ptr<Store> getInstance();
    virtual ~Store() = default;

    // Data and broker management
    virtual std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                               const std::map<std::string, std::any>& kwargs = {});
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // Lifecycle
    virtual void start(std::shared_ptr<DataSeries> data = nullptr, 
                      std::shared_ptr<Broker> broker = nullptr);
    virtual void stop();

    // Notification system
    void put_notification(const std::string& msg, 
                         const std::vector<std::any>& args = {}, 
                         const std::map<std::string, std::any>& kwargs = {});
    
    std::vector<Notification> get_notifications();

    // State
    bool is_started() const { return started_; }

protected:
    Store() = default;
    
    // Singleton instance
    static std::shared_ptr<Store> instance_;
    static std::mutex instance_mutex_;
    
    // State
    bool started_ = false;
    
    // Data and broker references
    std::vector<std::shared_ptr<DataSeries>> datas_;
    std::shared_ptr<Broker> broker_;
    std::shared_ptr<Cerebro> cerebro_;
    
    // Notification queue
    std::deque<Notification> notifs_;
    std::mutex notifs_mutex_;

    // Factory classes (to be set by derived classes)
    static std::function<std::shared_ptr<Broker>(const std::vector<std::any>&, const std::map<std::string, std::any>&)> BrokerCls;
    static std::function<std::shared_ptr<DataSeries>(const std::vector<std::any>&, const std::map<std::string, std::any>&)> DataCls;
};

// Template for creating specific store types
template<typename BrokerType, typename DataType>
class StoreTemplate : public Store {
public:
    static void registerClasses() {
        BrokerCls = [](const std::vector<std::any>& args, const std::map<std::string, std::any>& kwargs) -> std::shared_ptr<Broker> {
            return std::make_shared<BrokerType>();
        };
        
        DataCls = [](const std::vector<std::any>& args, const std::map<std::string, std::any>& kwargs) -> std::shared_ptr<DataSeries> {
            return std::make_shared<DataType>();
        };
    }
};

} // namespace backtrader