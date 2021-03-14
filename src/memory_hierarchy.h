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

#ifndef MEMORY_HIERARCHY_H_
#define MEMORY_HIERARCHY_H_

/* Type and interface definitions of memory hierarchy objects */

#include <stdint.h>
#include <iostream>
#include <map>
#include "g_std/g_vector.h"
#include "g_std/g_string.h"
#include "galloc.h"
#include "locks.h"
#include "debug.h"
#include "common/common_structures.h"
#include "common/global_const.h"
#include "ramulator/Request.h"

/** TYPES **/

/* Addresses are plain 64-bit uints. This should be kept compatible with PIN addrints */
typedef uint64_t Address;

/* Types of Access. An Access is a request that proceeds from lower to upper
 * levels of the hierarchy (core->l1->l2, etc.)
 */
typedef enum {
    GETS, // get line, exclusive permission not needed (triggered by a processor load)
    GETX, // get line, exclusive permission needed (triggered by a processor store o atomic access)
    PUTS, // clean writeback (lower cache is evicting this line, line was not modified)
    PUTX, // dirty writeback (lower cache is evicting this line, line was modified)
    OFFLOAD // offload task to PIM accelerator
    // EVICTION,
	// SETDIRTY,
	// WRITEBACK
} AccessType;

/* Types of Invalidation. An Invalidation is a request issued from upper to lower
 * levels of the hierarchy.
 */
typedef enum {
    INV,  // fully invalidate this line
    INVX, // invalidate exclusive access to this line (lower level can still keep a non-exclusive copy)
    FWD,  // don't invalidate, just send up the data (used by directories). Only valid on S lines.
} InvType;

/* Coherence states for the MESI protocol */
typedef enum {
    I, // invalid
    S, // shared (and clean)
    E, // exclusive and clean
    M  // exclusive and dirty
} MESIState;

typedef enum {
    WRITEBACK,
    WRITETHROUGH
}WritePolicy;

//Convenience methods for clearer debug traces
const char* AccessTypeName(AccessType t);
const char* InvTypeName(InvType t);
const char* MESIStateName(MESIState s);
const char* WritePolicyName(WritePolicy wp);
WritePolicy WritePolicyType(const char * name);

// inline bool IsGet(AccessType t) { return t == GETS || t == GETX; }
inline bool IsGet(AccessType t) { return t == GETS || t == GETX || t == OFFLOAD; } // We consider offload task as a special load instruction
inline bool IsPut(AccessType t) { return t == PUTS || t == PUTX; }
inline bool IsOffload(AccessType t) {return t == OFFLOAD;}


/* Memory request */
struct MemReq {
    Address lineAddr;
    AccessType type;
    uint32_t childId;
    MESIState* state;
    uint64_t cycle; //cycle where request arrives at component

    //Used for race detection/sync
    lock_t* childLock;
    MESIState initialState;

    //Requester id --- used for contention simulation
    uint32_t srcId;

    //Flags propagate across levels, though not to evictions
    //Some other things that can be indicated here: Demand vs prefetch accesses, TLB accesses, etc.
    enum Flag {
        IFETCH        = (1<<1), //For instruction fetches. Purely informative for now, does not imply NOEXCL (but ifetches should be marked NOEXCL)
        NOEXCL        = (1<<2), //Do not give back E on a GETS request (turns MESI protocol into MSI for this line). Used on e.g., ifetches and NUCA.
        NONINCLWB     = (1<<3), //This is a non-inclusive writeback. Do not assume that the line was in the lower level. Used on NUCA (BankDir).
        PUTX_KEEPEXCL = (1<<4), //Non-relinquishing PUTX. On a PUTX, maintain the requestor's E state instead of removing the sharer (i.e., this is a pure writeback)
        PREFETCH      = (1<<5), //Prefetch GETS access. Only set at level where prefetch is issued; handled early in MESICC
        PTW           = (1<<6), //For page table walks
    };
    uint32_t flags;
    
    uint64_t coreId;
    Address readAddr; // Read that caused the eviction
    uint64_t threadId;
    bool l1hit;//hit in l1cache?
    bool isPIMInst;
    //for PTW
    bool isFirstPTW;
    bool isLastPTW;

