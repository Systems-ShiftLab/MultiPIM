#ifndef __HMC_MEMORY_H
#define __HMC_MEMORY_H

#include "HMC.h"
#include "LogicLayer.h"
#include "LogicLayer.cpp"
#include "Memory.h"
#include "Packet.h"
#include "Statistics.h"
#include "common/common_functions.h"
#include "Icnt_Wrapper.h"
#include "MemoryTopology.h"
#include "MemTopoParser.h"
#include "debug.h"

#include "Locks.h"

using namespace std;
using namespace ramulator;

namespace ramulator
{

template<>
class Memory<HMC, Controller> : public MemoryBase
{
public:
  long max_address;

  long capacity_per_stack;
  ScalarStat dram_capacity;
  ScalarStat num_dram_cycles;
  ScalarStat num_incoming_requests;
  VectorStat num_read_requests;
  VectorStat num_write_requests;
  ScalarStat ramulator_active_cycles;
  VectorStat incoming_requests_per_channel;
  VectorStat incoming_read_reqs_per_channel;
  ScalarStat maximum_internal_bandwidth;
  ScalarStat maximum_link_bandwidth;
  ScalarStat read_bandwidth;
  ScalarStat write_bandwidth;

  ScalarStat read_latency_avg;
  ScalarStat read_latency_ns_avg;
  ScalarStat read_latency_sum;
  ScalarStat write_latency_avg;
  ScalarStat write_latency_ns_avg;
  ScalarStat write_latency_sum;
  ScalarStat queueing_latency_avg;
  ScalarStat queueing_latency_ns_avg;
  ScalarStat queueing_latency_sum;
  ScalarStat request_packet_latency_avg;
  ScalarStat request_packet_latency_ns_avg;
  ScalarStat request_packet_latency_sum;
  ScalarStat response_packet_latency_avg;
  ScalarStat response_packet_latency_ns_avg;
  ScalarStat response_packet_latency_sum;
  ScalarStat request_packet_network_latency_sum;
  ScalarStat request_packet_network_latency_avg;
  ScalarStat response_packet_network_latency_sum;
  ScalarStat response_packet_network_latency_avg;

  // shared by all Controller objects
  ScalarStat read_transaction_bytes;
  ScalarStat write_transaction_bytes;
  ScalarStat row_hits;
  ScalarStat row_misses;
  ScalarStat row_conflicts;
  VectorStat read_row_hits;
  VectorStat read_row_misses;
  VectorStat read_row_conflicts;
  VectorStat write_row_hits;
  VectorStat write_row_misses;
  VectorStat write_row_conflicts;

  ScalarStat req_queue_length_avg;
  ScalarStat req_queue_length_sum;
  ScalarStat read_req_queue_length_avg;
  ScalarStat read_req_queue_length_sum;
  ScalarStat write_req_queue_length_avg;
  ScalarStat write_req_queue_length_sum;
  ScalarStat rmab_length_sum;
  ScalarStat rmab_length_avg;

  VectorStat record_read_hits;
  VectorStat record_read_misses;
  VectorStat record_read_conflicts;
  VectorStat record_write_hits;
  VectorStat record_write_misses;
  VectorStat record_write_conflicts;

  ScalarStat total_pu_local_vault_read_requests;
  ScalarStat total_pu_local_vault_write_requests;
  ScalarStat total_pu_local_memory_read_requests;
  ScalarStat total_pu_local_memory_write_requests;
  ScalarStat total_pu_remote_memory_read_requests;
  ScalarStat total_pu_remote_memory_write_requests;

  VectorStat link_output_buffer_sizes_sum;
  VectorStat link_input_buffer_sizes_sum;
  VectorStat link_output_buffer_sizes_avg;
  VectorStat link_input_buffer_sizes_avg;

  VectorStat vault_switch_queued_packets_sum;
  VectorStat vault_switch_queued_packets_avg;
  VectorStat vault_switch_packet_service_latency_avg;
  VectorStat vault_switch_queued_route_packets_sum;
  VectorStat vault_switch_queued_route_packets_avg;
  VectorStat vault_switch_route_packet_service_latency_avg;

  DistributionStat memory_access_latency_dis;
  ScalarStat max_memory_access_latency;
  ScalarStat min_memory_access_latency;

  DistributionStat memory_access_network_latency_dis;
  ScalarStat req_end_switch_latency_sum;
  ScalarStat req_end_switch_latency_avg;
  ScalarStat resp_end_switch_latency_sum;
  ScalarStat resp_end_switch_latency_avg;

  DistributionStat memory_access_remote_hop_dis;
  DistributionStat memory_access_hop_dis;

  VectorStat memory_access_num_by_hop;
  VectorStat req_end_switch_latency_by_hop_avg;
  VectorStat request_packet_network_latency_by_hop_avg;
  VectorStat request_packet_latency_by_hop_avg;
  VectorStat resp_end_switch_latency_by_hop_avg;
  VectorStat response_packet_network_latency_by_hop_avg;
  VectorStat response_packet_latency_by_hop_avg;
  VectorStat memory_access_network_latency_by_hop_avg;
  VectorStat memory_access_latency_by_hop_avg;

  ScalarStat num_ptws;
  ScalarStat total_pu_local_vault_ptw_requests;
  ScalarStat total_pu_local_memory_ptw_requests;
  ScalarStat total_pu_remote_memory_ptw_requests;

  DistributionStat ptw_memory_access_latency_dis;
  ScalarStat max_ptw_memory_access_latency;
  ScalarStat min_ptw_memory_access_latency;

  DistributionStat ptw_memory_access_network_latency_dis;
  ScalarStat ptw_req_end_switch_latency_sum;
  ScalarStat ptw_req_end_switch_latency_avg;
  ScalarStat ptw_resp_end_switch_latency_sum;
  ScalarStat ptw_resp_end_switch_latency_avg;

  DistributionStat ptw_memory_access_remote_hop_dis;
  DistributionStat ptw_memory_access_hop_dis;

public:
    long clk = 0;
    bool pim_mode_enabled = false;
    enum class Type {
        RoChBgBaCuVaCl, // XXX The specification doesn't define row/column addressing
        RoBgBaCuVaChCl,
        RoBgBaChCuVaCl,
        RoChBaBgCuVaCl,
        MAX,
    } type = Type::RoChBgBaCuVaCl;

    std::map<std::string, Type> name_to_type = {
      {"RoChBgBaCuVaCl", Type::RoChBgBaCuVaCl},
      {"RoBgBaCuVaChCl", Type::RoBgBaCuVaChCl},
      {"RoBgBaChCuVaCl", Type::RoBgBaChCuVaCl},
      {"RoChBaBgCuVaCl", Type::RoChBaBgCuVaCl}};

    vector<list<int>> tags_pools;

    vector<Controller<HMC>*> ctrls;
    vector<LogicLayer<HMC>*> logic_layers;
    HMC * spec;

    vector<int> addr_bits;
    const Config& configs;
    int tx_bits;
    int cub_bits;
    int vaults_per_cub;
    int vaults_per_cub_bits;
    int stacks;
    int stack_links;
    int groupIdx;

    vector<long> num_pu_local_vault_read_requests;
    vector<long> num_pu_local_vault_write_requests;
    vector<long> num_pu_local_memory_read_requests;
    vector<long> num_pu_local_memory_write_requests;
    vector<long> num_pu_remote_memory_read_requests;
    vector<long> num_pu_remote_memory_write_requests;

    vector<long> memory_access_latency_by_hop_sum;
    vector<long> request_packet_latency_by_hop_sum;
    vector<long> response_packet_latency_by_hop_sum;
    vector<long> request_packet_network_latency_by_hop_sum;
    vector<long> response_packet_network_latency_by_hop_sum;
    vector<long> memory_access_network_latency_by_hop_sum;
    vector<long> req_end_switch_latency_by_hop_sum;
    vector<long> resp_end_switch_latency_by_hop_sum;

    bool data_mem_network_no_latency;
    bool ptw_mem_network_no_latency;
    bool enable_quadrant;
    IdealMemNetWorkType mem_network_no_latency_type;
    // bool enable_multithread;
    // bool pu_multithread_sim;

    lock_t send_lock;
    lock_t receive_lock;

    MemoryTopology memTopology;

