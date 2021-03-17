#ifndef __LOGICLAYER_H
#define __LOGICLAYER_H

#include "Config.h"
#include "Packet.h"
#include "HMC_Controller.h"
#include "Memory.h"
#include "MemoryTopology.h"

#include "Locks.h"
#include "Icnt_Wrapper.h"

#include <memory>
#include <vector>
#include <set>

namespace ramulator {

template<typename T>
class Link;

template<typename T>
class LogicLayer;

template<typename T>
class LinkMaster {
 public:
  function<void(Packet&)> send_via_link;
  Link<T>* link;
  Link<T>* other_side_link;
  LogicLayer<T>* logic_layer;
  std::deque<Packet> output_buffer;
  int buffer_max = 32;
  int buffer_occupied = 0;
  int available_token_count; // available token count on the other side
  long clk = 0;
  long next_packet_clk = 0;

  LinkMaster(const Config& configs, function<void(Packet&)> receive_from_link,
      Link<T>* link, LogicLayer<T>* logic_layer):
      send_via_link(receive_from_link), link(link), other_side_link(NULL), logic_layer(logic_layer),
      available_token_count(link->slave.buffer_max) {
  }

  void send();

  int available_space() {
    // return buffer_max - output_buffer.size();
    return buffer_max - buffer_occupied;
  }
  void set_other_side_link(Link<T>* o_link) {other_side_link = o_link;}

  void tick() {
    clk++;
    if (clk >= next_packet_clk) {
      send();
    }
  }
 private:
  // returns 0 if val == 0
  // returns 1<<leftmostbit if val > 0
  int leftmostbit(int val) {
    if (val == 0) {
      return 0;
    }
    int n = 0;
    while ((val >>= 1)) {
      n++;
    }
    return 1<<n;
  }
};

template<typename T>
class LinkSlave {
 public:
  Link<T>* link;
  Link<T>* other_side_link;
  int extracted_token_count = 0;//read out space (extract data from input buffer)
  std::deque<Packet> input_buffer;
  int buffer_max = 32;
  int buffer_occupied = 0;

  LinkSlave(const Config& configs, Link<T>* link): link(link),other_side_link(NULL) { }

  void receive(Packet& packet);

  int available_space() {
    // return buffer_max - input_buffer.size();
    return buffer_max - buffer_occupied;
  }
  void set_other_side_link(Link<T>* o_link) {other_side_link = o_link;}
  inline bool other_side_link_valid() {return other_side_link != NULL;}
};

template<typename T>
class Link {
 public:
  enum class Type {
    HOST_SOURCE_MODE,
    HOST,
    PASSTHRU,
    MAX
  } type;

  int id;
  int cub;
  int deviceIdx;
  int flit_size; // bytes
  int vaults_per_cube;
  int quadrant_size;

  Link<T>* other_side_link;
  LinkSlave<T> slave;
  LinkMaster<T> master;
  lock_t link_lock;

  Link(const Config& configs, Type type, int id, LogicLayer<T>* logic_layer,
       function<void(Packet&)> receive_from_link):type(type), id(id),cub(logic_layer->cub),
      slave(configs, this), master(configs, receive_from_link, this, logic_layer) 
  {
      deviceIdx = id % configs.get_int_value("stack_links");
      flit_size = logic_layer->spec->flit_bits / 8;
      vaults_per_cube = configs.get_vaults_per_stack();
      quadrant_size = vaults_per_cube / configs.get_int_value("stack_links");
      ramulator_futex_init(&link_lock); 
  }
  void set_other_side_link(Link<T>* o_link){
    other_side_link = o_link;
    slave.set_other_side_link(o_link);
    master.set_other_side_link(o_link);
  }

  inline bool belong_local_quadrant(int vault_id){
    assert((vault_id / vaults_per_cube) == cub);
    int local_vault_id = vault_id % vaults_per_cube;
    if((local_vault_id / quadrant_size) == deviceIdx) return true;
    return false;
  }
  inline void lock(){ ramulator_futex_lock(&link_lock);}
  inline void unlock(){ ramulator_futex_unlock(&link_lock);}

  inline bool switch_ejection_buffer_empty(){ return icnt_top(cub, deviceIdx) == NULL; }
  inline Packet* switch_eject_packet() { return (Packet*)icnt_pop(cub, deviceIdx); }
  inline Packet* switch_ejection_buffer_top() { return (Packet*)icnt_top(cub, deviceIdx); }
  inline void switch_ejection_buffer_pop()
  { 
    Packet* packet =  (Packet*)icnt_pop(cub, deviceIdx);
    if(packet != NULL)
      delete packet;
    // printf("Cub %d Link %d(%d) ejected a packet\n", cub, deviceIdx,id);
  }
  inline bool switch_injection_buffer_full(int size){ return !icnt_has_buffer(cub, deviceIdx,size); }
  inline void switch_inject_packet(Packet& packet, int size, int target_device){
    Packet* data = new Packet();
    *data = packet;
    // printf("Cub %d Link %d(%d) injecting a packet to %d\n", cub, deviceIdx, id, target_device);
    icnt_push(cub, deviceIdx, target_device, (void*)data, size,packet.type == Packet::Type::REQUEST, packet.req.type == Request::Type::READ);
    // printf("Cub %d Link %d(%d) injected a packet to %d\n", cub, deviceIdx, id, target_device);
  }
};

template<typename T>
class Switch {
 public:
  LogicLayer<T>* logic_layer;
  std::vector<Controller<T>*> vault_ctrls;
  const Config& configs;
  long clk = 0;
  int vaults_per_cube;
  int flit_size; //bytes
  bool data_mem_network_no_latency;
  bool enable_quadrant;

