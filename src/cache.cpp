#include "cache.h"
#include<cmath>

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
    int min_val = cache[set_num][0].lru_counter;
    int min_idx = 0;

    for(int i = 1; i < assoc; i++)
    {
        if(cache[set_num][i].lru_counter < min_val)
        {
            min_val = cache[set_num][i].lru_counter;
            min_idx = i;
        }
    }
    return min_idx;
}


void Cache::incrementLRUCounters(int set_num, int idx)
{
    
}