    Memory(const Config& configs, vector<Controller<HMC>*> ctrls)
        : configs(configs),
          ctrls(ctrls),
          spec(ctrls[0]->channel->spec),
          memTopology(MemTopoParser::getMemTopoFromXML(configs.get_topology_file())),
          addr_bits(int(HMC::Level::MAX))
    {
        // make sure 2^N channels/ranks
        // TODO support channel number that is not powers of 2
        int *sz = spec->org_entry.count;
        assert((sz[0] & (sz[0] - 1)) == 0);
        assert((sz[1] & (sz[1] - 1)) == 0);
        // validate size of one transaction
        int tx = (spec->prefetch_size * spec->channel_width / 8);
        tx_bits = calc_log2(tx);
        assert((1<<tx_bits) == tx);
        printf("tx_bits: %d\n", tx_bits);

        pim_mode_enabled = configs.pim_mode_enabled();
        capacity_per_stack = spec->channel_width / 8;
        groupIdx = 0;

        for (unsigned int lev = 0; lev < addr_bits.size(); lev++) {
          addr_bits[lev] = calc_log2(sz[lev]);
          capacity_per_stack *= sz[lev];
          printf("addr_bits[%d]=%d, sz[lev]=%d\n",lev,addr_bits[lev],sz[lev]);
        }
        printf("capacity_per_stack: %ld\n",capacity_per_stack);
        cub_bits = calc_log2(configs.get_stacks());
        printf("cub_bits: %d\n", cub_bits);
        vaults_per_cub_bits = calc_log2(sz[int(HMC::Level::Vault)]);
        vaults_per_cub = sz[int(HMC::Level::Vault)];
        max_address = capacity_per_stack * configs.get_stacks();

        addr_bits[int(HMC::Level::MAX) - 1] -= calc_log2(spec->prefetch_size);
        printf("addr_bits[%d]=%d\n",int(HMC::Level::MAX) - 1,addr_bits[int(HMC::Level::MAX) - 1]);

        // Initiating addressing
        if (configs.contains("addressing_type")) {
          assert(name_to_type.find(configs["addressing_type"]) != name_to_type.end());
          printf("configs[\"addressing_type\"] %s\n", configs["addressing_type"].c_str());
          type = name_to_type[configs["addressing_type"]];
        }

        // HMC
        // assert(spec->source_links > 0);
        // tags_pools.resize(spec->source_links);
        stacks = configs.get_int_value("stacks");
        stack_links = configs.get_int_value("stack_links");

        assert(memTopology.getTotalSourceModeLinkNum() == spec->source_links);
        assert(memTopology.linkspernode == stack_links);
        assert(memTopology.nodenum == stacks);

        tags_pools.resize(stacks*stack_links);
        for (auto & tags_pool : tags_pools) {
          for (int i = 0 ; i < spec->max_tags ; ++i) {
            tags_pool.push_back(i);
          }
        }
        data_mem_network_no_latency = configs.data_mem_network_no_latency();
        ptw_mem_network_no_latency = configs.ptw_mem_network_no_latency();
        mem_network_no_latency_type  = configs.get_mem_network_no_latency_type();
        enable_quadrant = configs.enable_quadrant();
        if(enable_quadrant) assert(vaults_per_cub % stack_links == 0);

        ramulator_futex_init(&send_lock);
        ramulator_futex_init(&receive_lock);
        
        // enable_multithread = configs.enable_multithread();
        // pu_multithread_sim = configs.enable_pu_multithread_sim();

        for (int i = 0 ; i < stacks ; ++i) {
            logic_layers.emplace_back(new LogicLayer<HMC>(configs, i, spec, ctrls,
              this, &memTopology,std::bind(&Memory<HMC>::receive_packets, this, std::placeholders::_1)));
        }

        icnt_wrapper_init(stacks, configs.get_mem_switch_network_config().c_str());
        icnt_create(stack_links, vaults_per_cub);
        icnt_init();

        //match link pairs
        connect_memnodes();
        
        // regStats
        dram_capacity
            .name("dram_capacity")
            .desc("Number of bytes in simulated DRAM")
            .precision(0)
            ;
        dram_capacity = max_address;

        num_dram_cycles
            .name("dram_cycles")
            .desc("Number of DRAM cycles simulated")
            .precision(0)
            ;

        num_incoming_requests
            .name("incoming_requests")
            .desc("Number of incoming requests to DRAM")
            .precision(0)
            ;

        num_read_requests
            .init(configs.get_core_num())
            .name("read_requests")
            .desc("Number of incoming read requests to DRAM")
            .precision(0)
            ;

        num_write_requests
            .init(configs.get_core_num())
            .name("write_requests")
            .desc("Number of incoming write requests to DRAM")
            .precision(0)
            ;

        incoming_requests_per_channel
            .init(sz[int(HMC::Level::Vault)]*configs.get_stacks())
            .name("incoming_requests_per_channel")
            .desc("Number of incoming requests to each DRAM channel")
            .precision(0)
            ;

        incoming_read_reqs_per_channel
            .init(sz[int(HMC::Level::Vault)]*configs.get_stacks())
            .name("incoming_read_reqs_per_channel")
            .desc("Number of incoming read requests to each DRAM channel")
            .precision(0)
            ;
        ramulator_active_cycles
            .name("ramulator_active_cycles")
            .desc("The total number of cycles that the DRAM part is active (serving R/W)")
            .precision(0)
            ;

        maximum_internal_bandwidth
            .name("maximum_internal_bandwidth")
            .desc("The theoretical maximum bandwidth (Bps) per cube")
            .precision(0)
            ;

        maximum_link_bandwidth
            .name("maximum_link_bandwidth")
            .desc("The theoretical maximum bandwidth (Bps) of all links per cube")
            .precision(0)
            ;

        read_bandwidth
            .name("read_bandwidth")
            .desc("Real total read bandwidth(Bps)")
            .precision(0)
            ;

        write_bandwidth
            .name("write_bandwidth")
            .desc("Real total write bandwidth(Bps)")
            .precision(0)
            ;
        read_latency_sum
            .name("read_latency_sum")
            .desc("The memory latency cycles (in memory time domain) sum for all read requests")
            .precision(0)
            ;
        read_latency_avg
            .name("read_latency_avg")
            .desc("The average memory latency cycles (in memory time domain) per request for all read requests")
            .precision(6)
            ;
        read_latency_ns_avg
            .name("read_latency_ns_avg")
            .desc("The average memory latency (ns) per request for all read requests in this channel")
            .precision(6)
            ;
        write_latency_sum
            .name("write_latency_sum")
            .desc("The memory latency cycles (in memory time domain) sum for all write requests")
            .precision(0)
            ;
        write_latency_avg
            .name("write_latency_avg")
            .desc("The average memory latency cycles (in memory time domain) per request for all write requests")
            .precision(6)
            ;
        write_latency_ns_avg
            .name("write_latency_ns_avg")
            .desc("The average memory latency (ns) per request for all write requests in this channel")
            .precision(6)
            ;
        queueing_latency_sum
            .name("queueing_latency_sum")
            .desc("The sum of time waiting in queue before first command issued")
            .precision(0)
            ;
        queueing_latency_avg
            .name("queueing_latency_avg")
            .desc("The average of time waiting in queue before first command issued")
            .precision(6)
            ;
        queueing_latency_ns_avg
            .name("queueing_latency_ns_avg")
            .desc("The average of time (ns) waiting in queue before first command issued")
            .precision(6)
            ;
        request_packet_latency_sum
            .name("request_packet_latency_sum")
            .desc("The memory latency cycles (in memory time domain) sum for all request packets transmission")
            .precision(0)
            ;
        request_packet_latency_avg
            .name("request_packet_latency_avg")
            .desc("The average memory latency cycles (in memory time domain) per request for all request packets transmission")
            .precision(6)
            ;
        request_packet_latency_ns_avg
            .name("request_packet_latency_ns_avg")
            .desc("The average memory latency (ns) per request for all request packets transmission")
            .precision(6)
            ;
        response_packet_latency_sum
            .name("response_packet_latency_sum")
            .desc("The memory latency cycles (in memory time domain) sum for all response packets transmission")
            .precision(0)
            ;
        response_packet_latency_avg
            .name("response_packet_latency_avg")
            .desc("The average memory latency cycles (in memory time domain) per response for all read/write response packets transmission")
            .precision(6)
            ;
        response_packet_latency_ns_avg
            .name("response_packet_latency_ns_avg")
            .desc("The average memory latency (ns) per response for all read/write response packets transmission")
            .precision(6)
            ;
        
        request_packet_network_latency_sum
            .name("request_packet_network_latency_sum")
            .desc("The memory network latency cycles (in memory time domain) sum for all request packets transmission")
            .precision(0)
            ;
        
        request_packet_network_latency_avg
            .name("request_packet_network_latency_avg")
            .desc("The memory network latency cycles per request (in memory time domain) for all request packets transmission")
            .precision(6)
            ;
        
        response_packet_network_latency_sum
            .name("response_packet_network_latency_sum")
            .desc("The memory network latency cycles (in memory time domain) sum for all response packets transmission")
            .precision(0)
            ;
        
        response_packet_network_latency_avg
            .name("response_packet_network_latency_avg")
            .desc("The memory network latency cycles per response (in memory time domain) for all response packets transmission")
            .precision(6)
            ;

        // shared by all Controller objects

        read_transaction_bytes
            .name("read_transaction_bytes")
            .desc("The total byte of read transaction")
            .precision(0)
            ;
        write_transaction_bytes
            .name("write_transaction_bytes")
            .desc("The total byte of write transaction")
            .precision(0)
            ;

        row_hits
            .name("row_hits")
            .desc("Number of row hits")
            .precision(0)
            ;
        row_misses
            .name("row_misses")
            .desc("Number of row misses")
            .precision(0)
            ;
        row_conflicts
            .name("row_conflicts")
            .desc("Number of row conflicts")
            .precision(0)
            ;

        read_row_hits
            .init(configs.get_core_num())
            .name("read_row_hits")
            .desc("Number of row hits for read requests")
            .precision(0)
            ;
        read_row_misses
            .init(configs.get_core_num())
            .name("read_row_misses")
            .desc("Number of row misses for read requests")
            .precision(0)
            ;
        read_row_conflicts
            .init(configs.get_core_num())
            .name("read_row_conflicts")
            .desc("Number of row conflicts for read requests")
            .precision(0)
            ;

        write_row_hits
            .init(configs.get_core_num())
            .name("write_row_hits")
            .desc("Number of row hits for write requests")
            .precision(0)
            ;
        write_row_misses
            .init(configs.get_core_num())
            .name("write_row_misses")
            .desc("Number of row misses for write requests")
            .precision(0)
            ;
        write_row_conflicts
            .init(configs.get_core_num())
            .name("write_row_conflicts")
            .desc("Number of row conflicts for write requests")
            .precision(0)
            ;

        req_queue_length_sum
            .name("req_queue_length_sum")
            .desc("Sum of read and write queue length per memory cycle.")
            .precision(0)
            ;
        req_queue_length_avg
            .name("req_queue_length_avg")
            .desc("Average of read and write queue length per memory cycle.")
            .precision(6)
            ;

        read_req_queue_length_sum
            .name("read_req_queue_length_sum")
            .desc("Read queue length sum per memory cycle.")
            .precision(0)
            ;
        read_req_queue_length_avg
            .name("read_req_queue_length_avg")
            .desc("Read queue length average per memory cycle.")
            .precision(6)
            ;

        write_req_queue_length_sum
            .name("write_req_queue_length_sum")
            .desc("Write queue length sum per memory cycle.")
            .precision(0)
            ;
        write_req_queue_length_avg
            .name("write_req_queue_length_avg")
            .desc("Write queue length average per memory cycle.")
            .precision(6)
            ;
        
        rmab_length_sum
            .name("rmab_length_sum")
            .desc("Remote memory access buffer length sum per memory cycle.")
            .precision(0)
            ;
        
        rmab_length_avg
            .name("rmab_length_avg")
            .desc("Remote memory access buffer length average per memory cycle.")
            .precision(6)
            ;

        record_read_hits
            .init(configs.get_core_num())
            .name("record_read_hits")
            .desc("record read hit count for this core when it reaches request limit or to the end")
            ;

        record_read_misses
            .init(configs.get_core_num())
            .name("record_read_misses")
            .desc("record_read_miss count for this core when it reaches request limit or to the end")
            ;

        record_read_conflicts
            .init(configs.get_core_num())
            .name("record_read_conflicts")
            .desc("record read conflict count for this core when it reaches request limit or to the end")
            ;

        record_write_hits
            .init(configs.get_core_num())
            .name("record_write_hits")
            .desc("record write hit count for this core when it reaches request limit or to the end")
            ;

        record_write_misses
            .init(configs.get_core_num())
            .name("record_write_misses")
            .desc("record write miss count for this core when it reaches request limit or to the end")
            ;

        record_write_conflicts
            .init(configs.get_core_num())
            .name("record_write_conflicts")
            .desc("record write conflict for this core when it reaches request limit or to the end")
            ;
        
        total_pu_local_vault_read_requests
            .name("total_pu_local_vault_read_requests")
            .desc("Number of incoming read requests from PUs to local vault")
            .precision(0)
            ;
        
        total_pu_local_vault_write_requests
            .name("total_pu_local_vault_write_requests")
            .desc("Number of incoming write requests from PUs to local vault")
            .precision(0)
            ;
        
        total_pu_local_memory_read_requests
            .name("total_pu_local_memory_read_requests")
            .desc("Number of incoming read requests from PUs to local memory")
            .precision(0)
            ;
        
        total_pu_local_memory_write_requests
            .name("total_pu_local_memory_write_requests")
            .desc("Number of incoming write requests from PUs to local memory")
            .precision(0)
            ;
        
        total_pu_remote_memory_read_requests
            .name("total_pu_remote_memory_read_requests")
            .desc("Number of incoming read requests from PUs to remote memory")
            .precision(0)
            ;
        
        total_pu_remote_memory_write_requests
            .name("total_pu_remote_memory_write_requests")
            .desc("Number of incoming write requests from PUs to remote memory")
            .precision(0)
            ;
        
        link_output_buffer_sizes_sum
            .init(stacks)
            .name("link_output_buffer_sizes_sum")
            .desc("record total link output buffer size of each memory per memory cycle")
            .precision(0)
            ;

        link_input_buffer_sizes_sum
            .init(stacks)
            .name("link_input_buffer_sizes_sum")
            .desc("record total link input buffer size of each memory per memory cycle")
            .precision(0)
            ;

        link_output_buffer_sizes_avg
            .init(stacks)
            .name("link_output_buffer_sizes_avg")
            .desc("record average link output buffer size per memory cycle")
            .precision(6)
            ;
        
        link_input_buffer_sizes_avg
            .init(stacks)
            .name("link_input_buffer_sizes_avg")
            .desc("record average link input buffer size per memory cycle")
            .precision(6)
            ;
        
        vault_switch_queued_packets_sum
            .init(stacks)
            .name("vault_switch_queued_packets_sum")
            .desc("record total packets queued in vault switch sum per memory cycle")
            .precision(0)
            ;

        vault_switch_queued_packets_avg
            .init(stacks)
            .name("vault_switch_queued_packets_avg")
            .desc("record average packets queued in vault switch per memory cycle")
            .precision(6)
            ;
        
        vault_switch_queued_route_packets_sum
            .init(stacks)
            .name("vault_switch_queued_route_packets_sum")
            .desc("record total route packets queued in vault switch sum per memory cycle")
            .precision(0)
            ;

        vault_switch_queued_route_packets_avg
            .init(stacks)
            .name("vault_switch_queued_route_packets_avg")
            .desc("record average route packets queued in vault switch per memory cycle")
            .precision(6)
            ;
        
        vault_switch_packet_service_latency_avg
            .init(stacks)
            .name("vault_switch_packet_service_latency_avg")
            .desc("record average packet service latency in vault switch")
            .precision(6)
            ;
        
        vault_switch_route_packet_service_latency_avg
            .init(stacks)
            .name("vault_switch_route_packet_service_latency_avg")
            .desc("record average route packet service latency in vault switch")
            .precision(6)
            ;
        
        memory_access_latency_dis.init(0,1999,100)
            .name("memory_access_latency_distribution")
            .desc("Distribution of memory access latency (memory cycles)")
            .precision(0);
	
        max_memory_access_latency
            .name("max_memory_access_latency")
            .desc("Max memory access latency (memory cycles)")
            .precision(0);
        
        min_memory_access_latency
            .name("min_memory_access_latency")
            .desc("Min memory access latency (memory cycles)")
            .precision(0);
        
        memory_access_network_latency_dis.init(0,1999,100)
            .name("memory_access_network_latency_distribution")
            .desc("Distribution of memory access network latency (memory cycles)")
            .precision(0);
        
        req_end_switch_latency_sum
            .name("req_end_switch_latency_sum")
            .desc("The end vault switch latency of request packet sum.")
            .precision(0)
            ;

        req_end_switch_latency_avg
            .name("req_end_switch_latency_avg")
            .desc("The end vault switch latency of request packet avg.")
            .precision(6)
            ;

        resp_end_switch_latency_sum
            .name("resp_end_switch_latency_sum")
            .desc("The end vault switch latency of response packet sum.")
            .precision(0)
            ;

        resp_end_switch_latency_avg
            .name("resp_end_switch_latency_avg")
            .desc("The end vault switch latency of response packet avg.")
            .precision(6)
            ;

        memory_access_remote_hop_dis.init(0,stacks*2,1)
            .name("memory_access_remote_hop_distribution")
            .desc("Distribution of memory access remote hops")
            .precision(2);

        memory_access_hop_dis.init(0,stacks*2,1)
            .name("memory_access_hop_distribution")
            .desc("Distribution of memory access hops")
            .precision(2);

        // by hop        
        memory_access_num_by_hop
            .init(stacks*2+1)
            .name("memory_access_num_by_hop")
            .desc("Record the number of total memory access by hop")
            .precision(0)
            ;

        memory_access_latency_by_hop_avg
            .init(stacks*2+1)
            .name("memory_access_latency_by_hop_avg")
            .desc("Record memory access latency avg by hop")
            .precision(6)
            ;

        request_packet_latency_by_hop_avg
            .init(stacks*2+1)
            .name("request_packet_latency_by_hop_avg")
            .desc("The average memory latency cycles (in memory time domain) by hop per request for all request packets transmission")
            .precision(6)
            ;

        response_packet_latency_by_hop_avg
            .init(stacks*2+1)
            .name("response_packet_latency_by_hop_avg")
            .desc("The average memory latency cycles (in memory time domain) by hop per response for all read/write response packets transmission")
            .precision(6)
            ;
        
        request_packet_network_latency_by_hop_avg
            .init(stacks*2+1)
            .name("request_packet_network_latency_by_hop_avg")
            .desc("The memory network latency cycles per request (in memory time domain) by hop for all request packets transmission")
            .precision(6)
            ;
        
        response_packet_network_latency_by_hop_avg
            .init(stacks*2+1)
            .name("response_packet_network_latency_by_hop_avg")
            .desc("The memory network latency cycles per response (in memory time domain) by hop for all response packets transmission")
            .precision(6)
            ;
        ///////
        memory_access_network_latency_by_hop_avg
            .init(stacks*2+1)
            .name("memory_access_network_latency_by_hop_avg")
            .desc("Record memory access network latency avg by hop")
            .precision(6)
            ;

        req_end_switch_latency_by_hop_avg
            .init(stacks*2+1)
            .name("req_end_switch_latency_by_hop_avg")
            .desc("The end vault switch latency of request packet avg by hop")
            .precision(6)
            ;

        resp_end_switch_latency_by_hop_avg
            .init(stacks*2+1)
            .name("resp_end_switch_latency_by_hop_avg")
            .desc("The end vault switch latency of response packet avg by hop")
            .precision(6)
            ;
        //PTW
        num_ptws
            .name("num_ptws")
            .desc("Number of PTWs")
            .precision(0)
            ;
        total_pu_local_vault_ptw_requests
            .name("total_pu_local_vault_ptw_requests")
            .desc("Number of incoming ptw requests from PUs to local vault")
            .precision(0)
            ;

        total_pu_local_memory_ptw_requests
            .name("total_pu_local_memory_ptw_requests")
            .desc("Number of incoming ptw requests from PUs to local memory")
            .precision(0)
            ;

        total_pu_remote_memory_ptw_requests
            .name("total_pu_remote_memory_ptw_requests")
            .desc("Number of incoming ptw requests from PUs to local vault")
            .precision(0)
            ;
        
        ptw_memory_access_latency_dis.init(0,1999,100)
            .name("ptw_memory_access_latency_distribution")
            .desc("Distribution of PTW memory access latency (memory cycles)")
            .precision(0);
	
        max_ptw_memory_access_latency
            .name("max_ptw_memory_access_latency")
            .desc("Max PTW memory access latency (memory cycles)")
            .precision(0);
        
        min_ptw_memory_access_latency
            .name("min_ptw_memory_access_latency")
            .desc("Min PTW memory access latency (memory cycles)")
            .precision(0);
        
        ptw_memory_access_network_latency_dis.init(0,1999,100)
            .name("ptw_memory_access_network_latency_distribution")
            .desc("Distribution of PTW memory access network latency (memory cycles)")
            .precision(0);
        
        ptw_req_end_switch_latency_sum
            .name("ptw_req_end_switch_latency_sum")
            .desc("The end vault switch latency of PTW request packet sum.")
            .precision(0)
            ;

        ptw_req_end_switch_latency_avg
            .name("ptw_req_end_switch_latency_avg")
            .desc("The end vault switch latency of PTW request packet avg.")
            .precision(6)
            ;

        ptw_resp_end_switch_latency_sum
            .name("ptw_resp_end_switch_latency_sum")
            .desc("The end vault switch latency of PTW response packet sum.")
            .precision(0)
            ;

        ptw_resp_end_switch_latency_avg
            .name("ptw_resp_end_switch_latency_avg")
            .desc("The end vault switch latency of PTW response packet avg.")
            .precision(6)
            ;

        ptw_memory_access_remote_hop_dis.init(0,stacks*2,1)
            .name("ptw_memory_access_remote_hop_distribution")
            .desc("Distribution of PTW memory access remote hops")
            .precision(2);

        ptw_memory_access_hop_dis.init(0,stacks*2,1)
            .name("ptw_memory_access_hop_distribution")
            .desc("Distribution of PTW memory access hops")
            .precision(2);

        max_memory_access_latency = 0;
        min_memory_access_latency = 0x7FFFFFFF;
        max_ptw_memory_access_latency = 0;
        min_ptw_memory_access_latency = 0x7FFFFFFF;

        for (auto ctrl : ctrls) {
          ctrl->read_transaction_bytes = &read_transaction_bytes;
          ctrl->write_transaction_bytes = &write_transaction_bytes;

          ctrl->row_hits = &row_hits;
          ctrl->row_misses = &row_misses;
          ctrl->row_conflicts = &row_conflicts;
          ctrl->read_row_hits = &read_row_hits;
          ctrl->read_row_misses = &read_row_misses;
          ctrl->read_row_conflicts = &read_row_conflicts;
          ctrl->write_row_hits = &write_row_hits;
          ctrl->write_row_misses = &write_row_misses;
          ctrl->write_row_conflicts = &write_row_conflicts;

          ctrl->queueing_latency_sum = &queueing_latency_sum;

          ctrl->req_queue_length_sum = &req_queue_length_sum;
          ctrl->read_req_queue_length_sum = &read_req_queue_length_sum;
          ctrl->write_req_queue_length_sum = &write_req_queue_length_sum;
          ctrl->rmab_length_sum = &rmab_length_sum;

          ctrl->record_read_hits = &record_read_hits;
          ctrl->record_read_misses = &record_read_misses;
          ctrl->record_read_conflicts = &record_read_conflicts;
          ctrl->record_write_hits = &record_write_hits;
          ctrl->record_write_misses = &record_write_misses;
          ctrl->record_write_conflicts = &record_write_conflicts;
          ctrl->set_pu_callback(std::bind(&Memory<HMC>::pu_receive_requests, this, placeholders::_1));
        }

        for (auto logic_layer : logic_layers) {
            logic_layer->link_output_buffer_sizes_sum = &link_output_buffer_sizes_sum;
            logic_layer->link_input_buffer_sizes_sum = &link_input_buffer_sizes_sum;
            logic_layer->vault_switch_queued_packets_sum = &vault_switch_queued_packets_sum;
            logic_layer->vault_switch_queued_route_packets_sum = &vault_switch_queued_route_packets_sum;
        }

        num_pu_local_vault_read_requests.resize(configs.get_pu_core_num(),0);
        num_pu_local_vault_write_requests.resize(configs.get_pu_core_num(),0);
        num_pu_local_memory_read_requests.resize(configs.get_pu_core_num(),0);
        num_pu_local_memory_write_requests.resize(configs.get_pu_core_num(),0);
        num_pu_remote_memory_read_requests.resize(configs.get_pu_core_num(),0);
        num_pu_remote_memory_write_requests.resize(configs.get_pu_core_num(),0);

        memory_access_latency_by_hop_sum.resize(stacks*2,0);
        request_packet_latency_by_hop_sum.resize(stacks*2,0);
        response_packet_latency_by_hop_sum.resize(stacks*2,0);
        request_packet_network_latency_by_hop_sum.resize(stacks*2,0);
        response_packet_network_latency_by_hop_sum.resize(stacks*2,0);
        memory_access_network_latency_by_hop_sum.resize(stacks*2,0);
        req_end_switch_latency_by_hop_sum.resize(stacks*2,0);
        resp_end_switch_latency_by_hop_sum.resize(stacks*2,0);
    }

