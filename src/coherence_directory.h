#ifndef COHERENCE_DIRECTORY_H_
#define COHERENCE_DIRECTORY_H_

#include <bitset>
#include "g_std/g_string.h"
#include "g_std/g_list.h"
#include "g_std/g_unordered_map.h"
#include "memory_hierarchy.h"
#include "cache.h"
#include "coherence_ctrls.h"
#include "constants.h"
#include "pad.h"
#include "stats.h"
#include "locks.h"

class Cache;
class Network;

class CDMESIBottomCC : public GlobAlloc {
    private:
        g_vector<MemObject*> parents;
        g_vector<uint32_t> parentRTTs;
        uint32_t numLines;
        uint32_t selfId;

        //Profiling counters
        // Counter profGETSHit, profGETSMiss, profGETXHit, profGETXMissIM /*from invalid*/, profGETXMissSM /*from S, i.e. upgrade misses*/;
        // Counter profINV, profINVX, profFWD /*received from upstream*/;
        //Counter profWBIncl, profWBCoh /* writebacks due to inclusion or coherence, received from downstream, does not include PUTS */;
        // TODO: Measuring writebacks is messy, do if needed
        Counter profGETNextLevelLat, profGETNetLat;

        // Counter profGETSCache;

        bool nonInclusiveHack;

        PAD();
        lock_t ccLock;
        PAD();

    public:
        CDMESIBottomCC(uint32_t _numLines, uint32_t _selfId, bool _nonInclusiveHack) : numLines(_numLines), selfId(_selfId), nonInclusiveHack(_nonInclusiveHack) {
            futex_init(&ccLock);
        }
        MemObject* getParent( Address lineAddr){return parents[getParentId(lineAddr)];}
        MemObject* getParent(uint32_t srcId, Address lineAddr){return parents[getParentId(srcId,lineAddr)];}

        void init(const g_vector<MemObject*>& _parents, Network* network, const char* name);

        void initStats(AggregateStat* parentStat) {
            // profGETSHit.init("hGETS", "GETS hits");
            // profGETXHit.init("hGETX", "GETX hits");
            // profGETSMiss.init("mGETS", "GETS misses");
            // profGETXMissIM.init("mGETXIM", "GETX I->M misses");
            // profGETXMissSM.init("mGETXSM", "GETX S->M misses (upgrade misses)");
            
            // profINV.init("INV", "Invalidates (from upper level)");
            // profINVX.init("INVX", "Downgrades (from upper level)");
            // profFWD.init("FWD", "Forwards (from upper level)");
            // profGETSCache.init("GETSCache", "Total GETS from an existing copy");
            profGETNextLevelLat.init("latGETnl", "GET request latency on next level");
            profGETNetLat.init("latGETnet", "GET request latency on network to next level");

            // parentStat->append(&profGETSHit);
            // parentStat->append(&profGETXHit);
            // parentStat->append(&profGETSMiss);
            // parentStat->append(&profGETXMissIM);
            // parentStat->append(&profGETXMissSM);
            // parentStat->append(&profINV);
            // parentStat->append(&profINVX);
            // parentStat->append(&profFWD);
            // parentStat->append(&profGETSCache);
            parentStat->append(&profGETNextLevelLat);
            parentStat->append(&profGETNetLat);
        }

        uint64_t processAccess(Address lineAddr, uint32_t lineId, AccessType type, uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId, uint32_t threadId, bool isPIMInst);
        
        inline void lock() {
            futex_lock(&ccLock);
        }

        inline void unlock() {
            futex_unlock(&ccLock);
        }

        uint64_t bypassAccess(MemReq &req){
            return parents[getParentId(req.srcId,req.lineAddr)]->access(req);
        }

        uint64_t accessProxy(Address lineAddr, AccessType type, uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId, uint32_t threadId, bool isPIMInst){
            uint64_t respCycle = cycle;
            uint32_t parentId = getParentId(srcId,lineAddr);
            MESIState state = MESIState::I;
            MemReq req = {lineAddr, type, selfId, &state, cycle, &ccLock, state, srcId, flags,  coreId};
            req.threadId = threadId;
            req.isPIMInst = isPIMInst;
            uint32_t nextLevelLat = parents[parentId]->access(req) - cycle;
            uint32_t netLat = parentRTTs[parentId];
            respCycle += nextLevelLat + netLat;
            return respCycle;
        }

        uint64_t processWriteBack(Address lineAddr, AccessType type, uint64_t cycle, uint32_t srcId, uint32_t coreId,uint32_t threadId, bool isPIMInst);

        void processInval(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback);

        //Could extend with isExclusive, isDirty, etc, but not needed for now.

    private:
        uint32_t getParentId(Address lineAddr);
        uint32_t getParentId(uint32_t srcid, Address lineAddr);
};


//Implements the "top" part: Keeps directory information, handles downgrades and invalidates
class CDMESITopCC : public GlobAlloc {
    private:
        struct Entry {
            uint32_t numSharers;
            std::bitset<MAX_CACHE_CHILDREN> sharers;
            bool exclusive;

