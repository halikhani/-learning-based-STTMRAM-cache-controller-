#pragma once

#include "cache.h"
#include <vector>

Cache* createCacheInstance(std::string& policy, long long cs, long long bs, long long sa, int level);


// classes defined
class CacheModel;


// cache class
class CacheModel{

    private:
        std::vector<Cache*> cache;
        int levels;
        
        long long* oldBlockBaseAddress;
        long long* newBlockBaseAddress;
        bool* miss;

    public:
        ~CacheModel();
        CacheModel(const char* descr);

        void access(long long address, bool write);
        void getLastAccessStats(int level, bool* miss, long long* newBlockBaseAddress, long long* oldBlockBaseAddress);
        long long getBlockSize(int level);
        int getLevels();
        
        void printCacheStatus(FILE* fp);
};
