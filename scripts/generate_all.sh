#!/bin/bash

# Output file
OUTPUT="EventCore_consolidated.txt"

# Empty the output file if it exists
> "$OUTPUT"

# Recursively find all files
find . -type f | sort | while read -r file; do
    echo "────────────────────────────────────────────" >> "$OUTPUT"
    echo "File: $file" >> "$OUTPUT"
    echo "────────────────────────────────────────────" >> "$OUTPUT"
    cat "$file" >> "$OUTPUT"
    echo -e "\n\n" >> "$OUTPUT"
done

echo "All files consolidated into $OUTPUT"

