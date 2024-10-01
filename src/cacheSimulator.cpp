#include "cacheSimulator.h"
#include<iomanip>
#include<cmath>

// RAW STATISTICS
void RawStatistics::printStats()
{
    cout << endl;
    cout << fixed << setprecision(4) << dec;
    cout << "===== Simulation results (raw) =====" << endl;
    cout << "  a. number of L1 reads:\t\t" << l1_reads << endl;
    cout << "  b. number of L1 read misses:\t\t" << l1_read_misses << endl;
    cout << "  c. number of L1 writes:\t\t" << l1_writes << endl;
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
    cout << "  o. number of writebacks from L2:\t\t" << l2_writebacks << endl;
    cout << "  p. total memory traffic:\t\t" << total_memory_traffic << endl;
}


// PERFORMANCE STATISTICS
void PerformanceStatistics::printStats()
{
    cout << endl;
    cout << fixed << setprecision(4) << dec;
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
                   uint l2_size, uint l2_assoc, string trace_file_name)
{
    this->l1_size = l1_size;
    this->l1_assoc = l1_assoc;
    this->l1_blocksize = l1_blocksize;
    this->n_vc_blocks = n_vc_blocks;
    this->l2_size = l2_size;
    this->l2_assoc = l2_assoc;
    this->trace_file_name = trace_file_name;

    l1_cache = Cache(l1_size, l1_assoc, l1_blocksize, n_vc_blocks);
    isVCEnabled = (n_vc_blocks > 0) ? true : false;

    // cout << "L2 size " << l2_size << endl;
    if(l2_size == 0)
    {
        isL2Exist = false;
        l2_cache = Cache();
    }
    else{
        isL2Exist = true;
        l2_cache = Cache(l2_size, l2_assoc, l1_blocksize, 0);
    }
}


// void CacheSimulator::sendRequests(vector<TraceEntry> trace_contents)
// {
//     for(auto traceEntry : trace_contents)
//     {
//         if(traceEntry.operation == "r")
//             sendReadRequest(traceEntry.addr);
//         else
//             sendWriteRequest(traceEntry.addr);
//     }
// }


