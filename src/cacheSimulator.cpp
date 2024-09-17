#include "cacheSimulator.h"
#include<iomanip>
#include<cmath>

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

        Cache::lookupRead function considers (L1+VC) configuration results combinedly
    */
    auto l1_read_result = l1_cache.lookupRead(addr);
    int l1_set_num = l1_cache.getSetNumber(addr);

    if(l1_read_result.first == true)    // L1 hit   (i.e L1+VC hit if VC is enabled)
    {
        // No need to pass down to further levels of memory
        l1_cache.readData(l1_set_num, l1_read_result.second);
    }
    else    // L1 miss
    {
        if(isL2Exist)   // L1+L2 config or (L1+VC)+ L2 config
        {
            auto l2_read_result = l2_cache.lookupRead(addr);
            int l2_set_num = l2_cache.getSetNumber(addr);

            if(l2_read_result.first == true) // L2 hit
            {
                // pass the value back to L1
                CacheBlock l2_block = l2_cache.getBlock(l2_set_num, l2_read_result.second);
                CacheBlock evictedBlock = l1_cache.evictAndReplaceBlock(l2_block, l1_set_num, l1_read_result.second);
                l1_cache.readData(l1_set_num, l1_read_result.second);

                // if evictedBlock from L1 is dirty, need to write to L2
                if(evictedBlock.dirty_bit == true)
                {
                    int l1_indexBits = log2(l1_size/(l1_blocksize * l1_assoc));
                    int l1_blockOffsetBits = log2(l1_blocksize);
                    uint64_t evictedBlock_addr = (evictedBlock.tag << (l1_indexBits + l1_blockOffsetBits)) 
                                            |  (l1_set_num << l1_blockOffsetBits);

                    auto l1_writeback_result = l2_cache.lookupWrite(evictedBlock_addr);
                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second);
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(evictedBlock, l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second);
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second);
                        
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                    }
                }
            }
            else    // L2 miss
            {
                CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));
                CacheBlock l2_newBlock = CacheBlock(l2_cache.getTag(addr));
                
                CacheBlock l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_read_result.second);
                CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, l2_read_result.second);
                l1_cache.readData(l1_set_num, l1_read_result.second);

                if(l1_evictedBlock.dirty_bit == true)
                {
                    int l1_indexBits = log2(l1_size/(l1_blocksize * l1_assoc));
                    int l1_blockOffsetBits = log2(l1_blocksize);
                    uint64_t l1_evictedBlock_addr = (l1_evictedBlock.tag << (l1_indexBits + l1_blockOffsetBits)) 
                                            |  (l1_set_num << l1_blockOffsetBits);

                    auto l1_writeback_result = l2_cache.lookupWrite(l1_evictedBlock_addr);
                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l1_evictedBlock, l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                    }
                }
            }
        }
        else    // L1 config or (L1+VC) config
        {
            CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));
            CacheBlock l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_read_result.second);
            l1_cache.readData(l1_set_num, l1_read_result.second);
            
            // If L1 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
        }
    }
}