    ~Memory()
    {
        for (auto ctrl: ctrls)
            delete ctrl;
        delete spec;
    }

    double clk_ns()
    {
        return spec->speed_entry.tCK;
    }

    void record_core(int coreid) {
      // TODO record multicore statistics
    }

    void tick()
    {
        clk++;
        num_dram_cycles++;
        bool is_active = false;
        is_active = is_active || icnt_busy();

        for (auto logic_layer : logic_layers){
            logic_layer->tick();
        }

        for (auto ctrl : ctrls){
            is_active = is_active || ctrl->is_active();
            ctrl->tick();
        }
        if (is_active){
            ramulator_active_cycles++;
        }
        //switch step
        icnt_transfer();
    }

    int assign_tag(int slid) {
      if (tags_pools[slid].empty()) {
        return -1;
      } else {
        int tag = tags_pools[slid].front();
        tags_pools[slid].pop_front();
        return tag;
      }
    }

    inline int get_cpu_slid(int target_cub, long addr){
        return memTopology.getCPURouteLink(target_cub);
    }

    Packet form_request_packet(const Request& req) {
      // All packets sent from host controller are Request packets
      long addr = req.addr;
    //   int cub = addr / capacity_per_stack;
      int cub = req.addr_vec[int(HMC::Level::Vault)] >> vaults_per_cub_bits;
      long adrs = addr;
      int max_block_bits = spec->maxblock_entry.flit_num_bits;
      clear_lower_bits(addr, max_block_bits);
    //   int slid = addr % spec->source_links;
      int slid = get_cpu_slid(cub, addr);
      int tag = assign_tag(slid); // may return -1 when no available tag // TODO recycle tags when request callback
      int lng = (req.type == Request::Type::READ) ?
                                                1 : 1 +  spec->payload_flits;
      Packet::Command cmd;
      switch (int(req.type)) {
        case int(Request::Type::READ):
          cmd = read_cmd_map[lng];
        break;
        case int(Request::Type::WRITE):
          cmd = write_cmd_map[lng];
        break;
        default: assert(false);
      }
      Packet packet(Packet::Type::REQUEST, cub, adrs, tag, lng, slid, cmd);
      packet.req = req;
      debug_hmc("cub: %d", cub);
      debug_hmc("adrs: %lx", adrs);
      debug_hmc("slid: %d", slid);
      debug_hmc("lng: %d", lng);
      debug_hmc("cmd: %d", int(cmd));
      // DEBUG:
      assert(packet.header.CUB.valid());
      assert(packet.header.ADRS.valid());
      assert(packet.header.TAG.valid()); // -1 also considered valid here...
      assert(packet.tail.SLID.valid());
      assert(packet.header.CMD.valid());
      return packet;
    }

