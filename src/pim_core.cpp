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

#include "pim_core.h"
#include "filter_cache.h"
#include "zsim.h"

#include "ramulator_mem_ctrl.h"
#define DEBUG_MSG(args...)
//#define DEBUG_MSG(args...) info(args)
#define FETCH_WIDTH 16

PIMCore::PIMCore(FilterCache* _l1i, FilterCache* _l1d, uint32_t _domain, g_string& _name)
    : Core(_name), l1i(_l1i), l1d(_l1d), instrs(0), curCycle(0), cRec(_domain, _name) 
{
    readInstrs = 0;
    writeInstrs = 0;
    memInstrs = 0;
    ifetches = 0;

    privateAccCycle = 0;
    sharedROAccCycle = 0;
    sharedRWAccCycle = 0;
}

uint64_t PIMCore::getPhaseCycles() const {
    return curCycle % zinfo->phaseLength;
}

void PIMCore::initStats(AggregateStat* parentStat) {
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

    ProxyStat* privateAccStat = new ProxyStat();
    privateAccStat->init("privateAccCycle", "Private data access latency", &privateAccCycle);
    coreStat->append(privateAccStat);

    ProxyStat* sharedROAccStat = new ProxyStat();
    sharedROAccStat->init("sharedROAccCycle", "Shared read-only data access latency", &sharedROAccCycle);
    coreStat->append(sharedROAccStat);

    ProxyStat* sharedRWAccStat = new ProxyStat();
    sharedRWAccStat->init("sharedRWAccCycle", "Shared read-write data access latency", &sharedRWAccCycle);
    coreStat->append(sharedRWAccStat);

    parentStat->append(coreStat);
}

void PIMCore::flushTlb(){
    l1i->flushTlb();
    l1d->flushTlb();
}

void PIMCore::updateTlb(Address ppn, Address new_ppn){
    if(l1i)
        l1i->updateTlb(ppn,new_ppn);
    if(l1d)
        l1d->updateTlb(ppn,new_ppn);
}

BaseTlb* PIMCore::getInsTlb(){return l1i->getTlb();}
BaseTlb* PIMCore::getDataTlb(){return l1d->getTlb();}

uint64_t PIMCore::clflush_page( Address ppn, uint64_t startCycle)
{
    Address start_cacheline = ppn<<(zinfo->page_shift-lineBits);
    uint64_t respCycle;
    uint64_t max_cycle = startCycle;
    for (int index = 0; index < 64; index++)
    {
        Address cacheline = start_cacheline + index;

        if(l1i){
            respCycle =  l1i->clflush( cacheline, startCycle, true);
            max_cycle = (max_cycle > respCycle)? max_cycle:respCycle;
        }
        if(l1d){
            respCycle =  l1d->clflush( cacheline, startCycle, true);
            max_cycle = (max_cycle > respCycle)? max_cycle:respCycle;
        }
    }
    return max_cycle;
}

void PIMCore::incMemAccLat(uint64_t cycles, uint32_t type)
{
    switch(type){
        case 1:
            privateAccCycle += cycles;
            break;
        case 2:
            sharedROAccCycle += cycles;
            break;
        case 3:
            sharedRWAccCycle += cycles;
            break;
        default:
            break;
    }
}

void PIMCore::contextSwitch(int32_t gid) {
    if (gid == -1) {
        l1i->contextSwitch();
        l1d->contextSwitch();
    }
}


void PIMCore::offloadFunction_begin()
{
    // printf("(pro %d, thread %d) offloadFunction_begin\n", procIdx, threadIdx);
    threadBBLStatus->offload_code++;
    if(threadBBLStatus->offload_code == 1){
        threadBBLStatus->need_offload = true;
        threadBBLStatus->can_offload = true;
    }
}

void PIMCore::offloadFunction_end()
{
    // printf("(pro %d, thread %d) offloadFunction_end\n", procIdx, threadIdx);
    threadBBLStatus->offload_code--;
    assert(threadBBLStatus->offload_code == 0);
}

int PIMCore::get_offload_code()
{
    // assert(threadBBLStatus);
    return threadBBLStatus->offload_code;
}


inline bool PIMCore::inOffloadBLK(){
    bool cur_pim_bbl = false;

    if(threadBBLStatus->offload_code){
        cur_pim_bbl = true;
    }else if(threadBBLStatus->need_offload){
        cur_pim_bbl = true;
    }
    return cur_pim_bbl;
}

