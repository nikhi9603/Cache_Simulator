#!/bin/bash

# Cache sizes in KB (from 2KB to 1MB, powers of 2)
cache_sizes=(2048 4096 8192 16384 32768 65536 131072)

# Associativity values to test
associativities=(1 2 4 8)

# Block size
block_size=32

# Input trace file
trace_file="gcc_trace.txt"

# Loop through each cache size
for cache_size in "${cache_sizes[@]}"; do
  # Loop through each associativity
  for assoc in "${associativities[@]}"; do
    # Run the cache simulator with the current parameters
    echo "Running cache_sim with cache_size=$cache_size, assoc=$assoc, block_size=$block_size"
    ./cache_sim "$cache_size" "$assoc" "$block_size" 0 262144 8 "$trace_file"
    
    # Optional: store output in a file
    # output_file="output_${cache_size}KB_assoc${assoc}.txt"
    # ./cache_sim "$cache_size" "$assoc" "$block_size" 0 0 0 "$trace_file" > "$output_file"
  done
done