    Packet form_request_packet(const Request& req, int src_cub) {
      // Request sent from PIM processor
      long addr = req.addr;
      int cub = req.addr_vec[int(HMC::Level::Vault)] >> vaults_per_cub_bits;
      long adrs = addr;
    //   int max_block_bits = spec->maxblock_entry.flit_num_bits;
    //   clear_lower_bits(addr, max_block_bits);
      int slid = get_route_link(src_cub,cub);// may return -1 when src_cub == cub
      int tag = -1;
      if(slid != -1)
        tag = assign_tag(slid); // may return -1 when no available tag // TODO recycle tags when request callback
    //   int tag = assign_tag(slid); // may return -1 when no available tag // TODO recycle tags when request callback
      int lng = (req.type == Request::Type::READ) ?
                                                1 : 1 +  spec->payload_flits;
      Packet::Command cmd;
      switch (int(req.type)) {
        case int(Request::Type::READ):
          cmd = read_cmd_map[lng];
        break;
        case int(Request::Type::WRITE):
          cmd = write_cmd_map[lng];
        break;
        default: assert(false);
      }
      Packet packet(Packet::Type::REQUEST, cub, adrs, tag, lng, slid, cmd);
      packet.req = req;
      debug_hmc("cub: %d", cub);
      debug_hmc("adrs: %lx", adrs);
      debug_hmc("slid: %d", slid);
      debug_hmc("lng: %d", lng);
      debug_hmc("cmd: %d", int(cmd));
      // DEBUG:
      assert(packet.header.CUB.valid());
      assert(packet.header.ADRS.valid());
      assert(packet.header.TAG.valid()); // -1 also considered valid here...
      assert(packet.tail.SLID.valid());
      assert(packet.header.CMD.valid());
      return packet;
    }