    bool pageShared;
    bool pageDirty;
    bool triggerPageShared;
    bool triggerPageDirty;

    bool nonCacheable;

    uint32_t accType;//0:undefined; 1:private; 2:shared ro; 3:shared rw

    inline void set(Flag f) {flags |= f;}
    inline bool is (Flag f) const {return flags & f;}
};

/* Invalidation/downgrade request */
struct InvReq {
    Address lineAddr;
    InvType type;
    // NOTE: writeback should start false, children pull it up to true
    bool* writeback;
    uint64_t cycle;
    uint32_t srcId;
    bool clflush;
};

struct InvPage {
    Address ppn;
    InvType type;
    // NOTE: writeback should start false, children pull it up to true
    bool* writeback;
    uint64_t cycle;
    uint32_t srcId;
    bool clflush;
};

/** INTERFACES **/

class AggregateStat;
class Network;

/* Base class for all memory objects (caches and memories) */
class MemObject : public GlobAlloc {
    public:
        //Returns response cycle
        virtual uint64_t access(MemReq& req) = 0;
        virtual void initStats(AggregateStat* parentStat) {}
        virtual const char* getName() {return NULL;}
        virtual void dump(){}
        virtual bool clflush(Address addr){assert(0);}
        virtual uint64_t offload(OffLoadMetaData* offload_task){assert(0);}
        virtual bool offloadable(){assert(0);}
        virtual uint64_t getFinishedOffloadInstrs() {assert(0);}
        virtual uint64_t getPendingEvents() {assert(0);}
        virtual uint64_t getCapacity() {assert(0);return 0;}
        virtual void getStats(MemStats &stat){}
        virtual int getHMCStacks(){return 0;}
};

/* Base class for all cache objects */
class BaseCache : public MemObject {
    public:
        virtual void setParents(uint32_t _childId, const g_vector<MemObject*>& parents, Network* network) = 0;
        virtual void setChildren(const g_vector<BaseCache*>& children, Network* network) = 0;
        virtual uint64_t invalidate(const InvReq& req) = 0;
		// virtual uint64_t clflush( Address pAddr, uint64_t curCycle){ return curCycle;}
		// virtual uint64_t clflush_all( Address lineAddr, InvType type, bool* reqWriteback, uint64_t reqCycle, uint32_t srcId){ return reqCycle;}
		// virtual void calculate_stats(){}
        virtual void addChild(BaseCache* child, Network* network){}
        virtual void addChildren(const g_vector<BaseCache*>& children, Network* network){}
        virtual uint32_t getChildrenNum() {return 0;}
};

class MemoryMapper: public GlobAlloc {
    public:
        virtual void getPhyAddr(int procIdx, uint64_t vAddr, uint64_t &pAddr) = 0;
};

/*--------Base class for TLB object----------*/
//forward declarition
class BasePageTableWalker;
class BaseTlb: public MemObject{
	public:
		virtual void clear_counter(){ std::cout<<"base clear counter"<<std::endl; }
		//flush all entries of tlb
		virtual bool flush_all() = 0;
		virtual void set_parent(BasePageTableWalker* base_pg_walker) = 0;
		virtual BasePageTableWalker* get_page_table_walker(){ return NULL;};
		virtual uint64_t calculate_stats(){};
        virtual uint64_t calculate_stats(std::ofstream &vmof){}
		virtual uint64_t get_access_time(){ return 0; }
        virtual void setSourceId(uint32_t id){};
        virtual void setFlags(uint32_t flags){};
        virtual uint32_t shootdown(Address vpn) {return 0;};
        virtual uint32_t update_entry(Address vpn, Address ppn) {return 0;};
        virtual uint32_t update_ppn(Address ppn, Address new_ppn) {return 0;};
        virtual uint32_t update_tlb_flags(Address ppn, bool shared, bool dirty){return 0;}
			
