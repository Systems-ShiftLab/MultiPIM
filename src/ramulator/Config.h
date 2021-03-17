#ifndef __CONFIG_H
#define __CONFIG_H

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <cassert>
#include "common/global_const.h"

namespace ramulator
{

class Config {
private:
    std::map<std::string, std::string> options;
    int stacks;
    int channels;
    int ranks;
    int subarrays;
    uint64_t pu_frequency = 2000*1000000;
    int pu_core_num = 0;
    int pu_instr_buffer_size = 32;
    int cpu_core_num = 0;
    int cacheline_size = 64;
    long expected_limit_insts = 0;
    bool disable_per_scheduling = false;
    bool split_trace = false;
    std::string org = "in_order";

    std::string pu_cache = "L1";
    int pu_l1cache_size = 32768;
    int pu_l1cache_ways = 4;
    PUTaskSchedulerType pu_task_scheduler = PU_SCHE_LOAD_BALANCE;
    int pu_task_queue_size = 1;
    int pu_rmab_size = 8;
    int pu_load_store_buffer_size = 16;

    bool pim_mode_enable = false;
    bool data_mem_net_no_latency = false;
    bool ptw_mem_net_no_latency = false;
    IdealMemNetWorkType mem_net_no_latency_type = IDEAL_MEM_NETWORK_LOCALVAULT;
    int vaults_per_stack = 32;
    std::string switch_network_config;

    bool pu_multithread_sim = false;
    bool pu_enable_tlb = false;
    std::string pu_pgt_walker_name = "sys.mem.ptw";
    PagingStyle pu_pgt_mode = LongMode_Normal;
    
    std::string pu_tlb_name = "sys.mem.tlbs";

    bool has_itlb = false;
    bool has_dtlb = false;

    int pu_itlb_req_queue_size = 8;
    int pu_itlb_entry_num = 128;
    int pu_itlb_hit_latency = 1;
    int pu_itlb_response_latency = 1;
    std::string pu_itlb_evict_policy = "LRU";

    int pu_dtlb_req_queue_size = 8;
    int pu_dtlb_entry_num = 128;
    int pu_dtlb_hit_latency = 1;
    int pu_dtlb_response_latency = 1;
    std::string pu_dtlb_evict_policy = "LRU";
    int pu_heartbeat_interval = -1;

    bool pu_enable_thread_map_mode = false;
public:
    Config() {}
    Config(const std::string& fname);
    void parse(const std::string& fname);
    std::string operator [] (const std::string& name) const {
      if (options.find(name) != options.end()) {
        return (options.find(name))->second;
      } else {
        return "";
      }
    }

    void enable_pu_thread_map_mode(bool enabled) { pu_enable_thread_map_mode = enabled; }
    bool pu_thread_map_mode_enabled() const {return pu_enable_thread_map_mode;}
    void set_mem_switch_network_config(std::string config_file) {switch_network_config = config_file;}
    std::string get_mem_switch_network_config() const {return switch_network_config;}

    void set_pu_heartbeat_interval(int interval) { pu_heartbeat_interval =  interval;}
    int get_pu_heartbeat_interval() const {return pu_heartbeat_interval;}

    int get_pu_max_task_queue_size() const { return pu_task_queue_size; }

    void set_pu_frequency(uint64_t frequency) {pu_frequency = frequency;}
    double get_pu_tick() const {return 1000000000.0/pu_frequency;}
    uint64_t get_pu_frequency() {return pu_frequency;}
    void set_pu_cache(std::string cache) {pu_cache = cache;}
    void set_pu_l1cache_size(int size) { pu_l1cache_size = size;}
    int  get_pu_l1cache_size() const { return pu_l1cache_size; }
    void set_pu_l1cache_ways(int ways) {pu_l1cache_ways = ways;}
    int  get_pu_l1cache_ways() const { return pu_l1cache_ways; }
    void set_pu_task_queue_size(int size) {pu_task_queue_size = size;}
    void set_pu_rmab_size(int size) {pu_rmab_size = size;}
    void set_pu_task_cheduler(std::string scheduler) {
      if(scheduler == "LoadBalance"){
        pu_task_scheduler = PU_SCHE_LOAD_BALANCE;
      } else {
        printf("PU scheduler %s is not supported yet!\n", scheduler.c_str());
        exit(0);
      }
    }
    int get_pu_rmab_size() const {return pu_rmab_size;}
    PUTaskSchedulerType get_pu_task_scheduler() const {return pu_task_scheduler;}

