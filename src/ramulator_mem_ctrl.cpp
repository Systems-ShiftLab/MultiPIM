#include <time.h>
#include <map>
#include <string>
#include "event_recorder.h"
#include "tick_event.h"
#include "timing_event.h"
#include "zsim.h"
#include "ramulator_mem_ctrl.h"
#include "common/common_functions.h"
#include "mmu/memory_management.h"

#include "tlb/common_tlb.h"

#ifdef _WITH_RAMULATOR_
#include "ramulator/ZsimWrapper.h"
#include "ramulator/Request.h"
#include "scheduler.h"
#include <cassert>
#include <iostream>

using namespace ramulator;

class RamulatorAccEvent : public TimingEvent {
    private:
        RamulatorMemory* dram;
        bool write;
        bool is_pim_inst;
        bool is_ptw;
        bool is_first_ptw;
        bool is_last_ptw;
        uint64_t ptw_start_scycle;
        Address addr;
        bool ideal_memnet;
        // specify which core this request sent from, for virtual address translation
        int coreid = -1;

    public:
        uint64_t sCycle;

        RamulatorAccEvent(RamulatorMemory* _dram, bool _write, Address _addr, int32_t _domain) 
            :  TimingEvent(0, 0, _domain), dram(_dram), write(_write), addr(_addr), is_pim_inst(false),is_ptw(false) {}
        RamulatorAccEvent(RamulatorMemory* _dram, bool _write, Address _addr, int32_t _domain, int _core_id, bool _pim_inst) 
            :  TimingEvent(0, 0, _domain), dram(_dram), write(_write), addr(_addr), coreid(_core_id), is_pim_inst(_pim_inst), is_ptw(false), is_first_ptw(false), is_last_ptw(false),ptw_start_scycle(-1) {}
        RamulatorAccEvent(RamulatorMemory* _dram, bool _write, Address _addr, int32_t _domain, int _core_id,  bool _pim_inst, bool _ptw, bool _is_first_ptw = false, bool _is_last_ptw = false) 
            :  TimingEvent(0, 0, _domain), dram(_dram), write(_write), addr(_addr), coreid(_core_id), is_pim_inst(_pim_inst), is_ptw(_ptw), is_first_ptw(_is_first_ptw), is_last_ptw(_is_last_ptw),ptw_start_scycle(-1) {}

        bool isWrite() const {
            return write;
        }

        Address getAddr() const {
            return addr;
        }

        int getCoreID() const {return coreid;}
        bool isPIMInst() const {return is_pim_inst;}
        bool isPTW() const {return is_ptw;}
        bool isFirstPTW() const {return is_first_ptw;}
        bool isLastPTW() const {return is_last_ptw;}
        uint64_t getFirstPTWStartCycle() {return ptw_start_scycle;}

        void setIdealMemNet(bool enable_ideal_memnet){ ideal_memnet = enable_ideal_memnet; }
        bool isIdealMemNet() const { return  ideal_memnet;}

        void parentDonePTW(uint64_t startCycle, uint64_t firstPTWStartCycle){
            if(is_ptw)
                ptw_start_scycle = firstPTWStartCycle;
            parentDone(startCycle);
        }

        void simulate(uint64_t startCycle) {
            if(is_ptw && is_first_ptw)
                ptw_start_scycle = startCycle;
            sCycle = startCycle;
            dram->enqueue(this, startCycle);
        }
};

RamulatorMemory::RamulatorMemory(uint64_t _cpuFreqHz, uint32_t _minLatency, uint32_t _minNoCLatency, uint32_t _domain, const g_string& _name)
{
    curCycle = 0;
    minLatency = _minLatency;
    minNoCLatency = _minNoCLatency;
    domain = _domain;
    name = _name;

    g_string delim = "-";
    auto start = 0U;
    auto end = name.find(delim);
    while (end != std::string::npos)
    {
        start = end + delim.length();
        end = name.find(delim, start);
    }
    id = atoi((name.substr(start, end)).c_str());

    //callback
    cb_func = std::bind(&RamulatorMemory::readComplete, this, std::placeholders::_1);

    memCapacity = zinfo->ramulatorWrapper->getMemorySize();

    TickEvent<RamulatorMemory>* tickEv = new TickEvent<RamulatorMemory>(this, domain);
    tickEv->queue(0);  // start the sim at time 0

    futex_init(&access_lock);
    futex_init(&cb_lock);
    futex_init(&enqueue_lock);

    completedRdWr = 0;
    completedPTWs = 0;
    clFlushMem = 0;
}