void CacheSimulator::sendReadRequest(long long int addr)
{
    /*
        Four configurations are investigated in this project:
        1. L1 + Memory
        2. L1 + L2 + Memory
        3. (L1 + VC) + Memory
        4. (L1 + VC) + L2 + Memory

        Cache::lookupRead function considers (L1+VC) configuration results combinedly
    */
    cout << "Cache Read: " << hex << addr << dec << endl;
    auto l1_read_result = l1_cache.lookupRead(addr);
    int l1_set_num = l1_cache.getSetNumber(addr);

    // cout << addr << " r: " << l1_cache.getSetNumber(addr) << " " << l1_cache.getTag(addr) << endl;

    if(l1_read_result.first == true)    // L1 hit   (i.e L1+VC hit if VC is enabled)
    {
        // No need to pass down to further levels of memory
        // l1_cache.readData(l1_set_num, l1_read_result.second.first);
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
                CacheBlock l2_block = l2_cache.getBlock(l2_set_num, l2_read_result.second.first);
                l2_block.tag = l1_cache.getTag(l2_cache.getBlockAddress(l2_set_num, l2_block.tag));
                // l2_cache.unsetDirty(l2_set_num, l2_read_result.second.first);
                l2_block.dirty_bit = false;

                CacheBlock evictedBlock;
                long long int evictedBlock_addr;

                if(l1_read_result.second.first == -1)   // eviction done from vc of L1
                {
                    evictedBlock = l1_read_result.second.second;
                    evictedBlock_addr = l1_cache.vc_cache->getBlockAddress(0, evictedBlock.tag);
                    l1_cache.evictAndReplaceBlock(l2_block, l1_set_num, -1);
                }
                else
                {
                    evictedBlock = l1_cache.evictAndReplaceBlock(l2_block, l1_set_num, l1_read_result.second.first);
                    evictedBlock_addr = l1_cache.getBlockAddress(l1_set_num, evictedBlock.tag);
                }
                

                // if evictedBlock from L1 is dirty, need to write to L2
                if(evictedBlock.valid_bit == true && evictedBlock.dirty_bit == true)
                {
                    evictedBlock.tag = l2_cache.getTag(evictedBlock_addr);
                    auto l1_writeback_result = l2_cache.lookupWrite(evictedBlock_addr);

                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second.first);
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(evictedBlock, l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second.first);
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second.first);
                        
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                        if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                    }
                }
            }
            else    // L2 miss
            {
                CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));
                CacheBlock l2_newBlock = CacheBlock(l2_cache.getTag(addr));
                
                CacheBlock l1_evictedBlock;
                long long int l1_evictedBlock_addr;

                if(l1_read_result.second.first == -1)   // eviction done from vc of L1
                {
                    l1_evictedBlock = l1_read_result.second.second;
                    l1_evictedBlock_addr = l1_cache.vc_cache->getBlockAddress(0, l1_evictedBlock.tag);
                    l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, -1);
                }
                else
                {
                    l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_read_result.second.first);
                    l1_evictedBlock_addr = l1_cache.getBlockAddress(l1_set_num, l1_evictedBlock.tag);
                }
                

                if(l1_evictedBlock.valid_bit == true && l1_evictedBlock.dirty_bit == true)
                {
                    l1_evictedBlock.tag = l2_cache.getTag(l1_evictedBlock_addr);
                    auto l1_writeback_result = l2_cache.lookupWrite(l1_evictedBlock_addr);

                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second.first);
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                        // if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l1_evictedBlock, l2_cache.getSetNumber(l1_evictedBlock_addr), -1);
                        CacheBlock l2_second_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                          

                        // CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l1_evictedBlock, l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        // l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);                        

                        // CacheBlock l2_second_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                        // // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                        // if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                        // if(l2_second_evictedBlock.valid_bit == true && l2_second_evictedBlock.dirty_bit == true) cout << "L2 Writeback second" << endl;
                    }
                }
                else
                {
                    CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, l2_read_result.second.first);
                    if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                }
                // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
            }
        }
        else    // L1 config or (L1+VC) config
        {
            CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));

            CacheBlock l1_evictedBlock;
            if(l1_read_result.second.first == -1)
            {
                l1_evictedBlock = l1_read_result.second.second;
                l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, -1);
            }
            else
            {
                l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_read_result.second.first);
            }
            // cout << "set " << l1_set_num << " :n1 " << l1_cache.getBlock(l1_set_num, l1_read_result.second).tag << " " << l1_cache.getBlock(l1_set_num, l1_read_result.second).valid_bit << endl;
            // cout << "set " << l1_set_num << " :n2 " << l1_cache.getBlock(l1_set_num, l1_read_result.second).tag <<  " " << l1_cache.getBlock(l1_set_num, l1_read_result.second).valid_bit << endl;
            // cout << endl;
            // l1_cache.readData(l1_set_num, l1_read_result.second.first);
            
            // If L1 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
        }
    }
}