    void set_pu_load_store_buffer_size(int size) {pu_load_store_buffer_size = size;}
    int get_pu_load_store_buffer_size() const {return pu_load_store_buffer_size;}

    void set_pu_instr_buffer_size(int size) { pu_instr_buffer_size = size; }
    int get_pu_instr_buffer_size() const { return pu_instr_buffer_size; }

    bool enable_data_mem_network_no_latency(bool no_latency) { data_mem_net_no_latency = no_latency; }
    bool data_mem_network_no_latency() const{ return data_mem_net_no_latency; }
    bool enable_ptw_mem_network_no_latency(bool no_latency) { ptw_mem_net_no_latency = no_latency; }
    bool ptw_mem_network_no_latency() const{ return ptw_mem_net_no_latency; }
    void set_mem_network_no_latency_type(int type) 
    {
      switch(type){
        case 0:
          mem_net_no_latency_type = IDEAL_MEM_NETWORK_LOCALVAULT;
          break;
        case 1:
          mem_net_no_latency_type = IDEAL_MEM_NETWORK_LOCALSTACK;
          break;
        default:
          printf("Wrong ideal memory network type: %d!\n", type);
          exit(0);
          break;
      }
    }
    IdealMemNetWorkType get_mem_network_no_latency_type() const { return mem_net_no_latency_type; }


    //TLB and page table walks
    void enable_pu_tlb(bool mode){pu_enable_tlb = mode;}
    bool pu_tlb_enabled() const {return pu_enable_tlb;}

    void set_pu_pgt_walker_name(std::string pgt_walker_name){pu_pgt_walker_name = pgt_walker_name;}
    std::string get_pu_pgt_walker_name() const {return pu_pgt_walker_name;}

    void set_pu_pgt_mode(PagingStyle pgt_mode){pu_pgt_mode = pgt_mode;}
    PagingStyle get_pu_pgt_mode() const {return pu_pgt_mode;}

    void set_pu_tlb_name(std::string tlb_name){pu_tlb_name = tlb_name;}
    std::string get_pu_tlb_name() const {return pu_tlb_name;}

    void set_pu_has_itlb() {has_itlb = true;}
    void set_pu_has_dtlb() {has_dtlb = true;}
    bool pu_has_itlb() const {return has_itlb;}
    bool pu_has_dtlb() const {return has_dtlb;}

    void set_pu_itlb_entry_num(int num){pu_itlb_entry_num = num;}
    int get_pu_itlb_entry_num() const {return pu_itlb_entry_num;}
    void set_pu_itlb_hit_latency(int lat){pu_itlb_hit_latency = lat;}
    int get_pu_itlb_hit_latency() const {return pu_itlb_hit_latency;}
    void set_pu_itlb_response_latency(int lat){pu_itlb_response_latency = lat;}
    int get_pu_itlb_response_latency() const {return pu_itlb_response_latency;}
    void set_pu_itlb_evict_policy(std::string mode){pu_itlb_evict_policy = mode;}
    std::string get_pu_itlb_evict_policy() const {return pu_itlb_evict_policy;}
    void set_pu_itlb_req_queue_size(int size) {pu_itlb_req_queue_size = size;}
    int get_pu_itlb_req_queue_size() const { return pu_itlb_req_queue_size; }

    void set_pu_dtlb_entry_num(int num){pu_dtlb_entry_num = num;}
    int get_pu_dtlb_entry_num() const {return pu_dtlb_entry_num;}
    void set_pu_dtlb_hit_latency(int lat){pu_dtlb_hit_latency = lat;}
    int get_pu_dtlb_hit_latency() const {return pu_dtlb_hit_latency;}
    void set_pu_dtlb_response_latency(int lat){pu_dtlb_response_latency = lat;}
    int get_pu_dtlb_response_latency() const {return pu_dtlb_response_latency;}
    void set_pu_dtlb_evict_policy(std::string mode){pu_dtlb_evict_policy = mode;}
    std::string get_pu_dtlb_evict_policy() const {return pu_dtlb_evict_policy;}
    void set_pu_dtlb_req_queue_size(int size) {pu_dtlb_req_queue_size = size;}
    int get_pu_dtlb_req_queue_size() const { return pu_dtlb_req_queue_size; }

