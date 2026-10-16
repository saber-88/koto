#!/bin/bash

BUILD_DIR="build"
TESTS=$(find $BUILD_DIR -maxdepth 1 -type f -name "test*" | sort)

if [ -z "$TESTS" ]; then
    echo "No tests found. Run cmake --build build first."
    exit 1
fi

for TEST in $TESTS; do
    TEST_NAME=$(basename $TEST)
    echo "─────────────────────────────────"
    echo "Running: $TEST_NAME"
    echo "Press Enter to start, Ctrl+C to skip to next"
    read
    $TEST
    echo ""
    echo "[$TEST_NAME] done — press Enter for next"
    read
done

echo "─────────────────────────────────"
echo "All tests complete."