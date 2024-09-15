#ifndef CACHE_SIM_H
#define CACHE_SIM_H

#include<iostream>
#include<vector>
#include "cache.h"
using namespace std;


struct RawStatistics
{
    uint l1_reads; 
    uint l1_read_misses;
    uint l1_writes;
    uint l1_write_misses;
    uint n_swap_requests;   // btw L1, VC
    double swap_request_rate;
    uint n_swaps;       // between L1, VC
    double l1_vc_miss_rate;   // combined L1+VC miss rate
    uint l1_writebacks;  // # of writebacks from L1 or its VC(if enabled) to next level
    
    uint l2_reads; 
    uint l2_read_misses;
    uint l2_writes;
    uint l2_write_misses; 
    double l2_miss_rate;   
    uint l2_wriebacks;

    double total_memory_traffic;

    void printStats();
};


struct SimulationStatistics
{
    RawStatistics raw_stats;
    double average_access_time;
    double energy_delay_product;
    double area_metric;
};


class CacheSimulator
{
private:
    Cache l1_cache;
    Cache l2_cache;
    bool isVCEnabled;
    bool isL2Exist;
    uint l1_size, l1_assoc, l1_blocksize, n_vc_blocks, l2_size, l2_assoc;
    SimulationStatistics simulation_stats;
    CacheStatistics l1_stats, l2_stats;

    void sendRequests(vector<pair<string, uint64_t>> trace_contents);
    void sendReadRequest(uint64_t addr);
    void sendWriteRequest(uint64_t addr);
    void findRawStatistics();
    void findAAT();
    void findEDP();
    void findArea();
public:
    CacheSimulator(uint l1_size, uint l1_assoc, uint l1_blocksize,
                   uint n_vc_blocks,
                   uint l2_size, uint l2_assoc,
                   vector<pair<string, uint64_t>> trace_contents) {}

    /**
     * @return Simulation statistics
     */
    SimulationStatistics getSimulationStats() {return simulation_stats;}
};

#endif