		virtual ~BaseTlb(){};
};
/*#-----------base class of paging--------------#*/
class Page;
class PageTable;
class BasePDTEntry;
class BaseCoreRecorder;
class BasePaging: public MemObject
{
	public:
		virtual ~BasePaging(){};
		virtual PagingStyle get_paging_style()=0;
		virtual PageTable* get_root_directory()=0;
		virtual Address access(MemReq& req )=0;
        virtual Address access(MemReq& req , g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, BaseCoreRecorder* cRec, bool sendPTW)=0;
        virtual bool unmap_page_table(Address addr)=0;
		virtual uint64_t remap_page_table( Address ppn,Address dst_ppn){return 0; };
        virtual int map_page_table(Address addr, Page* pg_ptr , BasePDTEntry*& mapped_entry){	return 0;	};
		virtual int map_page_table(Address addr, Page* pg_ptr )=0;
        virtual int map_page_table(uint32_t req_id, Address addr, Page* pg_ptr, bool is_write) = 0;
		virtual bool allocate_page_table(Address addr , Address size)=0;
		virtual void remove_root_directory()=0;
		virtual bool remove_page_table( Address addr , Address size)
		{ return true; }
        // virtual Address get_vpn( Address ppn ){return -1;}
        virtual bool is_page_shared(Address ppn){return false;}
		
        virtual void calculate_stats(std::ofstream &vmof){}
		virtual void calculate_stats(){}
        virtual void setCoreRecorder(BaseCoreRecorder* _cRec) {}
        virtual void lock() = 0;
        virtual void unlock() = 0;
};

/*--------Base class for PageTableWalker object----------*/
class BasePageTableWalker: public BaseCache
{
	public:
		//virtual bool add_child(const char* child_name , BaseTlb* tlb)=0;
		virtual BasePaging* GetPaging(){ return NULL;}
		virtual void SetPaging(uint32_t proc_id , BasePaging* copied_paging){}
		virtual void convert_to_dirty( Address block_id){}
        virtual void calculate_stats(std::ofstream &vmof){}
		virtual void calculate_stats(){}

        virtual void setParents(uint32_t _childId, const g_vector<MemObject*>& parents, Network* network) {};
        virtual void setChildren(const g_vector<BaseCache*>& children, Network* network){ assert(0);/*should never executed*/};
        virtual uint64_t invalidate(const InvReq& req){assert(0);/* should never executed */};
        virtual void setCoreRecorder(BaseCoreRecorder* _cRec) {}
        virtual uint64_t tlb_shootdown(Address vpn){return 0;}
        virtual uint64_t tlb_update(Address vpn, Address ppn){return 0;}
};

class PIMBasePageTableWalker;
class PIMBaseTlb: public GlobAlloc
{
	public:
		virtual ~PIMBaseTlb(){};
		virtual void tick() = 0;
		virtual bool receive( ramulator::Request& req ) = 0;
		virtual bool insert( Address vpn, Address ppn) = 0;
		virtual void set_parent(PIMBasePageTableWalker* pg_table_walker) = 0;
		virtual uint64_t calculate_stats() = 0;
		virtual uint32_t shootdown(Address vpn) = 0;
};

class PIMBasePageTableWalker: public GlobAlloc
{
	public:
		virtual ~PIMBasePageTableWalker(){};
		virtual void tick() = 0;
		virtual bool receive(ramulator::Request& req)  = 0;
		virtual void SetPaging( uint32_t proc_id , BasePaging* copied_paging) = 0;
		virtual void set_children(PIMBaseTlb* _itlb, PIMBaseTlb* _dtlb) = 0;

};

class BasePDTEntry;
struct Content: public GlobAlloc
{
  public:
	Content( BasePDTEntry* entry=NULL, Address page_no=0):
		  pgt_entry(entry), vpn(page_no){}
    Content( Content& forbid_copy){
        pgt_entry = forbid_copy.get_pgt_entry();
        vpn = forbid_copy.get_vpn();
    };
    ~Content(){
        printf("vpn %ld deleted\n",vpn);
    }
	
	Address get_vpn(){ return vpn; }
	BasePDTEntry* get_pgt_entry(){	return pgt_entry; }

  private:
	BasePDTEntry* pgt_entry;
	Address vpn;
};

#endif  // MEMORY_HIERARCHY_H_
