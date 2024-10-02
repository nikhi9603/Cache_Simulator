#!/bin/bash

# Cache sizes in KB (from 2KB to 1MB, powers of 2)
cache_sizes=(1024 2048 4096 8192 16384 32768)

# Block size
block_size=32

# Associativity
associativity=1

vc_blocks=(0 4 8 16)

# Input trace file
trace_file="gcc_trace.txt"

# Loop through each cache size
for cache_size in "${cache_sizes[@]}"; do
  # Loop through each associativity
  for vc in "${vc_blocks[@]}"; do
    # Run the cache simulator with the current parameters
    echo "Running cache_sim with cache_size=$cache_size, assoc=$assoc, block_size=$block_size"
    ./cache_sim "$cache_size" "$associativity" "$block_size" "$vc" 262144 8 "$trace_file"
    
    # Optional: store output in a file
    # output_file="output_${cache_size}KB_assoc${assoc}.txt"
    # ./cache_sim "$cache_size" "$assoc" "$block_size" 0 0 0 "$trace_file" > "$output_file"
  done
done
