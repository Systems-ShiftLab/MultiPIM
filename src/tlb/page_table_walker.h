/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
/*
 * Copyright (C) 2020 Chao Yu (yuchaocs@gmail.com)
 */
#ifndef PAGE_TABLE_WALKER_H
#define PAGE_TABLE_WALKER_H

#include <map>
#include <fstream>

#include "locks.h"
#include "g_std/g_string.h"
#include "memory_hierarchy.h"
#include "network.h"
#include "common/global_const.h"
#include "page-table/page_table.h"

#include "mmu/memory_management.h"
#include "tlb/common_tlb.h"
#include "tlb/tlb_entry.h"
#include "core.h"
#include "zsim.h"

template <class T>
class PageTableWalker: public BasePageTableWalker
{
	public:
		//access memory
		PageTableWalker(uint32_t line_shift, const g_string& name , PagingStyle style, bool enable_timing_mode): 
		line_shift(line_shift), pg_walker_name(name),enable_timing_mode(enable_timing_mode), procIdx((uint32_t)(-1))
		{
			mode = style;
			period = 0;
			dirty_evict = 0;
			total_evict = 0;
			allocated_page = 0;
			mmap_cached = 0;
			tlb_shootdown_overhead = 0;
			dram_map_overhead = 0;
			tlb_miss_exclude_shootdown = 0;
			tlb_miss_overhead = 0;
			clflush_overhead = 0;
			extra_write = 0;
			futex_init(&walker_lock);
		}
		~PageTableWalker(){}
		/*------tlb hierarchy related-------*/
		//bool add_child(const char* child_name ,  BaseTlb* tlb);
		/*------simulation timing and state related----*/
		uint64_t access( MemReq& req)
		{
			assert(paging);
			period++;
			Address addr = PAGE_FAULT_SIG;
			Address init_cycle = req.cycle;
			req.childId = selfId;
			req.childLock = &walker_lock;
			paging->lock();
			futex_lock(&walker_lock);
			// addr = paging->access(req);
			addr = paging->access(req, parents, parentRTTs, cRec, enable_timing_mode);

			tlb_miss_exclude_shootdown += (req.cycle - init_cycle);
			//page fault
			if( addr == PAGE_FAULT_SIG )
			{
				addr = do_page_fault(req , DRAM_PAGE_FAULT);
				//std::cout<<"allocate page:"<<addr<<std::endl;
			}
			if(req.triggerPageShared || req.triggerPageDirty){
				update_tlb_flags(addr, req.triggerPageShared, req.triggerPageDirty);
			}
			futex_unlock(&walker_lock);
			paging->unlock();
			tlb_miss_overhead += (req.cycle - init_cycle);
			return addr;	//find address
		}

		BasePaging* GetPaging() { return paging;}
		void SetPaging( uint32_t proc_id , BasePaging* copied_paging)
		{
			futex_lock(&walker_lock);
			procIdx = proc_id;
			paging = copied_paging;
			futex_unlock(&walker_lock);
		}

		const char* getName() {  return pg_walker_name.c_str(); }

		void calculate_stats()
		{
			info("%s evict time:%lu \t dirty evict time: %lu",getName(),total_evict,dirty_evict);
			info("%s allocated pages:%lu", getName(),allocated_page);
			info("%s TLB shootdown overhead:%llu", getName(), tlb_shootdown_overhead);
			info("%s DRAM page mapping overhead:%llu", getName(), dram_map_overhead);
			info("%s TLB miss overhead(exclude TLB shootdown and page fault): %llu", getName(),tlb_miss_exclude_shootdown);
			info("%s TLB miss overhead (include TLB shootdown and page fault): %llu",getName(), tlb_miss_overhead);
			info("%s clflush overhead caused by caching:%llu",getName(), clflush_overhead);
			info("%s clflush overhead caused extra write:%llu",getName(), extra_write);
		}
		void calculate_stats(std::ofstream &vmof)
		{
			vmof<<pg_walker_name<<" evict time:"<< total_evict <<"\t dirty evict time:"<<dirty_evict<<std::endl;
			vmof<<pg_walker_name<<" allocated pages:"<<allocated_page<<std::endl;
			vmof<<pg_walker_name<<" TLB shootdown overhead:"<<tlb_shootdown_overhead<<std::endl;
			vmof<<pg_walker_name<<" DRAM page mapping overhead:"<<dram_map_overhead<<std::endl;
			vmof<<pg_walker_name<<" TLB miss overhead(exclude TLB shootdown and page fault):"<<tlb_miss_exclude_shootdown<<std::endl;
			vmof<<pg_walker_name<<" TLB miss overhead (include TLB shootdown and page fault):"<<tlb_miss_overhead<<std::endl;
			vmof<<pg_walker_name<<" clflush overhead caused by caching:"<<clflush_overhead<<std::endl;
			vmof<<pg_walker_name<<" clflush overhead caused extra write:"<<extra_write<<std::endl;
		}
		
