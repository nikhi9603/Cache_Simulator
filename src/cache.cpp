#include "cache.h"
#include "parse.h"
#include<cmath>
#include<algorithm>

/****************************
 ****** CACHE BLOCK ********
****************************/
 
CacheBlock::CacheBlock(uint64_t tag)
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

    cache = vector<vector<CacheBlock>> (n_sets, vector<CacheBlock>(assoc, CacheBlock(0)));
    c_stats = CacheStatistics();
    findCactiCacheStatistics();
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
    c_stats = CacheStatistics();
}


/***************************
 * CACHE PRIVATE FUNCTIONS *
****************************/
 
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


int Cache::getSetNumber(uint64_t addr)
{
    uint64_t temp = addr >> n_blockOffsetBits; //block offset bits are removed
    uint64_t mask = 1 << (n_indexBits -1);
    return temp & mask;
}


uint64_t Cache::getTag(uint64_t addr)
{
    uint64_t mask = 64 - n_tagBits;
    uint64_t temp_tag = addr & mask;
    uint64_t tag = temp_tag >> (64 - n_tagBits);
    return tag;
}


CacheBlock Cache::evictAndReplaceBlock(CacheBlock incoming_cache_block, int set_num, int lru_idx)
{
    // lru_idx will be invalid block idx if exists
    if(lru_idx == -1)   lru_idx = findLRUBlock(set_num);

    CacheBlock lruCacheBlock = cache[set_num][lru_idx];
    cache[set_num][lru_idx] = incoming_cache_block;
    return lruCacheBlock;
}


void Cache::findCactiCacheStatistics()
{
    int cacti_result = get_cacti_results(cache_size, block_size, assoc, &c_stats.hitTime, &c_stats.energy, &c_stats.area);

    if(cacti_result > 0)    // Cacti failed for this cache configuration
    {
        c_stats.hitTime = 0.2;
    }
}


/***************************
 * CACHE PUBLIC FUNCTIONS *
****************************/
 