            void clear() {
                exclusive = false;
                numSharers = 0;
                sharers.reset();
            }

            bool isEmpty() {
                return numSharers == 0;
            }

            bool isExclusive() {
                return (numSharers == 1) && (exclusive);
            }
        };

        Entry* array;
        g_vector<BaseCache*> children;
        g_vector<uint32_t> childrenRTTs;
        uint32_t numLines;
        uint32_t cache_children_num;

        bool nonInclusiveHack;

        Counter sentInvs;
        Counter profGETSCache;
        Counter profINV, profINVX;

        PAD();
        lock_t ccLock;
        PAD();

    public:
        CDMESITopCC(uint32_t _numLines, bool _nonInclusiveHack) : numLines(_numLines), nonInclusiveHack(_nonInclusiveHack) {
            array = gm_calloc<Entry>(numLines);
            for (uint32_t i = 0; i < numLines; i++) {
                array[i].clear();
            }

            futex_init(&ccLock);
        }
        MemObject* getParent( Address lineAddr){return NULL;}
        MemObject* getParent(uint32_t srcId, Address lineAddr){return NULL;}

        void init(const g_vector<BaseCache*>& _children, Network* network, const char* name);

        uint32_t getChildrenNum() {return children.size();}

        uint64_t processAccess(Address lineAddr, uint32_t lineId, AccessType type, uint32_t childId,
                MESIState* childState, bool* inducedWriteback, uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId);
        
        uint64_t processWriteBack(Address lineAddr, uint32_t lineId, AccessType type,uint32_t childId, MESIState* childState, bool* evictEntry,uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId);

        uint64_t processInval(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId);

        inline void lock() {
            futex_lock(&ccLock);
        }

        inline void unlock() {
            futex_unlock(&ccLock);
        }

        void initStats(AggregateStat* parentStat) {
            sentInvs.init("sentInvs", "Total sent invs"); parentStat->append(&sentInvs);
            profGETSCache.init("GETSCache", "Total GETS from an existing copy");parentStat->append(&profGETSCache);
            profINV.init("INV", "Invalidates (to lower level)"); parentStat->append(&profINV);
            profINVX.init("INVX", "Downgrades (to  lower level)"); parentStat->append(&profINVX);
        }

        /* Replacement policy query interface */
        inline uint32_t numSharers(uint32_t lineId) {
            return array[lineId].numSharers;
        }

        uint64_t sendAllInvalidates(Address lineAddr, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId);

    private:
        uint64_t sendInvalidates(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId, bool is_clflush = false);
};

// Non-terminal CC; accepts GETS/X and PUTS/X accesses
class CDMESICC : public CC {
    private:
        CDMESITopCC* tcc;
        CDMESIBottomCC* bcc;
        uint32_t numLines;
        bool nonInclusiveHack;

        g_string name;

    public:
        //Initialization
        CDMESICC(uint32_t _numLines, bool _nonInclusiveHack, g_string& _name) : tcc(nullptr), bcc(nullptr),
            numLines(_numLines), nonInclusiveHack(_nonInclusiveHack), name(_name) {}

        void setParents(uint32_t childId, const g_vector<MemObject*>& parents, Network* network) {
            bcc = new CDMESIBottomCC(numLines, childId, nonInclusiveHack);
            bcc->init(parents, network, name.c_str());
        }

        void setChildren(const g_vector<BaseCache*>& children, Network* network) {
            tcc = new CDMESITopCC(numLines, nonInclusiveHack);
            tcc->init(children, network, name.c_str());
        }

        void addChild(BaseCache* child, Network* network) {
            panic("[%s] CDMESICC::addChild cannot be called!", name.c_str());
        }

        void addChildren(const g_vector<BaseCache*>& children, Network* network){
            panic("[%s] CDMESICC::addChildren cannot be called!", name.c_str());
        }

        uint64_t processInv(const InvReq& req, int32_t lineId, uint64_t startCycle){
            uint64_t respCycle = startCycle;
            if(lineId != -1){
                respCycle = tcc->processInval(req.lineAddr, lineId, req.type, req.writeback, startCycle, req.srcId); //send invalidates or downgrades to children
                bcc->processInval(req.lineAddr, lineId, req.type, req.writeback); //adjust our own state
            }
            bcc->unlock();
            return respCycle;
        }


        uint32_t getChildrenNum(){
            return tcc->getChildrenNum();
        }

        void initStats(AggregateStat* cacheStat) {
            bcc->initStats(cacheStat);
            tcc->initStats(cacheStat);
        }

        //Access methods
        bool startAccess(MemReq& req);

        bool shouldAllocate(const MemReq& req) {
            if ((req.type == GETS) || (req.type == GETX)) {
                return true;
            } else {
                assert((req.type == PUTS) || (req.type == PUTX));
                if (!nonInclusiveHack) {
                    panic("[%s] We lost inclusion on this line! 0x%lx, type %s, childId %d, childState %s", name.c_str(),
                            req.lineAddr, AccessTypeName(req.type), req.childId, MESIStateName(*req.state));
                }
                return false;
            }
        }