void CacheSimulator::sendWriteRequest(long long int addr)
{
    /*
        Four configurations are investigated in this project:
        1. L1 + Memory
        2. L1 + L2 + Memory
        3. (L1 + VC) + Memory
        4. (L1 + VC) + L2 + Memory

        Cache::lookupWrite function considers (L1+VC) configuration results combinedly
    */
    cout << "Cache Write: " << hex << addr << dec << endl;
    auto l1_write_result = l1_cache.lookupWrite(addr);
    int l1_set_num = l1_cache.getSetNumber(addr);

    // cout << addr << " : " << l1_cache.getSetNumber(addr) << " " << l1_cache.getTag(addr) << endl;

    if(l1_write_result.first == true)    // L1 hit   (i.e L1+VC hit if VC is enabled)
    {
        // No need to pass down to further levels of memory
        l1_cache.writeData(l1_set_num, l1_write_result.second.first);
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
                CacheBlock l2_block = l2_cache.getBlock(l2_set_num, l2_read_result.second.first);
                long long int l2_block_addr = l2_cache.getBlockAddress(l2_set_num, l2_block.tag);
                cout << "L2->L1 : " << hex << l2_block.tag;

                l2_block.tag = l1_cache.getTag(l2_block_addr);
                cout << " | L2 addr: " << hex << l2_block_addr << " | L1 tag"  << hex << l2_block.tag << endl;

                // l2_cache.unsetDirty(l2_set_num, l2_read_result.second.first);

                CacheBlock evictedBlock;
                long long int evictedBlock_addr;

                if(l1_write_result.second.first == -1)   // eviction done from vc of L1
                {
                    evictedBlock = l1_write_result.second.second;
                    evictedBlock_addr = l1_cache.vc_cache->getBlockAddress(0, evictedBlock.tag);
                    
                    cout << "VC->L2 : " << hex << evictedBlock.tag << " | VC addr: " << hex << evictedBlock_addr << " | L2 tag " << l2_cache.getTag(evictedBlock_addr) << endl;
                    l2_block.dirty_bit = true;
                    l1_cache.evictAndReplaceBlock(l2_block, l1_set_num, -1);
                }
                else
                {
                    evictedBlock = l1_cache.evictAndReplaceBlock(l2_block, l1_set_num, l1_write_result.second.first);
                    evictedBlock_addr = l1_cache.getBlockAddress(l1_set_num, evictedBlock.tag);
                    l1_cache.writeData(l1_set_num, l1_write_result.second.first);
                }


                // if evictedBlock from L1 is dirty, need to write to L2
                if(evictedBlock.valid_bit == true && evictedBlock.dirty_bit == true)
                {
                    auto l1_writeback_result = l2_cache.lookupWrite(evictedBlock_addr);
                    evictedBlock.tag = l2_cache.getTag(evictedBlock_addr);

                    if(l1_writeback_result.first == true)
                    {
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second.first);
                    }
                    else
                    {
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(evictedBlock, l2_cache.getSetNumber(evictedBlock_addr), -1);
                        l2_cache.writeData(l2_cache.getSetNumber(evictedBlock_addr), l1_writeback_result.second.first);
                        
                        // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                        if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                    }
                }
            }
            else    // L2 miss
            {
                CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));
                CacheBlock l2_newBlock = CacheBlock(l2_cache.getTag(addr));

                CacheBlock l1_evictedBlock;
                long long int l1_evictedBlock_addr;

                if(l1_write_result.second.first == -1)   // eviction done from vc of L1
                {
                    l1_evictedBlock = l1_write_result.second.second;
                    l1_evictedBlock_addr = l1_cache.vc_cache->getBlockAddress(0, l1_evictedBlock.tag);
                    cout << "VC->L2 : " << hex << l1_evictedBlock.tag << " | VC addr: " << hex << l1_evictedBlock_addr << " | L2 tag " << l2_cache.getTag(l1_evictedBlock_addr) << endl;

                    l1_newBlock.dirty_bit = true;
                    l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, -1);
                }
                else
                {
                    l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_write_result.second.first);
                    l1_evictedBlock_addr = l1_cache.getBlockAddress(l1_set_num, l1_evictedBlock.tag);
                    cout << "L1->L2 : " << hex << l1_evictedBlock.tag << " | L1 addr: " << hex << l1_evictedBlock_addr << " | L2 tag " << l2_cache.getTag(l1_evictedBlock_addr) << endl;

                    l1_cache.writeData(l1_set_num, l1_write_result.second.first);
                }
                
                if(l1_evictedBlock.valid_bit == true && l1_evictedBlock.dirty_bit == true)
                {
                    // long long int l1_evictedBlock_addr = l1_cache.getBlockAddress(l1_set_num, l1_evictedBlock.tag);
                    auto l1_writeback_result = l2_cache.lookupWrite(l1_evictedBlock_addr);

                    if(l1_writeback_result.first == true)
                    {
                        // l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        // CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                        // if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                        l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second.first);
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                    }
                    else
                    {
                        l1_evictedBlock.tag = l2_cache.getTag(l1_evictedBlock_addr);
                        // CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l1_evictedBlock, l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        // l2_cache.writeData(l2_cache.getSetNumber(l1_evictedBlock_addr), l1_writeback_result.second);
                        
                        // CacheBlock l2_second_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                        // // If L2 eviction is also dirty then write to memory, as of now for simulation, we are not doing anything
                        // if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                        // if(l2_second_evictedBlock.valid_bit == true && l2_second_evictedBlock.dirty_bit == true) cout << "L2 Writeback second" << endl;
                        CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l1_evictedBlock, l2_cache.getSetNumber(l1_evictedBlock_addr), -1);
                        CacheBlock l2_second_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, -1);
                    }
                }
                else
                {
                    CacheBlock l2_evictedBlock = l2_cache.evictAndReplaceBlock(l2_newBlock, l2_set_num, l2_read_result.second.first);
                    if(l2_evictedBlock.valid_bit == true && l2_evictedBlock.dirty_bit == true) cout << "L2 Writeback" << endl;
                }
            }
        }
        else    // L1 config or (L1+VC) config
        {
            CacheBlock l1_newBlock = CacheBlock(l1_cache.getTag(addr));

            CacheBlock l1_evictedBlock;
            if(l1_write_result.second.first == -1)
            {
                l1_evictedBlock = l1_write_result.second.second;
                l1_newBlock.dirty_bit = true;
                l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, -1);
            }
            else
            {
                l1_evictedBlock = l1_cache.evictAndReplaceBlock(l1_newBlock, l1_set_num, l1_write_result.second.first);
                l1_cache.writeData(l1_set_num, l1_write_result.second.first);
            }

            
            // cout << "After replace: " << dec << l1_write_result.second << endl;
            // l1_cache.printCacheSet(l1_set_num);
            // cout << "\n" << endl;
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
    raw_stats.swap_request_rate = (float)raw_stats.n_swap_requests/(raw_stats.l1_reads + raw_stats.l1_writes);
    raw_stats.n_swaps = l1_stats.n_swaps;

    raw_stats.l1_vc_miss_rate = (float)(raw_stats.l1_read_misses + raw_stats.l1_write_misses - raw_stats.n_swaps)/ (raw_stats.l1_reads + raw_stats.l1_writes);
    raw_stats.l1_writebacks = l1_stats.n_writebacks;

    raw_stats.l2_reads = l2_stats.n_reads;
    raw_stats.l2_read_misses = l2_stats.n_read_misses;
    raw_stats.l2_writes = l2_stats.n_writes;
    raw_stats.l2_write_misses = l2_stats.n_write_misses;
    raw_stats.l2_writebacks = l2_stats.n_writebacks;    

    if(isL2Exist)
    {
        raw_stats.l2_miss_rate =  (float)raw_stats.l2_read_misses / raw_stats.l2_reads;
        raw_stats.total_memory_traffic = raw_stats.l2_read_misses + raw_stats.l2_write_misses + raw_stats.l2_writebacks;
    }
    else
    {
        raw_stats.total_memory_traffic = raw_stats.l1_read_misses + raw_stats.l1_write_misses - raw_stats.n_swaps + raw_stats.l1_writebacks;
    }
    return raw_stats;
}


