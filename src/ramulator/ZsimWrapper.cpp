#include "ZsimWrapper.h"
#include "Config.h"
#include "Request.h"
#include "MemoryFactory.h"
#include "Memory.h"
#include "HMC_Memory.h"
#include "DDR3.h"
#include "DDR4.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "GDDR5.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "HMC.h"
#include "SALP.h"
#include <functional>
#include "Request.h"

#include <iostream>
#include "Statistics.h"

#include "log.h"

using namespace std;
using namespace ramulator;

ZsimWrapper::ZsimWrapper(int groupIdx, Config& configs, string& outDir, string& statsPrefix, uint64_t cpuFreqHz, int cacheline)
{
    const string& std_name = configs["standard"];
    if(std_name == "DDR3")
        mem = MemoryFactory<DDR3>::create(configs, cacheline);
    else if(std_name == "DDR4")
        mem = MemoryFactory<DDR4>::create(configs, cacheline);
    else if(std_name == "LPDDR3")
        mem = MemoryFactory<LPDDR3>::create(configs, cacheline);
    else if(std_name == "LPDDR4")
        mem = MemoryFactory<LPDDR4>::create(configs, cacheline);
    else if(std_name == "GDDR5")
        mem = MemoryFactory<GDDR5>::create(configs, cacheline);
    else if(std_name == "WideIO")
        mem = MemoryFactory<WideIO>::create(configs, cacheline);
    else if(std_name == "WideIO2")
        mem = MemoryFactory<WideIO2>::create(configs, cacheline);
    else if(std_name == "HBM")
        mem = MemoryFactory<HBM>::create(configs, cacheline);
    else if(std_name == "SALP-1")
        mem = MemoryFactory<SALP>::create(configs, cacheline);
    else if(std_name == "SALP-2")
        mem = MemoryFactory<SALP>::create(configs, cacheline);
    else if(std_name == "SALP-MASA")
        mem = MemoryFactory<SALP>::create(configs, cacheline);
    else if(std_name == "HMC"){
        mem = MemoryFactory<HMC>::create(configs, cacheline);
        mem->set_group_idx(groupIdx);
        info("HMC memory initialized");
        if(configs.pim_mode_enabled()){
            info("PIM mode enabled");
            if(configs.data_mem_network_no_latency()){
                info("Ideal data memory network mode is enabled");
            }
            if(configs.ptw_mem_network_no_latency()){
                info("Ideal PTW memory network mode is enabled");
            }
        }else{
            info("PIM mode disabled");
        }
    }else
        assert(0 && "unrecognized standard name");
    
    cpu_ns = (1000000000.0/cpuFreqHz);
    tCK = mem->clk_ns();
    diff_mem_ns = 0;
    info("cpu_ns %f, mem_ns %f",cpu_ns,tCK);
    assert(tCK >= cpu_ns);

    stats_name = outDir + "/";
    stats_name = stats_name + statsPrefix;
    stats_name = stats_name + "ramulator.stats";
    Stats::statlist.output(stats_name);
}

ZsimWrapper::~ZsimWrapper() {
    delete mem;
}

uint32_t ZsimWrapper::tick(uint64_t cycle)
{
    diff_mem_ns += cpu_ns;

    Stats::curTick++; // processor clock, global, for Statistics

    if(diff_mem_ns >= tCK) {
        mem->tick();
        diff_mem_ns -= tCK;
    }
    return 1;
}

void ZsimWrapper::tick()
{
    mem->tick();
}

int ZsimWrapper::memAccsssPosition(int coreid, long req_addr, bool ideal_memnet)
{
    return mem->memAccsssPosition(coreid, req_addr, ideal_memnet);
}

int ZsimWrapper::estimateMemHops(int coreid, long req_addr, bool is_pim, bool ideal_memnet)
{
    return mem->estimateMemHops(coreid,req_addr, is_pim, ideal_memnet);
}

void ZsimWrapper::printall()
{
    Stats::statlist.output(stats_name);
    Stats::statlist.printall();
}

bool ZsimWrapper::send(Request& req)
{
    return mem->send(req);
}

void ZsimWrapper::finish(void) {
    mem->finish();
}

int ZsimWrapper::getHMCStacks(){
    return mem->get_hmc_stacks();
}

int ZsimWrapper::getTargetVault(long addr){
    return mem->get_target_vault(addr);
}
int ZsimWrapper::getTargetStack(long addr){
    return mem->get_target_stack(addr);
}

uint64_t ZsimWrapper::getTotalMemReqs() {
    return mem->get_total_mem_req();
}

uint64_t ZsimWrapper::getMemorySize() {
    return mem->get_mem_size();
}