    void receive_packets(Packet packet) {
        debug_hmc("receive response packets@host controller");
        if (packet.flow_control) {
            return;
        }
        lock_receive();
        assert(packet.type == Packet::Type::RESPONSE);
        tags_pools[packet.header.SLID.value].push_back(packet.header.TAG.value);
        Request& req = packet.req;
        req.depart_hmc = clk;
        stat_mem_access(req);
        req.callback(req);
        unlock_receive();
    }

    void pu_receive_requests(Request& req){
        lock_receive();
        req.depart_hmc = clk;
        stat_mem_access(req);
        req.callback(req);
        unlock_receive();
    }

    inline void stat_mem_access(Request& req){
        long hmc_dur = req.depart_hmc - req.arrive_hmc;
        if (req.type == Request::Type::READ) {
            read_latency_sum += hmc_dur;
            debug_hmc("read_latency: %ld", hmc_dur);
        }else{
            assert(req.type == Request::Type::WRITE);
            write_latency_sum += hmc_dur;
            debug_hmc("write_latency: %ld", hmc_dur);
        }
        memory_access_hop_dis.sample(req.hops);
        if(req.hops >= 2){
            memory_access_remote_hop_dis.sample(req.hops - 2);
        }else{
            memory_access_remote_hop_dis.sample(0);
        }

        memory_access_latency_dis.sample(hmc_dur);
        if(max_memory_access_latency.value() < hmc_dur)
            max_memory_access_latency = hmc_dur;
        if(min_memory_access_latency.value() > hmc_dur)
            min_memory_access_latency = hmc_dur;
        
        memory_access_num_by_hop[req.hops]++;
        memory_access_latency_by_hop_sum[req.hops] += hmc_dur;
        
        long req_first_switch_lat = req.network[int(Request::NetworkStage::REQ_DEPART_FIRST_SWITCH)] -
                req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)];
        long req_last_switch_lat = req.network[int(Request::NetworkStage::REQ_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::REQ_ARRIVE_LAST_SWITCH)];
        assert(req_first_switch_lat >= 0 && req_last_switch_lat >= 0);
        long req_network_lat = req.network[int(Request::NetworkStage::REQ_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)];
        
