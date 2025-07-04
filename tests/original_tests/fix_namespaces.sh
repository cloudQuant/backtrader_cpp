#!/bin/bash

# 脚本：修复测试文件中的命名空间问题

# 为所有test_*.cpp文件添加必要的using namespace声明
for file in test_*.cpp; do
    if [ -f "$file" ]; then
        echo "Processing $file..."
        
        # 检查是否已经有using namespace backtrader::indicators;
        if ! grep -q "using namespace backtrader::indicators;" "$file"; then
            # 在using namespace backtrader;后面添加indicators命名空间
            sed -i '/using namespace backtrader;/a using namespace backtrader::indicators;' "$file"
        fi
        
        # 确保有基本的命名空间声明
        if ! grep -q "using namespace backtrader;" "$file"; then
            # 在第一个#include后添加命名空间声明
            sed -i '/^#include.*$/a \\nusing namespace backtrader;\nusing namespace backtrader::indicators;' "$file" | head -20
        fi
    fi
done

echo "Namespace fixes applied to all test files."