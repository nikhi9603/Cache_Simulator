#ifndef CACHE_H
#define CACHE_H

#include<iostream>
#include<vector>
using namespace std;

class CacheBlock
{
public:
    uint32_t tag;
    bool valid_bit;
    bool dirty_bit;
    int lru_counter;

    CacheBlock();
    CacheBlock(long long int tag);
};

struct CacheStatistics
{
    int n_reads = 0; 
    int n_read_misses = 0;
    int n_writes = 0;
    int n_write_misses = 0;
    int n_swap_requests = 0;
    int n_swaps = 0;       // between L1, VC
    int n_writebacks = 0;  // # of writebacks from Li or its VC(if enabled) to next level
    float hitTime = 0;
    float energy = 0;
    float area = 0;
    CacheStatistics* vc_statistics;

    // CacheStatistics() {vc_statistics = new CacheStatistics();}
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
    vector<vector<CacheBlock>> cache;

    CacheStatistics c_stats;

    /*
     * @brief It checks for cache_block only in its cache_set but not its VC
     * @return
     *   - When returned bool=true(lookup - hit), int=index of block found in cache_set with same tag
     * @return 
     *   - When returned bool=false(lookup - miss), int=index of `invalid block` if exists, else `lru block` index
     */
    pair<bool, int> lookupBlock(int set_num, long long int tag);      

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
    void incrementLRUCounters(int set_num, int idx);

    /*
     * @brief Swapping blocks between L1 and its victim cache(VC)
     * 
     * @param l1_set_num set number of L1 cache_block
     * @param l1_idx index of L1 cache_block in cache_set `l1_set_num`
     * @param vc_idx index of VC cache_block in set 0 (since VC is fully associative)
     */
    void swapBlocks(int l1_set_num, int l1_idx, int vc_idx);

    void findCactiCacheStatistics();

public:
    Cache* vc_cache;
    void printCacheSet(int set_num);
    Cache();

    /*
     * @param n_vc_blocks number of victim cache blocks (If 0 => Victim Cache is disabled)
     */
    Cache(int cache_size, int assoc, int block_size, int n_vc_blocks);

    int getSetNumber(long long int addr);
    long long int getTag(long long int addr);
    long long int getBlockAddress(int set_num, long long int tag);
    
    /*  @brief Reads the block at given addr 
     *  @return 
     *  1. When returned bool=false(read - miss):
     * 
     *  int = index of LRU/invalid cache block that should be evicted to allocate new block
     * 
     *  2. When returned bool=true(read - hit):
     *    
     *  int = index of cache block found in corresponding cache set
    */
    pair<bool, pair<int, CacheBlock>> lookupRead(long long int addr);

    /* 
     *  @brief Writes data to the block at given addr
     *  @return 
     *  1. When returned bool=false(write miss):
     * 
     *  int = index of LRU/invalid cache block that should be evicted to allocate new block
     * 
     *  2. When returned bool=true(write - hit):
     * 
     *  int = index of cache block found in corresponding cache set
    */
    pair<bool, pair<int, CacheBlock>> lookupWrite(long long int addr);

    // NOTE: lookupRead and lookupWrite are actually doing the same as they are not really reading/write in this function
    // Considered into two for now so that no of read misses etc.. can be counted seperately

    // void readData(int set_num, int idx);

    /*
     * @brief since its simulation, data is not taken as arg to write
     */
    void writeData(int set_num, int idx);

    CacheBlock getBlock(int set_num, int idx);
    
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


    // /*
    //  * @brief Evicts the LRU/invalid block from the cache_set
    //  * 
    //  * @param set_num - cache_set number
    //  * @param lru_idx   Index of block to evict. (If not known, give it as -1)
    //  * @return CacheBlock that's evicted
    //  */
    // CacheBlock evictBlock(int set_num, int lru_idx);

    // /*
    //  * @brief Replace LRU/invalid block from the cache_set with the new block
    //  * 
    //  * @param lru_idx Index of block to replace with new cache_block. (If not known, give it as -1)
    //  */
    // void replaceBlock(CacheBlock incoming_cache_block, int set_num, int lru_idx);

    /*
     * @brief prints the cache_blocks within each cache_set in the order of their recency of access (i.e. lru block at the last)
     */
    void printCacheContents();

    /*
     * @brief Returns Cache Simulation Statistics
     */
    CacheStatistics getCacheStatistics() {return c_stats;}

    void unsetDirty(int set_num, int idx);
};

#endif