void RamulatorMemory::getStats(MemStats &stat){
    stat.incomingMemReqs += incomingRdWr.get();
    stat.incomingPTWs += incomingPTWs.get();
    stat.issuedMemReqs += issuedRdWr.get();
    stat.issuedPTWs += issuedPTWs.get();
    stat.inflightMemReqs += inflightRequests.size() - inflightPTWs.size();
    stat.inflightPTWs += inflightPTWs.size();
    stat.completedMemReqs += completedRdWr;
    stat.completedPTWs += completedPTWs;
}

void RamulatorMemory::initStats(AggregateStat* parentStat) {
    AggregateStat* memStats = new AggregateStat();
    memStats->init(name.c_str(), "Memory controller stats");
    totalIdealMemNetReqs.init("totalIdealMemNetReqs", "totalIdealMemNetReqs"); memStats->append(&totalIdealMemNetReqs);
    profReads.init("rd", "Completed Read requests"); memStats->append(&profReads);
    profWrites.init("wr", "Completed Write requests"); memStats->append(&profWrites);
    profTotalRdLat.init("rdlat", "Total latency experienced by completed read requests"); memStats->append(&profTotalRdLat);
    profTotalWrLat.init("wrlat", "Total latency experienced by completed write requests"); memStats->append(&profTotalWrLat);
    minRdLat.init("minRdlat", "Min latency experienced by completed read requests"); memStats->append(&minRdLat); minRdLat.set(-1);
    minWrLat.init("MinWrlat", "Min latency experienced by completed write requests"); memStats->append(&minWrLat); minWrLat.set(-1);
    incomingRdWr.init("incomingRdWr", "Total incoming RdWr requests"); memStats->append(&incomingRdWr);
    issuedRdWr.init("issuedRdWr","Total issued RdWr requests"); memStats->append(&issuedRdWr);
    
    profPTWs.init("ptw", "Completed PTWs"); memStats->append(&profPTWs);
    profTotalPTWLat.init("ptwlat", "Total latency experienced by completed PTWs"); memStats->append(&profTotalPTWLat);
    minPTWLat.init("minPTWLat", "Min latency experienced by completed PTW requests"); memStats->append(&minPTWLat); minPTWLat.set(-1);
    incomingPTWs.init("incomingptw", "Total incoming PTWs"); memStats->append(&incomingPTWs);
    issuedPTWs.init("issuedptws","Total issued PTWs"); memStats->append(&issuedPTWs);

    profLVPTWs.init("lvptw", "Completed local vault PTWs"); memStats->append(&profLVPTWs);
    profTotalLVPTWLat.init("lvptwlat", "Total latency experienced by completed local vault PTWs"); memStats->append(&profTotalLVPTWLat);
    profLMPTWs.init("lmptw", "Completed local memory PTWs"); memStats->append(&profLMPTWs);
    profTotalLMPTWLat.init("lmptwlat", "Total latency experienced by completed local memory PTWs"); memStats->append(&profTotalLMPTWLat);
    profRMPTWs.init("rmptw", "Completed remote memory PTWs"); memStats->append(&profRMPTWs);
    profTotalRMPTWLat.init("rmptwlat", "Total latency experienced by completed remote memory PTWs"); memStats->append(&profTotalRMPTWLat);

    profTLBMisses.init("tlbMisses", "Completed TLB misses"); memStats->append(&profTLBMisses);
    profTotalTLBMissesLat.init("tlbMisseslat", "Total latency experienced by completed TLB misses"); memStats->append(&profTotalTLBMissesLat);
    minTLBMissLat.init("minTLBMissLat", "Min latency experienced by completed TLB misses"); memStats->append(&minTLBMissLat); minTLBMissLat.set(-1);
    incomingTLBMisses.init("incomingTLBMisses", "Total incoming TLB misses"); memStats->append(&incomingTLBMisses);

    transData.init("transdata", "Total data transfered between cpu memory memory"); memStats->append(&transData);
    memAccessLatencyHist.init("memAccessLatencyHist", "Memort access latency histogram", 20, 0, 2000);memStats->append(&memAccessLatencyHist);

    parentStat->append(memStats);
}

inline uint64_t RamulatorMemory::getPendingEvents(){
    uint64_t total_incomings = incomingRdWr.get();
    uint64_t total_completeds = completedRdWr;

    return total_incomings - total_completeds;
}

