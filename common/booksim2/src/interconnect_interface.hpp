#ifndef _INTERCONNECT_INTERFACE_HPP_
#define _INTERCONNECT_INTERFACE_HPP_

#include <vector>
#include <queue>
#include <deque>
#include <iostream>
#include <map>
using namespace std;


// Do not use #include since it will not compile in icnt_wrapper or change the makefile to make it
class Flit;
class HMCTrafficManager;
class IntersimConfig;
class Network;
class Stats;

//TODO: fixed_lat_icnt, add class support? support for signle network

class InterconnectInterface {
public:
  InterconnectInterface();
  virtual ~InterconnectInterface();
  static InterconnectInterface* New(unsigned n_subnet, const char* const config_file);
  virtual void CreateInterconnect(unsigned n_link,  unsigned n_vault);
  
  //node side functions
  virtual void Init();
  virtual void Push(unsigned subnet, unsigned input_deviceID, unsigned output_deviceID, void* data, unsigned int size, bool request, bool read);
  virtual void* Pop(unsigned subnet, unsigned ouput_deviceID);
  virtual void* Top(unsigned subnet, unsigned ouput_deviceID);
  virtual void Advance();
  virtual bool Busy() const;
  virtual bool HasBuffer(unsigned subnet, unsigned deviceID, unsigned int size) const;
  virtual void DisplayStats() const;
  virtual void DisplayOverallStats() const;
  unsigned GetFlitSize() const;
  unsigned GetSubNets() const { return _subnets; } ;
  
  virtual void DisplayState(FILE* fp) const;
  
  //booksim side functions
  void WriteOutBuffer( int subnet, int output, Flit* flit );
  void Transfer2BoundaryBuffer(int subnet, int output);
  
  int GetIcntTime() const;
  
  Stats* GetIcntStats(const string & name) const;
  
  Flit* GetEjectedFlit(int subnet, int node);
  
protected:
  
  class _BoundaryBufferItem {
  public:
    _BoundaryBufferItem():_packet_n(0) {}
    inline unsigned Size(void) const { return _buffer.size(); }
    inline bool HasPacket() const { return _packet_n; }
    void* PopPacket();
    void* TopPacket() const;
    void PushFlitData(void* data,bool is_tail);
    
  private:
    // queue<void *> _buffer;
    // queue<bool> _tail_flag;
    deque<void *> _buffer;
    deque<bool> _tail_flag;
    int _packet_n;
  };
  typedef queue<Flit*> _EjectionBufferItem;
  
  void _CreateBuffer( );
  void _CreateNodeMap(unsigned n_link, unsigned n_vault, unsigned n_node, int use_map, const char* const topology);
  void _DisplayMap(int dim,int count);
  
  // size: [subnets][nodes][vcs]
  vector<vector<vector<_BoundaryBufferItem> > > _boundary_buffer;
  unsigned int _boundary_buffer_capacity;
  // size: [subnets][nodes][vcs]
  vector<vector<vector<_EjectionBufferItem> > > _ejection_buffer;
  // size:[subnets][nodes]
  vector<vector<queue<Flit* > > > _ejected_flit_queue;
  
  unsigned int _ejection_buffer_capacity;
  unsigned int _input_buffer_capacity;
  
  vector<vector<int> > _round_robin_turn; //keep track of _boundary_buffer last used in icnt_pop
  
  HMCTrafficManager* _traffic_manager;
  unsigned _flit_size;
  IntersimConfig* _icnt_config;
  unsigned _n_link, _n_vault;
  vector<Network *> _net;
  int _vcs;
  int _subnets;
  
  //deviceID to icntID map
  //deviceID : Starts from 0 for links and then continues until mem nodes
  //which starts at location n_link and then continues to n_link+n_vault (last device)
  map<unsigned, unsigned> _node_map;
  
  //icntID to deviceID map
  map<unsigned, unsigned> _reverse_node_map;

};

#endif