    void set_pu_multithread_sim(bool enable) {pu_multithread_sim = enable;}
    bool enable_pu_multithread_sim() const {return pu_multithread_sim;}

    void set_org(std::string _org){org = _org;}
    void parse_to_const(const std::string& name, const std::string& value);
    void set_disable_per_scheduling(bool status){disable_per_scheduling = status;}
    void set_split_trace(bool status){split_trace = status;}
    bool get_disable_per_scheduling() const {return disable_per_scheduling;}
    bool get_split_trace() const {return split_trace;}
    bool contains(const std::string& name) const {
      if (options.find(name) != options.end()) {
        return true;
      } else {
        return false;
      }
    }

    void add (const std::string& name, const std::string& value) {
      if (!contains(name)) {
        options.insert(make_pair(name, value));
      } else {
        printf("ramulator::Config::add options[%s] already set.\n", name.c_str());
      }
    }

    void set(const std::string& name, const std::string& value) {
      options[name] = value;
      // TODO call this function only name maps to a constant
      parse_to_const(name, value);
    }

    void set_cpu_core_num(int _cpu_core_num) {cpu_core_num = _cpu_core_num;}
    void set_cacheline_size(int _cacheline_size) {cacheline_size = _cacheline_size;}
    void set_pu_core_num(int _pu_core_num) {pu_core_num = _pu_core_num;}


    int get_int_value(const std::string& name) const {
      assert(options.find(name) != options.end() && "can't find this argument");
      return atoi(options.find(name)->second.c_str());
    }
    int get_stacks() const {return get_int_value("stacks");}
    int get_channels() const {return get_int_value("channels");}
    int get_subarrays() const {return get_int_value("subarrays");}
    int get_ranks() const {return get_int_value("ranks");}
    void set_vaults_per_stack(int vaults) {vaults_per_stack = vaults;}
    int get_vaults_per_stack() const {return vaults_per_stack;}
    void set_pim_mode(bool enable_pim_mode) {pim_mode_enable = enable_pim_mode;}
    bool pim_mode_enabled () const {return pim_mode_enable;}
    std::string get_pu_type() const{
       /*if (contains("core_type")) {
           return options.find("core_type")->second;
       }*/
       return org;
    }
    std::string get_topology_file() const{
      if (contains("topology_file")){
        return options.find("topology_file")->second;
      }else{
        printf("topology_file path not specified!\n");
        assert(0);
      }
    }
    int get_cpu_core_num() const {return cpu_core_num;}
    int get_pu_core_num() const {return pu_core_num;}
    int get_core_num() const {return cpu_core_num + pu_core_num;}
    int get_cacheline_size() const {return cacheline_size;}
    long get_expected_limit_insts() const {
      if (contains("expected_limit_insts")) {
        return get_int_value("expected_limit_insts");
      } else {
        return 0;
      }
    }
    bool has_l3_cache() const {
         return (pu_cache == "all") || (pu_cache == "L3");
    }
    bool has_core_caches() const {
      return (pu_cache == "all" || pu_cache == "L1L2" || pu_cache == "L1");
    }
    bool has_l2_core_cache() const {
      return (pu_cache == "all" || pu_cache == "L1L2");
    }
    bool enable_multithread() const { return false; }
    bool calc_weighted_speedup() const {
      return (expected_limit_insts != 0);
    }
    bool record_cmd_trace() const {
      // the default value is false
      if (options.find("record_cmd_trace") != options.end()) {
        if ((options.find("record_cmd_trace"))->second == "on") {
          return true;
        }
        return false;
      }
      return false;
    }
    bool print_cmd_trace() const {
      // the default value is false
      if (options.find("print_cmd_trace") != options.end()) {
        if ((options.find("print_cmd_trace"))->second == "on") {
          return true;
        }
        return false;
      }
      return false;
    }
    bool enable_quadrant() const {
      //the default value is false
      if(options.find("quadrant") != options.end()) {
        if(options.find("quadrant")->second == "on"){
          return true;
        }
        return false;
      }
      return false;
    }
};


} /* namespace ramulator */

#endif /* _CONFIG_H */