PerformanceStatistics CacheSimulator::findPerformanceStats()
{
    PerformanceStatistics perf_stats;
    perf_stats.average_access_time = findAAT();
    perf_stats.energy_delay_product = findEDP();
    perf_stats.area_metric = findArea();
    return perf_stats;
}


SimulationStatistics CacheSimulator::getSimulationStats()
{
    simulation_stats.raw_stats = findRawStatistics();
    simulation_stats.perf_stats = findPerformanceStats();
    return simulation_stats;
}

float CacheSimulator::findAAT()
{
    float aat = 0;
    CacheStatistics l1_cache_stats = l1_cache.getCacheStatistics();
    CacheStatistics l2_cache_stats = l2_cache.getCacheStatistics();
    CacheStatistics vc_cache_stats; 
    if(isVCEnabled) vc_cache_stats = *l1_cache.getCacheStatistics().vc_statistics;

    float main_memory_access_latency = 20;
    float block_transfer_time = (float)l1_blocksize / 16;
    float miss_penalty = main_memory_access_latency + block_transfer_time;

    // cout << l1_cache_stats.hitTime << " " << l2_cache_stats.hitTime << " " << vc_cache_stats.hitTime << endl;
    // cout << l1_cache_stats.energy << " " << l2_cache_stats.energy << " " << vc_cache_stats.energy << endl;
    // cout << l1_cache_stats.area << " " << l2_cache_stats.area << " " << vc_cache_stats.area << endl;

    // Only L1 
    if(isVCEnabled == false && isL2Exist == false)
    {
        aat = l1_cache_stats.hitTime + (simulation_stats.raw_stats.l1_vc_miss_rate * miss_penalty);
    }

    // L1+VC
    if(isVCEnabled == true && isL2Exist == false)
    {
        aat = l1_cache_stats.hitTime + (simulation_stats.raw_stats.swap_request_rate * vc_cache_stats.hitTime) + (simulation_stats.raw_stats.l1_vc_miss_rate * miss_penalty);
    }

    // L1+L2
    if(isVCEnabled == false && isL2Exist == true)
    {
        aat = l1_cache_stats.hitTime + (simulation_stats.raw_stats.l1_vc_miss_rate * (l2_cache_stats.hitTime + (simulation_stats.raw_stats.l2_miss_rate * miss_penalty)));
    }

    // (L1+VC) + L2
    if(isVCEnabled == true && isL2Exist == true)
    {
        aat = l1_cache_stats.hitTime + (simulation_stats.raw_stats.swap_request_rate * vc_cache_stats.hitTime) + 
              (simulation_stats.raw_stats.l1_vc_miss_rate * (l2_cache_stats.hitTime + (simulation_stats.raw_stats.l2_miss_rate * miss_penalty)));
    }
    return aat;
}


