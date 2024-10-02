#include "cache.h"
#include "parse.h"
#include<cmath>
#include<algorithm>

/****************************
 ****** CACHE BLOCK ********
****************************/
 
CacheBlock::CacheBlock(long long int tag)
{
    this->tag = tag;
    valid_bit = true;
    dirty_bit = false;
    lru_counter = 0;
}

CacheBlock::CacheBlock()
{
    this->tag = tag;
    valid_bit = false;
    dirty_bit = false;
    lru_counter = 0;
}


/****************************
 **** CACHE CONSTRUCTORS ****
****************************/
 
Cache::Cache(int cache_size, int assoc, int block_size, int n_vc_blocks)
{
    this->cache_size = cache_size;
    this->assoc = assoc;
    this->block_size = block_size;
    this->n_sets = cache_size / (block_size * assoc);

    n_indexBits = log2(n_sets);
    n_blockOffsetBits = log2(block_size);
    n_tagBits = 64 - n_indexBits - n_blockOffsetBits;

    this->n_vc_blocks = n_vc_blocks;
    isVCEnabled = (n_vc_blocks > 0) ? true : false;

    if(isVCEnabled)
    {
        // VC is fully-associative cache
        uint vc_block_size = block_size;
        uint vc_cache_size = n_vc_blocks * vc_block_size;
        uint vc_assoc = n_vc_blocks;
        uint victimCache_n_vc_blocks = 0;

        vc_cache = new Cache(vc_cache_size, vc_assoc, vc_block_size, victimCache_n_vc_blocks);
    }
    else
    {
        vc_cache = new Cache();
    }

    cache = vector<vector<CacheBlock>> (n_sets, vector<CacheBlock>(assoc, CacheBlock()));

    for(int i = 0; i < n_sets; i++)
    {
        for(int j = 0; j < assoc; j++)
        {
            cache[i][j].lru_counter = j;
        }
    }

    findCactiCacheStatistics();
    c_stats.vc_statistics = &(vc_cache->c_stats);
}


Cache::Cache()
{
    cache_size = 0;
    assoc = 0;
    block_size = 0;
    n_sets = 0;
    isVCEnabled = false;
    n_vc_blocks = 0;
    vc_cache = nullptr; 
    c_stats.vc_statistics = nullptr;
}


/***************************
 * CACHE PRIVATE FUNCTIONS *
****************************/
 
pair<bool, int> Cache::lookupBlock(int set_num, long long int tag)
{
    int max_lru_val = -1;
    int max_lru_idx = -1;
    int invalid_idx = -1;
    int hit_idx = -1;

    for(int i = 0; i < assoc; i++)
    {
        if(cache[set_num][i].valid_bit == true && cache[set_num][i].tag == tag)
        {
            hit_idx = i;
            break;
        }
        // else if(invalid_idx == -1)
        // {
        //     // std::cout << cache[set_num][i].lru_counter << " " << max_lru_val << endl;
        //     if(cache[set_num][i].valid_bit == false)
        //     {
        //         // std::cout << "nope" << endl;
        //         invalid_idx = i;
        //     }
        //     else if(cache[set_num][i].lru_counter > max_lru_val)
        //     {
        //         // std::cout << "bool : " << cache[set_num][i].lru_counter << " " << max_lru_val << " " << (cache[set_num][i].lru_counter > max_lru_val) << endl;
        //         // if()
        //         // {
        //              max_lru_val = cache[set_num][i].lru_counter;
        //             max_lru_idx = i;
        //             // std::cout << "true " << i << endl;
        //         // }
        //         // else
        //         // {
        //         //     std::cout << "false" <<  cache[set_num][i].lru_counter << " " << max_lru_val << endl;
        //         // }
        //     }
            
        // }
        else
        {
            if(cache[set_num][i].lru_counter > max_lru_val)
            {
                max_lru_val = cache[set_num][i].lru_counter;
                max_lru_idx = i;
            }
        }
    }

    bool isHit = (hit_idx == -1) ? false : true;
    int return_idx;

    if(isHit)
    {
        return_idx = hit_idx;
    }
    // else if(invalid_idx != -1) // invalid block exists
    // {
    //     return_idx = invalid_idx;
    // }
    else
    {
        return_idx = max_lru_idx;
    }

    // std::cout << "Indices: " << dec << hit_idx << " " << invalid_idx << " " << max_lru_idx << endl;
    // std::cout << "Counters: ";
    // for(int i = 0; i < assoc; i++)
    // {
    //     std::cout << cache[set_num][i].lru_counter << " ";
    // }
    // std::cout << endl;
    return make_pair(isHit, return_idx);
}


