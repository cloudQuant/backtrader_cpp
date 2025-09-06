#!/usr/bin/env python3
"""
Pytest configuration for Python Bindings tests
"""

import sys
import os
from pathlib import Path

# Add the src directory to Python path
project_root = Path(__file__).parent.parent
src_dir = project_root / 'src'

if str(src_dir) not in sys.path:
    sys.path.insert(0, str(src_dir))

# Alternative: add build directory if src doesn't work
build_dir = project_root / 'build'
if str(build_dir) not in sys.path:
    sys.path.insert(0, str(build_dir))

def pytest_configure(config):
    """Configure pytest with custom markers"""
    config.addinivalue_line(
        "markers", "slow: marks tests as slow (deselect with '-m \"not slow\"')"
    )
    config.addinivalue_line(
        "markers", "integration: marks tests as integration tests"
    )
    config.addinivalue_line(
        "markers", "unit: marks tests as unit tests"
    )

def pytest_collection_modifyitems(config, items):
    """Modify test collection to add markers based on file names"""
    for item in items:
        # Mark integration tests
        if 'compatibility' in item.nodeid or 'strategy' in item.nodeid:
            item.add_marker('integration')

        # Mark unit tests for indicators
        if 'indicator' in item.nodeid or any(ind in item.nodeid for ind in ['sma', 'rsi', 'macd', 'bbands']):
            item.add_marker('unit')

        # Mark slow tests
        if 'performance' in item.nodeid or 'stress' in item.nodeid:
            item.add_marker('slow')
