/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */

/*
 * Copyright (C) 2020 Chao Yu (yuchaocs@gmail.com)
 */

#ifndef _PAGE_TABLE_H
#define _PAGE_TABLE_H
#include <map>
#include <iterator>
#include "common/global_const.h"
#include "common/common_functions.h"
#include "page-table/comm_page_table_op.h"
#include "page-table/page_table_entry.h"
#include "memory_hierarchy.h"
#include "locks.h"
/*#------legacy paging(supports 4KB&&4MB)--------#*/
class NormalPaging: public BasePaging
{
	public:
		NormalPaging(PagingStyle select);
		~NormalPaging();

		int map_page_table( Address addr , Page* pg_ptr ,BasePDTEntry*& mapped_entry);
		virtual int map_page_table( Address addr , Page* pg_ptr);
		virtual int map_page_table( uint32_t req_id, Address addr , Page* pg_ptr, bool is_write) { return map_page_table(addr, pg_ptr); }
		virtual bool unmap_page_table( Address addr );	
		//access
		virtual Address access( MemReq& req );
		virtual Address access(MemReq& req , g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, BaseCoreRecorder* cRec, bool sendPTW) {return access(req);}
		virtual void remove_root_directory();
	   //allocate page table for [addr , addr+size]
		virtual bool allocate_page_table(Address addr , Address size );

		virtual bool remove_page_table(Address addr,Address size);

		virtual PagingStyle get_paging_style()
		{
			return mode;
		}
		unsigned get_page_table_num()	
		{ return cur_page_table_num; }

		virtual PageTable* get_root_directory()
		{   return page_directory;	}
	
		virtual void calculate_stats()
		{
			long unsigned overhead = (long unsigned)cur_page_table_num*1024;
			info("page table number: %d", cur_page_table_num);
			info("overhead of extended storage:%f MB", (double)overhead/(double)(1024*1024));
		}
		virtual void calculate_stats(std::ofstream &vmof)
		{
			long unsigned overhead = (long unsigned)cur_page_table_num*1024;
			vmof<<"page table number:"<<cur_page_table_num<<std::endl;
			vmof<<"overhead of extended storage:"<< (double)overhead/(double)(1024*1024) <<" MB"<<std::endl;
		}
		virtual void lock() {futex_lock(&table_lock);}
        virtual void unlock() {futex_unlock(&table_lock);}
	protected:
		PageTable* allocate_one_pagetable(unsigned pd_entry_id, int& allocate_time );
		bool allocate_page_table(entry_list pd_entry);
		//allocate page table for [addr , addr+size]
		bool remove_page_table(entry_list pd_entry);
		bool remove_page_table(unsigned pd_entry_id);
	public:
	    PageTable* page_directory;	//page directory table
	private:
		PagingStyle mode;
		unsigned cur_page_table_num;
		BaseTlb* itlb;
		BaseTlb* dtlb; 
		lock_t table_lock;
};

/*#----Legacy-PAE-Paging(supports 4KB&&2MB)-----#*/
class PAEPaging: public BasePaging
{
	public:
		PAEPaging( PagingStyle select);
		~PAEPaging();
		//access
		virtual Address access(MemReq& req );
		virtual Address access(MemReq& req , g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, BaseCoreRecorder* cRec, bool sendPTW) {return access(req);}
		int map_page_table( Address addr , Page* pg_ptr , BasePDTEntry*&  mapped_entry);
		virtual int map_page_table( Address addr , Page* pg_ptr);
		virtual int map_page_table( uint32_t req_id, Address addr , Page* pg_ptr, bool is_write) { return map_page_table(addr, pg_ptr); }
		virtual bool unmap_page_table( Address addr );		

		//PAE-Huge mode
		bool allocate_pdt(Address addr , Address size);

		virtual bool allocate_page_table(Address addr , Address size);

		virtual void remove_root_directory();	//remove whole page table structures
		bool remove_pdt( Address addr , Address size);

		virtual bool remove_page_table( Address addr, Address size);
		virtual PagingStyle get_paging_style()
		{
			return mode;
		}

		unsigned get_page_table_num()
		{  return cur_pt_num;	}

		unsigned get_pdt_num()
		{ return cur_pdt_num;	}

		virtual PageTable* get_root_directory()
		{  return page_directory_pointer;	}
		
		virtual void lock() {futex_lock(&table_lock);}
        virtual void unlock() {futex_unlock(&table_lock);}

		virtual void calculate_stats()
		{
			long unsigned overhead = (long unsigned)(cur_pt_num+cur_pdt_num)*PAGE_SIZE;
			info("page directory table number: %d", cur_pdt_num);
			info("page table number: %d", cur_pt_num);
			info("overhead of extended storage:%f MB", (double)overhead/(double)(1024*1024));
		}
		virtual void calculate_stats(std::ofstream &vmof)
		{
			long unsigned overhead = (long unsigned)(cur_pt_num+cur_pdt_num)*PAGE_SIZE;
			vmof<<"page directory table number:"<<cur_pdt_num<<std::endl;
			vmof<<"page table number:"<< cur_pt_num<<std::endl;
			vmof<<"overhead of extended storage:"<< (double)overhead/(double)(1024*1024) <<" MB"<<std::endl;
		}

	protected:
	   //allocate page directory table,at most to 4 
		bool allocate_pdt(entry_list pdpt_entry);
		PageTable* allocate_pdt(unsigned pdpt_entry_id, int& alloc_time);


		//allocate page table , at most to 512
		bool allocate_page_table(pair_list pdp_entry);
		PageTable* allocate_page_table(unsigned pdpt_entry_id ,
				unsigned pdp_entry_id, int& alloc_time);

