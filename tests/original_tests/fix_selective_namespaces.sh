#!/bin/bash

# 修复命名空间问题 - 更智能的版本

# 移除之前添加的错误的命名空间声明
for file in test_*.cpp; do
    if [ -f "$file" ]; then
        echo "Cleaning $file..."
        
        # 移除重复的using namespace声明
        sed -i '/^using namespace backtrader;$/N;/^using namespace backtrader;\nusing namespace backtrader::indicators;$/!P;D' "$file"
        
        # 检查文件是否包含SMA, EMA, RSI等indicators命名空间中的类
        if grep -q "SMA\|EMA\|RSI\|MACD\|BBands" "$file"; then
            # 这些类在indicators命名空间中
            if ! grep -q "using namespace backtrader::indicators;" "$file"; then
                sed -i '/using namespace backtrader;/a using namespace backtrader::indicators;' "$file"
            fi
        else
            # 移除indicators命名空间声明（如果存在）
            sed -i '/using namespace backtrader::indicators;/d' "$file"
        fi
    fi
done

echo "Selective namespace fixes applied."