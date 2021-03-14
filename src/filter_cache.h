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

#ifndef FILTER_CACHE_H_
#define FILTER_CACHE_H_

#include "bithacks.h"
#include "cache.h"
#include "galloc.h"
#include "zsim.h"
#include "timing_event.h"
#include "mmu/memory_management.h"
#include "ramulator_mem_ctrl.h"

/* Extends Cache with an L0 direct-mapped cache, optimized to hell for hits
 *
 * L1 lookups are dominated by several kinds of overhead (grab the cache locks,
 * several virtual functions for the replacement policy, etc.). This
 * specialization of Cache solves these issues by having a filter array that
 * holds the most recently used line in each set. Accesses check the filter array,
 * and then go through the normal access path. Because there is one line per set,
 * it is fine to do this without grabbing a lock.
 */

class FilterCache : public Cache {
    private:
        struct FilterEntry {
            volatile Address rdAddr;
            volatile Address wrAddr;
            volatile uint64_t availCycle;

            void clear() {wrAddr = 0; rdAddr = 0; availCycle = 0;}
        };

        //Replicates the most accessed line of each set in the cache
        FilterEntry* filterArray;
        Address setMask;
        uint32_t numSets;
        uint32_t srcId; //should match the core
        uint32_t reqFlags;

        lock_t filterLock;
        uint64_t fGETSHit, fGETXHit;

        BaseTlb* tlb;
		lock_t tlb_lock;
		uint64_t tlb_access_num;

        bool enableFilter;

    public:
        FilterCache(bool _enableFilter, uint32_t _numSets, uint32_t _numLines, CC* _cc, CacheArray* _array,
                ReplPolicy* _rp, uint32_t _accLat, uint32_t _invLat, g_string& _name)
            : Cache(_numLines, _cc, _array, _rp, _accLat, _invLat, _name)
        {
            numSets = _numSets;
            setMask = numSets - 1;
            filterArray = gm_memalign<FilterEntry>(CACHE_LINE_BYTES, numSets);
            for (uint32_t i = 0; i < numSets; i++) filterArray[i].clear();
            futex_init(&filterLock);
            fGETSHit = fGETXHit = 0;
            srcId = -1;
            reqFlags = 0;

            enableFilter = _enableFilter;
            tlb = NULL;
            tlb_access_num = 0;
            futex_init(&tlb_lock);
        }

        void setSourceId(uint32_t id) {
            srcId = id;
        }

        void setFlags(uint32_t flags) {
            reqFlags = flags;
        }

        void  setTlb(BaseTlb* _tlb) { tlb = _tlb; }

        BaseTlb*  getTlb() { return tlb; }

        void flushTlb(){
            futex_lock(&tlb_lock);
            if(tlb != NULL)
                tlb->flush_all();
            futex_unlock(&tlb_lock);
        }
        void updateTlb(Address ppn, Address new_ppn){
            futex_lock(&tlb_lock);
            if(tlb != NULL)
                tlb->update_ppn(ppn, new_ppn);
            futex_unlock(&tlb_lock);
        }

        uint64_t TlbTranslate( ADDRINT vLineAddr, ADDRINT& pLineAddr,bool isLoad, uint64_t startCycle, bool is_pim_inst, bool& nonCacheable)
        {
            futex_lock(&tlb_lock);
            tlb_access_num++;
            MemReq req;
            req.lineAddr = vLineAddr;
            req.cycle = startCycle;
            req.srcId = srcId;
            req.isPIMInst = is_pim_inst;
            if(isLoad)
                req.type = GETS;
            else
                req.type = PUTS;
            pLineAddr = tlb->access(req);
            futex_unlock(&tlb_lock);
            return req.cycle;
        }

        void initStats(AggregateStat* parentStat) {
            AggregateStat* cacheStat = new AggregateStat();
            cacheStat->init(name.c_str(), "Filter cache stats");

            ProxyStat* fgetsStat = new ProxyStat();
            fgetsStat->init("fhGETS", "Filtered GETS hits", &fGETSHit);
            ProxyStat* fgetxStat = new ProxyStat();
            fgetxStat->init("fhGETX", "Filtered GETX hits", &fGETXHit);
            cacheStat->append(fgetsStat);
            cacheStat->append(fgetxStat);

            initCacheStats(cacheStat);
            parentStat->append(cacheStat);
        }

        inline uint64_t load(Address vAddr, uint64_t curCycle, uint64_t procIdx, uint64_t threadId, bool isPIMInst = false) {
            Address vLineAddr = vAddr >> lineBits;
            uint32_t idx = vLineAddr & setMask;
            if(!zinfo->nonCacheable && enableFilter){
                uint64_t availCycle = filterArray[idx].availCycle; //read before, careful with ordering to avoid timing races
                if (vLineAddr == filterArray[idx].rdAddr) {
                    fGETSHit++;
                    return MAX(curCycle, availCycle);
                }
            }
            return replace(vLineAddr, idx, true, curCycle, procIdx, threadId, isPIMInst);
        }