int Cache::findLRUBlock(int set_num)
{
    int max_val = -1;
    int max_idx = -1;

    for(int i = 0; i < assoc; i++)
    {
        // if(cache[set_num][i].valid_bit == false)
        // {
        //     max_idx = i;
        //     break;
        // }
        if(cache[set_num][i].lru_counter > max_val) 
        {
            max_val = cache[set_num][i].lru_counter;
            max_idx = i;
        }
    }
    return max_idx;
}


void Cache::incrementLRUCounters(int set_num, int idx)
{
    int cur_counter = cache[set_num][idx].lru_counter;

    for(int i = 0; i < assoc; i++)
    {
        if(cache[set_num][i].lru_counter < cur_counter) cache[set_num][i].lru_counter++;
    }
}


void Cache::swapBlocks(int l1_set_num, int l1_idx, int vc_idx)
{
    CacheBlock l1_block = cache[l1_set_num][l1_idx];
    CacheBlock vc_block = vc_cache->cache[0][vc_idx];

    // std::cout << "L1-VC Swap " << l1_idx << " " << vc_idx << endl;

    // While swapping, make sure that tags are changed (L1 <--> VC)
    long long int vc_block_addr, l1_block_addr, new_vc_block_tag, new_l1_block_tag;

    if(l1_block.valid_bit == true)
    {
        l1_block_addr = getBlockAddress(l1_set_num, l1_block.tag);
        new_l1_block_tag = vc_cache->getTag(l1_block_addr);
    }
    
    if(vc_block.valid_bit == true)
    {
        vc_block_addr = vc_cache->getBlockAddress(0, vc_block.tag);
        new_vc_block_tag = getTag(vc_block_addr);
    }

    l1_block.tag = new_l1_block_tag;
    l1_block.lru_counter = 0;
    vc_block.tag = new_vc_block_tag;
    vc_block.lru_counter = 0;

    vc_cache->cache[0][vc_idx] = l1_block;
    cache[l1_set_num][l1_idx] = vc_block;
    // std::cout << "During Swap : " << vc_cache->cache[0][vc_idx].tag << endl; 
}


void Cache::findCactiCacheStatistics()
{
    int cacti_result = get_cacti_results(cache_size, block_size, assoc, &c_stats.hitTime, &c_stats.energy, &c_stats.area);
    
    if(cacti_result > 0)    // Cacti failed for this cache configuration
    {
        // std::cout << "CACTI FAILED" << cacti_result << endl;
        c_stats.hitTime = 0.2;
    }
}


void Cache::printCacheSet(int set_num)
{
    std::cout << "  set " << hex << set_num << ":\t";

        for(auto cb: cache[set_num])
        {
            cout << hex << cb.tag << endl;
            if(cb.dirty_bit == true)
                std::cout << " D\t";
            else
                std::cout << "  \t";
        }
    std::cout << endl;
}



/***************************
 * CACHE PUBLIC FUNCTIONS *
****************************/
 
