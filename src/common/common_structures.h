/*
 * 2015-xxx by YuJie Chen
 * email:    YuJieChen_hust@163.com
 * function: extend zsim-nvmain with some other simulation,such as tlb,page table,page table,etc.  
 */

/*
 * Copyright (C) 2020 Chao Yu (yuchaocs@gmail.com)
 */

#ifndef _COMMON_STRUCTURES_H_
#define _COMMON_STRUCTURES_H_

#include <functional>
#include "g_std/g_vector.h"
#include "g_std/g_string.h"
#include "g_std/g_list.h"
#include "g_std/g_multimap.h"
#include "galloc.h"
#include "common/global_const.h"

template<class T>
class FlexiList
{
	public:
		FlexiList():head(NULL),tail(NULL)
		{
			size = 0;
		}
		bool is_empty()
		{
			if(head)
				return false;
			return true;
		}
		void clear()	
		{
			size = 0;
			head = tail = NULL;
		}
		//add block to the tail of list 
		void push_block_back(T *block)
		{
			if( head == NULL )
			{
				tail = head = block;
				tail->next = NULL;
				size++;
				return;
			}
			else if( head == tail)
			{
				head->next = block;
			}
			tail->next = block;
			tail = block;
			tail->next = NULL;
			size++;
		}

		T* fetch_head()
		{
			assert(head);
			T* tmp_block;
			tmp_block = head;
			if( head->next == NULL)
			{
				//std::cout<<"set head with NULL"<<std::endl;
				head =  NULL;
				tail = NULL;
			}
			else
			{
				head = head->next;
				tmp_block->next = NULL;
			}
			size--;
			return tmp_block;	
		}

		T* get_head()
		{
			assert(head);
			return head;
		}
		
		T* fetch_block(T* wanted)
		{
			assert(head);
			T* tmp_block,*pre_block;
			tmp_block = head;
			pre_block = NULL;
			while(tmp_block != NULL){
				if(tmp_block == wanted){
					if(tmp_block == head){
						head = head->next;
						tmp_block->next = NULL;
						if(head == NULL)
							tail = NULL;
					}else if(tmp_block == tail){
						pre_block->next = NULL;
						tail = pre_block;
					}else{
						pre_block->next = tmp_block->next;
						tmp_block->next = NULL;
					}
					break;
				}
				pre_block = tmp_block;
				tmp_block = tmp_block->next;
			}
			assert(tmp_block != NULL);
			size--;
			return tmp_block;	
		}

		int get_size()
		{
			return size;
		}

		void extend_back( FlexiList<T>* added_list)
		{
			if( added_list->is_empty())
				return;
			if( head == NULL)	
			{
				head = added_list->head;
				tail = added_list->tail;
				return;
			}
			tail->next = added_list->head;
			tail = added_list->tail;
			size += added_list->get_size();
			return;
		}

	T* head;
	T* tail;
	int size;
};

// Helper function to find section address
// adapted from http://outflux.net/aslr/aslr.c
struct Section : public GlobAlloc
{
    uintptr_t start;
    uintptr_t end;
	uint64_t offset;
	Section(uintptr_t s, uintptr_t e, 
			uint64_t off = -1): start(s),
			end(e),offset(off)
	{}
};

struct MemConfig : public GlobAlloc
{
	bool pim_mode_enabled;
	bool pim_tlb_enabled;
	bool data_mem_network_no_latency;
	bool ptw_mem_network_no_latency;
	int mem_network_no_latency_type;
	int pim_tlb_type;
    int pim_tlb_hit_lat;
    int pim_tlb_res_lat;
	int pim_tlb_req_queue_size;
	g_map<std::string,uint32_t> pim_tlbs_entry_num;
    g_map<std::string,std::string> pim_tlbs_evict_plicy;
	
	uint64_t pu_frequency;
	std::string pu_cache;
	std::string pu_task_scheculer;
	int pu_load_store_buffer_size;
	int pu_instr_buffer_size;
	int pu_task_queue_size;
	int pu_rmab_size;
	int pu_heartbeat_interval;
	int pu_l1_size;
	int pu_l1_ways;
	bool enable_memtrace_mode;
	bool pu_multithread_sim;
	bool pu_enable_thread_map_mode;
	std::string hmc_switch_network_conf;

