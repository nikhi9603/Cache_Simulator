#!/bin/bash

# Define L1 and L2 cache sizes (in KB)
L1_SIZES=(1024 2048 4096 8192 16384 32768 65536)  # L1 cache sizes
L2_SIZES=(32768 65536 131072 262144 524288 1048576)  # L2 cache sizes
BLOCKSIZE=32  # Fixed block size

# Loop through L1 and L2 cache sizes
for L1 in "${L1_SIZES[@]}"; do
    for L2 in "${L2_SIZES[@]}"; do
        # Only run if L1 is smaller than L2
        if [ $L1 -lt $L2 ]; then
            # Run the cache simulator and capture the output
            # Assuming the program outputs AAT in a readable format
            ./cache_sim $L1 4 32 0 $L2 8 gcc_trace.txt
            
            # Append the results to the output file
        fi
    done
done

echo "Simulation complete. Results saved to $OUTPUT_FILE."