		bool remove_pdt( entry_list pdpt_entry);
		bool remove_pdt( unsigned pdp_entry_id);

		bool remove_page_table(pair_list pdp_entry);
		bool remove_page_table(unsigned pdp_entry_id, unsigned pd_entry_id);
	private:
		PagingStyle mode;
		PageTable* page_directory_pointer;
		unsigned cur_pdt_num;  //current page directory table num
		unsigned cur_pt_num;  //current page table num
		lock_t table_lock;
};


/*#----LongMode-Paging(supports 4KB&&2MB&&1GB)---#*/
class LongModePaging: public BasePaging
{
	public:
		LongModePaging(PagingStyle selection);
		~LongModePaging();
		int map_page_table( Address addr , Page* pg_ptr , BasePDTEntry*& mapped_entry);
		virtual int map_page_table( Address addr , Page* pg_ptr);
		virtual int map_page_table( uint32_t req_id, Address addr , Page* pg_ptr, bool is_write);
		virtual bool unmap_page_table( Address addr );		
		//access
		virtual Address access(MemReq& req);
		virtual Address access(MemReq& req , g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, BaseCoreRecorder* cRec, bool sendPTW);
		virtual bool allocate_page_table(Address addr , Address size);
		virtual void remove_root_directory();
		virtual bool remove_page_table( Address addr , Address size);

		virtual PagingStyle get_paging_style()
		{ return mode;			}
	
		unsigned get_page_table_num()
		{ return cur_pt_num;	}

		unsigned get_page_directory_num()
		{ return cur_pd_num;	}

		unsigned get_page_directory_pointer_num()
		{ return cur_pdp_num;	}
		
		virtual PageTable* get_root_directory()
		{ return pml4;		}

		virtual void calculate_stats()
		{
			long unsigned overhead = (long unsigned)(cur_pt_num+cur_pd_num+cur_pdp_num)*PAGE_SIZE;
			info("Error migrated pages:%d",error_migrated_pages);
			info("page directory pointer number:%d", cur_pdp_num);
			info("page directory number:%d", cur_pd_num);
			info("page table number:%d", cur_pt_num);
			info("overhead of page table storage:%f MB", (double)overhead/(double)(1024*1024));
		}
		virtual void calculate_stats(std::ofstream &vmof)
		{
			long unsigned overhead = (long unsigned)(cur_pt_num+cur_pd_num+cur_pdp_num)*PAGE_SIZE;
			vmof<<"Error migrated pages:"<<error_migrated_pages<<std::endl;
			vmof<<"page directory pointer number:"<<cur_pdp_num<<std::endl;
			vmof<<"page directory number:"<<cur_pd_num<<std::endl;
			vmof<<"page table number:"<<cur_pt_num<<std::endl;
			vmof<<"overhead of page table storage:"<< (double)overhead/(double)(1024*1024) <<" MB"<<std::endl;
		}
		virtual void lock() {futex_lock(&table_lock);}
        virtual void unlock() {futex_unlock(&table_lock);}
	protected:
		//allocate multiple
		PageTable* allocate_page_directory_pointer( unsigned pml4_entry_id, int& alloc_time);
		bool allocate_page_directory_pointer(entry_list pml4_entry);
		
		PageTable* allocate_page_directory( unsigned pml4_entry_id , unsigned pdpt_entry_id, int& alloc_time);
		bool allocate_page_directory(pair_list high_level_entry);

		PageTable* allocate_page_table( unsigned pml4_entry_id , 
				unsigned pdpt_entry_id , unsigned pdt_entry_id, int& alloc_time);
		bool allocate_page_table(triple_list high_level_entry);

		//remove
		bool remove_page_directory_pointer(unsigned pml4_entry_id );
		bool remove_page_directory_pointer(entry_list pml4_entry);
		bool remove_page_directory(unsigned pml4_entry_id , unsigned pdp_entry_id);
		bool remove_page_directory(pair_list high_level_entry);
		bool remove_page_table(unsigned pml4_entry_id , unsigned pdp_entry_id , unsigned pd_entry_id);
		inline bool remove_page_table(triple_list high_level_entry);
	
		PageTable* get_tables(unsigned level , std::vector<unsigned> entry_id_vec);
	private:
		uint64_t loadPageTable(MemReq& req, uint64_t startCycle, uint64_t pageNo, uint32_t entry_id, g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, BaseCoreRecorder* cRec, bool sendPTW);
		uint64_t loadPageTables(MemReq& req, g_vector<uint64_t>& pgt_addrs, g_vector<MemObject*> &parents, g_vector<uint32_t> &parentRTTs, bool sendPTW);
		inline uint64_t getPGTAddr(uint64_t pageNo, uint32_t entry_id) __attribute__((always_inline));
	public:
		PageTable* pml4;
	private:
		PagingStyle mode;
		//number of page directory pointer at most 512
		uint64_t cur_pdp_num;
		uint64_t cur_pd_num;
		uint64_t cur_pt_num;
		
		lock_t table_lock;
		uint64_t error_migrated_pages;
};

// class PagingFactory
// {
// 	public:
// 		PagingFactory()
// 		{}
// 		static BasePaging* CreatePaging( PagingStyle mode)
// 		{
// 			std::string mode_str = pagingmode_to_string(mode);
// 			debug_printf("paging mode is "+mode_str);
// 			if( mode_str == "Legacy" )
// 				return (new NormalPaging(mode));
// 			else if( mode_str == "PAE")
// 				return (new PAEPaging(mode));	
// 			else if( mode_str == "LongMode")
// 				return (new LongModePaging(mode));
// 			else 
// 				return (new NormalPaging(Legacy_Normal));	//default is legacy paging
// 		}
// };
#endif
