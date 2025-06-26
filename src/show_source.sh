#!/bin/bash

DIR="."

find "$DIR" -maxdepth 1 -type f \( -name "*.c" -o -name "*.h" -o -name "*.vs" -o -name "*.fs" \) | sort | while IFS= read -r FILE; do
    echo "===== $FILE ====="
    cat "$FILE"
    echo
done