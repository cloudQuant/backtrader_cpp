#!/bin/bash

# Script to fix include paths in test files

echo "Fixing include paths in test files..."

# Fix common include replacements
for file in test_ind_*.cpp; do
    if [ -f "$file" ]; then
        echo "Processing $file..."
        
        # Replace test_common.h with test_common_simple.h
        sed -i 's/#include "test_common\.h"/#include "test_common_simple.h"/g' "$file"
        
        # Fix capitalized indicator includes
        sed -i 's/#include "indicators\/SMA\.h"/#include "indicators\/sma.h"/g' "$file"
        sed -i 's/#include "indicators\/EMA\.h"/#include "indicators\/ema.h"/g' "$file"
        sed -i 's/#include "indicators\/RSI\.h"/#include "indicators\/rsi.h"/g' "$file"
        sed -i 's/#include "indicators\/MACD\.h"/#include "indicators\/macd.h"/g' "$file"
        sed -i 's/#include "indicators\/BollingerBands\.h"/#include "indicators\/bollinger.h"/g' "$file"
        sed -i 's/#include "indicators\/ATR\.h"/#include "indicators\/atr.h"/g' "$file"
        sed -i 's/#include "indicators\/Stochastic\.h"/#include "indicators\/stochastic.h"/g' "$file"
        sed -i 's/#include "indicators\/CCI\.h"/#include "indicators\/cci.h"/g' "$file"
        sed -i 's/#include "indicators\/DEMA\.h"/#include "indicators\/dema.h"/g' "$file"
        sed -i 's/#include "indicators\/TEMA\.h"/#include "indicators\/tema.h"/g' "$file"
        sed -i 's/#include "indicators\/KAMA\.h"/#include "indicators\/kama.h"/g' "$file"
        sed -i 's/#include "indicators\/TRIX\.h"/#include "indicators\/trix.h"/g' "$file"
        sed -i 's/#include "indicators\/TSI\.h"/#include "indicators\/tsi.h"/g' "$file"
        sed -i 's/#include "indicators\/Momentum\.h"/#include "indicators\/momentum.h"/g' "$file"
        sed -i 's/#include "indicators\/Williams\.h"/#include "indicators\/williams.h"/g' "$file"
        sed -i 's/#include "indicators\/Aroon\.h"/#include "indicators\/aroon.h"/g' "$file"
        sed -i 's/#include "indicators\/UltimateOscillator\.h"/#include "indicators\/ultimateoscillator.h"/g' "$file"
        sed -i 's/#include "indicators\/Ichimoku\.h"/#include "indicators\/ichimoku.h"/g' "$file"
        sed -i 's/#include "indicators\/WMA\.h"/#include "indicators\/wma.h"/g' "$file"
        sed -i 's/#include "indicators\/HMA\.h"/#include "indicators\/hma.h"/g' "$file"
        sed -i 's/#include "indicators\/SMMA\.h"/#include "indicators\/smma.h"/g' "$file"
        sed -i 's/#include "indicators\/DMA\.h"/#include "indicators\/dma.h"/g' "$file"
        sed -i 's/#include "indicators\/DPO\.h"/#include "indicators\/dpo.h"/g' "$file"
        sed -i 's/#include "indicators\/Envelope\.h"/#include "indicators\/envelope.h"/g' "$file"
        sed -i 's/#include "indicators\/ROC\.h"/#include "indicators\/roc.h"/g' "$file"
        
        echo "  Fixed $file"
    fi
done

echo "Include path fixes completed!"