pair<bool, pair<int, CacheBlock>> Cache::lookupRead(long long int addr)
{
    c_stats.n_reads++;  // Read request
    pair<bool, pair<int, CacheBlock>> result = make_pair(false, make_pair(-1, CacheBlock(0)));

    int set_num = getSetNumber(addr);
    long long int tag = getTag(addr);
    // std::cout << "Read: addr: ";
    // cout << hex << addr ;
    // std::cout << " set: " << set_num << "    tag: ";
    // cout << hex << tag ;
    // std::cout << endl;
    
    // std::cout << "Before Read: " << endl; 
    // printCacheSet(set_num);

    pair<bool, int> lookupResult = lookupBlock(set_num, tag);
    int lru_counter = cache[set_num][lookupResult.second].lru_counter;

    if(lookupResult.first == true) // cache hit
    {
        // std::cout << "ReadHit\n" << endl;
        result.first = true;
        result.second.first = lookupResult.second;
        result.second.second = cache[set_num][lookupResult.second];
    }
    else    // cache_miss
    {
        // std::cout << "ReadMiss\n" << endl;
        c_stats.n_read_misses++;
        result.first = false;
        result.second.first = lookupResult.second;
        result.second.second = cache[set_num][lookupResult.second];

        if(isVCEnabled) 
        {
            if(cache[set_num][result.second.first].valid_bit == true)
            {
                // Sends a read request to VC
                // std::cout << "VC Cache Lookup" << endl;
                auto vc_readResult = vc_cache->lookupRead(addr);
                c_stats.n_swap_requests++;

                if(vc_readResult.first == true) // VC hit
                {
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second.first);
                    cache[set_num][lookupResult.second].lru_counter = lru_counter;
                    vc_cache->cache[0][vc_readResult.second.first].lru_counter = 0;
                    result.first = true;
                    result.second.second = cache[set_num][lookupResult.second];
                    c_stats.n_swaps++;
                }
                else    // VC miss
                {
                    // Readblock function does not evict block in our prog
                    // LRU of this cache should be placed in VC 
                    // And also VC block should be generally evicted and passed down to next level of memory
                    // Passing down is handled at simulator stage using `evictandReplaceBlock` functionality of Cache clas
                    // So VC block is kept in L1 (as new block adding is also done in simulator class) to ensure correctness of that => Indirectly we are swapping again    (atleast in our prog)
                    if(cache[set_num][lookupResult.second].valid_bit == true)
                    {
                        cache[set_num][lookupResult.second].tag = vc_cache->getTag(getBlockAddress(set_num, cache[set_num][lookupResult.second].tag));
                        CacheBlock vc_evictedBlock = vc_cache->evictAndReplaceBlock(cache[set_num][lookupResult.second], 0, vc_readResult.second.first);
                        cache[set_num][lookupResult.second].valid_bit = false;

                        result.second.first = -1;   // indicating block is evicted from vc cache
                        result.second.second = vc_evictedBlock;
                        if(vc_evictedBlock.valid_bit == true && vc_evictedBlock.dirty_bit == true)   c_stats.n_writebacks++;
                    }
                }
            }
        }
    }


    // Checking if block to be evicted is dirty to count write_backs
    if(result.first == true)   // finally if its a miss
    {
    //     if(cache[set_num][result.second].valid_bit == true && cache[set_num][result.second].dirty_bit == true)
    //     {
    //         std::cout << "Writeback - set: " << hex << set_num <<  " " << "tag - " << tag << endl;
    //         c_stats.n_writebacks++; // write back to next level will be handled at simulator
    //     }
    //     // cache[set_num][lookupResult.second].lru_counter = 0;
    // }
    // else
    // {
        incrementLRUCounters(set_num, lookupResult.second);
        cache[set_num][lookupResult.second].lru_counter = 0;
    }

    return result;
}