void PIMCore::join() {
    DEBUG_MSG("[%s] Joining, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
    curCycle = cRec.notifyJoin(curCycle);
    phaseEndCycle = zinfo->globPhaseCycles + zinfo->phaseLength;
    DEBUG_MSG("[%s] Joined, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
}

void PIMCore::leave() {
    cRec.notifyLeave(curCycle);
}

uint64_t PIMCore::getBoundCycles() const {return MAX(curCycle, zinfo->globPhaseCycles);}

void PIMCore::loadAndRecord(Address addr) {

    if(zinfo->maxOffloadInstrsIssued) return;

    memInstrs++;
    readInstrs++;
    uint64_t startCycle = curCycle;
    if (addr != ((Address)-1L)) {
        curCycle = l1d->load(addr, curCycle,procIdx, threadIdx, true);
        cRec.record(startCycle);
    }
}

void PIMCore::storeAndRecord(Address addr) {

    if(zinfo->maxOffloadInstrsIssued) return;
    
    memInstrs++;
    writeInstrs++;
    uint64_t startCycle = curCycle;
    if (addr != ((Address)-1L)) {
        curCycle = l1d->store(addr, curCycle,procIdx, threadIdx, true);
        cRec.record(startCycle);
    }
}

void PIMCore::bblAndRecord(Address bblAddr, BblInfo* bblInfo) {
    if(zinfo->maxOffloadInstrsIssued){
        //Allow wave phase can continue
        curCycle = curCycle + zinfo->phaseLength;
        return;
    }
    
    instrs += bblInfo->instrs;
    curCycle += bblInfo->instrs;

	Address tmp_addr;
    Address endBblAddr = bblAddr + bblInfo->bytes + FETCH_WIDTH -1;
    for (Address fetchAddr = bblAddr; fetchAddr < endBblAddr; fetchAddr+=FETCH_WIDTH) {
        ifetches++;
		uint64_t startCycle = curCycle;
        assert(fetchAddr != (Address)-1L);
        curCycle = l1i->load(fetchAddr, curCycle, procIdx, threadIdx, true);
        cRec.record(startCycle);
    }
}


InstrFuncPtrs PIMCore::GetFuncPtrs() {
    return {LoadAndRecordFunc, StoreAndRecordFunc, BblAndRecordFunc, BranchFunc, PredLoadAndRecordFunc, PredStoreAndRecordFunc, OffloadBegin, OffloadEnd, FPTR_ANALYSIS, {0}};
}

// LOIS
void PIMCore::OffloadBegin(THREADID tid) {
    static_cast<PIMCore*>(cores[tid])->offloadFunction_begin();
}
void PIMCore::OffloadEnd(THREADID tid) {
    static_cast<PIMCore*>(cores[tid])->offloadFunction_end();
}

void PIMCore::LoadAndRecordFunc(THREADID tid, ADDRINT addr, UINT32 size, BOOL inOffloadRegion) {
    PIMCore* core = static_cast<PIMCore*>(cores[tid]);
    bool cur_pim_bbl = inOffloadRegion;
    if(! cur_pim_bbl && core->inOffloadBLK())
        cur_pim_bbl = true;

    if(zinfo->skipNonOffloadBBL && !cur_pim_bbl)
        return;
    core->loadAndRecord(addr);
    // static_cast<PIMCore*>(cores[tid])->loadAndRecord(addr);
}

void PIMCore::StoreAndRecordFunc(THREADID tid, ADDRINT addr, UINT32 size, BOOL inOffloadRegion) {
    PIMCore* core = static_cast<PIMCore*>(cores[tid]);
    bool cur_pim_bbl = inOffloadRegion;
    if(! cur_pim_bbl && core->inOffloadBLK())
        cur_pim_bbl = true;

    if(zinfo->skipNonOffloadBBL && !cur_pim_bbl)
        return;
    core->storeAndRecord(addr);
    // static_cast<PIMCore*>(cores[tid])->storeAndRecord(addr);
}

void PIMCore::BblAndRecordFunc(THREADID tid, ADDRINT bblAddr, BblInfo* bblInfo, BOOL inOffloadRegion) {
    PIMCore* core = static_cast<PIMCore*>(cores[tid]);
    bool cur_pim_bbl = inOffloadRegion;
    if(! cur_pim_bbl && core->inOffloadBLK())
        cur_pim_bbl = true;

    if(zinfo->skipNonOffloadBBL && !cur_pim_bbl)
        return;
    core->bblAndRecord(bblAddr, bblInfo);

    while (core->curCycle > core->phaseEndCycle) {
        core->phaseEndCycle += zinfo->phaseLength;
        uint32_t cid = getCid(tid);
        uint32_t newCid = TakeBarrier(tid, cid);
        if (newCid != cid) break; /*context-switch*/
    }
}

void PIMCore::PredLoadAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred, UINT32 size, BOOL inOffloadRegion) {
    PIMCore* core = static_cast<PIMCore*>(cores[tid]);
    bool cur_pim_bbl = inOffloadRegion;
    if(! cur_pim_bbl && core->inOffloadBLK())
        cur_pim_bbl = true;

    if(zinfo->skipNonOffloadBBL && !cur_pim_bbl)
        return;
    if (pred) core->loadAndRecord(addr);
    // if (pred) static_cast<PIMCore*>(cores[tid])->loadAndRecord(addr);
}

void PIMCore::PredStoreAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred, UINT32 size, BOOL inOffloadRegion) {
    PIMCore* core = static_cast<PIMCore*>(cores[tid]);
    bool cur_pim_bbl = inOffloadRegion;
    if(! cur_pim_bbl && core->inOffloadBLK())
        cur_pim_bbl = true;

    if(zinfo->skipNonOffloadBBL && !cur_pim_bbl)
        return;
    if (pred) core->storeAndRecord(addr);
    // if (pred) static_cast<PIMCore*>(cores[tid])->storeAndRecord(addr);
}
