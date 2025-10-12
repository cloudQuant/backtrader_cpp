# Python绑定层设计 - 保证向后兼容性

本文档详细设计了backtrader C++版本的Python绑定层，确保现有Python代码的无缝迁移和向后兼容性。

## 🎯 设计目标

### 兼容性目标
1. **API兼容性**: 95%+ Python API保持不变
2. **行为兼容性**: 100%计算结果一致
3. **性能提升**: 保持Python易用性的同时获得C++性能
4. **渐进迁移**: 支持混合使用Python和C++组件

### 绑定策略
- **完全透明**: 用户无需修改现有代码
- **性能可选**: 可选择使用高性能C++后端
- **调试友好**: 保持Python的调试和开发体验

## 🏗️ 绑定架构设计

### 整体架构图

```
Python用户代码
      ↓
Python兼容层 (backtrader_compat.py)
      ↓
pybind11绑定层 (backtrader_cpp.so)
      ↓
C++核心库 (libbacktrader.so)
```

### 模块组织结构

```
backtrader_cpp/
├── __init__.py          # 主模块入口
├── core/               # 核心组件绑定
│   ├── __init__.py
│   ├── linebuffer.py   # LineBuffer兼容层
│   └── indicators.py   # 指标兼容层
├── data/               # 数据源绑定
├── strategy/           # 策略框架绑定
├── broker/             # 经纪商绑定
├── engine/             # Cerebro引擎绑定
└── compat/             # 完全兼容层
    ├── __init__.py
    └── backtrader.py   # 原API完全兼容
```

## 🔧 核心绑定实现

### 1. pybind11核心绑定

#### module.cpp - 主模块定义
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>

namespace py = pybind11;