        inline uint64_t store(Address vAddr, uint64_t curCycle, uint64_t procIdx, uint64_t threadId, bool isPIMInst = false) {
            // printf("(%d) store %x\n",threadId,vAddr);
            Address vLineAddr = vAddr >> lineBits;
            uint32_t idx = vLineAddr & setMask;
            if(!zinfo->nonCacheable && enableFilter){
                uint64_t availCycle = filterArray[idx].availCycle; //read before, careful with ordering to avoid timing races
                if (vLineAddr == filterArray[idx].wrAddr) {
                    fGETXHit++;
                    //NOTE: Stores don't modify availCycle; we'll catch matches in the core
                    //filterArray[idx].availCycle = curCycle; //do optimistic store-load forwarding
                    return MAX(curCycle, availCycle);
                }
            }
            return replace(vLineAddr, idx, false, curCycle, procIdx, threadId, isPIMInst);
        }

        uint64_t replace(Address vLineAddr, uint32_t idx, bool isLoad, uint64_t curCycle, uint64_t procIdx, uint64_t threadId, bool isPIMInst) {
            Address pLineAddr;
            // printf("Coming...\n");
            uint64_t respCycle = curCycle;
            uint64_t addrTransCycle = curCycle;
            bool nonCacheable = false;
            if(tlb != NULL){
                respCycle = TlbTranslate(vLineAddr, pLineAddr, isLoad, respCycle ,isPIMInst, nonCacheable);
                // printf("TLB v 0x%x <-> p 0x%x\n",vLineAddr, pLineAddr);
            }else if(zinfo->addressRandomization){
                zinfo->memoryMapper->getPhyAddr(procIdx, vLineAddr, pLineAddr);
            }else{
                panic("Either using TLB or addressRandomization for virtual address translation!")
                // pLineAddr = procMask | vLineAddr;
            }
            addrTransCycle = respCycle - addrTransCycle;
            uint64_t dataAccCycle = respCycle;

            TimingRecord ptwTR;
            ptwTR.clear();
            if(tlb !=NULL && zinfo->ptw_enable_timing_mode){
                if(zinfo->eventRecorders[srcId]->hasRecord()){
                    ptwTR = zinfo->eventRecorders[srcId]->popRecord();
                    if(!ptwTR.isPTW){
                        // normally not happen
                        zinfo->eventRecorders[srcId]->pushRecord(ptwTR);
                        ptwTR.clear();
                    }
                }
            }
            MESIState dummyState = MESIState::I;
            futex_lock(&filterLock);

            MemReq req = {pLineAddr, isLoad? GETS : ((zinfo->nonCacheable || nonCacheable)? PUTX : GETX), 0, &dummyState, respCycle, &filterLock, dummyState, srcId, reqFlags, srcId};
            req.threadId = procIdx << 16 | threadId;
            req.isPIMInst = isPIMInst;
            req.nonCacheable = nonCacheable;
            respCycle  = access(req);
            dataAccCycle = respCycle - dataAccCycle;

            if(ptwTR.isValid()) {
                if(zinfo->eventRecorders[srcId]->hasRecord()){
                    TimingRecord tr = zinfo->eventRecorders[srcId]->popRecord();
                    ptwTR.endEvent->addChild(tr.startEvent,zinfo->eventRecorders[srcId]);
                }
                zinfo->eventRecorders[srcId]->pushRecord(ptwTR);
            }

            if(!zinfo->nonCacheable && enableFilter){
                //Due to the way we do the locking, at this point the old address might be invalidated, but we have the new address guaranteed until we release the lock

                //Careful with this order
                Address oldAddr = filterArray[idx].rdAddr;
                filterArray[idx].wrAddr = isLoad? -1L : vLineAddr;
                filterArray[idx].rdAddr = vLineAddr;

                //For LSU simulation purposes, loads bypass stores even to the same line if there is no conflict,
                //(e.g., st to x, ld from x+8) and we implement store-load forwarding at the core.
                //So if this is a load, it always sets availCycle; if it is a store hit, it doesn't
                if (oldAddr != vLineAddr) filterArray[idx].availCycle = respCycle;
            }

            futex_unlock(&filterLock);
            // printf("Finished\n");
            // printf("respCycle@replace %d (%s)\n",respCycle,isLoad?"Load":"Store");
            return respCycle;
        }

