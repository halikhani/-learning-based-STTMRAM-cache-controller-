#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>

// classes defined
class Cache;

// helper functions
bool isValidConfig(long long cs, long long bs, long long sa);
long long hexToDec(char hexVal[]);
int log2(long long x);

void incReads();
void incWrites();
long long getReads();
long long getWrites();

Cache* createCacheInstance(std::string& policy, long long cs, long long bs, long long sa, int level);



// cache class
class Cache{

    private:
        long long hits, misses;
        long long* cacheBlocks;
        int level;
        std::string policy;

    public:
        void incHits();
        void incMisses();
        int getLevel();
        std::string getPolicy();
        long long getTag(long long address);
        long long getIndex(long long address);
        long long getBlockPosition(long long address);
        long long getCacheBlockTag(long long blockPosition);
        void insert(long long address, long long blockToReplace);
        long long getOffsetSize();
        long long getTagSize();
        long long getIndexSize();
        long long getBlockSize();

        long long getHits();
        long long getMisses();
        float getHitRate();

        virtual long long getBlockToReplace(long long address) = 0;
        virtual void update(long long blockToReplace, int status) = 0;

        virtual ~Cache();

    protected:
        Cache(long long cacheSize, long long blockSize, long long setAssociativity, int level, std::string policy);
        long long cacheSize;
        long long blockSize;
        long long setAssociativity;
        long long numberOfSets;
        int offsetSize;
        int indexSize;
};
