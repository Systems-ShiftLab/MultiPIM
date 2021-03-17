#include <time.h>
#include "event_recorder.h"
#include "tick_event.h"
#include "timing_event.h"
#include "memory_hierarchy.h"
#include "core.h"
#include "zsim.h"
#include "heartbeat.h"

using namespace std;

HeartBeat::HeartBeat(const g_string& _name,bool pim_enabled, uint32_t _domain){
    name = _name;
    domain = _domain;
    globalFreqKHz = 1000 * zinfo->globalFreqMHz;
    freqKHz = 1000* zinfo->freqMHz;
    pim_mode_enabled = pim_enabled;
    // TickEvent<HeartBeat> *tickEv = new TickEvent<HeartBeat>(this, domain);
    // tickEv->queue(0); // start the sim at time 0
}

uint32_t HeartBeat::tick(uint64_t cycle)
{
    totalCycle.inc();
    totalGlobalCycle.set(toGlobalCycle(totalCycle.get()));
    if (zinfo->heartbeatInterval && (totalGlobalCycle.get() % zinfo->heartbeatInterval == 0))
    {

        MemStats stats;
        for (int i = 0; i < zinfo->numMemoryControllers; i++)
        {
            zinfo->memoryControllers[i]->getStats(stats);
        }

        time_t timep;
        time(&timep);
        char tmp[64];
        strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
        
        uint64_t totalInstrs = 0;
        uint64_t totalOffloadInstrs = 0;
        uint64_t totalMemInstrs = 0;
        for (uint32_t i = 0; i < zinfo->numCores; i++)
        {
            totalInstrs += zinfo->cores[i]->getInstrs();
            totalMemInstrs += zinfo->cores[i]->getMemInstrs();
            totalOffloadInstrs += zinfo->cores[i]->getOffloadInstrs();
        }

        double IPC = (double)totalInstrs/totalGlobalCycle.get();
        printf("[%s][%lu,%lu] Mem_Reqs [%lu,<%lu,%lu>,<%lu,%lu>,<%lu,%lu>,<%lu,%lu>], Instrs [%lu,%lu], IPC [%f]\n", tmp, totalGlobalCycle.get(),totalCycle.get(), totalMemInstrs, stats.incomingMemReqs,stats.incomingPTWs, stats.issuedMemReqs,stats.issuedPTWs, stats.inflightMemReqs, stats.inflightPTWs, stats.completedMemReqs,stats.completedPTWs, totalInstrs,totalOffloadInstrs, IPC);
    }
    return 1;
}

/* uint32_t HeartBeat::tick(uint64_t cycle)
{
    totalCycle.inc();
    if (zinfo->heartbeatInterval && (totalCycle.get() % zinfo->heartbeatInterval == 0))
    {

        MemStats stats;
        for (int i = 0; i < zinfo->numMemoryControllers; i++)
        {
            zinfo->memoryControllers[i]->getStats(stats);
        }

        time_t timep;
        time(&timep);
        char tmp[64];
        strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));

        uint64_t totalOffloadInstrs = 0;
        uint64_t totalBoundCycles = 0;
        uint64_t totalInstrs = 0;
        for (uint32_t i = 0; i < zinfo->numCores; i++)
        {
            totalOffloadInstrs += zinfo->cores[i]->getOffloadInstrs();
            totalBoundCycles += zinfo->cores[i]->getBoundCycles();
            totalInstrs += zinfo->cores[i]->getInstrs();
        }

        // double IPC = (double)totalInstrs/totalCycle.get();
        double IPC1 = (double)totalInstrs/totalBoundCycles;

        if (pim_mode_enabled)
        {
            if(enable_pu_thread_map_mode){
                double IPC2 = (double)totalOffloadInstrs/totalCycle.get();
                printf("[%s][%lu] Host_Reqs [%lu,%lu,%lu], Instrs [%lu], IPC [%f,%f]\n", tmp, totalCycle.get(),  stats.incomingMemReqs, stats.inflightMemReqs, stats.completedMemReqs, totalOffloadInstrs, IPC1,IPC2);
                // printf("[%s][%lu] Host_Reqs [%lu,%lu,%lu], Offload_Instrs [%lu,%lu], Instrs [%lu], IPC [%f,%f], PU Util [%.2f%]\n", tmp, totalCycle.get(), 
                //         stats.incomingMemReqs, stats.inflightMemReqs, stats.completedMemReqs, 
                //         stats.incomingOffloadInstrs, stats.PURetiredInstr, totalOffloadInstrs, IPC1,IPC2, stats.PUUtilization*100);
            }else{
                double IPC2 = (double)stats.PURetiredInstr/totalCycle.get();
                printf("[%s][%lu] Host_Reqs [%lu,%lu,%lu], Offloads [%lu,(%lu)%lu,%lu], Offload_Instrs [%lu,%lu,%lu], Instrs [%lu], IPC [%f,%f], PU Util [%.2f%]\n", tmp, totalCycle.get(), 
                        stats.incomingMemReqs, stats.inflightMemReqs, stats.completedMemReqs, 
                        stats.incomingOffloads, stats.maxInflightOffloads, stats.inflightOffloads, stats.completedOffloads, 
                        stats.incomingOffloadInstrs, stats.inflightOffloadInstrs, stats.completedOffloadInstrs, totalOffloadInstrs, IPC1,IPC2, stats.PUUtilization*100);
            }
        }
        else
        {
            double IPC2 = (double)totalOffloadInstrs/totalCycle.get();
            printf("[%s][%lu] Host_Reqs [%lu,%lu,%lu], Instrs [%lu], IPC [%f,%f]\n", tmp, totalCycle.get(),  stats.incomingMemReqs, stats.inflightMemReqs, stats.completedMemReqs, totalOffloadInstrs, IPC1,IPC2);
        }
    }
    return 1;
} */

void HeartBeat::initStats(AggregateStat* parentStat)
{
    AggregateStat* heartbeatStats = new AggregateStat();
    heartbeatStats->init(name.c_str(), "Heartbeat stats");
    totalCycle.init("totalCycle", "Total cycles"); heartbeatStats->append(&totalCycle);
    totalGlobalCycle.init("totalGlobalCycle", "Total global cycles"); heartbeatStats->append(&totalGlobalCycle);
    parentStat->append(heartbeatStats);

}