	MemConfig(){
		pim_mode_enabled = false;
		pim_tlb_enabled = false;
		data_mem_network_no_latency = false;
		ptw_mem_network_no_latency = false;
		mem_network_no_latency_type = 0;
		enable_memtrace_mode = false;
		pu_multithread_sim = false;
		pu_enable_thread_map_mode = false;
		pim_tlb_type = COMMONTLB;
		pim_tlb_hit_lat = 1;
		pim_tlb_res_lat = 1;
		pim_tlb_req_queue_size = 8;
		pim_tlbs_entry_num.clear();
		pim_tlbs_evict_plicy.clear();

		pu_frequency = 2000;
		pu_cache = "L1";
		pu_load_store_buffer_size = 16;
		pu_instr_buffer_size = 32;
		pu_task_queue_size = 1;
		pu_rmab_size = 8;
		pu_heartbeat_interval = -1;
		pu_l1_size = 32768;
		pu_l1_ways = 4;
		hmc_switch_network_conf = "";
	}
};

class MemStats : public GlobAlloc
{
public:
	uint64_t incomingMemReqs;
	uint64_t incomingPTWs;
	uint64_t completedMemReqs;
	uint64_t completedPTWs;
	uint64_t issuedMemReqs;
	uint64_t issuedPTWs;
	uint64_t inflightMemReqs;
	uint64_t inflightPTWs;
	uint64_t incomingOffloads;
	uint64_t completedOffloads;
	uint64_t issuedOffloads;
	uint64_t inflightOffloads;
	uint64_t incomingOffloadInstrs;
	uint64_t completedOffloadInstrs;
	uint64_t issuedOffloadInstrs;
	uint64_t inflightOffloadInstrs;
	uint64_t maxInflightOffloads;
	uint64_t PURetiredInstr;
	double PUUtilization;
	MemStats(){
		incomingMemReqs = 0;
		incomingPTWs = 0;
		completedMemReqs = 0;
		completedPTWs = 0;
		issuedMemReqs = 0;
		issuedPTWs = 0;
		inflightMemReqs = 0;
		inflightPTWs = 0;
		incomingOffloads = 0;
		completedOffloads = 0;
		issuedOffloads = 0;
		inflightOffloads = 0;
		incomingOffloadInstrs = 0;
		completedOffloadInstrs = 0;
		issuedOffloadInstrs = 0;
		inflightOffloadInstrs = 0;
		maxInflightOffloads = 0;
		PUUtilization = 0.0;
		PURetiredInstr = 0;
	}
};

struct BblInfo;  // defined in decoder.h/core.h

class OffloadBbl : public GlobAlloc
{
public:
	BblInfo* bbl;
	g_vector<Address> ifetchAddrs;
	g_vector<Address> loadAddrs;
	g_vector<Address> storeAddrs;
};

class BasePaging;
class MemoryMapper;
class OffLoadMetaData : public GlobAlloc
{
public:
	uint32_t srcId;
	uint64_t cycle;
	uint64_t taskIdx;
	uint32_t procIdx;
	uint32_t threadIdx;
	uint64_t numInstrs;
	uint64_t numMemReqs;
	BasePaging * paging;
	MemoryMapper* mmp;
	g_list<OffloadBbl> offloadBbls;
	std::function<void(uint64_t)> callback;

public:
	OffLoadMetaData() {}
	OffLoadMetaData(uint32_t pid, uint32_t tid, BasePaging * _paging, MemoryMapper* _mmp) : 
					procIdx(pid), threadIdx(tid), numInstrs(0), numMemReqs(0),paging(_paging), mmp(_mmp) {}
	~OffLoadMetaData() {}
};

class BBLStatus : GlobAlloc
{
public:
	uint32_t offload_code; // each thread has an offload_code;
	bool can_offload;
	bool need_offload;
	bool pre_offload;
	OffLoadMetaData *pim_task;
public:
	BBLStatus() : offload_code(0), can_offload(false), need_offload(false), pre_offload(true), pim_task(NULL) {}
};

class StreamOffloadMetaData : public GlobAlloc
{
public:
	uint32_t procIdx;
	uint32_t threadIdx;
	uint32_t numInstrs;
	BasePaging * paging;
	MemoryMapper* mmp;
	OffloadBbl offloadBbl;

public:
	StreamOffloadMetaData(){}
	StreamOffloadMetaData(uint32_t pid, uint32_t tid, BasePaging * _paging, MemoryMapper* _mmp):
		procIdx(pid), threadIdx(tid),paging(_paging), mmp(_mmp){}
	~StreamOffloadMetaData(){}
};
#endif
