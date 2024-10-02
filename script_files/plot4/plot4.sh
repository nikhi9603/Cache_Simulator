#!/bin/bash

# Cache sizes in KB (from 2KB to 1MB, powers of 2)
cache_sizes=(1024 2048 4096 8192 16384 32768)

# Block size
blocksizes=(16 32 64 128)

# Associativity
associativity=4

# Input trace file
trace_file="gcc_trace.txt"

# Loop through each cache size
for cache_size in "${cache_sizes[@]}"; do
  # Loop through each associativity
  for block_size in "${blocksizes[@]}"; do
    # Run the cache simulator with the current parameters
    echo "Running cache_sim with cache_size=$cache_size, assoc=$assoc, block_size=$block_size"
    ./cache_sim "$cache_size" "$associativity" "$block_size" 0 0 0 "$trace_file"
    
    # Optional: store output in a file
    # output_file="output_${cache_size}KB_assoc${assoc}.txt"
    # ./cache_sim "$cache_size" "$assoc" "$block_size" 0 0 0 "$trace_file" > "$output_file"
  done
done
