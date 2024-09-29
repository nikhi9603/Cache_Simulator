#include "cache.h"
#include "parse.h"
#include<cmath>
#include<algorithm>

void hexVal(uint64_t dec_val)
{
    cout << hex << dec_val;
}

/****************************
 ****** CACHE BLOCK ********
****************************/
 
CacheBlock::CacheBlock(uint64_t tag)
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
 
pair<bool, int> Cache::lookupBlock(int set_num, uint64_t tag)
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
        else if(invalid_idx == -1)
        {
            // cout << cache[set_num][i].lru_counter << " " << max_lru_val << endl;
            if(cache[set_num][i].valid_bit == false)
            {
                // cout << "nope" << endl;
                invalid_idx = i;
            }
            else if(cache[set_num][i].lru_counter > max_lru_val)
            {
                // cout << "bool : " << cache[set_num][i].lru_counter << " " << max_lru_val << " " << (cache[set_num][i].lru_counter > max_lru_val) << endl;
                // if()
                // {
                     max_lru_val = cache[set_num][i].lru_counter;
                    max_lru_idx = i;
                    // cout << "true " << i << endl;
                // }
                // else
                // {
                //     cout << "false" <<  cache[set_num][i].lru_counter << " " << max_lru_val << endl;
                // }
            }
            
        }
    }

    bool isHit = (hit_idx == -1) ? false : true;
    int return_idx;

    if(isHit)
    {
        return_idx = hit_idx;
    }
    else if(invalid_idx != -1) // invalid block exists
    {
        return_idx = invalid_idx;
    }
    else
    {
        return_idx = max_lru_idx;
    }

    cout << "Indices: " << dec << hit_idx << " " << invalid_idx << " " << max_lru_idx << endl;
    cout << "Counters: " << cache[set_num][0].lru_counter << " " << cache[set_num][1].lru_counter << endl;
    return make_pair(isHit, return_idx);
}


int Cache::findLRUBlock(int set_num)
{
    int max_val = -1;
    int max_idx = -1;

    for(int i = 0; i < assoc; i++)
    {
        if(cache[set_num][i].valid_bit == false)
        {
            max_idx = i;
            break;
        }
        else if(cache[set_num][i].lru_counter > max_val) // valid block
        {
            max_val = cache[set_num][i].lru_counter;
            max_idx = i;
        }
    }
    return max_idx;
}


void Cache::incrementLRUCounters(int set_num)
{
    for(int i = 0; i < assoc; i++)
    {
        if(cache[set_num][i].valid_bit == true) cache[set_num][i].lru_counter++;
    }
}


void Cache::swapBlocks(int l1_set_num, int l1_idx, int vc_idx)
{
    CacheBlock l1_block = cache[l1_set_num][l1_idx];
    CacheBlock vc_block = vc_cache->cache[0][vc_idx];

    // cout << "L1-VC Swap " << l1_idx << " " << vc_idx << endl;

    // While swapping, make sure that tags are changed (L1 <--> VC)
    uint64_t vc_block_addr, l1_block_addr, new_vc_block_tag, new_l1_block_tag;

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
    // cout << "During Swap : " << vc_cache->cache[0][vc_idx].tag << endl; 
}


void Cache::findCactiCacheStatistics()
{
    int cacti_result = get_cacti_results(cache_size, block_size, assoc, &c_stats.hitTime, &c_stats.energy, &c_stats.area);
    
    if(cacti_result > 0)    // Cacti failed for this cache configuration
    {
        cout << "CACTI FAILED" << cacti_result << endl;
        c_stats.hitTime = 0.2;
    }
}


void Cache::printCacheSet(int set_num)
{
    cout << "  set " << set_num << ":\t";

        for(auto cb: cache[set_num])
        {
            hexVal(cb.tag);

            if(cb.dirty_bit == true)
                cout << " D\t";
            else
                cout << "  \t";
        }
    cout << endl;
}



/***************************
 * CACHE PUBLIC FUNCTIONS *
****************************/
 
pair<bool, int> Cache::lookupRead(uint64_t addr)
{
    c_stats.n_reads++;  // Read request
    pair<bool, int> result = make_pair(false, -1);

    int set_num = getSetNumber(addr);
    uint64_t tag = getTag(addr);
    cout << "Read: addr: ";
    hexVal(addr);
    cout << " set: " << set_num << "    tag: ";
    hexVal(tag);
    cout << endl;
    
    cout << "Before Read: " << endl; 
    printCacheSet(set_num);

    pair<bool, int> lookupResult = lookupBlock(set_num, tag);

    if(lookupResult.first == true) // cache hit
    {
        cout << "ReadHit\n" << endl;
        result.first = true;
        result.second = lookupResult.second;
    }
    else    // cache_miss
    {
        cout << "ReadMiss\n" << endl;
        c_stats.n_read_misses++;
        result.first = false;
        result.second = lookupResult.second;

        if(isVCEnabled) 
        {
            if(cache[set_num][result.second].valid_bit == true)
            {
                // Sends a read request to VC
                cout << "VC Cache Lookup" << endl;
                auto vc_readResult = vc_cache->lookupRead(addr);
                c_stats.n_swap_requests++;

                if(vc_readResult.first == true) // VC hit
                {
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second);
                    result.first = true;
                    c_stats.n_swaps++;
                }
                else    // VC miss
                {
                    // Readblock function does not evict block in our prog
                    // LRU of this cache should be placed in VC 
                    // And also VC block should be generally evicted and passed down to next level of memory
                    // Passing down is handled at simulator stage using `evictandReplaceBlock` functionality of Cache clas
                    // So VC block is kept in L1 (as new block adding is also done in simulator class) to ensure correctness of that => Indirectly we are swapping again    (atleast in our prog)
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second);
                }
            }
        }
    }

    incrementLRUCounters(set_num);

    // Checking if block to be evicted is dirty to count write_backs
    if(result.first == false)   // finally if its a miss
    {
        if(cache[set_num][result.second].valid_bit == true && cache[set_num][result.second].dirty_bit == true)
        {
            cout << "Writeback - set: " << hex << set_num <<  " " << "tag - " << tag << endl;
            c_stats.n_writebacks++; // write back to next level will be handled at simulator
        }
        cache[set_num][lookupResult.second].lru_counter = assoc;
    }
    else
    {
        cache[set_num][lookupResult.second].lru_counter = 0;
    }

    return result;
}


