"""
Backtrader C++ - High-performance backtrader-compatible Python bindings

This module provides a backtrader-compatible API that uses C++ backend for performance.
Only functions and classes from C++ bindings are available. No Python-level implementations.
"""

import sys
import os
from pathlib import Path

# Get the directory where this __init__.py file is located
_current_dir = Path(__file__).parent

# Add the current directory to Python path to find compiled module
if str(_current_dir) not in sys.path:
    sys.path.insert(0, str(_current_dir))

# Import the compiled C++ module
try:
    from .backtrader_cpp import *
    _cpp_module_available = True
except ImportError as e:
    print(f"Error: Could not import backtrader_cpp C++ module: {e}")
    print("Make sure the module is compiled and installed correctly")
    print("No Python fallback implementations are allowed.")
    raise ImportError("backtrader_cpp C++ module is required")

# Version information
__version__ = getattr(sys.modules.get('backtrader_cpp.backtrader_cpp'), '__version__', '0.4.0')

def get_version():
    """Get version information"""
    return __version__

def __getattr__(name):
    """Only allow access to C++ bound items and submodules"""
    cpp_module = sys.modules['backtrader_cpp.backtrader_cpp']

    # Check for submodules first
    if name in ['utils', 'metabase', 'feeds', 'indicators', 'analyzers', 'observers', 'sizers', 'brokers', 'stores', 'writers']:
        if hasattr(cpp_module, name):
            return getattr(cpp_module, name)
        else:
            # Return a dummy module that raises errors
            class DummyModule:
                def __getattr__(self, attr):
                    raise AttributeError(f"Module '{name}' has no attribute '{attr}'. "
                                       f"Only C++ bound functions and classes are available.")
            return DummyModule()

    # Check for regular attributes
    if hasattr(cpp_module, name):
        return getattr(cpp_module, name)

    # Check for specific backtrader compatibility items
    if name in ['TimeFrame', 'Position', 'Order', 'LineSeries']:
        if hasattr(cpp_module, name):
            return getattr(cpp_module, name)

    raise AttributeError(f"'{__name__}' has no attribute '{name}'. "
                        f"Only C++ bound functions and classes are available.")

# For compatibility, also make it available as 'backtrader' module
sys.modules['backtrader'] = sys.modules[__name__]