        inline uint64_t load(Address vAddr, uint64_t curCycle) {
            Address vLineAddr = vAddr >> lineBits;
            uint32_t idx = vLineAddr & setMask;
            uint64_t availCycle = filterArray[idx].availCycle; //read before, careful with ordering to avoid timing races
            if (vLineAddr == filterArray[idx].rdAddr) {
                fGETSHit++;
                return MAX(curCycle, availCycle);
            } else {
                return replace(vLineAddr, idx, true, curCycle);
            }
        }

        inline uint64_t store(Address vAddr, uint64_t curCycle) {
            Address vLineAddr = vAddr >> lineBits;
            uint32_t idx = vLineAddr & setMask;
            uint64_t availCycle = filterArray[idx].availCycle; //read before, careful with ordering to avoid timing races
            if (vLineAddr == filterArray[idx].wrAddr) {
                fGETXHit++;
                //NOTE: Stores don't modify availCycle; we'll catch matches in the core
                //filterArray[idx].availCycle = curCycle; //do optimistic store-load forwarding
                return MAX(curCycle, availCycle);
            } else {
                return replace(vLineAddr, idx, false, curCycle);
            }
        }

        uint64_t replace(Address vLineAddr, uint32_t idx, bool isLoad, uint64_t curCycle) {
            Address pLineAddr = procMask | vLineAddr;
            MESIState dummyState = MESIState::I;
            futex_lock(&filterLock);
            MemReq req = {pLineAddr, isLoad? GETS : GETX, 0, &dummyState, curCycle, &filterLock, dummyState, srcId, reqFlags, srcId};
            uint64_t respCycle  = access(req);

            //Due to the way we do the locking, at this point the old address might be invalidated, but we have the new address guaranteed until we release the lock

            //Careful with this order
            Address oldAddr = filterArray[idx].rdAddr;
            filterArray[idx].wrAddr = isLoad? -1L : vLineAddr;
            filterArray[idx].rdAddr = vLineAddr;

            //For LSU simulation purposes, loads bypass stores even to the same line if there is no conflict,
            //(e.g., st to x, ld from x+8) and we implement store-load forwarding at the core.
            //So if this is a load, it always sets availCycle; if it is a store hit, it doesn't
            if (oldAddr != vLineAddr) filterArray[idx].availCycle = respCycle;

            futex_unlock(&filterLock);
            // printf("respCycle@replace %d (%s)\n",respCycle,isLoad?"Load":"Store");
            return respCycle;
        }

        uint64_t invalidate(const InvReq& req) {
            Cache::startInvalidate();  // grabs cache's downLock
            futex_lock(&filterLock);
            if(!zinfo->nonCacheable && enableFilter){
                uint32_t idx = req.lineAddr & setMask; //works because of how virtual<->physical is done...
                // if ((filterArray[idx].rdAddr | procMask) == req.lineAddr) { //FIXME: If another process calls invalidate(), procMask will not match even though we may be doing a capacity-induced invalidation!
                if ((filterArray[idx].rdAddr) == req.lineAddr) {
                    filterArray[idx].wrAddr = -1L;
                    filterArray[idx].rdAddr = -1L;
                }
            }
            uint64_t respCycle = Cache::finishInvalidate(req); // releases cache's downLock
            futex_unlock(&filterLock);
            return respCycle;
        }

        void contextSwitch() {
            futex_lock(&filterLock);
            if(!zinfo->nonCacheable && enableFilter){
                for (uint32_t i = 0; i < numSets; i++) filterArray[i].clear();
            }
            futex_unlock(&filterLock);
        }
        inline uint64_t clflush( Address lineAddr, uint64_t curCycle, bool flush_all)
		{
			uint64_t respCycle = curCycle;
			bool req_write_back = true;				
            InvReq req = {lineAddr, INV,  &req_write_back, curCycle, srcId, true};
			respCycle = clflush_all(req);
			return respCycle;
		}
        inline uint64_t clflush_page(Address ppn, uint64_t startCycle)
        {
            Address start_cacheline = ppn<<(zinfo->page_shift-lineBits);
            uint64_t respCycle;
            uint64_t max_cycle = startCycle;
            for (int index = 0; index < (zinfo->page_size /zinfo->lineSize); index++)
            {
                Address cacheline = start_cacheline + index;
                int32_t lineId = array->lookup(cacheline, NULL, false);
                if(lineId != -1){
                    // respCycle = clflush( cacheline, startCycle, true);
                    bool req_write_back = true;				
                    InvReq req = {cacheline, INV,  &req_write_back, startCycle, srcId, true};
                    respCycle = clflush_all(req);
                    max_cycle = (max_cycle > respCycle)? max_cycle:respCycle;
                }
            }
            return max_cycle;
        }
	
};

#endif  // FILTER_CACHE_H_