        uint64_t bypassAccess(MemReq& req) {
            return bcc->bypassAccess(req);
        }

        uint64_t processWriteThroughAccess(MemReq& req) {
            uint64_t respCycle = bcc->bypassAccess(req);

            //If write detected, invalidate all copies
            if(req.type == PUTX){
                bool lowerLevelWriteback = false;
                respCycle = tcc->sendAllInvalidates(req.lineAddr, INV, &lowerLevelWriteback, respCycle,req.srcId);
            }
            return respCycle;
        }

        uint64_t processEviction(const MemReq& triggerReq, Address wbLineAddr, int32_t lineId, uint64_t startCycle){
            panic("!?");
            return startCycle;
        }

        uint64_t processWriteBack(const MemReq& req, int32_t lineId, uint64_t startCycle,bool *evictEntry) {
            assert(lineId != -1);
            uint64_t respCycle = startCycle;
            //update data to physical
            respCycle = bcc->processWriteBack(req.lineAddr, req.type, respCycle, req.srcId, req.coreId, req.threadId, req.isPIMInst);
            //update directory
            respCycle = tcc->processWriteBack(req.lineAddr, lineId, req.type, req.childId,req.state, evictEntry, respCycle, req.srcId, req.flags, req.coreId);
            return respCycle;
        }

        uint64_t processAccess(const MemReq& req, int32_t lineId, uint64_t startCycle, uint64_t* getDoneCycle = nullptr) {
            uint64_t respCycle = startCycle;
            // printf("Line %d processAccess second branch\n",lineId);
            //Prefetches are side requests and get handled a bit differently
            bool isPrefetch = req.flags & MemReq::PREFETCH;
            assert(!isPrefetch || req.type == GETS);
            uint32_t flags = req.flags & ~MemReq::PREFETCH; //always clear PREFETCH, this flag cannot propagate up

            //get the latest copy of data from memory or sharer
            respCycle = bcc->processAccess(req.lineAddr, lineId, req.type, respCycle, req.srcId, flags, req.coreId, req.threadId, req.isPIMInst);

            if (!isPrefetch){ //prefetches only touch bcc; the demand request from the core will pull the line to lower level
                //At this point, the line is in a good state w.r.t. upper levels
                bool lowerLevelWriteback = false;
                //change directory info, invalidate other children if needed, tell requester about its state
                respCycle = tcc->processAccess(req.lineAddr, lineId, req.type, req.childId, req.state,
                                               &lowerLevelWriteback, respCycle, req.srcId, flags, req.coreId);
            }
            return respCycle;
        }

        void endAccess(const MemReq& req) {
            //Relock child before we unlock ourselves (hand-over-hand)
            if (req.childLock) {
                futex_lock(req.childLock);
            }

            bcc->unlock();
            tcc->unlock();
        }

        //Inv methods
        void startInv() {
            bcc->lock(); //note we don't grab tcc; tcc serializes multiple up accesses, down accesses don't see it
        }
        void endInv(){
			bcc->unlock();
		}

        MemObject* getParent( Address lineAddr ){return bcc->getParent(lineAddr);}
        MemObject* getParent(uint32_t srcId, Address lineAddr ){return bcc->getParent(srcId,lineAddr);}

        //Repl policy interface
        uint32_t numSharers(uint32_t lineId) {return tcc->numSharers(lineId);}
        bool isValid(uint32_t lineId) {return false;}
};

class CoherenceDirectory: public Cache {
private:
    bool idealCoherence;
    Counter profGETS, profGETX, profPUTS, profPUTX /*received from downstream*/;
public:
    CoherenceDirectory(uint32_t _numLines, CC* _cc, CacheArray* _array, ReplPolicy* _rp, bool _idealCoherence, uint32_t _accLat, uint32_t _invLat, const g_string& _name);

    virtual uint64_t access(MemReq &req);

    void initStats(AggregateStat* parentStat) {
        AggregateStat* cacheStat = new AggregateStat();
        cacheStat->init(name.c_str(), "Coherence directory stats");
        initCacheStats(cacheStat);

        profGETS.init("GETS", "GETS (from lower level)");
        profGETX.init("GETX", "GETX (from lower level)");
        profPUTS.init("PUTS", "Clean evictions (from lower level)");
        profPUTX.init("PUTX", "Dirty evictions (or Writes) (from lower level)");
        
        cacheStat->append(&profGETX);
        cacheStat->append(&profGETS);
        cacheStat->append(&profPUTS);
        cacheStat->append(&profPUTX);

        parentStat->append(cacheStat);
    }

    virtual uint64_t invalidate(const InvReq &req);

private:
    void startInvalidate(); // grabs cc's downLock
    uint64_t finishInvalidate(const InvReq& req); // performs inv and releases downLock
};

#endif