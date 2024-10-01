#ifndef CACHE_SIM_H
#define CACHE_SIM_H

#include<iostream>
#include<vector>
#include "cache.h"
#include "trace.h"
using namespace std;


struct RawStatistics
{
    uint l1_reads; 
    uint l1_read_misses;
    uint l1_writes;
    uint l1_write_misses;
    uint n_swap_requests;   // btw L1, VC
    float swap_request_rate;
    uint n_swaps;       // between L1, VC
    float l1_vc_miss_rate;   // combined L1+VC miss rate
    uint l1_writebacks;  // number of writebacks from L1 or its VC(if enabled) to next level
    
    uint l2_reads; 
    uint l2_read_misses;
    uint l2_writes;
    uint l2_write_misses; 
    float l2_miss_rate;   
    uint l2_writebacks;

    int total_memory_traffic;

    void printStats();
};

struct PerformanceStatistics
{
    float average_access_time;
    float energy_delay_product;
    float area_metric;

    void printStats();
};

struct SimulationStatistics
{
    RawStatistics raw_stats;
    PerformanceStatistics perf_stats;

    void printStats() {raw_stats.printStats(); cout << endl; perf_stats.printStats();}
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
    string trace_file_name;

    // void sendRequests(vector<TraceEntry> trace_contents);

    RawStatistics findRawStatistics();
    PerformanceStatistics findPerformanceStats();
    float findAAT();
    float findEDP();
    float findArea();
public:
    CacheSimulator(uint l1_size, uint l1_assoc, uint l1_blocksize,
                   uint n_vc_blocks,
                   uint l2_size, uint l2_assoc, string trace_file_name);

    /*
     * @return Simulation statistics
     */
    SimulationStatistics getSimulationStats();

    void sendReadRequest(long long int addr);
    void sendWriteRequest(long long int addr);

    void printSimulationStats() { simulation_stats.printStats(); }

    void printCacheContents();

    void printSimulatorConfiguration();
};

#endif