        long resp_first_switch_lat = req.network[int(Request::NetworkStage::RESP_DEPART_FIRST_SWITCH)] -
                req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)];
        long resp_last_switch_lat = req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)];
        assert(resp_first_switch_lat >= 0 && resp_last_switch_lat >= 0);
        long resp_network_lat = req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)];

        assert(req_network_lat >= 0 && resp_network_lat >= 0);

        if(req.req_hops == 1){
            req_end_switch_latency_sum += req_first_switch_lat;
            resp_end_switch_latency_sum += resp_first_switch_lat;
            req_end_switch_latency_by_hop_sum[req.hops] += req_first_switch_lat;
            resp_end_switch_latency_by_hop_sum[req.hops] += resp_first_switch_lat;
        }else{
            req_end_switch_latency_sum += (req_first_switch_lat+req_last_switch_lat);
            resp_end_switch_latency_sum += (resp_first_switch_lat+resp_last_switch_lat);
            req_end_switch_latency_by_hop_sum[req.hops] += (req_first_switch_lat+req_last_switch_lat);
            resp_end_switch_latency_by_hop_sum[req.hops] += (resp_first_switch_lat+resp_last_switch_lat);
        }
        memory_access_network_latency_dis.sample(req_network_lat + resp_network_lat);
        memory_access_network_latency_by_hop_sum[req.hops] += req_network_lat + resp_network_lat;

        request_packet_network_latency_sum += req_network_lat;
        response_packet_network_latency_sum += resp_network_lat;
        request_packet_network_latency_by_hop_sum[req.hops] += req_network_lat;
        response_packet_network_latency_by_hop_sum[req.hops] += resp_network_lat;

        request_packet_latency_sum += req.arrive - req.arrive_hmc;
        request_packet_latency_by_hop_sum[req.hops] += req.arrive - req.arrive_hmc;
        debug_hmc("request_packet_latency: %ld", req.arrive - req.arrive_hmc);
        response_packet_latency_sum += req.depart_hmc - req.depart;
        response_packet_latency_by_hop_sum[req.hops] += req.depart_hmc - req.depart;
        // printf("response_packet_latency: %ld, %ld\n", req.depart_hmc, req.depart);
        debug_hmc("response_packet_latency: %ld", req.depart_hmc - req.depart);

        if(req.is_ptw)
            stat_ptw_access(req);
    }

    inline void stat_ptw_access(Request& req){
        long hmc_dur = req.depart_hmc - req.arrive_hmc;
        ptw_memory_access_hop_dis.sample(req.hops);
        if(req.hops >= 2){
            ptw_memory_access_remote_hop_dis.sample(req.hops - 2);
        }else{
            ptw_memory_access_remote_hop_dis.sample(0);
        }

        ptw_memory_access_latency_dis.sample(hmc_dur);
        if(max_ptw_memory_access_latency.value() < hmc_dur)
            max_ptw_memory_access_latency = hmc_dur;
        if(min_ptw_memory_access_latency.value() > hmc_dur)
            min_ptw_memory_access_latency = hmc_dur;
        
        long req_first_switch_lat = req.network[int(Request::NetworkStage::REQ_DEPART_FIRST_SWITCH)] -
                req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)];
        long req_last_switch_lat = req.network[int(Request::NetworkStage::REQ_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::REQ_ARRIVE_LAST_SWITCH)];
        assert(req_first_switch_lat >= 0 && req_last_switch_lat >= 0);
        long req_network_lat = req.network[int(Request::NetworkStage::REQ_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)];
        
        long resp_first_switch_lat = req.network[int(Request::NetworkStage::RESP_DEPART_FIRST_SWITCH)] -
                req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)];
        long resp_last_switch_lat = req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)];
        assert(resp_first_switch_lat >= 0 && resp_last_switch_lat >= 0);
        long resp_network_lat = req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] -
                req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)];

        assert(req_network_lat >= 0 && resp_network_lat >= 0);

        if(req.req_hops == 1){
            ptw_req_end_switch_latency_sum += req_first_switch_lat;
            ptw_resp_end_switch_latency_sum += resp_first_switch_lat;
        }else{
            ptw_req_end_switch_latency_sum += (req_first_switch_lat+req_last_switch_lat);
            ptw_resp_end_switch_latency_sum += (resp_first_switch_lat+resp_last_switch_lat);
        }
        ptw_memory_access_network_latency_dis.sample(req_network_lat + resp_network_lat);
    }

    int memAccsssPosition(int coreid, long req_addr, bool ideal_memnet){
        // only for PIM access
        // 0:local vault, 1:local stack, 2: remote stack
        int position = -1;
        vector<int> addr_vec(addr_bits.size());
        long addr = req_addr;
        address_mapping(addr,addr_vec);
        int target_vault_id = addr_vec[int(HMC::Level::Vault)];
        if (target_vault_id == coreid)
        {
            position = 0;
        }
        else if (ideal_memnet)
        {
            if (mem_network_no_latency_type == IDEAL_MEM_NETWORK_LOCALSTACK)
                position = 1;
            else
                position = 0;
        }
        else
        {
            int cur_cub = coreid / vaults_per_cub;
            int target_cub = target_vault_id / vaults_per_cub;
            if (target_cub == cur_cub)
            {
                //local memory
                position = 1;
            }
            else
            {
                //remote cube
                position = 2;
            }
        }

        return position;
    }

    int estimateMemHops(int coreid, long req_addr, bool is_pim, bool ideal_memnet){
        vector<int> addr_vec(addr_bits.size());
        int hops;
        long addr = req_addr;
        address_mapping(addr,addr_vec);
        int target_vault_id = addr_vec[int(HMC::Level::Vault)];
        if(is_pim){
            if(target_vault_id == coreid){
                hops = 0;
            }else if (ideal_memnet){
                if(mem_network_no_latency_type == IDEAL_MEM_NETWORK_LOCALSTACK)
                    hops = 2;
                else 
                    hops = 0;
            }else{
                int cur_cub = coreid / vaults_per_cub;
                int target_cub = target_vault_id / vaults_per_cub;
                if (target_cub == cur_cub){
                    //local memory
                    hops = 2;
                }else{
                    //remote cube
                    if(enable_quadrant){
                        auto route_link = logic_layers[cur_cub]->get_link_by_num(get_route_link(cur_cub, target_cub));
                        if(route_link->belong_local_quadrant(coreid)){
                            hops = 0;
                        }else{
                            hops = 1;
                        }
                    }else{
                        hops = 1;
                    }
                    while(cur_cub != target_cub){
                        auto  route_link = logic_layers[cur_cub]->get_link_by_num(get_route_link(cur_cub, target_cub));
                        cur_cub = route_link->other_side_link->cub;
                        if(cur_cub == target_cub){
                            if(!route_link->other_side_link->belong_local_quadrant(target_vault_id))
                                hops++;
                        }else{
                            hops++;
                        }
                    }
                    hops = hops*2;
                }
            }
        }else{
            if (ideal_memnet){
                if(mem_network_no_latency_type == IDEAL_MEM_NETWORK_LOCALSTACK)
                    hops = 2;
                else 
                    hops = 0;
            }else{
                int target_cub = addr_vec[int(HMC::Level::Vault)] >> vaults_per_cub_bits;
                long adrs = req_addr;
                int max_block_bits = spec->maxblock_entry.flit_num_bits;
                clear_lower_bits(adrs, max_block_bits);
                int slid = get_cpu_slid(target_cub, adrs);
                int slid_cub = slid / stack_links;
                if(target_cub == slid_cub){
                    if(enable_quadrant){
                        auto slid_link = logic_layers[slid_cub]->get_link_by_num(slid);
                        if(slid_link->belong_local_quadrant(target_vault_id)){
                            hops = 0;
                        }else{
                            hops = 2;
                        }
                    }else{
                        hops = 2;
                    }
                }else{
                    hops = 1;
                    int cur_cub = slid_cub;
                    while(cur_cub != target_cub){
                        auto  route_link = logic_layers[cur_cub]->get_link_by_num(get_route_link(cur_cub, target_cub));
                        cur_cub = route_link->other_side_link->cub;
                        if(cur_cub == target_cub){
                            if(!route_link->other_side_link->belong_local_quadrant(target_vault_id))
                                hops++;
                        }else{
                            hops++;
                        }
                    }
                    hops = hops*2;
                }
            }
        }

        return hops;
    }

    int get_hmc_stacks(){ return stacks; }
    void set_group_idx(int idx){ groupIdx = idx; }

    inline void lock_receive(){
        // if(pu_multithread_sim) 
        ramulator_futex_lock(&receive_lock);
    }
    inline void unlock_receive(){
        // if(pu_multithread_sim) 
        ramulator_futex_unlock(&receive_lock);
    }

    inline void lock_send(){
        ramulator_futex_lock(&send_lock);
    }
    inline void unlock_send(){
        ramulator_futex_unlock(&send_lock);
    }

    inline void address_mapping(long addr, vector<int> &addr_vec){
        // Each transaction size is 2^tx_bits, so first clear the lowest tx_bits bits
        clear_lower_bits(addr, tx_bits);
        switch(int(type)) {
          case int(Type::RoChBgBaCuVaCl): {
            int max_block_col_bits =  spec->maxblock_entry.flit_num_bits - tx_bits;
            addr_vec[int(HMC::Level::Column)] = slice_lower_bits(addr, max_block_col_bits);
            addr_vec[int(HMC::Level::Vault)]  = slice_lower_bits(addr, addr_bits[int(HMC::Level::Vault)] + cub_bits);
            addr_vec[int(HMC::Level::Bank)]   = slice_lower_bits(addr, addr_bits[int(HMC::Level::Bank)]);
            addr_vec[int(HMC::Level::BankGroup)] = slice_lower_bits(addr, addr_bits[int(HMC::Level::BankGroup)]);
            int column_MSB_bits = slice_lower_bits(addr, addr_bits[int(HMC::Level::Column)] - max_block_col_bits);
            addr_vec[int(HMC::Level::Column)] = addr_vec[int(HMC::Level::Column)] | (column_MSB_bits << max_block_col_bits);
            addr_vec[int(HMC::Level::Row)] = slice_lower_bits(addr, addr_bits[int(HMC::Level::Row)]);
          }
          break;
          case int(Type::RoBgBaCuVaChCl): {
            int max_block_col_bits =  spec->maxblock_entry.flit_num_bits - tx_bits;
            addr_vec[int(HMC::Level::Column)] = slice_lower_bits(addr, max_block_col_bits);
            int column_MSB_bits = slice_lower_bits(addr, addr_bits[int(HMC::Level::Column)] - max_block_col_bits);
            addr_vec[int(HMC::Level::Column)] = addr_vec[int(HMC::Level::Column)] | (column_MSB_bits << max_block_col_bits);
            addr_vec[int(HMC::Level::Vault)]  = slice_lower_bits(addr, addr_bits[int(HMC::Level::Vault)] + cub_bits);
            addr_vec[int(HMC::Level::Bank)]   = slice_lower_bits(addr, addr_bits[int(HMC::Level::Bank)]);
            addr_vec[int(HMC::Level::BankGroup)] = slice_lower_bits(addr, addr_bits[int(HMC::Level::BankGroup)]);
            addr_vec[int(HMC::Level::Row)] = slice_lower_bits(addr, addr_bits[int(HMC::Level::Row)]);
          }
          break;
          case int(Type::RoBgBaChCuVaCl): {
            int max_block_col_bits =
                spec->maxblock_entry.flit_num_bits - tx_bits;
            addr_vec[int(HMC::Level::Column)] =
                slice_lower_bits(addr, max_block_col_bits);
            addr_vec[int(HMC::Level::Vault)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::Vault)] + cub_bits);
            int column_MSB_bits =
              slice_lower_bits(
                  addr, addr_bits[int(HMC::Level::Column)] - max_block_col_bits);
            addr_vec[int(HMC::Level::Column)] =
              addr_vec[int(HMC::Level::Column)] | (column_MSB_bits << max_block_col_bits);
            addr_vec[int(HMC::Level::Bank)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::Bank)]);
            addr_vec[int(HMC::Level::BankGroup)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::BankGroup)]);
            addr_vec[int(HMC::Level::Row)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::Row)]);
          }
          break;
          case int(Type::RoChBaBgCuVaCl): {
            int max_block_col_bits =
                spec->maxblock_entry.flit_num_bits - tx_bits;
            addr_vec[int(HMC::Level::Column)] =
                slice_lower_bits(addr, max_block_col_bits);
            addr_vec[int(HMC::Level::Vault)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::Vault)] + cub_bits);
            addr_vec[int(HMC::Level::BankGroup)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::BankGroup)]);
            addr_vec[int(HMC::Level::Bank)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::Bank)]);
            int column_MSB_bits =
              slice_lower_bits(
                  addr, addr_bits[int(HMC::Level::Column)] - max_block_col_bits);
            addr_vec[int(HMC::Level::Column)] =
              addr_vec[int(HMC::Level::Column)] | (column_MSB_bits << max_block_col_bits);
            addr_vec[int(HMC::Level::Row)] =
                slice_lower_bits(addr, addr_bits[int(HMC::Level::Row)]);
          }
          break;
          default:
              assert(false);
        }
    }
    int get_target_vault(long addr){
        vector<int> addr_vec(addr_bits.size());
        address_mapping(addr,addr_vec);
        int target_vault_id = addr_vec[int(HMC::Level::Vault)];
        return target_vault_id;
    }
    int get_target_stack(long addr){
        vector<int> addr_vec(addr_bits.size());
        address_mapping(addr,addr_vec);
        int target_vault_id = addr_vec[int(HMC::Level::Vault)];
        return target_vault_id / vaults_per_cub;
    }

    bool send(Request req)
    {
        lock_send();
        debug_hmc("receive request packets@host controller");
        req.initial_addr = req.addr;
        req.addr_vec.resize(addr_bits.size());

        address_mapping(req.addr,req.addr_vec);

        if(req.reqid == -1)
            req.reqid = num_incoming_requests.value();

        req.arrive_hmc = clk;
        int target_vault_id = req.addr_vec[int(HMC::Level::Vault)];
        if(req.is_pim_inst){
            if(req.ideal_memnet && mem_network_no_latency_type == IDEAL_MEM_NETWORK_LOCALSTACK){
                int ideal_puid = (target_vault_id/vaults_per_cub)*vaults_per_cub + (req.pu_id % vaults_per_cub);
                req.pu_id = ideal_puid;
            }
            int puid = req.pu_id;
            int coreid = configs.get_cpu_core_num() + puid;

            Packet packet = form_request_packet(req, puid >> vaults_per_cub_bits);
            
            bool local_vault = false;
            bool local_memory = false;
            bool remote_memory = false;
            if(target_vault_id == puid){
                if(!(ctrls[puid]->receive(packet))){
                    unlock_send();
                    return false;
                }
                local_vault = true;
            }else {
                if(req.ideal_memnet && mem_network_no_latency_type == IDEAL_MEM_NETWORK_LOCALVAULT){
                    if (!(ctrls[target_vault_id]->receive(packet))){
                        unlock_send();
                        return false;
                    }
                    local_vault = true;
                }else {
                    if(!ctrls[puid]->receive_remote_request(packet)){
                        unlock_send();
                        return false;
                    }
                    if(target_vault_id / vaults_per_cub ==  puid / vaults_per_cub)
                        local_memory = true;
                    else
                        remote_memory = true;
                }
            }
            
            if (req.type == Request::Type::READ) {
                ++num_read_requests[coreid];
                ++incoming_read_reqs_per_channel[target_vault_id];
                if(local_vault){
                    num_pu_local_vault_read_requests[puid]++;
                    num_pu_local_memory_read_requests[puid]++;
                    if(req.is_ptw){
                        total_pu_local_vault_ptw_requests++;
                        num_ptws++;
                    }
                }else if(local_memory){
                    num_pu_local_memory_read_requests[puid]++;
                    if(req.is_ptw){
                        total_pu_local_memory_ptw_requests++;
                        num_ptws++;
                    }
                }else{
                    num_pu_remote_memory_read_requests[puid]++;
                    if(req.is_ptw){
                        total_pu_remote_memory_ptw_requests++;
                        num_ptws++;
                    }
                }
            }else if (req.type == Request::Type::WRITE) {
                ++num_write_requests[coreid];
                if(local_vault){
                    num_pu_local_vault_write_requests[puid]++;
                    num_pu_local_memory_write_requests[puid]++;
                }else if(local_memory){
                    num_pu_local_memory_write_requests[puid]++;
                }else{
                    num_pu_remote_memory_write_requests[puid]++;
                }
            }
            ++incoming_requests_per_channel[target_vault_id];
            ++num_incoming_requests;
            unlock_send();
            return true;
        }else{
            int coreid = req.coreid;
            Packet packet = form_request_packet(req);

            if (packet.header.TAG.value == -1) {
              debug_hmc("tag for link %d not available", packet.tail.SLID.value);
              unlock_send();
              return false;
            }

            int slid_cub = packet.tail.SLID.value / stack_links;
            Link<HMC> *link =
                logic_layers[slid_cub]->links_map[packet.tail.SLID.value].get();

            if (packet.total_flits <= link->slave.available_space()){
                //   printf("sending request %d, addr %ld to link %d, target cub %d\n",req.reqid,req.initial_addr,link->id,packet.header.CUB.value);
                link->slave.receive(packet);
                if (req.type == Request::Type::READ){
                    ++num_read_requests[coreid];
                    ++incoming_read_reqs_per_channel[target_vault_id];
                    if(req.is_ptw){
                        num_ptws++;
                    }
                }else if (req.type == Request::Type::WRITE){
                    ++num_write_requests[coreid];
                }
                ++incoming_requests_per_channel[target_vault_id];
                ++num_incoming_requests;
                unlock_send();
                return true;
            } else {
                unlock_send();
                return false;
            }
        }
     }

    int pending_requests()
    {
        int reqs = 0;
        for (auto ctrl: ctrls)
            reqs += ctrl->readq.size() + ctrl->writeq.size() + ctrl->otherq.size() + ctrl->pending.size();
        return reqs;
    }

    void finish(void) {
        dram_capacity = max_address;
        int *sz = spec->org_entry.count;
        maximum_internal_bandwidth =
            spec->speed_entry.rate * 1e6 * spec->channel_width * sz[int(HMC::Level::Vault)] / 8;
        maximum_link_bandwidth =
            spec->link_width * 2 * spec->hmc_links * spec->lane_speed * 1e9 / 8;

        long dram_cycles = num_dram_cycles.value();
        long total_read_req = num_read_requests.total();
        long total_write_req = num_write_requests.total();
        for (auto ctrl : ctrls) {
            ctrl->finish(dram_cycles);
        }
        read_bandwidth = read_transaction_bytes.value() * 1e9 / (dram_cycles * clk_ns());
        write_bandwidth = write_transaction_bytes.value() * 1e9 / (dram_cycles * clk_ns());;
        read_latency_avg = read_latency_sum.value() / total_read_req;
        write_latency_avg = write_latency_sum.value() / total_write_req;
        queueing_latency_avg = queueing_latency_sum.value() / (total_read_req + total_write_req);
        request_packet_network_latency_avg = request_packet_network_latency_sum.value() / (total_read_req + total_write_req);
        response_packet_network_latency_avg = response_packet_network_latency_sum.value() / (total_read_req + total_write_req);
        request_packet_latency_avg = request_packet_latency_sum.value() / (total_read_req + total_write_req);
        response_packet_latency_avg = response_packet_latency_sum.value() / (total_read_req+total_write_req);
        read_latency_ns_avg = read_latency_avg.value() * clk_ns();
        write_latency_ns_avg = write_latency_avg.value() * clk_ns();
        queueing_latency_ns_avg = queueing_latency_avg.value() * clk_ns();
        request_packet_latency_ns_avg = request_packet_latency_avg.value() * clk_ns();
        response_packet_latency_ns_avg = response_packet_latency_avg.value() * clk_ns();
        req_queue_length_avg = req_queue_length_sum.value() / dram_cycles;
        read_req_queue_length_avg = read_req_queue_length_sum.value() / dram_cycles;
        write_req_queue_length_avg = write_req_queue_length_sum.value() / dram_cycles;
        rmab_length_avg = rmab_length_sum.value() / dram_cycles;

        ptw_req_end_switch_latency_avg = ptw_req_end_switch_latency_sum.value()/num_ptws.value();
        ptw_resp_end_switch_latency_avg = ptw_resp_end_switch_latency_sum.value()/num_ptws.value();

        for(int i = 0; i < configs.get_pu_core_num(); i++){
            total_pu_local_vault_read_requests += num_pu_local_vault_read_requests[i];
            total_pu_local_vault_write_requests += num_pu_local_vault_write_requests[i];
            total_pu_local_memory_read_requests += num_pu_local_memory_read_requests[i];
            total_pu_local_memory_write_requests += num_pu_local_memory_write_requests[i];
            total_pu_remote_memory_read_requests += num_pu_remote_memory_read_requests[i];
            total_pu_remote_memory_write_requests += num_pu_remote_memory_write_requests[i];
        }

        for (int i = 0 ; i < stacks ; ++i) {
            link_output_buffer_sizes_avg[i] = (link_output_buffer_sizes_sum[i].value() / stack_links)/dram_cycles;
            link_input_buffer_sizes_avg[i] = (link_input_buffer_sizes_sum[i].value() / stack_links)/dram_cycles;
            vault_switch_queued_packets_avg[i] = vault_switch_queued_packets_sum[i].value() / dram_cycles;
            vault_switch_packet_service_latency_avg[i] = logic_layers[i]->xbar.get_packet_service_lat_avg();
            vault_switch_queued_route_packets_avg[i] = vault_switch_queued_route_packets_sum[i].value() / dram_cycles;
            vault_switch_route_packet_service_latency_avg[i] = logic_layers[i]->xbar.get_route_packet_service_lat_avg();
        }

        req_end_switch_latency_avg = req_end_switch_latency_sum.value() / (total_read_req + total_write_req);
        resp_end_switch_latency_avg = resp_end_switch_latency_sum.value() / (total_read_req + total_write_req);

        for (int i = 0; i <= stacks*2 ; ++i){
            if(memory_access_num_by_hop[i].value()){
                memory_access_latency_by_hop_avg[i] = (float)memory_access_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                request_packet_latency_by_hop_avg[i] = (float)request_packet_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                response_packet_latency_by_hop_avg[i] = (float)response_packet_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                request_packet_network_latency_by_hop_avg[i] = (float)request_packet_network_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                response_packet_network_latency_by_hop_avg[i] = (float)response_packet_network_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                memory_access_network_latency_by_hop_avg[i] = (float)memory_access_network_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                req_end_switch_latency_by_hop_avg[i] = (float)req_end_switch_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
                resp_end_switch_latency_by_hop_avg[i] = (float)resp_end_switch_latency_by_hop_sum[i]/memory_access_num_by_hop[i].value();
            }
        }
    }

    long get_total_mem_req(){return num_incoming_requests.value();}
    long get_mem_size() {return dram_capacity.value();}