  long inject_packets = 0;
  long eject_packets = 0;
  long packet_service_latency_sum = 0;
  long inject_route_packets = 0;
  long eject_route_packets = 0;
  long route_packet_service_latency_sum = 0;

  Switch(const Config& configs, LogicLayer<T>* logic_layer,
         std::vector<Controller<T>*> vault_ctrls):
      logic_layer(logic_layer), vault_ctrls(vault_ctrls), configs(configs),
      vaults_per_cube(configs.get_vaults_per_stack()), flit_size(logic_layer->spec->flit_bits / 8), 
      data_mem_network_no_latency(configs.data_mem_network_no_latency()), enable_quadrant(configs.enable_quadrant()) {}

  void tick();
  void vault_inject(std::set<int>& used_links);
  void vault_eject(std::set<int>& used_vaults);
  void link_inject(std::set<int>& used_vaults);
  void link_eject(std::set<int>& used_links);

  long get_queued_packets()
  {
    assert(inject_packets >= eject_packets);
    return inject_packets - eject_packets;
  }
  long get_queued_route_packets()
  {
    assert(inject_route_packets >= eject_route_packets);
    return inject_route_packets - eject_route_packets;
  }
  double get_packet_service_lat_avg()
  {
    if(eject_packets == 0)
      return 0;
    return (double)packet_service_latency_sum/eject_packets;
  }
  double get_route_packet_service_lat_avg()
  {
    if(eject_route_packets == 0)
      return 0;
    return (double)route_packet_service_latency_sum/eject_route_packets;
  }
};

template<typename T>
class LogicLayer {
 public:
  T* spec;
  MemoryBase* mem;
  int cub;
  double one_flit_cycles; // = ceil(128/(30/# of lane) / 0.8)
  Switch<T> xbar;
  std::vector<std::shared_ptr<Link<T>>> source_mode_host_links;
  std::vector<std::shared_ptr<Link<T>>> host_links;
  std::vector<std::shared_ptr<Link<T>>> links;
  std::map<int,std::shared_ptr<Link<T>>> links_map;
  // we don't distinguish pass_thr link with host links
  std::vector<std::shared_ptr<Link<T>>> pass_thru_links;

  VectorStat* link_output_buffer_sizes_sum;
  VectorStat* link_input_buffer_sizes_sum;
  VectorStat* vault_switch_queued_packets_sum;
  VectorStat* vault_switch_queued_route_packets_sum;

  LogicLayer(const Config& configs, int cub, T* spec,
      std::vector<Controller<T>*> vault_ctrls, MemoryBase* mem,
      MemoryTopology * memmoryTopology, function<void(Packet&)> host_ctrl_recv):
      spec(spec), mem(mem), cub(cub), xbar(configs, this, vault_ctrls)
  {
    // initialize some system parameters
    one_flit_cycles =
        (((float)(spec->flit_bits))/(spec->lane_speed * spec->link_width))/mem->clk_ns();

    int source_mode_host_links_num = memmoryTopology->getSourceModeLinkNum(cub);
    int host_links_num = memmoryTopology->getHostModeLinkNum(cub);
    
    int stack_links = configs.get_int_value("stack_links");
    int link_id = cub * stack_links;
    for (int i = 0 ; i < source_mode_host_links_num ; ++i) {
      Link<T> * link_ptr = new Link<T>(configs, Link<T>::Type::HOST_SOURCE_MODE, 
                      link_id, this, host_ctrl_recv);
      source_mode_host_links.emplace_back(link_ptr);
      links.emplace_back(link_ptr);
      links_map.insert(std::make_pair(link_id,link_ptr));
      // links_map[link_id]=link_ptr;
      link_id++;
    }
    for (int i = 0 ; i < host_links_num ; ++i) {
      Link<T> * link_ptr = new Link<T>(configs, Link<T>::Type::HOST, 
                      link_id, this, host_ctrl_recv);
      host_links.emplace_back(link_ptr);
      links.emplace_back(link_ptr);
      links_map.insert(std::make_pair(link_id,link_ptr));
      // links_map[link_id]=link_ptr;
      link_id++;
    }

    int pass_thr_links_num = 0;
    for (int i = 0 ; i < pass_thr_links_num ; ++i) {
      // TODO
      link_id++;
    }
  }

  void tick();

  Link<T>* get_link(int needed_space){
    for(auto link : links){
      if(needed_space <= link.get()->slave.available_space())
        return link.get();
    }
    return NULL;
  }
  
  Link<T>* get_link_by_num(int num) {
    assert(links_map.find(num) != links_map.end());
    return links_map[num].get();
  }
};

} /* namespace ramulator */
#endif /*__LOGICLAYER_H*/