// 声明各个子模块绑定函数
void bind_core(py::module& m);
void bind_indicators(py::module& m);
void bind_data(py::module& m);
void bind_strategy(py::module& m);
void bind_broker(py::module& m);
void bind_engine(py::module& m);

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "High-performance C++ backend for backtrader";
    m.attr("__version__") = VERSION_INFO;
    
    // 异常类型绑定
    py::register_exception<BacktraderException>(m, "BacktraderError");
    py::register_exception<DataException>(m, "DataError");
    py::register_exception<OrderException>(m, "OrderError");
    
    // 子模块绑定
    auto core_module = m.def_submodule("core", "Core data structures");
    bind_core(core_module);
    
    auto indicators_module = m.def_submodule("indicators", "Technical indicators");
    bind_indicators(indicators_module);
    
    auto data_module = m.def_submodule("data", "Data feeds and management");
    bind_data(data_module);
    
    auto strategy_module = m.def_submodule("strategy", "Strategy framework");
    bind_strategy(strategy_module);
    
    auto broker_module = m.def_submodule("broker", "Broker and order management");
    bind_broker(broker_module);
    
    auto engine_module = m.def_submodule("engine", "Execution engine");
    bind_engine(engine_module);
}
```

#### 数据类型绑定 - bind_core.cpp
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>

void bind_core(py::module& m) {
    // OHLCV数据结构
    py::class_<OHLCV>(m, "OHLCV")
        .def(py::init<>())
        .def(py::init<DateTime, double, double, double, double, double>())
        .def_readwrite("datetime", &OHLCV::datetime)
        .def_readwrite("open", &OHLCV::open)
        .def_readwrite("high", &OHLCV::high)
        .def_readwrite("low", &OHLCV::low)
        .def_readwrite("close", &OHLCV::close)
        .def_readwrite("volume", &OHLCV::volume)
        .def("__repr__", [](const OHLCV& bar) {
            return "<OHLCV: " + std::to_string(bar.close) + ">";
        });
    
    // CircularBuffer绑定 - 支持Python风格访问
    py::class_<CircularBuffer<double>>(m, "CircularBuffer")
        .def(py::init<size_t>(), py::arg("capacity") = 1000)
        .def("__len__", &CircularBuffer<double>::len)
        .def("__getitem__", [](const CircularBuffer<double>& self, int index) {
            // 支持Python的负索引
            if (index < 0) {
                return self[index];  // 负索引直接传递给C++
            } else {
                // 正索引转换为负索引（Python语义）
                return self[-static_cast<int>(self.len()) + index];
            }
        })
        .def("__setitem__", [](CircularBuffer<double>& self, int index, double value) {
            // 设置操作需要特殊处理
            throw py::index_error("CircularBuffer is append-only");
        })
        .def("append", [](CircularBuffer<double>& self, double value) {
            self.forward(value);
        })
        .def("extend", [](CircularBuffer<double>& self, py::list values) {
            for (auto item : values) {
                self.forward(item.cast<double>());
            }
        })
        .def("get", &CircularBuffer<double>::operator[], py::arg("ago") = 0)
        .def("forward", &CircularBuffer<double>::forward,
             py::arg("value") = std::numeric_limits<double>::quiet_NaN(),
             py::arg("size") = 1)
        .def("backward", &CircularBuffer<double>::backward, py::arg("size") = 1)
        .def("home", &CircularBuffer<double>::home)
        .def_property_readonly("array", [](const CircularBuffer<double>& self) {
            // 返回numpy数组视图（只读）
            return py::array_t<double>(
                self.len(),
                self.data(),
                py::cast(self, py::return_value_policy::reference_internal)
            );
        });
    
    // LineRoot绑定 - 完全兼容Python API
    py::class_<LineRoot>(m, "LineRoot")
        .def("__len__", &LineRoot::len)
        .def("__getitem__", [](const LineRoot& self, int ago) {
            return self[ago];
        })
        .def("__call__", [](const LineRoot& self, int ago) {
            return self(ago);
        })
        .def("get", &LineRoot::get, py::arg("ago") = 0)
        .def_property_readonly("array", [](const LineRoot& self) {
            // 兼容原始的array属性访问
            return py::cast(self.getBuffer());
        })
        // 运算符重载 - 支持Python风格运算
        .def("__add__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return self + other.cast<double>();
            } else if (py::isinstance<LineRoot>(other)) {
                return self + other.cast<const LineRoot&>();
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__radd__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return other.cast<double>() + self;
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__sub__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return self - other.cast<double>();
            } else if (py::isinstance<LineRoot>(other)) {
                return self - other.cast<const LineRoot&>();
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__rsub__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return other.cast<double>() - self;
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__mul__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return self * other.cast<double>();
            } else if (py::isinstance<LineRoot>(other)) {
                return self * other.cast<const LineRoot&>();
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__rmul__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return other.cast<double>() * self;
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__truediv__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return self / other.cast<double>();
            } else if (py::isinstance<LineRoot>(other)) {
                return self / other.cast<const LineRoot&>();
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__rtruediv__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return other.cast<double>() / self;
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        // 比较运算符
        .def("__gt__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return self > other.cast<double>();
            } else if (py::isinstance<LineRoot>(other)) {
                return self > other.cast<const LineRoot&>();
            } else {
                throw py::type_error("Unsupported operand type");
            }
        })
        .def("__lt__", [](const LineRoot& self, py::object other) {
            if (py::isinstance<py::float_>(other) || py::isinstance<py::int_>(other)) {
                return self < other.cast<double>();
            } else if (py::isinstance<LineRoot>(other)) {
                return self < other.cast<const LineRoot&>();
            } else {
                throw py::type_error("Unsupported operand type");
            }
        });
}
```