pair<bool, int> Cache::lookupBlock(int set_num, uint64_t tag)
{
    int max_lru_val = -1;
    int max_lru_idx = -1;
    int invalid_idx = -1;
    int hit_idx = -1;

    for(int i = 0; i < assoc; i++)
    {
        if(cache[set_num][i].tag == tag)
        {
            hit_idx = i;
            break;
        }
        else if(invalid_idx == -1)
        {
            if(cache[set_num][i].valid_bit == false)
            {
                invalid_idx = i;
            }
            else if(cache[set_num][i].lru_counter > max_lru_val)
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
    else if(invalid_idx != -1) // invalid block exists
    {
        return_idx = invalid_idx;
    }
    else
    {
        return_idx = max_lru_idx;
    }

    return make_pair(isHit, return_idx);
}


pair<bool, pair<int,CacheBlock>> Cache::readBlock(uint64_t addr)
{
    c_stats.n_reads++;  // Read request
    pair<bool, pair<int,CacheBlock>> result = make_pair(false, make_pair(-1, CacheBlock(0)));

    int set_num = getSetNumber(addr);
    uint64_t tag = getTag(addr);

    pair<bool, int> lookupResult = lookupBlock(set_num, tag);

    if(lookupResult.first == false) // cache miss
    {
        c_stats.n_read_misses++;
        result.first = lookupResult.first;
        result.second.first = lookupResult.second;
        result.second.second = cache[set_num][lookupResult.second];

        if(cache[set_num][lookupResult.second].valid_bit == false)    // Need not involve VC as this cache itself has empty space
        {
            // Invalid block should be replaced with new Block with the given addr
            CacheBlock newBlock = CacheBlock(tag);
            evictAndReplaceBlock(newBlock, set_num, lookupResult.second);
        }
        else    
        {
            if(isVCEnabled) 
            {
                // Sends a read request to VC
                auto vc_readResult = vc_cache->readBlock(addr);
                c_stats.n_swap_requests++;

                if(vc_readResult.first == true) // VC hit
                {
                    swapBlocks(set_num, lookupResult.second, vc_readResult.second.first);
                    result.first = true;
                    result.second.second = cache[set_num][lookupResult.second];
                    c_stats.n_swaps++;
                }
                else    // VC miss
                {
                    // Readblock function replaces with new block -> So VC read request would have replaced with new block
                    // But LRU of this cache should be replaced in VC and newBlock in this cache
                    vc_cache->cache[0][vc_readResult.second.first] = cache[set_num][lookupResult.second];
                    result.second.second = vc_readResult.second.second;
                    
                    cache[set_num][lookupResult.second] = CacheBlock(tag);   // newBlock
                }
            }
            else
            {
                // LRU block should be replaced with new Block with the given addr
                CacheBlock newBlock = CacheBlock(tag);
                evictAndReplaceBlock(newBlock, set_num, lookupResult.second);
            }
        }
    }

    incrementLRUCounters(set_num);
    cache[set_num][lookupResult.second].lru_counter = 0;    // read request makes it MRU block

    // Checking if evicted block is dirty to count write_backs
    if(result.first == false)   // finally if its a miss
    {
        if(result.second.second.valid_bit == true && result.second.second.dirty_bit == true)
        {
            c_stats.n_writebacks++; // write back to next level will be handled at simulator
        }
    }

    return result;
}


pair<bool, pair<int,CacheBlock>> Cache::writeBlock(uint64_t addr)
{
    c_stats.n_writes++;
    pair<bool, pair<int,CacheBlock>> result = make_pair(false, make_pair(-1, CacheBlock(0)));

    int set_num = getSetNumber(addr);
    uint64_t tag = getTag(addr);

    pair<bool, int> lookupResult = lookupBlock(set_num, tag);

    if(lookupResult.first == false)     // cache-write miss
    {
        c_stats.n_write_misses++;
        result.first = lookupResult.first;
        result.second.first = lookupResult.second;
        result.second.second = cache[set_num][lookupResult.second];

        if(cache[set_num][lookupResult.second].valid_bit == false)    // Need not involve VC as this cache itself has empty space
        {
            // Invalid block should be replaced with new Block with the given addr
            CacheBlock newBlock = CacheBlock(tag);
            evictAndReplaceBlock(newBlock, set_num, lookupResult.second);

            // after allocation, data will be written => block becomes dirty
            cache[set_num][lookupResult.second].dirty_bit = true;  
        }
        else
        {
            if(isVCEnabled)
            {
                // Sends a write requesr to VC
                auto vc_writeResult = vc_cache->writeBlock(addr);
                c_stats.n_swap_requests++;

                if(vc_writeResult.first == true)    // VC hit
                {
                    swapBlocks(set_num, lookupResult.second, vc_writeResult.second.first);
                    result.first = true;
                    result.second.second = cache[set_num][lookupResult.second];
                    cache[set_num][lookupResult.second].dirty_bit = true;
                    c_stats.n_swaps++;
                }
                else    // VC miss
                {
                    // Writeblock function replaces with new block when missed-> So VC write request would have replaced with new block
                    // But LRU of this cache should be replaced in VC and newBlock in this cache
                    vc_cache->cache[0][vc_writeResult.second.first] = cache[set_num][lookupResult.second];
                    result.second.second = vc_writeResult.second.second;

                    cache[set_num][lookupResult.second] = CacheBlock(tag);   // newBlock
                    cache[set_num][lookupResult.second].dirty_bit = true;
                }
            }
            else
            {
                // LRU block should be replaced with new Block with the given addr
                CacheBlock newBlock = CacheBlock(tag);
                evictAndReplaceBlock(newBlock, set_num, lookupResult.second);
                cache[set_num][lookupResult.second].dirty_bit = true;
            }
        }
    }

    incrementLRUCounters(set_num);
    cache[set_num][lookupResult.second].lru_counter = 0;    // write request makes it MRU block

    // Checking if evicted block is dirty to count write_backs
    if(result.first == false)   // finally if its a miss
    {
        if(result.second.second.valid_bit == true && result.second.second.dirty_bit == true)
        {
            c_stats.n_writebacks++; // write back to next level will be handled at simulator
        }
    }

    return result;
}

void Cache::swapBlocks(int l1_set_num, int l1_idx, int vc_idx)
{
    CacheBlock temp = cache[l1_set_num][l1_idx];
    cache[l1_set_num][l1_idx] = vc_cache->cache[0][vc_idx];
    vc_cache->cache[0][vc_idx] = temp;
}


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
            cout << cb.tag << " ";

            if(cb.dirty_bit == true)
                cout << "D\t";
            else
                cout << " \t";
        }
    }
}


