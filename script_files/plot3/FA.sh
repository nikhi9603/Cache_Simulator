#!/bin/bash

# Cache sizes in KB (from 2KB to 1MB, powers of 2)
cache_sizes=(2048 4096 8192 16384 32768 65536 131072)

# Block size
block_size=32

# Input trace file
trace_file="gcc_trace.txt"

# Loop through each cache size
for cache_size in "${cache_sizes[@]}"; do
  # Calculate number of blocks based on cache size and block size (fully associative)
  num_blocks=$((cache_size / block_size))
  
  # Run the cache simulator with the current parameters
  echo "Running cache_sim with cache_size=$cache_size, fully associative, block_size=$block_size"
  ./cache_sim "$cache_size" "$num_blocks" "$block_size" 0 262144 8 "$trace_file"
  
  # Optional: store output in a file
  # output_file="output_${cache_size}KB_fully_assoc.txt"
  # ./cache_sim "$cache_size" "$num_blocks" "$block_size" 0 0 0 "$trace_file" > "$output_file"
done