inline uint64_t RamulatorMemory::access(MemReq& req) {
    
    switch (req.type) {
        case PUTS:
        case PUTX:
            *req.state = I;
            break;
        case GETS:
            *req.state = req.is(MemReq::NOEXCL)? S : E;
            break;
        case GETX:
            *req.state = M;
            break;

        default: panic("!?");
    }

    bool ideal_memnet = enableIdealMemNet(req);
    Address addr = req.lineAddr << lineBits;
    int memhops = zinfo->ramulatorWrapper->estimateMemHops(req.srcId, addr, req.isPIMInst, ideal_memnet);
    uint64_t respCycle = req.cycle + minLatency + memhops*minNoCLatency;
    assert(respCycle > req.cycle);

    if ((req.type != PUTS /*discard clean writebacks*/) && zinfo->eventRecorders[req.srcId]) {
        bool isWrite = (req.type == PUTX);
        // assert(addr >= 0 && addr < memCapacity);
        RamulatorAccEvent* memEv = new (zinfo->eventRecorders[req.srcId]) RamulatorAccEvent(this, isWrite, addr, domain,req.srcId, req.isPIMInst, req.is(MemReq::Flag::PTW), req.isFirstPTW, req.isLastPTW);
        memEv->setMinStartCycle(req.cycle);
        memEv->setIdealMemNet(ideal_memnet);
        futex_lock(&access_lock);
        TimingRecord tr = {addr, req.cycle, respCycle, req.type, memEv, memEv, req.threadId, req.is(MemReq::Flag::PTW)};
        zinfo->eventRecorders[req.srcId]->pushRecord(tr);
        if(req.is(MemReq::Flag::PTW)){
            incomingPTWs.inc();
            if(req.isFirstPTW)
                incomingTLBMisses.inc();
        }else{
            incomingRdWr.inc();
        }

        futex_unlock(&access_lock);
    }
    return respCycle;
}

inline TlbEntry* RamulatorMemory::LookupTlb(uint32_t coreId, Address ppn)
{
    TlbEntry* entry = NULL;
	uint32_t proc_id = zinfo->cores[coreId]->GetProcIdx();;
	CommonTlb<TlbEntry>* itlb = dynamic_cast<CommonTlb<TlbEntry>*>(zinfo->cores[coreId]->getInsTlb());
	CommonTlb<TlbEntry>* dtlb = dynamic_cast<CommonTlb<TlbEntry>*>(zinfo->cores[coreId]->getDataTlb());
	if( itlb )
	   entry = itlb->look_up_pa(ppn);
	if(!entry && dtlb)
	{
		debug_printf("look up data tlb");
		entry = dtlb->look_up_pa(ppn);
	}
    return entry;
}

inline Address RamulatorMemory::pAddrTovAddr(uint32_t coreId, Address paddr)
{
    uint64_t ppn = paddr >> (zinfo->page_shift - lineBits);
    uint64_t offset = (paddr << lineBits) & (zinfo->page_size-1);
    TlbEntry* entry = NULL;
	uint32_t proc_id = zinfo->cores[coreId]->GetProcIdx();;
	CommonTlb<TlbEntry>* itlb = dynamic_cast<CommonTlb<TlbEntry>*>(zinfo->cores[coreId]->getInsTlb());
	CommonTlb<TlbEntry>* dtlb = dynamic_cast<CommonTlb<TlbEntry>*>(zinfo->cores[coreId]->getDataTlb());
	if( itlb )
	   entry = itlb->look_up_pa(ppn);
	if(!entry && dtlb)
	{
		debug_printf("look up data tlb");
		entry = dtlb->look_up_pa(ppn);
	}
    if(entry){
        return ((entry->v_page_no << zinfo->page_shift)|offset) >> lineBits;
    }
    return -1;
}

inline bool RamulatorMemory::enableIdealMemNet(MemReq& req)
{
    if ((zinfo->dataMemNetworkNoLatency && !req.is(MemReq::Flag::PTW)) || (zinfo->ptwMemNetworkNoLatency && req.is(MemReq::Flag::PTW)) )
    {
        return true;
    }
    return false;
}

uint32_t RamulatorMemory::tick(uint64_t cycle) {
    curCycle++;
    if(zinfo->heartBeats)
        zinfo->heartBeats->tick(cycle);
    if(zinfo->ramulatorWrapper)
        zinfo->ramulatorWrapper->tick(cycle);
    return 1;
}