float CacheSimulator::findArea()
{
    float area = 0;
    area += l1_cache.getCacheStatistics().area;
    if(isL2Exist) area += l2_cache.getCacheStatistics().area;
    if(isVCEnabled) area += l1_cache.getCacheStatistics().vc_statistics->area;
    return area;
}


float CacheSimulator::findEDP()
{
    float edp = 0;
    float total_energy = 0;
    float total_access_time = 0;

    float main_memory_access_energy = 0.05;
    float main_memory_access_latency = 20;

    CacheStatistics l1_cache_stats = l1_cache.getCacheStatistics();
    CacheStatistics l2_cache_stats = l2_cache.getCacheStatistics();
    CacheStatistics vc_cache_stats; 
    if(isVCEnabled) vc_cache_stats = *l1_cache.getCacheStatistics().vc_statistics;
    RawStatistics raw_stats = simulation_stats.raw_stats;

    total_energy += (raw_stats.l1_reads + raw_stats.l1_writes) * l1_cache_stats.energy;
    total_energy += (raw_stats.l1_read_misses + raw_stats.l1_write_misses) * l1_cache_stats.energy;

    total_access_time += (raw_stats.l1_reads + raw_stats.l1_writes) * l1_cache_stats.hitTime;
    total_access_time += (raw_stats.l1_read_misses + raw_stats.l1_write_misses) * l1_cache_stats.hitTime;

    if(isVCEnabled) 
    {
        total_energy += (2 * raw_stats.n_swap_requests) * vc_cache_stats.energy;
        total_access_time += (2 * raw_stats.n_swap_requests) * vc_cache_stats.hitTime;
    }

    if(isL2Exist)
    {
        total_energy += (raw_stats.l2_reads + raw_stats.l2_writes) * l2_cache_stats.energy;
        total_energy += (raw_stats.l2_read_misses + raw_stats.l2_write_misses) * l2_cache_stats.energy;
        total_energy += (raw_stats.l2_read_misses + raw_stats.l2_write_misses) * main_memory_access_energy;
        total_energy += raw_stats.l2_writebacks * main_memory_access_energy;

        total_access_time += (raw_stats.l2_reads + raw_stats.l2_writes) * l2_cache_stats.hitTime;
        total_access_time += (raw_stats.l2_read_misses + raw_stats.l2_write_misses) * l2_cache_stats.hitTime;
        total_access_time += (raw_stats.l2_read_misses + raw_stats.l2_write_misses) * main_memory_access_latency;
        total_access_time += raw_stats.l2_writebacks * main_memory_access_latency;
    }
    else
    {
        total_energy += (raw_stats.l1_read_misses + raw_stats.l1_write_misses - raw_stats.n_swaps) * main_memory_access_energy;
        total_energy += raw_stats.l1_writebacks * main_memory_access_energy;

        total_access_time += (raw_stats.l1_read_misses + raw_stats.l1_write_misses - raw_stats.n_swaps) * main_memory_access_latency;
        total_access_time += raw_stats.l1_writebacks * main_memory_access_latency;
    }
    
    // total_energy = total_energy / pow(10,9);
    // total_access_time = total_access_time / pow(10, 9);
    // cout << "total  energy " << total_energy << endl;
    edp = total_energy * simulation_stats.perf_stats.average_access_time * (raw_stats.l1_reads + raw_stats.l1_writes);
    return edp;
}

void CacheSimulator::printCacheContents()
{
    cout << endl;
    cout << "===== L1 contents =====" << endl;
    l1_cache.printCacheContents();

    if(isVCEnabled)
    {
        cout << endl;
        cout << "===== VC contents =====" << endl;
        l1_cache.vc_cache->printCacheContents();
    }

    if(isL2Exist)
    {
        cout << endl;
        cout << "===== L2 contents =====" << endl;
        l2_cache.printCacheContents();
    }
}

void CacheSimulator::printSimulatorConfiguration()
{
    cout << "===== Simulator configuration =====" << endl;
    cout << "L1_SIZE:\t" << l1_size << endl;
    cout << "L1_ASSOC:\t" << l1_assoc << endl;
    cout << "L1_BLOCKSIZE:\t" << l1_blocksize << endl;
    cout << "VC_NUM_BLOCKS:\t" << n_vc_blocks << endl;
    cout << "L2_SIZE:\t" << l2_size << endl;
    cout << "L2_ASSOC:\t" << l2_assoc << endl;
    cout << "trace_file:\t" << trace_file_name << endl;
}