private:

    int calc_log2(int val){
        int n = 0;
        while ((val >>= 1))
            n ++;
        return n;
    }

    bool tmp_bool = false;
public:
    int slice_lower_bits(long& addr, int bits)
    {

        int lbits = addr & ((1<<bits) - 1);
        addr >>= bits;
        return lbits;
    }
    void clear_lower_bits(long& addr, int bits)
    {
        addr >>= bits;
    }

    void clear_higher_bits(long& addr, long mask) {
        addr = (addr & mask);
    }

    int get_route_link(int cur_cub, int target_cub){
        if(!(cur_cub >= 0 && cur_cub < stacks) && (target_cub >= 0 && target_cub < stacks)){
            printf("%d,%d,%d\n",cur_cub,target_cub,stacks);
            Dump_Trace();
        }
        assert((cur_cub >= 0 && cur_cub < stacks) && (target_cub >= 0 && target_cub < stacks));
        int routeid = memTopology.getRouteLink(cur_cub,target_cub);
        return routeid;
    }

    int get_route_link1(int cur_cub, int target_cub){
        if(!(cur_cub >= 0 && cur_cub < stacks) && (target_cub >= 0 && target_cub < stacks)){
            printf("%d,%d,%d\n",cur_cub,target_cub,stacks);
            Dump_Trace();
        }
        assert((cur_cub >= 0 && cur_cub < stacks) && (target_cub >= 0 && target_cub < stacks));
        int route = get_route_link_Dragonfly(cur_cub, target_cub);
        assert(route == memTopology.getRouteLink(cur_cub,target_cub));
        return route;
        // if(topology == DRAGONFLY){
        //     return get_route_link_Dragonfly(cur_cub, target_cub);
        // }else if(topology == MESH){
        //     return get_route_link_Mesh(cur_cub, target_cub);
        // }else if(topology == SINGLE){
        //     return -1;
        // }else{
        //     assert(0 && "Unsupported topology!");
        // }
    }
