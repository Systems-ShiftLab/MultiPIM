#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include "stats.h"
#include "galloc.h"
#include "pad.h"

class HeartBeat : public GlobAlloc{
    private:
        g_string name;
        Counter totalCycle;
        Counter totalGlobalCycle;
        uint32_t domain;
        uint32_t freqKHz;
        uint32_t globalFreqKHz;
        bool pim_mode_enabled;
    
    public:
        HeartBeat(const g_string& _name, bool pim_enabled, uint32_t _domain);
        uint32_t tick(uint64_t cycle);
        void initStats(AggregateStat* parentStat);
    private:
        inline uint64_t toGlobalCycle(uint64_t cycle) { return (cycle)*globalFreqKHz/freqKHz; }
};

#endif