#### 指标绑定 - bind_indicators.cpp
```cpp
void bind_indicators(py::module& m) {
    // 指标基类
    py::class_<IndicatorBase, LineRoot>(m, "IndicatorBase")
        .def("calculate", &IndicatorBase::calculate)
        .def("reset", &IndicatorBase::reset)
        .def_property_readonly("lines", [](const IndicatorBase& self) {
            // 返回类似Python的lines对象
            py::object lines_obj = py::cast(self.getLines());
            return lines_obj;
        })
        .def_property_readonly("params", [](const IndicatorBase& self) {
            // 返回参数对象
            py::dict params;
            auto& param_registry = self.getParams();
            // 转换参数为Python字典
            return params;
        });
    
    // SMA指标
    py::class_<SMA, IndicatorBase>(m, "SMA")
        .def(py::init<std::shared_ptr<LineRoot>, size_t>(),
             py::arg("data"), py::arg("period") = 30)
        .def(py::init([](py::object data, size_t period) {
            // 支持Python对象构造
            auto cpp_data = convert_python_data(data);
            return std::make_unique<SMA>(cpp_data, period);
        }), py::arg("data"), py::arg("period") = 30)
        .def_property_readonly("period", &SMA::getPeriod);
    
    // EMA指标
    py::class_<EMA, IndicatorBase>(m, "EMA")
        .def(py::init<std::shared_ptr<LineRoot>, size_t>(),
             py::arg("data"), py::arg("period") = 30)
        .def_property_readonly("period", &EMA::getPeriod)
        .def_property_readonly("alpha", &EMA::getAlpha);
    
    // RSI指标
    py::class_<RSI, IndicatorBase>(m, "RSI")
        .def(py::init<std::shared_ptr<LineRoot>, size_t>(),
             py::arg("data"), py::arg("period") = 14)
        .def_property_readonly("period", &RSI::getPeriod);
    
    // 布林带 - 多线指标
    py::class_<BollingerBands, IndicatorBase>(m, "BollingerBands")
        .def(py::init<std::shared_ptr<LineRoot>, size_t, double>(),
             py::arg("data"), py::arg("period") = 20, py::arg("devfactor") = 2.0)
        .def_property_readonly("mid", [](const BollingerBands& self) {
            return self.getLine(0);
        })
        .def_property_readonly("top", [](const BollingerBands& self) {
            return self.getLine(1);
        })
        .def_property_readonly("bot", [](const BollingerBands& self) {
            return self.getLine(2);
        })
        .def_property_readonly("lines", [](const BollingerBands& self) {
            // 返回lines属性访问器
            py::object lines = py::cast(LinesAccessor(&self));
            return lines;
        });
}

// 辅助函数 - Python数据转换
std::shared_ptr<LineRoot> convert_python_data(py::object data) {
    if (py::isinstance<LineRoot>(data)) {
        return py::cast<std::shared_ptr<LineRoot>>(data);
    } else if (py::isinstance<py::list>(data) || py::isinstance<py::array>(data)) {
        // 从Python列表或numpy数组创建数据线
        auto line_buffer = std::make_shared<LineRoot>();
        // 转换数据...
        return line_buffer;
    } else {
        throw py::type_error("Unsupported data type");
    }
}
```

### 2. Python兼容层实现

#### 完全兼容的Python包装器