void CacheSimulator::sendWriteRequest(uint64_t addr)
{
    /*
        Four configurations are investigated in this project:
        1. L1 + Memory
        2. L1 + L2 + Memory
        3. (L1 + VC) + Memory
        4. (L1 + VC) + L2 + Memory

        Cache::lookupWrite function considers (L1+VC) configuration results combinedly
    */
    auto l1_write_result = l1_cache.lookupWrite(addr);
    int l1_set_num = l1_cache.getSetNumber(addr);

    if(l1_write_result.first == true)    // L1 hit   (i.e L1+VC hit if VC is enabled)
    {
        // No need to pass down to further levels of memory
        l1_cache.writeData(l1_set_num, l1_write_result.second);
    }
    else    // L1 miss
    {
        if(isL2Exist)   // L1+L2 config or (L1+VC)+ L2 config
        {
            auto l2_write_result = l2_cache.lookupWrite(addr);
            int l2_set_num = l2_cache.getSetNumber(addr);

            if(l2_write_result.first == true) // L2 hit
            {
                // pass the value back to L1
                CacheBlock l2_block = l2_cache.getBlock(l2_set_num, l2_write_result.second);
                CacheBlock evictedBlock = l1_cache.evictAndReplaceBlock(l2_block, l1_set_num, l2_write_result.second);
                l1_cache.writeData(l1_set_num, l1_write_result.second);

                // if evictedBlock from L1 is dirty, need to write to L2
                if(evictedBlock.dirty_bit == true)
                {
                    int l1_indexBits = log2(l1_size/(l1_blocksize * l1_assoc));
                    int l1_blockOffsetBits = log2(l1_blocksize);
                    uint64_t evictedBlock_addr = (evictedBlock.tag << (l1_indexBits + l1_blockOffsetBits)) 
                                            |  (l1_set_num << l1_blockOffsetBits);

                    auto l1_writeback_result = l2_cache.lookupWrite(evictedBlock_addr);
                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second);
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(evictedBlock, l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second);
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second);
                        
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                    }
                }
            }
            else    // L2 miss
            {
                CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));
                CacheBlock l2_newBlock = CacheBlock(l2_cache.getTag(addr));
                
                CacheBlock l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_write_result.second);
                CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, l2_write_result.second);
                l1_cache.writeData(l1_set_num, l1_write_result.second);

                if(l1_evictedBlock.dirty_bit == true)
                {
                    int l1_indexBits = log2(l1_size/(l1_blocksize * l1_assoc));
                    int l1_blockOffsetBits = log2(l1_blocksize);
                    uint64_t l1_evictedBlock_addr = (l1_evictedBlock.tag << (l1_indexBits + l1_blockOffsetBits)) 
                                            |  (l1_set_num << l1_blockOffsetBits);

                    auto l1_writeback_result = l2_cache.lookupWrite(l1_evictedBlock_addr);
                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l1_evictedBlock, l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                    }
                }
            }
        }
        else    // L1 config or (L1+VC) config
        {
            CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));
            CacheBlock l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_write_result.second);
            l1_cache.writeData(l1_set_num, l1_write_result.second);
            
            // If L1 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
        }
    }
}


RawStatistics CacheSimulator::findRawStatistics()
{
    RawStatistics raw_stats;
    CacheStatistics l1_stats = l1_cache.getCacheStatistics();
    CacheStatistics l2_stats = l2_cache.getCacheStatistics();

    raw_stats.l1_reads = l1_stats.n_reads;
    raw_stats.l1_read_misses = l1_stats.n_read_misses;
    raw_stats.l1_writes = l1_stats.n_writes;
    raw_stats.l1_write_misses = l1_stats.n_write_misses;

    raw_stats.n_swap_requests = l1_stats.n_swap_requests;
    raw_stats.swap_request_rate = (raw_stats.n_swap_requests)/(raw_stats.l1_reads + raw_stats.l1_writes);
    raw_stats.n_swaps = l1_stats.n_swaps;

    raw_stats.l1_vc_miss_rate = (raw_stats.l1_read_misses + raw_stats.l1_write_misses - raw_stats.n_swaps)/ (raw_stats.l1_reads + raw_stats.l1_writes);
    raw_stats.l1_writebacks = l1_stats.n_writebacks;

    raw_stats.l2_reads = l2_stats.n_reads;
    raw_stats.l2_read_misses = l2_stats.n_read_misses;
    raw_stats.l2_writes = l2_stats.n_writes;
    raw_stats.l2_write_misses = l2_stats.n_write_misses;
    raw_stats.l2_wriebacks = l2_stats.n_writebacks;    
    raw_stats.l2_miss_rate = raw_stats.l2_read_misses / raw_stats.l2_reads;

    if(isL2Exist)
    {
        raw_stats.total_memory_traffic = raw_stats.l2_read_misses + raw_stats.l2_write_misses + raw_stats.l2_wriebacks;
    }
    else
    {
        raw_stats.total_memory_traffic = raw_stats.l1_read_misses + raw_stats.l1_write_misses - raw_stats.n_swaps + raw_stats.l1_writebacks;
    }
}
