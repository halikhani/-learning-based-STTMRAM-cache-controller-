#include "cache_model.h"
#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "plru.h"
#include "lru.h"
#include "srrip.h"
#include "nru.h"
#include "lfu.h"
#include "fifo.h"

Cache* createCacheInstance(std::string& policy, long long cs, long long bs, long long sa, int level){
    // check validity here and exit if invalid
    if(policy == "plru"){
        Cache* cache = new PLRU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "lru"){
        Cache* cache = new LRU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "srrip"){
        Cache* cache = new SRRIP(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "nru"){
        Cache* cache = new NRU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "lfu"){
        Cache* cache = new LFU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "fifo"){
        Cache* cache = new FIFO(cs, bs, sa, level);
        return cache;
    }
    std::cout << "unreachable code" << std::endl;
    exit(1);
}

// CacheModel class

CacheModel::CacheModel(const char* descr){
    std::ifstream params;
    params.open(descr);
    std::string word;
    params >> word;
    this->levels = std::stoi(word.c_str());
//    std::cout << "levels " << this->levels << std::endl;

    this->cache = std::vector<Cache*>(levels);

    int iterator = 0;
    while(iterator < this->levels){
        std::string policy;
        params >> policy;
        long long cs, bs, sa; //cacheSize, blockSize, setAssociativity
        params >> word; cs = std::stoll(word.c_str());
        params >> word; bs = std::stoll(word.c_str());
        params >> word; sa = std::stoll(word.c_str());
        this->cache[iterator] = createCacheInstance(policy, cs, bs, sa, iterator+1);
        iterator++;
    }
    params.close();
    
    this->newBlockBaseAddress = (long long*) malloc(this->levels * sizeof(long long));
    this->oldBlockBaseAddress = (long long*) malloc(this->levels * sizeof(long long));
    this->miss = (bool*) malloc(this->levels * sizeof(bool));
    for(int i=0; i<this->levels; i++) {
        this->newBlockBaseAddress[i] = -1;
        this->oldBlockBaseAddress[i] = -1;
        this->miss[i] = true;
    }
}


CacheModel::~CacheModel(){
    for(int levelItr=0; levelItr<this->levels; levelItr++)
        delete cache[levelItr];
    free(this->newBlockBaseAddress);
    free(this->oldBlockBaseAddress);
    free(this->miss);
}

void CacheModel::access(long long address, bool write){
    //TODO: the prototype requires a "write" parameter requied for implementing a write back policy (still TODO)
    //In particular it will be necessary to enhance the current model by adding a DIRTY flag.
    //Moreover the write error injection requires the actual data to be saved in the cache model to inject errors on actually written bits
    bool found;
    int levelItr;
    
    //DO NOTE: this loop CANNOT be interrupted prematurely using found flag since, it has to update all values in miss, newBlockBaseAddress and oldBlockBaseAddress
    for(levelItr=0, found=false; levelItr<this->levels; levelItr++){  
        long long block = this->cache[levelItr]->getBlockPosition(address);
        // getBlockPosition will be implemented in cache.cpp
        if(!found){
            if(block == -1){ //cache miss
                this->cache[levelItr]->incMisses();
                // incMisses will be implemented in cache.cpp
                long long blockToReplace = this->cache[levelItr]->getBlockToReplace(address);

                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // update attributes used for error injection. indeed there is no block replacement so new block and old one are the same
                // here we are replacing a block
                this->miss[levelItr] = true;
                long long oldTag= this->cache[levelItr]->getCacheBlockTag(blockToReplace);
                if(oldTag==-1) //no valid page in the cache
                    this->oldBlockBaseAddress[levelItr] = -1;
                else {
                    long long oldIndex = this->cache[levelItr]->getIndex(address);
                    this->oldBlockBaseAddress[levelItr] = (oldTag << (this->cache[levelItr]->getIndexSize() + this->cache[levelItr]->getOffsetSize())) | (oldIndex <<this->cache[levelItr]->getOffsetSize());
                } 
                //Dummy trick to put all offset bits to 0
                this->newBlockBaseAddress[levelItr] = address >> this->cache[levelItr]->getOffsetSize() << this->cache[levelItr]->getOffsetSize();
                
                //std::cout << "M C: " << levelItr << " B: "  << blockToReplace << " O: " << this->oldBlockBaseAddress[levelItr] << " N: " << this->newBlockBaseAddress[levelItr] << " ";
                //std::cout << "Index: " << this->cache[levelItr]->getIndex(this->oldBlockBaseAddress[levelItr]) << " " << this->cache[levelItr]->getIndex(this->newBlockBaseAddress[levelItr]) << " ";
                //std::cout << this->cache[levelItr]->getTagSize()<< " " << this->cache[levelItr]->getIndexSize() << " " << this->cache[levelItr]->getOffsetSize() <<  std::endl;
                
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                // getBlockToEvict will be implemented in policy.cpp
                this->cache[levelItr]->insert(address, blockToReplace);
                // insert will be implemented in cache.cpp
                this->cache[levelItr]->update(blockToReplace, 0);
                // update will be implemented in policy.cpp; will include updating the tree as in plru or updating the count as in lfu; 0 denotes miss                
            }
            else{ //cache hit
                this->cache[levelItr]->incHits();
                // incHits will be implemented in cache.cpp
                this->cache[levelItr]->update(block, 1);
                // update will be implemented in policy.cpp; 1 denotes hit
                found=true;

                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // update attributes used for error injection. indeed there is no block replacement so new block and old one are the same
                this->miss[levelItr] = false;
                this->newBlockBaseAddress[levelItr] = address >> this->cache[levelItr]->getOffsetSize() << this->cache[levelItr]->getOffsetSize(); //the offset bits are put to 0
                this->oldBlockBaseAddress[levelItr] = this->newBlockBaseAddress[levelItr];                
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //std::cout << "H C: " << levelItr << " B: " << block << " O: " << this->oldBlockBaseAddress[levelItr] << " N: " << this->newBlockBaseAddress[levelItr] << " " << std::endl;
            }
        } else{ //DO NOTE if there was a cache hit in a higher cache level, I just need to update variables. NO CACHE HIT at this time!
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // update attributes used for error injection. indeed there is no block replacement so new block and old one are the same
            this->miss[levelItr] = false; //DO NOTE: I assume an inclusive cache model
            this->newBlockBaseAddress[levelItr] = address >> this->cache[levelItr]->getOffsetSize() << this->cache[levelItr]->getOffsetSize(); //the offset bits are put to 0
            this->oldBlockBaseAddress[levelItr] = this->newBlockBaseAddress[levelItr];                
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //std::cout << "F C: " << levelItr << " B: " << block << " O: " << this->oldBlockBaseAddress[levelItr] << " N: " << this->newBlockBaseAddress[levelItr] << " " << std::endl;
        }
    }
}

void CacheModel::getLastAccessStats(int level, bool* miss, long long* newBlockBaseAddress, long long* oldBlockBaseAddress) {
    *miss = this->miss[level-1]; //DO NOTE: the array index spans from 0 to n-1; the levels from 1 to n 
    *newBlockBaseAddress = this->newBlockBaseAddress[level-1];
    *oldBlockBaseAddress = this->oldBlockBaseAddress[level-1];
}


long long CacheModel::getBlockSize(int level){
    return this->cache[level-1]->getBlockSize();
}

int CacheModel::getLevels(){
    return this->levels;
}

void CacheModel::printCacheStatus(FILE* fp){
    for(int i=0; i<this->levels; i++){
        fprintf(fp, "L%d: %s\t\t\t\t\t\tHit Rate: %f\n",  
            this->cache[i]->getLevel(), this->cache[i]->getPolicy().c_str(), 
            this->cache[i]->getHitRate());
        fprintf(fp, "Accesses: %lld\t\tHits: %lld\t\tMisses: %lld\n",
            this->cache[i]->getHits()+this->cache[i]->getMisses(), 
            this->cache[i]->getHits(), this->cache[i]->getMisses());
        fprintf(fp, "\n");    
    }
}