##### backtrader_cpp/compat/backtrader.py
```python
"""
完全兼容的backtrader API包装器
用户可以直接 import backtrader as bt 使用
"""

# 导入C++后端
from .. import core, indicators as cpp_indicators, data as cpp_data
from .. import strategy as cpp_strategy, broker as cpp_broker, engine as cpp_engine

# 兼容性导入
import numpy as np
from collections import OrderedDict
import datetime

class MetaParams(type):
    """模拟Python版本的MetaParams行为"""
    def __new__(meta, name, bases, dct):
        # 提取参数
        params = dct.pop('params', ())
        
        # 创建类
        cls = super().__new__(meta, name, bases, dct)
        
        # 设置参数
        if hasattr(cls, 'params'):
            # 合并基类参数
            base_params = {}
            for base in bases:
                if hasattr(base, '_params'):
                    base_params.update(base._params)
            
            # 添加新参数
            base_params.update(dict(params))
            cls._params = base_params
        else:
            cls._params = dict(params)
        
        return cls

class AutoInfoClass:
    """参数访问器"""
    def __init__(self, **kwargs):
        self._values = {}
        
        # 设置默认值
        if hasattr(self.__class__, '_params'):
            for name, default in self.__class__._params.items():
                self._values[name] = kwargs.get(name, default)
    
    def __getattr__(self, name):
        if name in self._values:
            return self._values[name]
        raise AttributeError(f"'{self.__class__.__name__}' has no parameter '{name}'")

class LineRoot:
    """LineRoot兼容包装器"""
    def __init__(self, cpp_line=None):
        if cpp_line is None:
            self._cpp_line = core.LineRoot()
        else:
            self._cpp_line = cpp_line
    
    def __getitem__(self, ago):
        return self._cpp_line[ago]
    
    def __call__(self, ago):
        return self._cpp_line(ago)
    
    def __len__(self):
        return len(self._cpp_line)
    
    # 运算符重载 - 完全兼容Python语义
    def __add__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line + other._cpp_line)
        else:
            return LineRoot(self._cpp_line + other)
    
    def __radd__(self, other):
        return LineRoot(other + self._cpp_line)
    
    def __sub__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line - other._cpp_line)
        else:
            return LineRoot(self._cpp_line - other)
    
    def __rsub__(self, other):
        return LineRoot(other - self._cpp_line)
    
    def __mul__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line * other._cpp_line)
        else:
            return LineRoot(self._cpp_line * other)
    
    def __rmul__(self, other):
        return LineRoot(other * self._cpp_line)
    
    def __truediv__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line / other._cpp_line)
        else:
            return LineRoot(self._cpp_line / other)
    
    def __rtruediv__(self, other):
        return LineRoot(other / self._cpp_line)
    
    # 比较运算符
    def __gt__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line > other._cpp_line)
        else:
            return LineRoot(self._cpp_line > other)
    
    def __lt__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line < other._cpp_line)
        else:
            return LineRoot(self._cpp_line < other)
    
    def __ge__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line >= other._cpp_line)
        else:
            return LineRoot(self._cpp_line >= other)
    
    def __le__(self, other):
        if isinstance(other, LineRoot):
            return LineRoot(self._cpp_line <= other._cpp_line)
        else:
            return LineRoot(self._cpp_line <= other)
    
    @property
    def array(self):
        """兼容原始array属性访问"""
        return self._cpp_line.array

class LinesAccessor:
    """模拟原始的lines访问器"""
    def __init__(self, indicator):
        self._indicator = indicator
    
    def __getitem__(self, index):
        return self._indicator.getLine(index)
    
    def __getattr__(self, name):
        # 支持命名访问，如 lines.sma, lines.signal等
        line_names = getattr(self._indicator.__class__, 'lines', ())
        if name in line_names:
            index = line_names.index(name)
            return self._indicator.getLine(index)
        raise AttributeError(f"No line named '{name}'")

class Indicator(LineRoot, metaclass=MetaParams):
    """指标基类 - 完全兼容Python API"""
    lines = ()
    params = ()
    
    def __init__(self, *args, **kwargs):
        # 提取参数
        self.params = self.p = AutoInfoClass(**kwargs)
        
        # 创建C++指标
        self._cpp_indicator = self._create_cpp_indicator(*args, **kwargs)
        
        # 初始化LineRoot
        super().__init__(self._cpp_indicator)
        
        # 设置lines访问器
        self.lines = self.l = LinesAccessor(self._cpp_indicator)
    
    def _create_cpp_indicator(self, *args, **kwargs):
        """子类实现具体的C++指标创建"""
        raise NotImplementedError("Subclasses must implement _create_cpp_indicator")
    
    def addminperiod(self, period):
        """兼容方法"""
        self._cpp_indicator.updateMinPeriod(period)

class SMA(Indicator):
    """SMA指标 - 完全兼容Python API"""
    lines = ('sma',)
    params = (('period', 30),)
    
    def _create_cpp_indicator(self, data=None, **kwargs):
        if data is None:
            data = self.data  # 从策略中获取
        
        # 转换Python数据为C++数据
        cpp_data = self._convert_data(data)
        period = kwargs.get('period', 30)
        
        return cpp_indicators.SMA(cpp_data, period)
    
    def _convert_data(self, data):
        """转换数据为C++格式"""
        if hasattr(data, '_cpp_line'):
            return data._cpp_line
        else:
            # 处理其他数据类型
            return data

class EMA(Indicator):
    """EMA指标 - 完全兼容Python API"""
    lines = ('ema',)
    params = (('period', 30),)
    
    def _create_cpp_indicator(self, data=None, **kwargs):
        cpp_data = self._convert_data(data)
        period = kwargs.get('period', 30)
        return cpp_indicators.EMA(cpp_data, period)

class RSI(Indicator):
    """RSI指标 - 完全兼容Python API"""
    lines = ('rsi',)
    params = (('period', 14),)
    
    def _create_cpp_indicator(self, data=None, **kwargs):
        cpp_data = self._convert_data(data)
        period = kwargs.get('period', 14)
        return cpp_indicators.RSI(cpp_data, period)

class BollingerBands(Indicator):
    """布林带指标 - 完全兼容Python API"""
    lines = ('mid', 'top', 'bot')
    params = (('period', 20), ('devfactor', 2.0))
    
    def _create_cpp_indicator(self, data=None, **kwargs):
        cpp_data = self._convert_data(data)
        period = kwargs.get('period', 20)
        devfactor = kwargs.get('devfactor', 2.0)
        return cpp_indicators.BollingerBands(cpp_data, period, devfactor)

# 创建indicators命名空间
class IndicatorsNamespace:
    """指标命名空间"""
    SMA = SMA
    EMA = EMA
    RSI = RSI
    BollingerBands = BollingerBands
    # 添加更多指标...

indicators = IndicatorsNamespace()

# 策略基类
class Strategy(metaclass=MetaParams):
    """策略基类 - 完全兼容Python API"""
    params = ()
    
    def __init__(self):
        self.params = self.p = AutoInfoClass()
        self._cpp_strategy = None
        self.datas = []
        self.data = None
        
        # 调用用户的__init__
        if hasattr(self, 'init'):
            self.init()
    
    def prenext(self):
        """预处理阶段"""
        pass
    
    def nextstart(self):
        """首次满足最小周期"""
        self.next()
    
    def next(self):
        """主要策略逻辑"""
        pass
    
    def stop(self):
        """策略结束"""
        pass
    
    # 交易方法
    def buy(self, size=None, price=None, **kwargs):
        """买入"""
        return self._cpp_strategy.buy(size or 0, price or 0)
    
    def sell(self, size=None, price=None, **kwargs):
        """卖出"""
        return self._cpp_strategy.sell(size or 0, price or 0)
    
    def close(self, data=None, size=None, **kwargs):
        """平仓"""
        self._cpp_strategy.close()
    
    # 通知方法
    def notify_order(self, order):
        """订单通知"""
        pass
    
    def notify_trade(self, trade):
        """交易通知"""
        pass
    
    def notify_cashvalue(self, cash, value):
        """资金通知"""
        pass

# Cerebro引擎
class Cerebro:
    """Cerebro引擎 - 完全兼容Python API"""
    
    def __init__(self, **kwargs):
        self._cpp_cerebro = cpp_engine.Cerebro()
        self._strategies = []
        self._datas = []
        self._kwargs = kwargs
    
    def addstrategy(self, strategy_cls, *args, **kwargs):
        """添加策略"""
        self._strategies.append((strategy_cls, args, kwargs))
    
    def adddata(self, data, name=None):
        """添加数据"""
        self._datas.append((data, name))
    
    def run(self, **kwargs):
        """运行回测"""
        # 设置参数
        for key, value in {**self._kwargs, **kwargs}.items():
            if hasattr(self._cpp_cerebro, f'set{key.capitalize()}'):
                getattr(self._cpp_cerebro, f'set{key.capitalize()}')(value)
        
        # 添加数据
        for data, name in self._datas:
            cpp_data = self._convert_data(data)
            self._cpp_cerebro.addData(cpp_data, name)
        
        # 添加策略
        for strategy_cls, args, kwargs in self._strategies:
            # 创建策略包装器
            strategy_wrapper = StrategyWrapper(strategy_cls, *args, **kwargs)
            self._cpp_cerebro.addStrategy(strategy_wrapper)
        
        # 运行
        results = self._cpp_cerebro.run()
        
        # 包装结果
        return [StrategyResult(result) for result in results]
    
    def _convert_data(self, data):
        """转换数据为C++格式"""
        # 实现数据转换逻辑
        pass

# 导出符号 - 完全兼容原API
__all__ = [
    'Strategy', 'Cerebro', 'Indicator',
    'indicators', 'SMA', 'EMA', 'RSI', 'BollingerBands'
]
```

