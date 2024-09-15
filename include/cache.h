#ifndef CACHE_H
#define CACHE_H

#include<iostream>
#include<vector>
using namespace std;

class CacheBlock
{
public:
    uint64_t tag;
    bool valid_bit;
    bool dirty_bit;
    int lru_counter;

    CacheBlock(uint64_t tag) {}
};

class CacheStatistics
{
public:
    uint n_reads = 0;
    uint n_read_misses = 0;
    uint n_writes = 0;
    uint n_write_misses = 0;
    uint n_swap_requests = 0;
    uint n_swaps = 0;       // between L1, VC
    uint n_writebacks = 0;  // # of writebacks from Li or its VC(if enabled) to next level
    double hitTime;
    double energy;
    double area;

    CacheStatistics* vc_statistics;
};

/**
 * @brief Set-Associative Cache
 */
class Cache
{
private:
    uint block_size;
    uint assoc;
    uint cache_size;
    uint n_sets;
    bool isVCEnabled;
    uint n_vc_blocks;
    int n_setBits;
    int n_indexBits;
    int n_tagBits;

    Cache* vc_cache; 
    vector<vector<CacheBlock>> cache;      

    CacheStatistics c_stats;

    /*
     * @param set_num  set number in which LRU block has to be found
     * @return Index of LRU block in the given set of set-associative cache.
     */
    int findLRUBlock(int set_num);

    int getSetNumber(uint64_t addr);
    uint64_t getTag(uint64_t addr);

    /*
     * @brief Evicts the LRU block to place new_block in that cache_set
     * 
     * @param incoming_cache_block new block which needs to replace LRU block
     * @param set_num set number for which lru eviction and replacement happens in that set of the cache
     * @param idx index of LRU block in the cache_set if known else give -1
     * @return LRU cache block that's evicted from the cache
     */
    CacheBlock evictAndReplaceBlock(CacheBlock incoming_cache_block, int set_num, int idx);
public:
    Cache() {};
    /*
     * @param n_vc_blocks number of victim cache blocks (If 0 => Victim Cache is disabled)
     */
    Cache(int cache_size, int assoc, int block_size, int n_vc_blocks) {}

    /*
     * @return 
     *   - When returned bool=false(lookup - miss), int=index of LRU block in cache_set
     *  @return
     *   - When returned bool=true(lookup -hit), int=index of block found in cache_set with same tag
     */
    pair<bool, int> lookupBlock(uint64_t tag);      

    /** 
     *  @return 
     *  1. When returned bool=false(read - miss):
     * 
     *          - CacheBlock = LRU cache block in corresponding cache set (that's evicted) 
     * 
     *          - int = -1(as it is deleted now)
     * 
     *  2. When returned bool=true(read - hit):
     *    
     *    - CacheBlock = cache block found in corresponding cache set
     * 
     *    - int = index of cache block found in corresponding cache set
    */
    pair<bool, pair<int,CacheBlock>> readBlock(uint64_t addr);

    /** 
     *  @return 
     *  1. When returned bool=false(write miss):
     * 
     *          - CacheBlock = LRU cache block in corresponding cache set (that's evicted) 
     * 
     *          - int = -1(as it is deleted now)
     * 
     *  2. When returned bool=true(write - hit):
     *    
     *    - CacheBlock = cache block found in corresponding cache set
     * 
     *    - int = index of cache block found in corresponding cache set
    */
    pair<bool, CacheBlock> writeBlock(uint64_t addr);


    void swapBlocks();

    /*
     * @brief prints the cache_blocks within each cache_set in the order of their recency of access (i.e. lru block at the last)
     */
    void printCacheContents();
};

#endif