		Address do_page_fault(MemReq& req, PAGE_FAULT fault_type)
		{
			//allocate one page from Zone_Normal area
			debug_printf("page falut, allocate free page through buddy allocator");
			Page *page = NULL;
			Address vAddr = req.lineAddr << line_shift;
			if (zinfo->buddy_allocator)
			{
				page = zinfo->buddy_allocator->allocate_pages(0);
				if (page)
				{
					//TLB shootdown
					Address vpn = vAddr >> (zinfo->page_shift);
					tlb_shootdown(req, vpn);
					if (zinfo->enable_shared_memory)
					{
						if (!map_shared_region(req, page))
						{
							//std::cout<<"map "<< pg_walker_name<<":"<<vAddr<<"->"<<page->pageNo<<std::endl;
							Address overhead = paging->map_page_table(req.srcId, vAddr, page, (req.type == PUTS)?true:false);
							if(overhead == -1){
								panic("Map page table filed!");
							}
							if(enable_timing_mode){
								dram_map_overhead += overhead;					
								req.cycle += overhead;
							}
						}
					}
					else
					{
						Address overhead = paging->map_page_table(req.srcId, vAddr, page,(req.type == PUTS)?true:false);
						if(overhead == -1){
							panic("Map page table filed!");
						}
						if(enable_timing_mode){
							dram_map_overhead += overhead;
							req.cycle += overhead;
						}
					}
					req.pageDirty = (req.type == PUTS)?true:false;
					allocated_page++;
					return page->pageNo;
				}else{
					panic("Cannot allocate a page for data!");
				}
			}
			//update page table
			return (vAddr >> zinfo->page_shift);
		}