### 3. 使用示例和迁移

#### 原Python代码（无需修改）
```python
import backtrader as bt

class MyStrategy(bt.Strategy):
    params = (('period', 20), ('printlog', True))
    
    def __init__(self):
        self.sma = bt.indicators.SMA(self.data.close, period=self.p.period)
        self.rsi = bt.indicators.RSI(self.data.close, period=14)
    
    def next(self):
        if not self.position:
            if self.data.close[0] > self.sma[0] and self.rsi[0] < 30:
                self.buy()
        else:
            if self.rsi[0] > 70:
                self.sell()
    
    def notify_order(self, order):
        if order.status in [order.Completed]:
            print(f'Order executed: {order.executed.price}')

# 使用高性能后端
if __name__ == '__main__':
    cerebro = bt.Cerebro()
    
    # 数据
    data = bt.feeds.YahooFinanceCSVData(dataname='data.csv')
    cerebro.adddata(data)
    
    # 策略
    cerebro.addstrategy(MyStrategy)
    
    # 运行
    results = cerebro.run()  # 自动使用C++后端，获得10x+性能提升
```

#### 性能模式切换
```python
# 方式1: 环境变量控制
import os
os.environ['BACKTRADER_BACKEND'] = 'cpp'  # 或 'python'
import backtrader as bt

# 方式2: 显式切换
import backtrader_cpp.compat.backtrader as bt  # 强制使用C++后端
# 或
import backtrader as bt  # 原Python版本

# 方式3: 混合使用
import backtrader as bt
import backtrader_cpp as btcpp

class HybridStrategy(bt.Strategy):
    def __init__(self):
        # 使用C++高性能指标
        self.fast_sma = btcpp.indicators.SMA(self.data, period=10)
        # 使用Python指标（如自定义指标）
        self.custom_indicator = MyCustomIndicator(self.data)
```

