#!/bin/bash

# Script to fix common indicator compilation issues

echo "Fixing common indicator compilation patterns..."

# Fix set_minperiod to _minperiod
find ../../src/indicators/ -name "*.cpp" -exec sed -i 's/set_minperiod/_minperiod/g' {} \;

echo "Fixed set_minperiod -> _minperiod"

# Fix Lines assignment patterns (this is more complex, so we'll do it file by file)
# The pattern is:  lines = std::make_shared<Lines>(N);
# Should be replaced with appropriate add_line calls

echo "Fixed common patterns. Manual fixes may still be needed for Lines setup."