pair<bool, int> Cache::lookupWrite(uint64_t addr)
{
    c_stats.n_writes++;
    pair<bool, int> result = make_pair(false, -1);

    int set_num = getSetNumber(addr);
    uint64_t tag = getTag(addr);

    pair<bool, int> lookupResult = lookupBlock(set_num, tag);

    cout << "Write: addr: ";
    hexVal(addr);
    cout << " set: " << set_num << "    tag: ";
    hexVal(tag);
    cout << endl;
    
    cout << "Before Write: " << dec << lookupResult.second << endl; 
    printCacheSet(set_num);


    if(lookupResult.first == true) // cache hit
    {
        cout << "Write Hit\n" << endl;
        result.first = true;
        result.second = lookupResult.second;
    }
    else    // cache-write miss
    {
        cout << "Write Miss\n" << endl;
        c_stats.n_write_misses++;
        result.first = false;
        result.second = lookupResult.second;

        if(isVCEnabled) 
        {
            if(cache[set_num][result.second].valid_bit == true)
            {
                // Sends a read request to VC
                auto vc_readResult = vc_cache->lookupRead(addr);
                c_stats.n_swap_requests++;

                if(vc_readResult.first == true) // VC hit
                {
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second);
                    result.first = true;
                    c_stats.n_swaps++;
                }
                else    // VC miss
                {
                    // Writeblock function does not evict block in our prog
                    // LRU of this cache should be placed in VC 
                    // And also VC block should be generally evicted and passed down to next level of memory
                    // Passing down is handled at simulator stage using `evictandReplaceBlock` functionality of Cache clas
                    // So VC block is kept in L1 (as new block adding is also done in simulator class) to ensure correctness of that => Indirectly we are swapping again    (atleast in our prog)
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second);
                }
            }
        }
    }

    incrementLRUCounters(set_num);

    // Checking if block to be evicted is dirty to count write_backs
    if(result.first == false)   // finally if its a miss
    {
        if(cache[set_num][result.second].valid_bit == true && cache[set_num][result.second].dirty_bit == true)
        {
            cout << "Writeback - set: " << hex << set_num <<  " " << "tag - " << tag << endl;
            c_stats.n_writebacks++; // write back to next level will be handled at simulator
        }
        cache[set_num][lookupResult.second].lru_counter = assoc;
    }
    else
    {
        cache[set_num][lookupResult.second].lru_counter = 0;
    }

    return result;
}


int Cache::getSetNumber(uint64_t addr)
{
    uint64_t temp = addr >> n_blockOffsetBits; //block offset bits are removed
    uint64_t mask = (1 << (n_indexBits)) -1;
    return temp & mask;
}


uint64_t Cache::getTag(uint64_t addr)
{
    uint64_t tag = addr >> (n_blockOffsetBits + n_indexBits); //block offset & index_bits bits are removed
    return tag;
}


uint64_t Cache::getBlockAddress(int set_num, uint64_t tag)
{
    uint64_t addr = tag << ((n_indexBits + n_blockOffsetBits)) |  (set_num << n_blockOffsetBits);
    return addr;
}


void Cache::readData(int set_num, int idx)
{
    return;     // since its simulation, data is not read
}


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

    CacheBlock lruCacheBlock = cache[set_num][lru_idx];
    // cout << "set " << set_num << " :e " << incoming_cache_block.tag << endl;
    incoming_cache_block.lru_counter = 0;
    cache[set_num][lru_idx] = incoming_cache_block;
    cout << "While replace: set " << set_num << " " << hex << incoming_cache_block.tag << " lru_idx: " << dec << lru_idx<< endl;
    printCacheSet(set_num);
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
        if(a.lru_counter > b.lru_counter)
        {
            return true;
        }
        else if(a.lru_counter == b.lru_counter)
        {
            if(a.valid_bit == true)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    };

    for(int i = 0; i < n_sets; i++)
    {
        cout << "  set " << i << ":\t";

        // Sort cache_blocks cooresponding to set i
        vector<CacheBlock> cache_blocks = cache[i];
        sort(cache_blocks.begin(), cache_blocks.end(), sorting_comparator);

        for(auto cb: cache_blocks)
        {
            // cout << cb.tag << " ";
            hexVal(cb.tag);

            if(cb.dirty_bit == true)
                cout << " D\t";
            else
                cout << "  \t";
        }
        cout << endl;
    }
}


