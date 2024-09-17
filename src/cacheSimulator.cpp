#include "cacheSimulator.h"
#include<iomanip>

// RAW STATISTICS
void RawStatistics::printStats()
{
    cout << fixed << setprecision(4);
    cout << "===== Simulation results (raw) =====" << endl;
    cout << "  a. number of L1 reads:\t\t" << l1_reads << endl;
    cout << "  b. number of L1 read misses:\t\t" << l1_read_misses << endl;
    cout << "  c. number of L1 writes:\t\t" << l1_writes;
    cout << "  d. number of L1 write misses:\t\t" << l1_write_misses << endl;
    cout << "  e. number of swap requests:\t\t" << n_swap_requests << endl;
    cout << "  f. swap request rate:\t\t" << swap_request_rate << endl;
    cout << "  g. number of swaps:\t\t" << n_swaps << endl;
    cout << "  h. combined L1+VC miss rate:\t\t" << l1_vc_miss_rate << endl;
    cout << "  i. number writebacks from L1/VC:\t\t" << l1_writebacks << endl;
    cout << "  j. number of L2 reads:\t\t" << l2_reads << endl;
    cout << "  k. number of L2 read misses:\t\t" << l2_read_misses << endl;
    cout << "  l. number of L2 writes:\t\t" << l2_writes << endl;
    cout << "  m. number of L2 write misses:\t\t" << l2_write_misses << endl;
    cout << "  n. L2 miss rate:\t\t" << l2_miss_rate << endl;
    cout << "  o. number of writebacks from L2:\t\t" << l2_wriebacks << endl;
    cout << "  p. total memory traffic:\t\t" << total_memory_traffic << endl;
}


// PERFORMANCE STATISTICS
void PerformanceStatistics::printStats()
{
    cout << fixed << setprecision(4);
    cout << "===== Simulation results (performance) =====" << endl;
    cout << "  1. average access time:\t\t" << average_access_time << endl;
    cout << "  2. energy-delay product:\t\t" << energy_delay_product << endl;
    cout << "  3. total area:\t\t" << area_metric << endl;
}


/****************************
****** CACHE SIMULATOR ******
****************************/
 
CacheSimulator::CacheSimulator(uint l1_size, uint l1_assoc, uint l1_blocksize,
                   uint n_vc_blocks,
                   uint l2_size, uint l2_assoc,
                   vector<TraceEntry> trace_contents)
{
    this->l1_size = l1_size;
    this->l1_assoc = l1_assoc;
    this->l1_blocksize = l1_blocksize;
    this->n_vc_blocks = n_vc_blocks;
    this->l2_size = l2_size;
    this->l2_assoc = l2_assoc;

    l1_cache = Cache(l1_size, l1_assoc, l1_blocksize, n_vc_blocks);
    isVCEnabled = (n_vc_blocks > 0) ? true : false;

    if(l2_size == 0)
    {
        isL2Exist = false;
        l2_cache = Cache();
    }
    {
        isL2Exist = true;
        l2_cache = Cache(l2_size, l2_assoc, l1_blocksize, 0);
    }

    // Simulating by sending requests and finding stats;
    sendRequests(trace_contents);
    
    l1_stats = l1_cache.getCacheStatistics();
    l2_stats = l2_cache.getCacheStatistics();
    simulation_stats = findSimulationStats();
}


void CacheSimulator::sendRequests(vector<TraceEntry> trace_contents)
{
    for(auto traceEntry : trace_contents)
    {
        if(traceEntry.operation == "r")
            sendReadRequest(traceEntry.addr);
        else
            sendWriteRequest(traceEntry.addr);
    }
}


void CacheSimulator::sendReadRequest(uint64_t addr)
{
    /*
        Four configurations are investigated in this project:
        1. L1 + Memory
        2. L1 + L2 + Memory
        3. (L1 + VC) + Memory
        4. (L1 + VC) + L2 + Memory

        Cache::ReadBlock function considers (L1+VC) configuration results combinedly
    */
    auto l1_read_result = l1_cache.readBlock(addr);

    if(l1_read_result.first == true)    // L1 hit   (i.e L1+VC hit if VC is enabled)
    {
        // No need to pass down to further levels of memory
    }
    else    // L1 miss
    {
        if(isL2Exist)   // L1+L2 config or (L1+VC)+ L2 config
        {
            // check if the evicted block after L1 read is dirty
        }
        else    // L1 config or (L1+VC) config
        {

        }
    }
}
