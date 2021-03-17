/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */
#include <iterator>
#include "page-table/reversed_page_table.h"
#include "common/common_functions.h"

#include "mmu/zone.h"
//addr: virtual address
//pg_ptr: point to page table
int ReversedPaging::map_page_table( Address addr, Page* pg_ptr)
{
	BasePDTEntry* mapped_entry = NULL;
	int latency = paging->map_page_table(addr, pg_ptr,mapped_entry);
	Address vpn = addr >>(zinfo->page_shift);
	Address ppn = ((Page*)pg_ptr)->pageNo; 
	//std::cout<<"map page table, ppn:"<<ppn<<" vpn:"<<vpn<<std::endl;
	if( mapped_entry )
	{
		Content* content = new Content( mapped_entry, vpn);
		assert( !reversed_pgt.count(ppn) );
		reversed_pgt[ppn] = content;
	}
	assert(reversed_pgt.count(ppn));
	return latency;
}

int ReversedPaging::map_page_table( uint32_t req_id, Address addr , Page* pg_ptr, bool is_write)
{
	BasePDTEntry* mapped_entry;
	int latency = paging->map_page_table(addr, pg_ptr,mapped_entry);
	Address vpn = addr >>(zinfo->page_shift);
	Address ppn = ((Page*)pg_ptr)->pageNo; 
	// std::cout<<"map page table, ppn:"<<ppn<<" vpn:"<<vpn<<std::endl;
	if( mapped_entry )
	{
		// std::cout<<"map page table, ppn:"<<ppn<<" vpn:"<<vpn<<std::endl;
		Content* content = new Content( mapped_entry, vpn);
		assert( !reversed_pgt.count(ppn) );
		reversed_pgt[ppn] = content;
		mapped_entry->set_lrequester(req_id);
		mapped_entry->set_accessed();
		if(is_write)
			mapped_entry->set_dirty();
	}
	assert(reversed_pgt.count(ppn));
	return latency;
}

uint64_t ReversedPaging::remap_page_table( Address ppn, Address dst_ppn)
{
	void* page_ptr = NULL;
	void* dst_ptr = NULL;
	page_ptr = (void*)zinfo->memory_node->get_page_ptr(ppn);
	dst_ptr = zinfo->memory_node->get_page_ptr(dst_ppn);
	assert(page_ptr);
	assert(dst_ptr);
	uint64_t latency = 0;
	futex_lock(&reversed_pgt_lock);

	// assert(reversed_pgt.count(ppn));

	if( reversed_pgt.count(ppn) )
	{
		// std::cout<<"remap:"<<ppn<<" -> "<<dst_ppn<<" vpn:"<<get_vpn(ppn)<<std::endl;
		latency += zinfo->mem_access_time;
		reversed_pgt[dst_ppn] = reversed_pgt[ppn];
		(reversed_pgt[dst_ppn])->get_pgt_entry()->set_next_level_address(dst_ptr);
		(reversed_pgt[dst_ppn])->get_pgt_entry()->remapped_times++;
		reversed_pgt.erase(ppn);
		
		//(reversed_pgt[ppn])->get_pgt_entry()->clear_dirty();
		remapped_page_num++;
	}
	// if( (reversed_pgt).count(dst_ppn) )
	// {
	// 	//std::cout<<"remap:"<<dst_ppn<<"("<<ppn<<")"<<" -> "<<dst_ppn<<"("<<dst_dram<<")"<<" vpn:"<<get_vpn(dst_ppn)<<std::endl;
	// 	latency += zinfo->mem_access_time;
	// 	(reversed_pgt[dst_ppn])->get_pgt_entry()->set_next_level_address(page_ptr);
	// 	//(reversed_pgt[dst_ppn])->get_pgt_entry()->clear_dirty();
	// }
	futex_unlock(&reversed_pgt_lock);
	return latency;
}

void ReversedPaging::calculate_stats()
{
	paging->calculate_stats();
}

void ReversedPaging::calculate_stats(std::ofstream &vmof)
{
	long unsigned page_num = (long unsigned)reversed_pgt.size();
	vmof << "Total pages:" << page_num<<", remapped pages:"<<remapped_page_num << std::endl;
	paging->calculate_stats(vmof);
}