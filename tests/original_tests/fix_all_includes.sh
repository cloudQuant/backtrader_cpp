#!/bin/bash

echo "修复所有测试文件的头文件引用..."

# 修复常见的大小写问题
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/\([A-Z][a-zA-Z]*\)\.h"/#include "indicators\/\L\1.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "analyzers\/\([A-Z][a-zA-Z]*\)\.h"/#include "analyzers\/\L\1.h"/g' {} \;

# 修复特定的头文件引用
find . -name "test_*.cpp" -exec sed -i 's/#include "LineRoot\.h"/#include "lineroot.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "Common\.h"/#include "lineseries.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "strategy\/Strategy\.h"/#include "strategy.h"/g' {} \;

# 修复indicators的特定引用
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/SMA\.h"/#include "indicators\/sma.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/EMA\.h"/#include "indicators\/ema.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/RSI\.h"/#include "indicators\/rsi.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/MACD\.h"/#include "indicators\/macd.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/CrossOver\.h"/#include "indicators\/crossover.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/BollingerBands\.h"/#include "indicators\/bollinger.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/ATR\.h"/#include "indicators\/atr.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/WMA\.h"/#include "indicators\/wma.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/DEMA\.h"/#include "indicators\/dema.h"/g' {} \;
find . -name "test_*.cpp" -exec sed -i 's/#include "indicators\/Momentum\.h"/#include "indicators\/momentum.h"/g' {} \;

echo "头文件引用修复完成！"