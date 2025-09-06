"""Version information for backtrader-cpp"""

__version__ = "1.0.0"
__version_info__ = tuple(int(i) for i in __version__.split("."))

# C++ core library version compatibility
CORE_VERSION = "1.0.0"
MIN_CORE_VERSION = "1.0.0"

# Python API compatibility version
API_VERSION = "2.0.0"  # Based on Python Backtrader 1.9.78+

# Build information (will be set during build process)
BUILD_DATE = ""
BUILD_COMMIT = ""
BUILD_COMPILER = ""