pair<bool, pair<int, CacheBlock>> Cache::lookupWrite(long long int addr)
{
    c_stats.n_writes++;
    pair<bool, pair<int,CacheBlock>> result = make_pair(false, make_pair(-1, CacheBlock(0)));

    int set_num = getSetNumber(addr);
    long long int tag = getTag(addr);

    pair<bool, int> lookupResult = lookupBlock(set_num, tag);
    int lru_counter = cache[set_num][lookupResult.second].lru_counter;
    // std::cout << "LookupResukt lru idx: " << lookupResult.second << " , counter : " << lru_counter << endl;

    // std::cout << "Write: addr: ";
    // cout << hex << addr ;
    // std::cout << " set: " << hex << set_num << "    tag: ";
    // cout << hex << tag;
    // std::cout << endl;
    
    // std::cout << "Before Write: " << dec << lookupResult.second << endl; 
    // printCacheSet(set_num);


    if(lookupResult.first == true) // cache hit
    {
        // std::cout << "Write Hit\n" << endl;
        result.first = true;
        result.second.first = lookupResult.second;
        result.second.second = cache[set_num][lookupResult.second];
    }
    else    // cache-write miss
    {
        // std::cout << "Write Miss\n" << endl;
        c_stats.n_write_misses++;
        result.first = false;
        result.second.first = lookupResult.second;
        result.second.second = cache[set_num][lookupResult.second];

        if(isVCEnabled) 
        {
            if(cache[set_num][result.second.first].valid_bit == true)
            {
                // Sends a read request to VC
                auto vc_readResult = vc_cache->lookupRead(addr);
                c_stats.n_swap_requests++;

                if(vc_readResult.first == true) // VC hit
                {
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second.first);
                    vc_cache->cache[0][vc_readResult.second.first].lru_counter = 0;
                    cache[set_num][lookupResult.second].lru_counter = lru_counter;
                    // std::cout << "Swap hit - counter " << 
                    result.first = true;
                    result.second.second = cache[set_num][lookupResult.second];
                    c_stats.n_swaps++;
                }
                else    // VC miss
                {
                    // Writeblock function does not evict block in our prog
                    // LRU of this cache should be placed in VC 
                    // And also VC block should be generally evicted and passed down to next level of memory
                    // Passing down is handled at simulator stage using `evictandReplaceBlock` functionality of Cache clas
                    // So VC block is kept in L1 (as new block adding is also done in simulator class) to ensure correctness of that => Indirectly we are swapping again    (atleast in our prog)
                    if(cache[set_num][lookupResult.second].valid_bit == true)
                    {
                        cache[set_num][lookupResult.second].tag = vc_cache->getTag(getBlockAddress(set_num, cache[set_num][lookupResult.second].tag));
                        CacheBlock vc_evictedBlock = vc_cache->evictAndReplaceBlock(cache[set_num][lookupResult.second], 0, vc_readResult.second.first);
                        cache[set_num][lookupResult.second].valid_bit = false;

                        result.second.first = -1;   // indicating block is evicted from vc cache
                        result.second.second = vc_evictedBlock;
                        if(vc_evictedBlock.valid_bit == true && vc_evictedBlock.dirty_bit == true)   c_stats.n_writebacks++;
                    }
                    // CacheBlock vc_evictedBlock = vc_cache->evictAndReplaceBlock(cache[set_num][lookupResult.second], 0, vc_readResult.second.first);
                    // cache[set_num][lookupResult.second].valid_bit = false;

                    // result.second.first = -1;   // indicating block is evicted from vc cache
                    // result.second.second = vc_evictedBlock;
                    // if(vc_evictedBlock.dirty_bit == true)   c_stats.n_writebacks++;
                }
            }
        }
    }

    // std::cout << "After increment counters - " << cache[set_num][lookupResult.second].lru_counter << " : ";
    // for(int i = 0; i < assoc; i++)
    // {
    //     std::cout << cache[set_num][i].lru_counter << " ";
    // }
    // std::cout << endl;

    // Checking if block to be evicted is dirty to count write_backs
    if(result.first == true)   
    {
    //     if(cache[set_num][result.second].valid_bit == true && cache[set_num][result.second].dirty_bit == true)
    //     {
    //         std::cout << "Writeback - set: " << hex << set_num <<  " " << "tag - " << tag << endl;
    //         c_stats.n_writebacks++; // write back to next level will be handled at simulator
    //     }
    //     // cache[set_num][lookupResult.second].lru_counter = 0;
    // }
    // else
    // {
        incrementLRUCounters(set_num, lookupResult.second);
        cache[set_num][lookupResult.second].lru_counter = 0;

        // std::cout << "After increment counters - " << cache[set_num][lookupResult.second].lru_counter << " : ";
        // for(int i = 0; i < assoc; i++)
        // {
        //     std::cout << cache[set_num][i].lru_counter << " ";
        // }
        // std::cout << endl;
    }

    return result;
}


