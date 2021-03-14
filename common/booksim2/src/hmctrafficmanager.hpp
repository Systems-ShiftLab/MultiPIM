
#ifndef _HMC_TRAFFIC_MANAGER_HPP
#define _HMC_TRAFFIC_MANAGER_HPP

#include <iostream>
#include <vector>
#include <list>

#include "config_utils.hpp"
#include "stats.hpp"
#include "trafficmanager.hpp"
#include "booksim.hpp"
#include "booksim_config.hpp"
#include "flit.hpp"

class HMCTrafficManager : public TrafficManager {
  
protected:
  virtual void _RetireFlit( Flit *f, int dest );
  virtual void _GeneratePacket(int source, int stype, int cl, int time, int subnet, int package_size, const Flit::FlitType& packet_type, void* const data, int dest);
  virtual int  _IssuePacket( int source, int cl );
  virtual void _Step();
  
  // record size of _partial_packets for each subnet
  vector<vector<vector<list<Flit *> > > > _input_queue;
  
public:
  
  HMCTrafficManager( const Configuration &config, const vector<Network *> & net );
  virtual ~HMCTrafficManager( );
  
  // correspond to TrafficManger::Run/SingleSim
  void Init();
  
  // TODO: if it is not good...
  friend class InterconnectInterface;
  
  
  
  //    virtual void WriteStats( ostream & os = cout ) const;
  //    virtual void DisplayStats( ostream & os = cout ) const;
  //    virtual void DisplayOverallStats( ostream & os = cout ) const;
  
};


#endif

