#!/bin/bash

export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

echo "=== Running All Compiled Tests ==="
echo "Date: $(date)"
echo

total=0
passed=0
failed=0

# Run each test
for test in test_*; do
    if [[ -x "$test" && ! "$test" =~ \.(cpp|o|txt|log|sh)$ ]]; then
        echo -n "Running $test... "
        total=$((total + 1))
        
        if timeout 10s ./"$test" >/dev/null 2>&1; then
            echo "PASSED"
            passed=$((passed + 1))
        else
            echo "FAILED"
            failed=$((failed + 1))
        fi
    fi
done

echo
echo "=== Test Summary ==="
echo "Total tests: $total"
echo "Passed: $passed ($(( passed * 100 / total ))%)"
echo "Failed: $failed ($(( failed * 100 / total ))%)"
echo

if [[ $total -eq 84 ]]; then
    echo "✓ All 84 test files compiled and executed!"
else
    echo "Note: Expected 84 tests, found $total"
fi