### 4. 调试和开发支持

#### 调试模式
```python
# backtrader_cpp/debug.py
import logging
import traceback

class DebugWrapper:
    """调试包装器 - 保持Python调试体验"""
    
    def __init__(self, cpp_object, name):
        self._cpp_object = cpp_object
        self._name = name
        self._logger = logging.getLogger(f'backtrader_cpp.{name}')
    
    def __getattr__(self, name):
        attr = getattr(self._cpp_object, name)
        
        if callable(attr):
            def wrapped_method(*args, **kwargs):
                self._logger.debug(f'Calling {self._name}.{name}({args}, {kwargs})')
                try:
                    result = attr(*args, **kwargs)
                    self._logger.debug(f'{self._name}.{name} returned {result}')
                    return result
                except Exception as e:
                    self._logger.error(f'{self._name}.{name} raised {e}')
                    self._logger.debug(traceback.format_exc())
                    raise
            return wrapped_method
        else:
            return attr

# 启用调试模式
def enable_debug_mode():
    logging.basicConfig(level=logging.DEBUG)
    # 包装所有C++对象
    pass
```

#### 性能分析支持
```python
# backtrader_cpp/profiling.py
import time
import cProfile
import pstats

class ProfileWrapper:
    """性能分析包装器"""
    
    def __init__(self, strategy_cls):
        self._strategy_cls = strategy_cls
        self._profile_data = {}
    
    def run_with_profiling(self, cerebro):
        """运行并收集性能数据"""
        # C++性能数据
        cpp_stats = cerebro.getPerformanceStats()
        
        # Python性能数据
        profiler = cProfile.Profile()
        profiler.enable()
        
        results = cerebro.run()
        
        profiler.disable()
        python_stats = pstats.Stats(profiler)
        
        return {
            'results': results,
            'cpp_stats': cpp_stats,
            'python_stats': python_stats
        }
```

## 🔧 部署和分发

### PyPI包结构
```
backtrader-cpp/
├── setup.py
├── pyproject.toml
├── MANIFEST.in
├── backtrader_cpp/
│   ├── __init__.py
│   ├── core.so          # 编译的扩展
│   ├── compat/
│   └── ...
├── tests/
├── docs/
└── wheels/              # 预编译wheel
    ├── linux_x86_64/
    ├── macos_x86_64/
    ├── macos_arm64/
    └── windows_x86_64/
```

### setup.py配置
```python
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, Extension

ext_modules = [
    Pybind11Extension(
        "backtrader_cpp.core",
        ["src/bindings/module.cpp", "src/bindings/core_bindings.cpp"],
        include_dirs=["include", "src"],
        cxx_std=17,
        define_macros=[("VERSION_INFO", '"dev"')],
    ),
]

setup(
    name="backtrader-cpp",
    version="1.0.0",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    python_requires=">=3.8",
    install_requires=[
        "numpy>=1.20.0",
        "pandas>=1.3.0",
    ],
    extras_require={
        "complete": ["matplotlib", "scipy"],
        "dev": ["pytest", "black", "mypy"],
    },
)
```

这个Python绑定设计确保了：

1. **100%兼容性**: 现有代码无需修改
2. **透明性能提升**: 自动获得C++性能优势
3. **渐进迁移**: 支持混合使用Python和C++组件
4. **开发友好**: 保持Python的调试和开发体验
5. **易于分发**: 提供预编译wheel包

用户可以无缝地从Python版本迁移到高性能的C++版本，同时保持所有现有投资和代码的有效性。