		uint64_t tlb_shootdown(Address vpn)
		{
			uint32_t overhead = 0;
			BaseTlb *tmp_tlb = NULL;
			for (uint64_t i = 0; i < zinfo->numCores; i++)
			{
				//instruction TLB shootdown
				tmp_tlb = zinfo->cores[i]->getInsTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->shootdown(vpn);
					overhead = MAX(overhead, s_overhead);
				}
				
				tmp_tlb = zinfo->cores[i]->getDataTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->shootdown(vpn);
					overhead = MAX(overhead, s_overhead);
				}
			}

			if(enable_timing_mode){
				tlb_shootdown_overhead += overhead;
				return overhead;
			}

			return 0;
		}

		uint64_t tlb_update(Address vpn, Address ppn)
		{
			uint32_t overhead = 0;
			BaseTlb *tmp_tlb = NULL;
			for (uint64_t i = 0; i < zinfo->numCores; i++)
			{
				//instruction TLB shootdown
				tmp_tlb = zinfo->cores[i]->getInsTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->update_entry(vpn, ppn);
					overhead = MAX(overhead, s_overhead);
				}
				
				tmp_tlb = zinfo->cores[i]->getDataTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->update_entry(vpn, ppn);
					overhead = MAX(overhead, s_overhead);
				}
			}

			if(enable_timing_mode){
				tlb_shootdown_overhead += overhead;
				return overhead;
			}

			return 0;
		}

	private:
		bool inline map_shared_region( MemReq& req , Page* page)
		{
			Address vaddr = req.lineAddr;
			//std::cout<<"find out shared region"<<std::endl;
			if (!zinfo->shared_region[procIdx].empty())
			{
				int vm_size = zinfo->shared_region[procIdx].size();
				//std::cout<<"mmap_cached:"<<std::dec<<mmap_cached
				//	<<" vm size:"<<std::dec<<vm_size<<std::endl;
				Address vm_start = zinfo->shared_region[procIdx][mmap_cached].start;
				Address vm_end = zinfo->shared_region[procIdx][mmap_cached].end;
				Address vpn = vaddr >> (zinfo->page_shift);
				//belong to the shared memory region
				//examine whether in mmap_cached region (examine mmap_cached firstly)
				if (find_shared_vm(vaddr,
								zinfo->shared_region[procIdx][mmap_cached]))
				{
					Address overhead = map_all_shared_memory(vaddr, page);
					if(enable_timing_mode){
						req.cycle += overhead;
						dram_map_overhead += overhead;
					}
					return true;
				}
				//after mmap_cached
				else if (vpn > vm_end && mmap_cached < vm_size - 1)
				{
					mmap_cached++;
					for (; mmap_cached < vm_size; mmap_cached++)
					{
						if (find_shared_vm(vaddr,
										zinfo->shared_region[procIdx][mmap_cached]))
						{
							Address overhead = map_all_shared_memory(vaddr, page);
							if(enable_timing_mode){
								req.cycle += overhead;
								dram_map_overhead += overhead;
							}
							return true;
						}
					}
					mmap_cached--;
				}
				//before mmap_cached
				else if (vpn < vm_start && mmap_cached > 0)
				{
					mmap_cached--;
					for (; mmap_cached >= 0; mmap_cached--)
					{
						if (find_shared_vm(vaddr,
										zinfo->shared_region[procIdx][mmap_cached]))
						{
							Address overhead = map_all_shared_memory(vaddr, page);
							if(enable_timing_mode){
								req.cycle += overhead;
								dram_map_overhead += overhead;
							}
							return true;
						}
					}
					mmap_cached++;
				}
			}
			return false;
		}

		bool inline find_shared_vm(Address vaddr, Section vm_sec)
		{
			Address vpn = vaddr >> (zinfo->page_shift);
			if (vpn >= vm_sec.start && vpn < vm_sec.end)
				return true;
			return false;
		}

		int inline map_all_shared_memory( Address va, Page* page)
		{
			int latency = 0;
			//std::cout<<"map all shared memory:"<<va<<std::endl;
			for (uint32_t i = 0; i < zinfo->numProcs; i++)
			{
				assert(zinfo->paging_array[i]);
				//std::cout<<"map "<<i<<std::endl;
				//std::cout<<"map "<<i<<" "<<va<<"->"<<(((Page*)page)->pageNo)<<std::endl;
				latency += zinfo->paging_array[i]->map_page_table(va, page);
			}
			return latency;
		}

		inline void tlb_shootdown( MemReq& req, Address vpn)
		{
			uint32_t overhead = 0;
			BaseTlb *tmp_tlb = NULL;
			for (uint64_t i = 0; i < zinfo->numCores; i++)
			{
				//instruction TLB shootdown
				tmp_tlb = zinfo->cores[i]->getInsTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->shootdown(vpn);
					overhead = MAX(overhead, s_overhead);
				}
				
				tmp_tlb = zinfo->cores[i]->getDataTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->shootdown(vpn);
					overhead = MAX(overhead, s_overhead);
				}
			}

			if(enable_timing_mode){
				tlb_shootdown_overhead += overhead;
				req.cycle += overhead;
			}
		}

		uint32_t update_tlb_flags(Address ppn, bool shared, bool dirty)
		{
			uint32_t overhead = 0;
			BaseTlb *tmp_tlb = NULL;
			for (uint64_t i = 0; i < zinfo->numCores; i++)
			{
				//instruction TLB shootdown
				tmp_tlb = zinfo->cores[i]->getInsTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->update_tlb_flags(ppn, shared, dirty);
					overhead = MAX(overhead, s_overhead);
				}
				
				tmp_tlb = zinfo->cores[i]->getDataTlb();
				if(tmp_tlb){
					uint32_t s_overhead = tmp_tlb->update_tlb_flags(ppn, shared, dirty);
					overhead = MAX(overhead, s_overhead);
				}
			}
		}

		void setParents(uint32_t childId, const g_vector<MemObject*>& _parents, Network* network)
		{
			selfId = childId;
			parents.resize(_parents.size());
			parentRTTs.resize(_parents.size());
			for (uint32_t p = 0; p < parents.size(); p++)
			{
				parents[p] = _parents[p];
				parentRTTs[p] = (network) ? network->getRTT(pg_walker_name.c_str(), parents[p]->getName()) : 0;
			}
		}

		void setCoreRecorder(BaseCoreRecorder* _cRec){cRec = _cRec;}

	public:
		bool enable_timing_mode;
		PagingStyle mode;
		g_string pg_walker_name;
	    BasePaging* paging;
		BaseCoreRecorder* cRec;// the Core Recorder of corresponding core
		g_vector<MemObject*> parents;
        g_vector<uint32_t> parentRTTs;
		uint32_t selfId;
		uint64_t period;
		unsigned long long tlb_shootdown_overhead;
		unsigned long long dram_map_overhead;

		unsigned long long tlb_miss_exclude_shootdown;
		unsigned long long tlb_miss_overhead;

		unsigned long long clflush_overhead;
		unsigned long long extra_write;
	private:
		uint32_t procIdx;
		uint32_t line_shift;
		int mmap_cached;
		Address allocated_page;
		Address total_evict;
		Address dirty_evict;
		lock_t walker_lock;
};
#endif

