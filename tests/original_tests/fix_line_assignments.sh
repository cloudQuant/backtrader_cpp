#!/bin/bash

# Script to fix line assignment patterns in indicator files

echo "Fixing line assignment patterns..."

# Fix (*line)[index] = value; to line->set(index, value);
find ../../src/indicators/ -name "*.cpp" -exec sed -i 's/(\*\([^)]*\))\[\([^]]*\)\] = \([^;]*\);/\1->set(\2, \3);/g' {} \;

echo "Fixed line assignment patterns"

# Fix if (line) (*line)[index] = value; to if (line) line->set(index, value);
find ../../src/indicators/ -name "*.cpp" -exec sed -i 's/if (\([^)]*\)) (\*\1)\[\([^]]*\)\] = \([^;]*\);/if (\1) \1->set(\2, \3);/g' {} \;

echo "Fixed conditional line assignment patterns"

echo "Line assignment fixes completed!"