int Cache::getSetNumber(long long int addr)
{
    long long int temp = addr >> n_blockOffsetBits; //block offset bits are removed
    long long int mask = (1 << (n_indexBits)) -1;
    return temp & mask;
}


long long int Cache::getTag(long long int addr)
{
    long long int tag = (addr >> n_blockOffsetBits) >> n_indexBits; //block offset & index_bits bits are removed
    return tag;
}


long long int Cache::getBlockAddress(int set_num, long long int tag)
{
    long long int addr = ((tag << n_blockOffsetBits) <<  n_indexBits) |  (set_num << n_blockOffsetBits);
    return addr;
}


// void Cache::readData(int set_num, int idx)
// {
//     return;     // since its simulation, data is not read
// }


void Cache::writeData(int set_num, int idx)
{
    // since its simulation, data is not taken as arg to write
    cache[set_num][idx].dirty_bit = true;
}


CacheBlock Cache::getBlock(int set_num, int idx)
{
    return cache[set_num][idx];
}


CacheBlock Cache::evictAndReplaceBlock(CacheBlock incoming_cache_block, int set_num, int lru_idx)
{
    // lru_idx will be invalid block idx if exists
    if(lru_idx == -1)   lru_idx = findLRUBlock(set_num);

    incrementLRUCounters(set_num, lru_idx);

    CacheBlock lruCacheBlock = cache[set_num][lru_idx];

    if(lruCacheBlock.valid_bit == true && lruCacheBlock.dirty_bit == true)
    {
        c_stats.n_writebacks++;
    }
    // std::cout << "set " << set_num << " :e " << incoming_cache_block.tag << endl;
    incoming_cache_block.lru_counter = 0;
    cache[set_num][lru_idx] = incoming_cache_block;
    // std::cout << "While replace: set " << hex << set_num << " " << hex << incoming_cache_block.tag << " lru_idx: " << dec << lru_idx<< endl;
    // printCacheSet(set_num);
    // std::cout << "While replace counters:";
    // for(int i = 0; i < assoc; i++)
    // {
    //     std::cout << cache[set_num][i].lru_counter << " ";
    // }
    // std::cout << endl;
    return lruCacheBlock;
}

// CacheBlock Cache::evictBlock(int set_num, int lru_idx)
// {
//     // lru_idx will be invalid block idx if exists
//     if(lru_idx == -1)   lru_idx = findLRUBlock(set_num);

//     CacheBlock lruCacheBlock = cache[set_num][lru_idx];
//     cache[set_num][lru_idx].valid_bit = false;
//     return lruCacheBlock;
// }


// void Cache::replaceBlock(CacheBlock incoming_cache_block, int set_num, int lru_idx)
// {
//     // lru_idx will be invalid block idx if exists
//     if(lru_idx == -1)   lru_idx = findLRUBlock(set_num);

//     // incoming_cache_block.lru_counter = 0;
//     cache[set_num][lru_idx] = incoming_cache_block;
// }


void Cache::printCacheContents()
{
    // For printing, mru -> lru blocks, we need to sort based on lru_counters
    // Comparator for sorting
    auto sorting_comparator = [] (CacheBlock a, CacheBlock b)
    {
        if(a.lru_counter < b.lru_counter)
        {
            return true;
        }
        else
        {
            return false;
        }
    };

    for(int i = 0; i < n_sets; i++)
    {
        std::cout << "  set " << dec << i << ":\t";

        // Sort cache_blocks cooresponding to set i
        vector<CacheBlock> cache_blocks = cache[i];
        sort(cache_blocks.begin(), cache_blocks.end(), sorting_comparator);

        for(auto cb: cache_blocks)
        {
            std::cout << hex << cb.tag ;

            if(cb.dirty_bit == true)
                std::cout << " D\t";
            else
                std::cout << "  \t";
        }
        std::cout << endl;
    }
}

void Cache::unsetDirty(int set_num, int idx)
{
    cache[set_num][idx].dirty_bit = false;
}
