#ifndef CACHE_SIM_H
#define CACHE_SIM_H

#include<iostream>
#include<vector>
#include "cache.h"
using namespace std;

class CacheSimulator
{
private:
    Cache l1_cache;
    Cache l2_cache;
    bool isVCenabled;
    bool isL2Exist;

public:
    CacheSimulator(uint l1_size, uint l1_assoc, uint l1_blocksize,
                   uint n_vc_blocks,
                   uint l2_size, uint l2_assoc,
                   vector<pair<string, uint64_t>> trace_contents) {}
};

#endif