void RamulatorMemory::readComplete(ramulator::Request& req) {
    if(req.clflush){
        return;
    }
    futex_lock(&cb_lock);
    std::map<uint64_t, RamulatorAccEvent*>::iterator it = inflightRequests.find(uint64_t(req.reqid));
    assert((it != inflightRequests.end()));
    RamulatorAccEvent* ev = it->second;
    // printf("Finished mem req at 0x%x\n", ev->getAddr());
    uint32_t lat = curCycle + 1 - ev->sCycle;

    uint32_t core_id = req.pu_id;
    if (!req.is_pim_inst)
    {
        core_id = req.coreid;
    }
    zinfo->cores[core_id]->incMemAccLat(lat, req.acc_type);

    if (ev->isWrite()){
        profWrites.inc();
        profTotalWrLat.inc(lat);
        if (minWrLat.get() > lat)
            minWrLat.set(lat);

        completedRdWr++;
    }else{
        if (ev->isPTW()){
            profPTWs.inc();
            profTotalPTWLat.inc(lat);
            if (minPTWLat.get() > lat)
                minPTWLat.set(lat);

            if (req.hops == 0){
                profLVPTWs.inc();
                profTotalLVPTWLat.inc(lat);
                profLMPTWs.inc();
                profTotalLMPTWLat.inc(lat);
            }else if (req.hops == 2){
                profLMPTWs.inc();
                profTotalLMPTWLat.inc(lat);
            }else{
                profRMPTWs.inc();
                profTotalRMPTWLat.inc(lat);
            }

            if (ev->isLastPTW()){
                uint32_t firstPTWStartCycle = ev->getFirstPTWStartCycle();
                assert(firstPTWStartCycle != -1);
                uint32_t tlbMissLat = curCycle + 1 - firstPTWStartCycle;
                profTLBMisses.inc();
                profTotalTLBMissesLat.inc(tlbMissLat);
                if (minTLBMissLat.get() > tlbMissLat)
                    minTLBMissLat.set(tlbMissLat);
            }

            completedPTWs++;
            inflightPTWs.erase(it->first);
        }else{
            profReads.inc();
            profTotalRdLat.inc(lat);
            if (minRdLat.get() > lat)
                minRdLat.set(lat);

            completedRdWr++;
        }
    }
    memAccessLatencyHist.sample(lat);

    transData.inc(64);
    inflightRequests.erase(it);
    futex_unlock(&cb_lock);
    ev->release();
    if(ev->isPTW() && !ev->isLastPTW()){
        ev->donePTW(curCycle+1, ev->getFirstPTWStartCycle());
    }else{
        // printf("Memacc done @ domain %d cycle %d\n",ev->getDomain(), curCycle+1);
        ev->done(curCycle+1);
    }
}

void RamulatorMemory::enqueue(RamulatorAccEvent* ev, uint64_t cycle) {
    Request::Type req_type;
    if(ev->isWrite())
        req_type = Request::Type::WRITE;
    else
        req_type = Request::Type::READ;

    Address ramulator_addr = ev->getAddr();
    bool pim_inst = ev->isPIMInst();
    int32_t coreid, puid;
    if(pim_inst){
        coreid = -1;
        puid = ev->getCoreID();
    }else{
        coreid = ev->getCoreID();
        puid = -1;
    }
    futex_lock(&enqueue_lock);
    Request req(ramulator_addr, req_type, cb_func, coreid, puid, pim_inst, ev->isPTW());
    req.reqid = zinfo->ramulatorWrapper->getTotalMemReqs();
    req.ideal_memnet = ev->isIdealMemNet();

    if(zinfo->ramulatorWrapper->send(req)){
        // printf("Issuing memacc to Ramulator @ domain %d cycle %d\n",ev->getDomain(), curCycle+1);
        if(ev->isPTW()){
            issuedPTWs.inc();
            inflightPTWs.insert(std::pair<uint64_t, RamulatorAccEvent*>(req.reqid, ev));
        }else{
            issuedRdWr.inc();
        }
        inflightRequests.insert(std::pair<uint64_t, RamulatorAccEvent*>(req.reqid, ev));
        ev->hold();
        if(req.ideal_memnet)
            totalIdealMemNetReqs.inc();
    }else{
        ev->requeue(cycle+1);
    }
    futex_unlock(&enqueue_lock);
}

bool RamulatorMemory::clflush(Address addr) {
    ramulator::Request req(addr, ramulator::Request::Type::WRITE, cb_func, 0);
    req.clflush = true;
    futex_lock(&enqueue_lock);
    req.reqid = zinfo->ramulatorWrapper->getTotalMemReqs();
    if(zinfo->ramulatorWrapper->send(req)){
        futex_unlock(&enqueue_lock);
        return true;
    }
    futex_unlock(&enqueue_lock);
    return false;
}

uint64_t RamulatorMemory::offload(OffLoadMetaData* offloaded_task){return offloaded_task->cycle;}
#else
#endif
