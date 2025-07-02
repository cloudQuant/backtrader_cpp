"""
Backtrader C++ - High-performance Python bindings for backtrader
Phase 0 Prototype Implementation
"""

try:
    from .backtrader_cpp_native import *
    from .backtrader_cpp_native import core, indicators
    
    __version__ = backtrader_cpp_native.__version__
    
    # 便利导入
    from .backtrader_cpp_native.core import LineRoot, CircularBuffer, isNaN, isValid, isFinite
    from .backtrader_cpp_native.indicators import SMA, EMA
    
    # Python包装器和实用工具
    from .utils import create_line_from_list, test_performance
    
except ImportError as e:
    import warnings
    warnings.warn(f"Failed to import native C++ modules: {e}")
    
    # 提供fallback或者清晰的错误信息
    def _not_available(*args, **kwargs):
        raise RuntimeError("C++ native modules are not available. Please check the installation.")
    
    LineRoot = _not_available
    SMA = _not_available
    EMA = _not_available
    
__all__ = ['LineRoot', 'CircularBuffer', 'SMA', 'EMA', 'isNaN', 'isValid', 'isFinite',
           'create_line_from_list', 'test_performance', 'core', 'indicators']