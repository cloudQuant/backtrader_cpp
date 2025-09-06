/**
 * @file cerebro_bindings.cpp
 * @brief Cerebro engine bindings for backtrader-cpp
 * 
 * This file defines Python bindings for the Cerebro class, which is the main
 * orchestration engine for backtesting and live trading.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "cerebro.h"
#include "strategy.h"
#include "dataseries.h"
#include "analyzer.h"
#include "broker.h"

namespace py = pybind11;
using namespace pybind11::literals;

// Python strategy adapter to bridge Python strategy classes with C++
class PythonStrategyAdapter : public backtrader::Strategy {
private:
    py::object python_strategy_class_;
    py::dict strategy_params_;
    py::object strategy_instance_;
    
public:
    PythonStrategyAdapter(py::object strategy_class, py::dict params) 
        : python_strategy_class_(strategy_class), strategy_params_(params) {
        
        // Create strategy instance with parameters
        if (params.empty()) {
            strategy_instance_ = strategy_class();
        } else {
            strategy_instance_ = strategy_class(**params);
        }
        
        // Set up data access for Python strategy
        setup_python_data_access();
    }
    
    void init() override {
        // Call Python strategy's init method if it exists
        if (py::hasattr(strategy_instance_, "init")) {
            try {
                strategy_instance_.attr("init")();
            } catch (const py::error_already_set& e) {
                throw std::runtime_error("Error in Python strategy init(): " + std::string(e.what()));
            }
        }
    }
    
    void next() override {
        // Update Python strategy's data references
        update_python_data_references();
        
        // Call Python strategy's next method
        if (py::hasattr(strategy_instance_, "next")) {
            try {
                strategy_instance_.attr("next")();
            } catch (const py::error_already_set& e) {
                throw std::runtime_error("Error in Python strategy next(): " + std::string(e.what()));
            }
        }
    }
    
    void start() override {
        if (py::hasattr(strategy_instance_, "start")) {
            try {
                strategy_instance_.attr("start")();
            } catch (const py::error_already_set& e) {
                throw std::runtime_error("Error in Python strategy start(): " + std::string(e.what()));
            }
        }
    }
    
    void stop() override {
        if (py::hasattr(strategy_instance_, "stop")) {
            try {
                strategy_instance_.attr("stop")();
            } catch (const py::error_already_set& e) {
                throw std::runtime_error("Error in Python strategy stop(): " + std::string(e.what()));
            }
        }
    }
    
    void notify_order(std::shared_ptr<backtrader::Order> order) override {
        if (py::hasattr(strategy_instance_, "notify_order")) {
            try {
                strategy_instance_.attr("notify_order")(order);
            } catch (const py::error_already_set& e) {
                throw std::runtime_error("Error in Python strategy notify_order(): " + std::string(e.what()));
            }
        }
    }
    
    void notify_trade(std::shared_ptr<backtrader::Trade> trade) override {
        if (py::hasattr(strategy_instance_, "notify_trade")) {
            try {
                strategy_instance_.attr("notify_trade")(trade);
            } catch (const py::error_already_set& e) {
                throw std::runtime_error("Error in Python strategy notify_trade(): " + std::string(e.what()));
            }
        }
    }
    
    py::object get_python_instance() const {
        return strategy_instance_;
    }
    
private:
    void setup_python_data_access() {
        // Set up data attribute for Python strategy to access C++ data
        if (py::hasattr(strategy_instance_, "__dict__")) {
            auto strategy_dict = strategy_instance_.attr("__dict__");
            // Add data reference (will be set when data is added to strategy)
            strategy_dict["data"] = py::none();
            strategy_dict["datas"] = py::list();
            
            // Add broker reference  
            strategy_dict["broker"] = py::none();
            
            // Add utility methods
            strategy_dict["buy"] = py::cpp_function([this](py::kwargs kwargs) {
                return this->handle_buy_order(kwargs);
            });
            
            strategy_dict["sell"] = py::cpp_function([this](py::kwargs kwargs) {
                return this->handle_sell_order(kwargs);
            });
            
            strategy_dict["close"] = py::cpp_function([this](py::kwargs kwargs) {
                return this->handle_close_order(kwargs);
            });
            
            strategy_dict["getposition"] = py::cpp_function([this](py::object data) {
                return this->handle_get_position(data);
            });
        }
    }
    
    void update_python_data_references() {
        if (py::hasattr(strategy_instance_, "__dict__")) {
            auto strategy_dict = strategy_instance_.attr("__dict__");
            
            // Update data reference
            if (!datas.empty()) {
                strategy_dict["data"] = datas[0];
                
                py::list py_datas;
                for (auto& data : datas) {
                    py_datas.append(data);
                }
                strategy_dict["datas"] = py_datas;
            }
            
            // Update broker reference
            strategy_dict["broker"] = broker_ptr();
        }
    }
    
    std::shared_ptr<backtrader::Order> handle_buy_order(py::kwargs kwargs) {
        double size = kwargs.contains("size") ? kwargs["size"].cast<double>() : 0.0;
        double price = kwargs.contains("price") ? kwargs["price"].cast<double>() : 0.0;
        
        if (size == 0.0 && price == 0.0) {
            return buy();
        } else if (price == 0.0) {
            return buy(size);
        } else {
            return buy(size, price);
        }
    }
    
    std::shared_ptr<backtrader::Order> handle_sell_order(py::kwargs kwargs) {
        double size = kwargs.contains("size") ? kwargs["size"].cast<double>() : 0.0;
        double price = kwargs.contains("price") ? kwargs["price"].cast<double>() : 0.0;
        
        if (size == 0.0 && price == 0.0) {
            return sell();
        } else if (price == 0.0) {
            return sell(size);
        } else {
            return sell(size, price);
        }
    }
    
    std::shared_ptr<backtrader::Order> handle_close_order(py::kwargs kwargs) {
        return close();
    }
    
    double handle_get_position(py::object data) {
        if (data.is_none()) {
            return getposition();
        } else {
            auto data_series = data.cast<std::shared_ptr<backtrader::DataSeries>>();
            return getposition(data_series);
        }
    }
};

// Python analyzer adapter
class PythonAnalyzerAdapter : public backtrader::Analyzer {
private:
    py::object python_analyzer_class_;
    py::dict analyzer_params_;
    py::object analyzer_instance_;
    
public:
    PythonAnalyzerAdapter(py::object analyzer_class, py::dict params)
        : python_analyzer_class_(analyzer_class), analyzer_params_(params) {
        
        if (params.empty()) {
            analyzer_instance_ = analyzer_class();
        } else {
            analyzer_instance_ = analyzer_class(**params);
        }
    }
    
    void start() override {
        if (py::hasattr(analyzer_instance_, "start")) {
            analyzer_instance_.attr("start")();
        }
    }
    
    void next() override {
        if (py::hasattr(analyzer_instance_, "next")) {
            analyzer_instance_.attr("next")();
        }
    }
    
    void stop() override {
        if (py::hasattr(analyzer_instance_, "stop")) {
            analyzer_instance_.attr("stop")();
        }
    }
    
    py::object get_analysis() override {
        if (py::hasattr(analyzer_instance_, "get_analysis")) {
            return analyzer_instance_.attr("get_analysis")();
        }
        return py::dict();
    }
    
    py::object get_python_instance() const {
        return analyzer_instance_;
    }
};

void bind_cerebro(py::module& m) {
    // Main Cerebro class
    py::class_<backtrader::Cerebro>(m, "Cerebro", 
        "Main engine for backtesting and live trading")
        .def(py::init<>(),
             "Create a new Cerebro instance")
        
        // Data management
        .def("adddata", 
             [](backtrader::Cerebro& self, std::shared_ptr<backtrader::DataSeries> data, py::kwargs kwargs) {
                 std::string name = kwargs.contains("name") ? 
                     kwargs["name"].cast<std::string>() : "";
                 return self.adddata(data, name);
             },
             "Add a data feed to the cerebro",
             py::arg("data"),
             R"(
             Add a data feed to the cerebro engine.
             
             Parameters:
                 data: DataSeries object containing OHLCV data
                 name: Optional name for the data feed
                 
             Returns:
                 Data feed ID for reference
             )")
        
        // Strategy management
        .def("addstrategy",
             [](backtrader::Cerebro& self, py::object strategy_class, py::kwargs kwargs) {
                 // Create Python strategy adapter
                 py::dict params;
                 for (auto item : kwargs) {
                     params[item.first] = item.second;
                 }
                 
                 auto adapter = std::make_shared<PythonStrategyAdapter>(strategy_class, params);
                 return self.addstrategy(adapter);
             },
             "Add a strategy class to cerebro",
             py::arg("strategy_class"),
             R"(
             Add a strategy to the cerebro engine.
             
             Parameters:
                 strategy_class: Python strategy class
                 **kwargs: Strategy parameters
                 
             Returns:
                 Strategy ID for reference
             )")
        
        // Analyzer management
        .def("addanalyzer",
             [](backtrader::Cerebro& self, py::object analyzer_class, py::kwargs kwargs) {
                 py::dict params;
                 for (auto item : kwargs) {
                     params[item.first] = item.second;
                 }
                 
                 auto adapter = std::make_shared<PythonAnalyzerAdapter>(analyzer_class, params);
                 return self.addanalyzer(adapter);
             },
             "Add an analyzer to cerebro",
             py::arg("analyzer_class"),
             R"(
             Add an analyzer to track strategy performance.
             
             Parameters:
                 analyzer_class: Python analyzer class
                 **kwargs: Analyzer parameters
                 
             Returns:
                 Analyzer ID for reference
             )")
        
        // Execution control
        .def("run",
             [](backtrader::Cerebro& self, py::kwargs kwargs) {
                 bool runonce = kwargs.contains("runonce") ? 
                     kwargs["runonce"].cast<bool>() : true;
                 bool preload = kwargs.contains("preload") ? 
                     kwargs["preload"].cast<bool>() : true;
                 int maxcpus = kwargs.contains("maxcpus") ? 
                     kwargs["maxcpus"].cast<int>() : 1;
                 
                 self.setRunOnce(runonce);
                 self.setPreload(preload);
                 // TODO: Set maxcpus when multi-threading is implemented
                 
                 return self.run();
             },
             "Run the backtest",
             R"(
             Execute the backtest with configured strategies and data.
             
             Parameters:
                 runonce: Use vectorized execution (default: True)
                 preload: Preload all data (default: True)  
                 maxcpus: Maximum CPU cores to use (default: 1)
                 
             Returns:
                 List of strategy instances with results
             )")
        
        // Configuration methods
        .def("setRunOnce", &backtrader::Cerebro::setRunOnce,
             "Set runonce mode for vectorized execution",
             py::arg("runonce"))
        
        .def("setPreload", &backtrader::Cerebro::setPreload,
             "Set preload mode for data loading",
             py::arg("preload"))
        
        // Broker access
        .def("broker", &backtrader::Cerebro::broker,
             "Get the broker instance",
             py::return_value_policy::reference,
             "Get reference to the broker for cash/value operations")
        
        // Plotting integration (placeholder for future implementation)
        .def("plot",
             [](backtrader::Cerebro& self, py::kwargs kwargs) {
                 // TODO: Implement plotting functionality
                 py::print("Plotting not yet implemented in C++ version");
                 py::print("Consider using Python matplotlib with result data");
                 return py::none();
             },
             "Plot the backtest results",
             R"(
             Plot strategy results and indicators.
             
             Note: Full plotting functionality is planned for future release.
             For now, extract data and use Python matplotlib directly.
             )")
        
        // Optimization support (placeholder)
        .def("optstrategy",
             [](backtrader::Cerebro& self, py::object strategy_class, py::kwargs kwargs) {
                 // TODO: Implement strategy optimization
                 throw std::runtime_error("Strategy optimization not yet implemented");
             },
             "Add strategy for optimization",
             py::arg("strategy_class"),
             "Add a strategy with parameter ranges for optimization")
        
        // Status and information
        .def("__repr__", [](const backtrader::Cerebro& self) {
             return "<Cerebro strategies=" + std::to_string(0) + // TODO: Get actual count
                    " datas=" + std::to_string(0) + 
                    " analyzers=" + std::to_string(0) + ">";
         });

    // Convenience functions for common operations
    m.def("quickstart",
          [](std::shared_ptr<backtrader::DataSeries> data, py::object strategy_class, py::kwargs kwargs) {
              auto cerebro = std::make_unique<backtrader::Cerebro>();
              cerebro->adddata(data);
              
              py::dict params;
              for (auto item : kwargs) {
                  params[item.first] = item.second;
              }
              
              auto adapter = std::make_shared<PythonStrategyAdapter>(strategy_class, params);
              cerebro->addstrategy(adapter);
              
              return cerebro->run();
          },
          "Quick backtest with minimal setup",
          py::arg("data"), py::arg("strategy_class"),
          R"(
          Run a quick backtest with minimal configuration.
          
          Parameters:
              data: DataSeries with OHLCV data
              strategy_class: Python strategy class
              **kwargs: Strategy parameters
              
          Returns:
              Strategy results list
          )");
}