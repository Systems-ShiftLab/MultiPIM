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

#ifndef CORE_H_
#define CORE_H_

#include <stdint.h>
#include "decoder.h"
#include "g_std/g_string.h"
#include "stats.h"
#include "memory_hierarchy.h"
#include "core_recorder.h"

/* Analysis function pointer struct
 * As an artifact of having a shared code cache, we need these to be the same for different core types.
 */
struct InstrFuncPtrs {  // NOLINT(whitespace)
    void (*loadPtr)(THREADID, ADDRINT, UINT32, BOOL);
    void (*storePtr)(THREADID, ADDRINT, UINT32, BOOL);
    void (*bblPtr)(THREADID, ADDRINT, BblInfo*, BOOL);
    void (*branchPtr)(THREADID, ADDRINT, BOOL, ADDRINT, ADDRINT, BOOL);
    // Same as load/store functions, but last arg indicated whether op is executing
    void (*predLoadPtr)(THREADID, ADDRINT, BOOL, UINT32, BOOL);
    void (*predStorePtr)(THREADID, ADDRINT, BOOL, UINT32, BOOL);
    void (*OffloadBegin)(THREADID);
    void (*OffloadEnd)(THREADID);
    uint64_t type;
    uint64_t pad[1];
    //NOTE: By having the struct be a power of 2 bytes, indirect calls are simpler (w/ gcc 4.4 -O3, 6->5 instructions, and those instructions are simpler)
};

struct PhaseCycleBreakdown {
    uint64_t totalUnstallCycles;
    uint64_t totalIfetchCycles;
    uint64_t totalAddressTransCycles;
    uint64_t totalLocalVaultCycles;
    uint64_t totalLocalStackCycles;
    uint64_t totalRemoteStackCycles;
    uint64_t totalCacheHitCycles;
    void reset(){
        totalUnstallCycles = 0;
        totalIfetchCycles = 0;
        totalAddressTransCycles = 0;
        totalLocalVaultCycles = 0;
        totalLocalStackCycles = 0;
        totalRemoteStackCycles = 0;
        totalCacheHitCycles = 0;
    }
};

//TODO: Switch type to an enum by using sizeof macros...
#define FPTR_ANALYSIS (0L)
#define FPTR_JOIN (1L)
#define FPTR_NOP (2L)
#define FPTR_RETRY (3L)


//Generic core class

class Core : public GlobAlloc {
    private:
        uint64_t lastUpdateCycles;
        uint64_t lastUpdateInstrs;
    protected:
        g_string name;
        uint32_t coreIdx;
        uint32_t threadIdx;
        uint32_t procIdx;
    public:
        PhaseCycleBreakdown phaseCycleBreakdown;

    public:
        explicit Core(g_string& _name) : lastUpdateCycles(0), lastUpdateInstrs(0),  name(_name)  {
            //Get the coreIdx from the name
            g_string delim = "-";
            auto start = 0U;
            auto end = name.find(delim);
            while (end != std::string::npos)
            {
                start = end + delim.length();
                end = name.find(delim, start);
            }

            // LOIS: We update this values in each basic block
            coreIdx = atoi((name.substr(start, end)).c_str());
            threadIdx = coreIdx;
            phaseCycleBreakdown.reset();
        }
	    virtual void offloadFunction_begin()  = 0;
	    virtual void offloadFunction_end()  = 0;
        virtual int get_offload_code()  = 0;

        virtual void SetProcIdx( uint32_t proc ){procIdx = proc;}
		virtual uint32_t GetProcIdx(){return procIdx;}
        virtual void SetThreadIdx( uint32_t tid ){threadIdx = tid;}
		virtual uint32_t GetThreadIdx(){return threadIdx;}
        virtual void SetThreadBBLStatus(BBLStatus* bblStatus){}

        virtual uint64_t getInstrs() const = 0; // typically used to find out termination conditions or dumps
        virtual uint64_t getOffloadInstrs() const = 0; // LOIS: typically used to find out termination conditions or dumps
        virtual uint64_t getMemInstrs() const = 0;
        virtual uint64_t getPhaseCycles() const = 0; // used by RDTSC faking --- we need to know how far along we are in the phase, but not the total number of phases
        virtual uint64_t getCycles() const = 0;
        virtual uint64_t getBoundCycles() const {}
        virtual uint64_t getEffectiveCycles() const {}

        virtual void incAddressTransCycles(uint64_t cycles) { phaseCycleBreakdown.totalAddressTransCycles += cycles; }
        virtual void incPhaseUnstallCycles(uint64_t cycles) { phaseCycleBreakdown.totalUnstallCycles += cycles; }
        virtual void incLocalVaultCycles(uint64_t cycles) { phaseCycleBreakdown.totalLocalVaultCycles += cycles; }
        virtual void incLocalStackCycles(uint64_t cycles) { phaseCycleBreakdown.totalLocalStackCycles += cycles; }
        virtual void incRemoteStackCycles(uint64_t cycles) { phaseCycleBreakdown.totalRemoteStackCycles += cycles; }
        virtual void incCacheHitCycles(uint64_t cycles) { phaseCycleBreakdown.totalCacheHitCycles += cycles; }
        virtual void incIfetchCycles(uint64_t cycles){ phaseCycleBreakdown.totalIfetchCycles += cycles;}
        virtual PhaseCycleBreakdown& getPhaseCycleBreakdown(){  return phaseCycleBreakdown; }
        virtual void incMemAccLat(uint64_t cycles, uint32_t type){}

        virtual void initStats(AggregateStat* parentStat) = 0;
        virtual void contextSwitch(int32_t gid) = 0; //gid == -1 means descheduled, otherwise this is the new gid

        //Called by scheduler on every leave and join action, before barrier methods are called
        virtual void leave() {}
        virtual void join() {}

        virtual InstrFuncPtrs GetFuncPtrs() = 0;
        virtual void SetPaging(BasePaging* paging){}
        virtual void flushTlb(){}
        virtual void updateTlb(Address ppn, Address new_ppn){}
		virtual BaseTlb* getInsTlb(){ return NULL;}
		virtual BaseTlb* getDataTlb(){ return NULL;}
		virtual BaseCache* getInsCache() { return NULL;}
		virtual BaseCache* getDataCache() { return NULL;}
		virtual uint64_t clflush( Address startAddr, uint64_t startCycle){ return startCycle; }
        virtual uint64_t clflush_cacheline( Address startAddr, uint64_t startCycle){return startCycle;}
        virtual uint64_t clflush_page( Address ppn, uint64_t startCycle){ return startCycle; }
        virtual BaseCoreRecorder* getCoreRecorder() {return NULL;}
        virtual void cPhaseAdvance() {}
};

#endif  // CORE_H_

