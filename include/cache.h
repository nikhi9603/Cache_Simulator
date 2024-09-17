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
    uint lru_counter;

    CacheBlock(uint64_t tag) {}
};

struct CacheStatistics
{
    uint n_reads = 0; 
    uint n_read_misses = 0;
    uint n_writes = 0;
    uint n_write_misses = 0;
    uint n_swap_requests = 0;
    uint n_swaps = 0;       // between L1, VC
    uint n_writebacks = 0;  // # of writebacks from Li or its VC(if enabled) to next level
    float hitTime = 0;
    float energy = 0;
    float area = 0;
    CacheStatistics* vc_statistics;

    CacheStatistics() {vc_statistics = new CacheStatistics();}
};

/**
 * @brief Set-Associative Cache
 */
class Cache
{
private:
    uint cache_size;
    uint assoc;
    uint block_size;
    uint n_sets;

    bool isVCEnabled;
    uint n_vc_blocks;

    uint n_tagBits;
    uint n_indexBits;
    uint n_blockOffsetBits;

    Cache* vc_cache; 
    vector<vector<CacheBlock>> cache;      

    CacheStatistics c_stats;

    /*
     * @brief Finds LRU Block or Invalid block in the cache_set
     * @param set_num  set number in which LRU/invalid block has to be found
     * @return 
     * - If there exits an `invalid` block, then it returns index of invalid cache block
     * 
     * - Else LRU block index
     */
    int findLRUBlock(int set_num);

    /*
     * @brief Increments the lru counters of all `valid` cache_blocks in the cache_set
     * 
     * @param set_num set number of cache_set 
     */
    void incrementLRUCounters(int set_num);

    int getSetNumber(uint64_t addr) {};
    uint64_t getTag(uint64_t addr) {};

    /*
     * @brief Evicts the LRU/invalid block to place new_block in the cache_set.  
        
        (LRU counters are not incremented)
     * 
     * - If there exists an `invalid` block in the cache_set, it considers to replace with invalid block. 
     * 
     * - Else lru block.
     * 
     * @param incoming_cache_block new block which needs to replace LRU/invalid block
     * @param set_num set number in which replacement happens
     * @param idx index of LRU/invalid block in the cache_set if known else give -1
     * @return LRU/Invalid cache block that's evicted from the cache
     */
    CacheBlock evictAndReplaceBlock(CacheBlock incoming_cache_block, int set_num, int lru_idx);

    void findCactiCacheStatistics();
public:
    Cache() {};

    /*
     * @param n_vc_blocks number of victim cache blocks (If 0 => Victim Cache is disabled)
     */
    Cache(int cache_size, int assoc, int block_size, int n_vc_blocks) {}

    /*
     * @return
     *   - When returned bool=true(lookup - hit), int=index of block found in cache_set with same tag
     * @return 
     *   - When returned bool=false(lookup - miss), int=index of `invalid block` if exists, else `lru block` index
     */
    pair<bool, int> lookupBlock(int set_num, uint64_t tag);      

    /*  @brief Reads the block at given addr (For the sake of simulation, we are not considering data in the blocks)
     *  @return 
     *  1. When returned bool=false(read - miss):
     * 
     *          - CacheBlock = LRU/Invalid cache block in corresponding cache set (that's evicted) 
     * 
     *          - int = previous index of LRU/invalid cache block that's evicted.
     * 
     *  2. When returned bool=true(read - hit):
     *    
     *    - CacheBlock = cache block found in corresponding cache set
     * 
     *    - int = index of cache block found in corresponding cache set
    */
    pair<bool, pair<int,CacheBlock>> readBlock(uint64_t addr);

    /* 
     *  @brief Writes data to the block at given addr (For the sake of simulation, we are not considering data in the block)
     *  @return 
     *  1. When returned bool=false(write miss):
     * 
     *          - CacheBlock = LRU/Invalid cache block in corresponding cache set (that's evicted) 
     * 
     *          - int = -1(as it is deleted now)
     * 
     *  2. When returned bool=true(write - hit):
     *    
     *    - CacheBlock = cache block found in corresponding cache set
     * 
     *    - int = index of cache block found in corresponding cache set
    */
    pair<bool, pair<int,CacheBlock>> writeBlock(uint64_t addr);


    /*
     * @brief Swapping blocks between L1 and its victim cache(VC)
     * 
     * @param l1_set_num set number of L1 cache_block
     * @param l1_idx index of L1 cache_block in cache_set `l1_set_num`
     * @param vc_idx index of VC cache_block in set 0 (since VC is fully associative)
     */
    void swapBlocks(int l1_set_num, int l1_idx, int vc_idx);

    /*
     * @brief prints the cache_blocks within each cache_set in the order of their recency of access (i.e. lru block at the last)
     */
    void printCacheContents();

    /*
     * @brief Returns Cache Simulation Statistics
     */
    CacheStatistics getCacheStatistics() {return c_stats;}
};

#endif