private:
    long lrand(void) {
        if(sizeof(int) < sizeof(long)) {
            return static_cast<long>(rand()) << (sizeof(int) * 8) | rand();
        }

        return rand();
    }
    void connect_memnodes(){
        for(int nodeId = 0; nodeId < memTopology.nodenum; nodeId++){
            for (int innerlinkId = 0; innerlinkId < memTopology.linkspernode; innerlinkId++){
                if(!memTopology.nodes[nodeId][innerlinkId]){
                    int outerlinkId = nodeId*memTopology.linkspernode + innerlinkId;
                    int neighlinkId = memTopology.interconnections[outerlinkId];
                    if(neighlinkId != -1){
                        int neighnodeId = neighlinkId/memTopology.linkspernode;
                        logic_layers[nodeId]->get_link_by_num(outerlinkId)->set_other_side_link(logic_layers[neighnodeId]->get_link_by_num(neighlinkId));
                    }
                }
            }
        }
    }
    // void connect_hmcs(){
    //     if(topology == DRAGONFLY){
    //         return connect_hmcs_Dragonfly();
    //     }else if(topology == MESH){
    //         return connect_hmcs_Mesh();
    //     }else if(topology == SINGLE){
    //         return;
    //     }else{
    //         assert(0 && "Unsupported topology!");
    //     }
    // }

    void connect_hmcs_Mesh();
    void connect_hmcs_Dragonfly();
    // return link id for PIM
    int get_route_link_Mesh(int cur_cub, int target_cub);
    int get_route_link_Dragonfly(int cur_cub, int target_cub);
};

} /*namespace ramulator*/

#endif /*__HMC_MEMORY_H*/
