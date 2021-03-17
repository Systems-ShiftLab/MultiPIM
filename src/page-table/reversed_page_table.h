/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#ifndef __REVERSED_PGT__
#define __REVERSED_PGT__
#include "g_std/g_unordered_map.h"
#include "g_std/g_list.h"
#include "page-table/page_table_entry.h"
#include "page-table/page_table.h"
#include "common/global_const.h"
#include "memory_hierarchy.h"
#include "locks.h"
#include "zsim.h"
#include "mmu/page.h"
class ReversedPaging: public BasePaging
{
	public:
		ReversedPaging(std::string mode_str , PagingStyle mode):BasePaging()
		{
			union
			{
				NormalPaging* normal_paging;
				PAEPaging* pae_paging;
				LongModePaging* longmode_paging;
			};
			std::cout<<"mode_str"<<std::endl;
			std::cout<<"mode_str:"<<mode_str<<std::endl;
			if( mode_str == "Legacy")
			{
				normal_paging = gm_memalign<NormalPaging>(CACHE_LINE_BYTES, 1);	
				paging = new (normal_paging) NormalPaging(mode);
			}
			else if( mode_str == "PAE")
			{
				pae_paging = gm_memalign<PAEPaging>(CACHE_LINE_BYTES, 1);	
				paging = new (pae_paging) PAEPaging(mode);
			}
			else if( mode_str == "LongMode")
			{
				longmode_paging = gm_memalign<LongModePaging>(CACHE_LINE_BYTES, 1);	
				paging = new (longmode_paging)LongModePaging(mode);
			}
			assert(paging);
			std::cout<<"create reversed paging"<<std::endl;
			remapped_page_num = 0;
			
			futex_init(&reversed_pgt_lock);
		}
		virtual Address access(MemReq& req)
		{
			//std::cout<<"reversed paging access"<<std::endl;
			return paging->access(req);
		}
		virtual Address access(MemReq& req , g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, BaseCoreRecorder* cRec, bool sendPTW)
		{
			return paging->access(req,parents,parentRTTs,cRec,sendPTW);
		}
		// /****important: find vpn according to ppn****/
		// virtual Address get_vpn( Address ppn )
		// {
		// 	assert( (reversed_pgt).count(ppn));
		// 	Address vpn = (reversed_pgt)[ppn]->get_vpn();
		// 	return vpn;
		// }
		/****important: find vpn according to ppn end****/
		virtual bool is_page_shared(Address ppn)
		{
			assert( (reversed_pgt).count(ppn));
			// assert((reversed_pgt)[ppn]->get_pgt_entry() != NULL);
			bool shared = (reversed_pgt)[ppn]->get_pgt_entry()->is_shared();
			return shared;
		}
		virtual PagingStyle get_paging_style()
		{
			return paging->get_paging_style();
		}
		
		virtual PageTable* get_root_directory()
		{
			return paging->get_root_directory();
		}
		
		virtual bool unmap_page_table( Address addr)
		{
			return paging->unmap_page_table(addr);
		}

		virtual int map_page_table( Address addr, Page* pg_ptr);
		virtual int map_page_table( uint32_t req_id, Address addr , Page* pg_ptr, bool is_write);
		
		virtual uint64_t remap_page_table( Address ppn, Address dst_ppn);

		virtual bool allocate_page_table(Address addr, Address size)
		{
			return paging->allocate_page_table(addr, size);
		}

		virtual void remove_root_directory()
		{
			paging->remove_root_directory();
		}

		virtual bool remove_page_table( Address addr, Address size)
		{
			return remove_page_table( addr, size);
		}
		virtual void lock() 
		{
			futex_lock(&reversed_pgt_lock);
		}
        virtual void unlock() 
		{
			futex_unlock(&reversed_pgt_lock);
		}
		
		virtual void calculate_stats();
		virtual void calculate_stats(std::ofstream &vmof);

	private:
		BasePaging* paging;
		/*****for inverted page table*****/
		//g_unordered_map<uint32_t, Content* > reversed_pgt;
		lock_t reversed_pgt_lock;
		g_unordered_map<Address, Content* > reversed_pgt;
		uint64_t remapped_page_num;
};
#endif
