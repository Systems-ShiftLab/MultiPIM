/** $lic$
 * Copyright (C) 2012-2015 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "timing_core.h"
#include "filter_cache.h"
#include "zsim.h"

#include "ramulator_mem_ctrl.h"
#define DEBUG_MSG(args...)
//#define DEBUG_MSG(args...) info(args)
#define FETCH_WIDTH 16

TimingCore::TimingCore(FilterCache* _l1i, FilterCache* _l1d, uint32_t _domain, g_string& _name)
    : Core(_name), l1i(_l1i), l1d(_l1d), instrs(0), curCycle(0), cRec(_domain, _name) 
{
    enable_pim_mode = false;
    readInstrs = 0;
    writeInstrs = 0;
    memInstrs = 0;
    ifetches = 0;
    offload_instrs = 0;
    offlod_mem_instrs = 0;
}

TimingCore::TimingCore(FilterCache* _l1i, FilterCache* _l1d, uint32_t _domain, g_string& _name, bool _enable_pim_mode)
    : Core(_name), l1i(_l1i), l1d(_l1d), instrs(0), curCycle(0), cRec(_domain, _name) 
{
    enable_pim_mode = _enable_pim_mode;
    readInstrs = 0;
    writeInstrs = 0;
    memInstrs = 0;
    ifetches = 0;
    offload_instrs = 0;
    offlod_mem_instrs = 0;
}

uint64_t TimingCore::getPhaseCycles() const {
    return curCycle % zinfo->phaseLength;
}

void TimingCore::initStats(AggregateStat* parentStat) {
    AggregateStat* coreStat = new AggregateStat();
    coreStat->init(name.c_str(), "Core stats");

    auto x = [this]() { return cRec.getUnhaltedCycles(curCycle); };
    LambdaStat<decltype(x)>* cyclesStat = new LambdaStat<decltype(x)>(x);
    cyclesStat->init("cycles", "Simulated unhalted cycles");
    coreStat->append(cyclesStat);

    auto y = [this]() { return cRec.getContentionCycles(); };
    LambdaStat<decltype(y)>* cCyclesStat = new LambdaStat<decltype(y)>(y);
    cCyclesStat->init("cCycles", "Cycles due to contention stalls");
    coreStat->append(cCyclesStat);

    ProxyStat* instrsStat = new ProxyStat();
    instrsStat->init("instrs", "Simulated instructions", &instrs);
    coreStat->append(instrsStat);

    ProxyStat* readInstrsStat = new ProxyStat();
    readInstrsStat->init("readInstrs", "Simulated memory read instructions", &readInstrs);
    coreStat->append(readInstrsStat);

    ProxyStat* writeInstrsStat = new ProxyStat();
    writeInstrsStat->init("writeInstrs", "Simulated memory write instructions", &writeInstrs);
    coreStat->append(writeInstrsStat);

    ProxyStat* memInstrsStat = new ProxyStat();
    memInstrsStat->init("memInstrs", "Simulated memory instructions", &memInstrs);
    coreStat->append(memInstrsStat);

    ProxyStat* ifetchesStat = new ProxyStat();
    ifetchesStat->init("ifetchs", "Simulated instruction fetches", &ifetches);
    coreStat->append(ifetchesStat);

    // ProxyStat* offloadInstrsStat = new ProxyStat();
    // offloadInstrsStat->init("offloadInstrs", "Simulated offload instructions", &offload_instrs);

    // ProxyStat* offloadMemInstrsStat = new ProxyStat();
    // offloadMemInstrsStat->init("offloadMemInstrs", "Simulated offload mem instructions", &offlod_mem_instrs);

    parentStat->append(coreStat);
}

void TimingCore::flushTlb(){
    l1i->flushTlb();
    l1d->flushTlb();
}

BaseTlb* TimingCore::getInsTlb(){return l1i->getTlb();}
BaseTlb* TimingCore::getDataTlb(){return l1d->getTlb();}

void TimingCore::contextSwitch(int32_t gid) {
    if (gid == -1) {
        l1i->contextSwitch();
        l1d->contextSwitch();
    }
}

void TimingCore::join() {
    DEBUG_MSG("[%s] Joining, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
    curCycle = cRec.notifyJoin(curCycle);
    phaseEndCycle = zinfo->globPhaseCycles + zinfo->phaseLength;
    DEBUG_MSG("[%s] Joined, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
}

void TimingCore::leave() {
    cRec.notifyLeave(curCycle);
}

uint64_t TimingCore::getBoundCycles() const {return MAX(curCycle, zinfo->globPhaseCycles);}

void TimingCore::offloadFunction_begin()
{
    // printf("(pro %d, thread %d) offloadFunction_begin\n", procIdx, threadIdx);
    threadBBLStatus->offload_code++;
    if(threadBBLStatus->offload_code == 1){
        threadBBLStatus->need_offload = true;
        if(enable_pim_mode){
            threadBBLStatus->can_offload = true;
        }
    }
}

void TimingCore::offloadFunction_end()
{
    // printf("(pro %d, thread %d) offloadFunction_end\n", procIdx, threadIdx);
    threadBBLStatus->offload_code--;
    assert(threadBBLStatus->offload_code == 0);
}

int TimingCore::get_offload_code()
{
    // assert(threadBBLStatus);
    return threadBBLStatus->offload_code;
}

void TimingCore::loadAndRecord(Address addr) {
    // if(zinfo->skipNonOffloadBBL && zinfo->pimGranularity == 1 && !get_offload_code()) return;

    // if(zinfo->maxOffloadInstrsIssued) return;

    memInstrs++;
    readInstrs++;
    uint64_t startCycle = curCycle;
    if (addr != ((Address)-1L)) {
        curCycle = l1d->load(addr, curCycle,procIdx, threadIdx, enable_pim_mode);
        cRec.record(startCycle);
    }
}

void TimingCore::storeAndRecord(Address addr) {
    // if(zinfo->skipNonOffloadBBL && zinfo->pimGranularity == 1 && !get_offload_code()) return;

    // if(zinfo->maxOffloadInstrsIssued) return;
    
    memInstrs++;
    writeInstrs++;
    uint64_t startCycle = curCycle;
    if (addr != ((Address)-1L)) {
        curCycle = l1d->store(addr, curCycle,procIdx, threadIdx, enable_pim_mode);
        cRec.record(startCycle);
    }
}

void TimingCore::bblAndRecord(Address bblAddr, BblInfo* bblInfo) {
    // if(zinfo->skipNonOffloadBBL && zinfo->pimGranularity == 1 && !pim_in_bulk) return;

    // if(zinfo->maxOffloadInstrsIssued){
    //     //Allow wave phase can continue
    //     curCycle = curCycle + zinfo->phaseLength;
    //     return;
    // }
    
    instrs += bblInfo->instrs;
    curCycle += bblInfo->instrs;

	Address tmp_addr;
    Address endBblAddr = bblAddr + bblInfo->bytes + FETCH_WIDTH -1;
    for (Address fetchAddr = bblAddr; fetchAddr < endBblAddr; fetchAddr+=FETCH_WIDTH) {
        ifetches++;
		uint64_t startCycle = curCycle;
        assert(fetchAddr != (Address)-1L);
        curCycle = l1i->load(fetchAddr, curCycle, procIdx, threadIdx, enable_pim_mode);
        cRec.record(startCycle);
    }
}


InstrFuncPtrs TimingCore::GetFuncPtrs() {
    return {LoadAndRecordFunc, StoreAndRecordFunc, BblAndRecordFunc, BranchFunc, PredLoadAndRecordFunc, PredStoreAndRecordFunc, OffloadBegin, OffloadEnd, FPTR_ANALYSIS, {0}};
}

// LOIS
void TimingCore::OffloadBegin(THREADID tid) {
    static_cast<TimingCore*>(cores[tid])->offloadFunction_begin();
}
void TimingCore::OffloadEnd(THREADID tid) {
    static_cast<TimingCore*>(cores[tid])->offloadFunction_end();
}

void TimingCore::LoadAndRecordFunc(THREADID tid, ADDRINT addr, UINT32 size, BOOL inOffloadRegion) {
    if(zinfo->enable_pim_mode && zinfo->skipNonOffloadBBL && !inOffloadRegion)
        return;
    static_cast<TimingCore*>(cores[tid])->loadAndRecord(addr);
}

void TimingCore::StoreAndRecordFunc(THREADID tid, ADDRINT addr, UINT32 size, BOOL inOffloadRegion) {
    if(zinfo->enable_pim_mode && zinfo->skipNonOffloadBBL && !inOffloadRegion)
        return;
    static_cast<TimingCore*>(cores[tid])->storeAndRecord(addr);
}

void TimingCore::BblAndRecordFunc(THREADID tid, ADDRINT bblAddr, BblInfo* bblInfo, BOOL inOffloadRegion) {
    if(zinfo->enable_pim_mode && zinfo->skipNonOffloadBBL && !inOffloadRegion)
        return;
    TimingCore* core = static_cast<TimingCore*>(cores[tid]);
    core->bblAndRecord(bblAddr, bblInfo);

    while (core->curCycle > core->phaseEndCycle) {
        core->phaseEndCycle += zinfo->phaseLength;
        uint32_t cid = getCid(tid);
        uint32_t newCid = TakeBarrier(tid, cid);
        if (newCid != cid) break; /*context-switch*/
    }
}

void TimingCore::PredLoadAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred, UINT32 size, BOOL inOffloadRegion) {
    if(zinfo->enable_pim_mode && zinfo->skipNonOffloadBBL && !inOffloadRegion)
        return;
    if (pred) static_cast<TimingCore*>(cores[tid])->loadAndRecord(addr);
}

void TimingCore::PredStoreAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred, UINT32 size, BOOL inOffloadRegion) {
    if(zinfo->enable_pim_mode && zinfo->skipNonOffloadBBL && !inOffloadRegion)
        return;
    if (pred) static_cast<TimingCore*>(cores[tid])->storeAndRecord(addr);
}
