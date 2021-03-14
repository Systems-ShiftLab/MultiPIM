#ifndef __ZSIM_WRAPPER_H
#define __ZSIM_WRAPPER_H

#include <string>

#include "Config.h"
#include "common/common_structures.h"

using namespace std;

namespace ramulator
{

class Request;
class MemoryBase;
class Config;
class PU;

class ZsimWrapper : public GlobAlloc
{
private:
    MemoryBase *mem;
    string stats_name;
    uint64_t cpuFreqHz;
    double cpu_ns;
    double diff_mem_ns;
public:
    double tCK;
    ZsimWrapper(int groupIdx, Config& configs, string& outDir, string& statsPrefix, uint64_t cpuFreqHz, int cacheline);
    ~ZsimWrapper();
    void tick();
    uint32_t tick(uint64_t cycle);
    uint64_t getTotalMemReqs();
    uint64_t getMemorySize();
    void printall();
    bool send(Request& req);
    void finish(void);
    int getHMCStacks();
    int getTargetVault(long addr);
    int getTargetStack(long addr);
    int memAccsssPosition(int coreid, long req_addr, bool ideal_memnet);
    int estimateMemHops(int coreid, long req_addr, bool is_pim, bool ideal_memnet);
};

} /*namespace ramulator*/

#endif /*__ZSIM_WRAPPER_H*/
