#ifndef RAMULATOR_MEM_CTRLS_H_
#define RAMULATOR_MEM_CTRLS_H_

#include <map>
#include <deque>
#include <string>
#include <functional>
#include "g_std/g_string.h"
#include "common/common_structures.h"
#include "memory_hierarchy.h"
#include "pad.h"
#include "stats.h"
#include "locks.h"

#include "tlb/tlb_entry.h"

namespace ramulator {
    class Request;
}

class RamulatorAccEvent;
class RamulatorOffLoadEvent;

class RamulatorMemory : public MemObject {
    private:
        g_string name;
        uint32_t minLatency;
        uint32_t minNoCLatency;
        uint32_t domain;
        uint32_t id;

        uint64_t memCapacity;

        // std::multimap<uint64_t, RamulatorAccEvent*> inflightRequests;//<reqid, ev*>
        g_map<uint64_t, RamulatorAccEvent*> inflightRequests;//<reqid, ev*>
        g_map<uint64_t, RamulatorAccEvent*> inflightPTWs;//<reqid, ev*>

        lock_t access_lock;
        lock_t enqueue_lock;
        lock_t cb_lock;
        
        uint64_t curCycle;
        // R/W stats
        PAD();
        Counter totalIdealMemNetReqs;
        Counter profReads;
        Counter profWrites;
        Counter profTotalRdLat;
        Counter profTotalWrLat;
        Counter minRdLat;
        Counter minWrLat;
        Counter incomingRdWr;
        Counter issuedRdWr;
        Counter profPTWs;
        Counter profTotalPTWLat;
        Counter minPTWLat;
        Counter incomingPTWs;
        Counter issuedPTWs;
        Counter profLVPTWs;
        Counter profTotalLVPTWLat;
        Counter profLMPTWs;
        Counter profTotalLMPTWLat;
        Counter profRMPTWs;
        Counter profTotalRMPTWLat;
        Counter profTLBMisses;
        Counter profTotalTLBMissesLat;
        Counter minTLBMissLat;
        Counter incomingTLBMisses;
        Counter transData;
        uint64_t completedRdWr;
        uint64_t completedPTWs;
        uint64_t clFlushMem;
        HistogramCounter memAccessLatencyHist;
        PAD();

    public:
        RamulatorMemory(uint64_t _cpuFreqHz, uint32_t _minLatency, uint32_t _minNoCLatency, uint32_t _domain, const g_string& _name);

        const char* getName() {return name.c_str();}
        void initStats(AggregateStat* parentStat);
        inline uint64_t access(MemReq& req);
        uint32_t tick(uint64_t cycle);
        void enqueue(RamulatorAccEvent* ev, uint64_t cycle);
        bool clflush(Address addr);
        void dump(){};
        inline uint64_t offload(OffLoadMetaData* offloaded_task);
        inline uint64_t getPendingEvents();
        void getStats(MemStats &stat);
        inline TlbEntry* LookupTlb(uint32_t coreId, Address ppn);
        inline Address pAddrTovAddr(uint32_t coreId, Address paddr);
        inline bool enableIdealMemNet(MemReq& req);

    private:
        void readComplete(ramulator::Request& req);
        std::function<void(ramulator::Request&)> cb_func;
};

#endif  // RAMULATOR_MEM_CTRLS_H_
