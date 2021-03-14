#ifndef __LOGICLAYER_CPP
#define __LOGICLAYER_CPP

#include "LogicLayer.h"

#include <cmath>
#include <vector>
#include <set>

namespace ramulator {

template<typename T>
void LinkMaster<T>::send() {
  if (output_buffer.size() > 0 &&
      available_token_count >= output_buffer.front().total_flits) {
    if(link->type != Link<T>::Type::HOST_SOURCE_MODE)
      debug_hmc("send data packet @ link %d master to link %d master", link->id,other_side_link->id);
    else
      debug_hmc("send data packet @ link %d master", link->id);

    Packet packet = output_buffer.front();
    output_buffer.pop_front();
    buffer_occupied -= packet.total_flits;
    int rtc = leftmostbit(link->slave.extracted_token_count);
    debug_hmc("link->slave.extracted_token_count: %d, RTC: %d",
        link->slave.extracted_token_count, rtc);
    link->slave.extracted_token_count -= rtc;
    packet.tail.RTC.value = rtc;

    debug_hmc("packet.total_flits: %d", packet.total_flits);

    if (link->type != Link<T>::Type::HOST_SOURCE_MODE) {
      available_token_count -= packet.total_flits;
      // HOST and PASS_THRU links only send packet to other stack
      assert(other_side_link != NULL);
      assert(packet.total_flits <= other_side_link->slave.available_space());
      other_side_link->slave.receive(packet);
    }else{
      // only response packet to cpu from HOST_SOURCE_MODE link
      // assert(packet.type == Packet::Type::RESPONSE);
      // printf("Cub %d Link %d send packet to CPU\n",logic_layer->cub, link->id);
      packet.req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] = clk;
      send_via_link(packet);
    }

    next_packet_clk = clk +
        ceil(packet.total_flits * logic_layer->one_flit_cycles);
    debug_hmc("clk %ld", clk);
    debug_hmc("next_packet_clk %ld", next_packet_clk);
  } else {
    // if(output_buffer.size() > 0)
    //   printf("Link %d master can't send data packet %ld to link %d (%d, %d)\n",link->id, output_buffer.front().req.reqid,other_side_link->id,output_buffer.front().total_flits,available_token_count);
    // send flow control packet
    if (link->type != Link<T>::Type::HOST_SOURCE_MODE)
      debug_hmc("send flow control packet @ link %d master to link %d master", link->id,other_side_link->id);
    else
      debug_hmc("send flow control packet @ link %d master", link->id);
    if (link->slave.extracted_token_count > 0) {
      int rtc = leftmostbit(link->slave.extracted_token_count);
      debug_hmc("link->slave.extracted_token_count: %d, RTC: %d",
          link->slave.extracted_token_count, rtc);
      link->slave.extracted_token_count -= rtc;
      Packet tret_packet(Packet::Type::TRET, rtc);
      if (link->type != Link<T>::Type::HOST_SOURCE_MODE) {
        assert(other_side_link != NULL);
        other_side_link->slave.receive(tret_packet);
        debug_hmc("send TRET packet @ link %d master", link->id);
        // if(tret_packet.total_flits <= other_side_link->slave.available_space()){
        //   other_side_link->slave.receive(tret_packet);
        //   debug_hmc("send TRET packet @ link %d master", link->id);
        // }else{
        //   next_packet_clk = clk + ceil(logic_layer->one_flit_cycles);//null packet
        //   debug_hmc("send NULL packet @ link %d master", link->id);
        // }
      }else{
        send_via_link(tret_packet);
        next_packet_clk = clk +
          ceil(tret_packet.total_flits * logic_layer->one_flit_cycles);
        debug_hmc("send TRET packet @ link %d master", link->id);
      }
      debug_hmc("clk: %ld", clk);
      debug_hmc("next_packet_clk %ld", next_packet_clk);
    } else {
      // send NULL packet here: no need to do anything
      // NULL packet has one flit
      debug_hmc("send NULL packet @ link %d master", link->id);
      next_packet_clk = clk + ceil(logic_layer->one_flit_cycles);
      debug_hmc("clk: %ld", clk);
      debug_hmc("next_packet_clk %ld", next_packet_clk);
    }
  }
}

template<typename T>
void LinkSlave<T>::receive(Packet& packet) {
  if (packet.flow_control) {
    debug_hmc("receive flow control packet @ link %d slave", link->id);
    int rtc = packet.tail.RTC.value;
    link->master.available_token_count += rtc;
    // drop this packet
  } else {
    debug_hmc("receive data packet @ link %d slave", link->id);
    int rtc = packet.tail.RTC.value;
    link->master.available_token_count += rtc;
    input_buffer.push_back(packet);
    buffer_occupied += packet.total_flits;
    debug_hmc("input_buffer.size() %ld @ link %d slave",
        input_buffer.size(), link->id);
  }
}

template<typename T>
void Switch<T>::vault_inject(std::set<int>& used_links) {
  //PIM remote request and mem response
  int vault_start = logic_layer->cub * vaults_per_cube;
  int vault_end = vault_start + vaults_per_cube;
  for (auto vault_ctrl : vault_ctrls) {
    if(vault_ctrl->channel->id < vault_start || vault_ctrl->channel->id >= vault_end)
      continue;
    bool response_packet;// sed packets in a RR manner
    Packet packet;
    if (vault_ctrl->next_response_packet){
      if (!vault_ctrl->response_packets_buffer.empty()){
        packet = vault_ctrl->response_packets_buffer.front();
        response_packet = true;
        // next will process pim remote request packet
        vault_ctrl->next_response_packet = false;
      } else if (!vault_ctrl->pim_remote_packets_buffer.empty()){
        packet = vault_ctrl->pim_remote_packets_buffer.front();
        response_packet = false;
        // next will process response packet
      } else {
        continue;
      }
    }else{
      if (!vault_ctrl->pim_remote_packets_buffer.empty()){
        packet = vault_ctrl->pim_remote_packets_buffer.front();
        response_packet = false;
        // next will process response packet
        vault_ctrl->next_response_packet = true;
      }else if (!vault_ctrl->response_packets_buffer.empty()){
        packet = vault_ctrl->response_packets_buffer.front();
        response_packet = true;
        // next will process pim remote request packet
      }else{
        continue;
      }
    }
    // printf("Cub %d Vault %d injected a packet, reqid %ld\n",logic_layer->cub, vault_ctrl->deviceIdx,packet.req.reqid);
    if(enable_quadrant){
      if(response_packet){
        int slid = packet.header.SLID.value; // identify the target of transmission
        int source_cub = slid / configs.get_int_value("stack_links");
        if(slid != -1 && source_cub == logic_layer->cub){
          // response to CPU
          Link<T>* source_link = logic_layer->get_link_by_num(slid);
          if(source_link->belong_local_quadrant(vault_ctrl->channel->id)){
            assert(source_link->type == Link<T>::Type::HOST_SOURCE_MODE);
            assert(packet.type == Packet::Type::RESPONSE);

            // response to directed connected  CPU
            if(used_links.find(source_link->id) != used_links.end()){
              continue;
            }
            if(packet.total_flits <= source_link->master.available_space()){
              packet.req.hops++;
              packet.req.resp_hops++;
              packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)] = clk;
              packet.req.network[int(Request::NetworkStage::RESP_DEPART_FIRST_SWITCH)] = clk;
              if(packet.req.resp_hops == 1){
                packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)] = clk;
              }
              source_link->master.output_buffer.push_back(packet);
              source_link->master.buffer_occupied += packet.total_flits;
              vault_ctrl->response_packets_buffer.pop_front();
              used_links.insert(source_link->id);
            }
            continue;
          }
        }else if(packet.req.pu_id < vault_start || packet.req.pu_id >= vault_end){
          // send response to other HMC
          Link<T> *route_link = logic_layer->get_link_by_num(logic_layer->mem->get_route_link(logic_layer->cub, source_cub));
          if(route_link->belong_local_quadrant(vault_ctrl->channel->id)){
            assert(route_link->type != Link<T>::Type::HOST_SOURCE_MODE);
            assert(slid != -1);

            if(used_links.find(route_link->id) != used_links.end()){
              continue;
            }
            if(packet.total_flits <= route_link->master.available_space()){
              packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)] = clk;
              packet.req.network[int(Request::NetworkStage::RESP_DEPART_FIRST_SWITCH)] = clk;
              packet.req.hops++;
              packet.req.resp_hops++;
              route_link->master.output_buffer.push_back(packet);
              route_link->master.buffer_occupied += packet.total_flits;
              vault_ctrl->response_packets_buffer.pop_front();
              used_links.insert(route_link->id);
            }
            continue;
          }
        }
      }else{
        // send pim remote memory request to other HMC
        int target_vault = packet.req.addr_vec[int(HMC::Level::Vault)];
        if( target_vault < vault_start || target_vault >= vault_end){
          //send pim remote memory response to other HMC
          assert(packet.header.CUB.value != logic_layer->cub);
          Link<T> *route_link = logic_layer->get_link_by_num(logic_layer->mem->get_route_link(logic_layer->cub, packet.header.CUB.value));
          assert(route_link->type != Link<T>::Type::HOST_SOURCE_MODE);
          if(route_link->belong_local_quadrant(vault_ctrl->channel->id)){
            if(used_links.find(route_link->id) != used_links.end()){
              continue;
            }

            if(packet.total_flits <= route_link->master.available_space()){
              // assert(packet.req.is_pim_inst);
              packet.req.hops++;
              packet.req.req_hops++;
              packet.req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)] = clk;
              packet.req.network[int(Request::NetworkStage::REQ_DEPART_FIRST_SWITCH)] = clk;
              route_link->master.output_buffer.push_back(packet);
              route_link->master.buffer_occupied += packet.total_flits;
              vault_ctrl->pim_remote_packets_buffer.pop_front();
              used_links.insert(route_link->id);
            }
            continue;
          }
        }
      }
    }

    int data_size = packet.total_flits * flit_size;
    int target_deviceIdx = -1;

    /* if(response_packet && packet.req.is_pim_inst){
      if(!packet.req.network[int(Request::NetworkStage::RESP_WAIT_NETWORK)])
        packet.req.network[int(Request::NetworkStage::RESP_WAIT_NETWORK)] = clk;
    }else{
      //PIM remote request
      if(!packet.req.network[int(Request::NetworkStage::REQ_WAIT_NETWORK)])
        packet.req.network[int(Request::NetworkStage::REQ_WAIT_NETWORK)] = clk;
    } */

    if(!vault_ctrl->switch_injection_buffer_full(data_size)){

      if(response_packet){// either pim inst or non-pim inst
        int slid = packet.header.SLID.value; // identify the target of transmission
        int source_cub = slid / configs.get_int_value("stack_links");
        if(slid != -1 && source_cub == logic_layer->cub){
          // response to CPU
          Link<T>* source_link = logic_layer->get_link_by_num(slid);
          assert(source_link->type == Link<T>::Type::HOST_SOURCE_MODE);
          assert(packet.type == Packet::Type::RESPONSE);
          // response to directed connected  CPU
          target_deviceIdx = source_link->deviceIdx;
          packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)] = clk;
        }else{
          // send response to other HMC or other vault in tha same HMC
          if(packet.req.pu_id >= vault_start && packet.req.pu_id < vault_end){
            // send response to vault in tha same HMC
            assert(slid == -1);
            auto target_ctrl = vault_ctrls[packet.req.pu_id];
            target_deviceIdx = target_ctrl->deviceIdx;
            packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)] = clk;
          }else{
            // send response to other HMC
            Link<T> *route_link = logic_layer->get_link_by_num(logic_layer->mem->get_route_link(logic_layer->cub, source_cub));
            assert(route_link->type != Link<T>::Type::HOST_SOURCE_MODE);
            assert(slid != -1);
            target_deviceIdx = route_link->deviceIdx;
          }
        }
        packet.req.hops++;
        packet.req.resp_hops++;
        packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_NETWORK)] = clk;
        inject_packets++;
        packet.req.arrive_switch = clk;
        vault_ctrl->switch_inject_packet(packet, data_size, target_deviceIdx);
        vault_ctrl->response_packets_buffer.pop_front();
      }else{
        // send pim remote memory request to other HMC or other local vault pu
        int target_vault = packet.req.addr_vec[int(HMC::Level::Vault)];
        if( target_vault >= vault_start && target_vault < vault_end){
          //send pim remote memory request to other local vault pu
          auto target_ctrl = vault_ctrls[target_vault];
          target_deviceIdx = target_ctrl->deviceIdx;
          // printf("Vault %d inject PIM reqest to local vault %d\n",target_vault);
        }else{
          //send pim remote memory response to other HMC
          assert(packet.header.CUB.value != logic_layer->cub);
          Link<T> *route_link = logic_layer->get_link_by_num(logic_layer->mem->get_route_link(logic_layer->cub, packet.header.CUB.value));
          assert(route_link->type != Link<T>::Type::HOST_SOURCE_MODE);
          target_deviceIdx = route_link->deviceIdx;
        }
        packet.req.hops++;
        packet.req.req_hops++;
        packet.req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)] = clk;
        inject_packets++;
        packet.req.arrive_switch = clk;
        vault_ctrl->switch_inject_packet(packet, data_size, target_deviceIdx);
        vault_ctrl->pim_remote_packets_buffer.pop_front();
      }
    }
  }
}

template<typename T>
void Switch<T>::vault_eject(std::set<int>& used_vaults) {
  //PIM response and mem request
  int vault_start = logic_layer->cub * vaults_per_cube;
  int vault_end = vault_start + vaults_per_cube;
  for (auto vault_ctrl : vault_ctrls) {
    if(vault_ctrl->channel->id < vault_start || vault_ctrl->channel->id >= vault_end)
      continue;
    if (used_vaults.find(vault_ctrl->channel->id) != used_vaults.end()) {
      continue; // This port has been occupied in this cycle
    }
    Packet* packet = vault_ctrl->switch_ejection_buffer_top();
    if(!packet)
      continue;
    Request& req = packet->req;
    // printf("Cub %d Vault %d ejected a packet, reqid %ld\n",logic_layer->cub, vault_ctrl->deviceIdx,req.reqid);
    //if request
    if(packet->type == Packet::Type::REQUEST){
      // request packets
      assert(req.addr_vec[int(HMC::Level::Vault)] == vault_ctrl->channel->id);
      if(vault_ctrl->can_receive(*packet)) {
        // printf("request %d, addr %ld received by cub %d @ %ld\n",packet->req.reqid, packet->req.addr,logic_layer->cub, clk);
        packet->req.network[int(Request::NetworkStage::REQ_DEPART_NETWORK)] = clk;
        if(packet->req.req_hops == 1){
          packet->req.network[int(Request::NetworkStage::REQ_DEPART_FIRST_SWITCH)] = clk;
        }
        eject_packets++;
        packet_service_latency_sum += (clk - packet->req.arrive_switch);
        vault_ctrl->receive(*packet);
        vault_ctrl->switch_ejection_buffer_pop();
        used_vaults.insert(vault_ctrl->channel->id);
      }
    }else{
      //PIM response
      assert(packet->type == Packet::Type::RESPONSE);
      assert(req.pu_id == vault_ctrl->channel->id);
      packet->req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] = clk;
      if(packet->req.resp_hops == 1){
        packet->req.network[int(Request::NetworkStage::RESP_DEPART_FIRST_SWITCH)] = clk;
      }
      eject_packets++;
      packet_service_latency_sum += (clk - packet->req.arrive_switch);
      vault_ctrl->receive_pim_response(*packet);
      vault_ctrl->switch_ejection_buffer_pop();
      used_vaults.insert(vault_ctrl->channel->id);
    }
  }
}

template<typename T>
void Switch<T>::link_inject(std::set<int>& used_vaults) {
  for (auto link : logic_layer->links) {
    if (link->slave.input_buffer.empty()) {
      continue;
    }

    Packet& packet = link->slave.input_buffer.front();
    Request& req = packet.req;
    if(packet.flow_control)
      continue;

    if(enable_quadrant){
      if(packet.type == Packet::Type::REQUEST){
        if (packet.header.CUB.value == logic_layer->cub) {
          int vault_id = req.addr_vec[int(HMC::Level::Vault)];
          if(link->belong_local_quadrant(vault_id)){
            if (used_vaults.find(vault_id) != used_vaults.end()) {
              continue;
            }
            auto target_ctrl = vault_ctrls[vault_id];
            if(target_ctrl->can_receive(packet)) {
              packet.req.hops++;
              packet.req.req_hops++;
              packet.req.network[int(Request::NetworkStage::REQ_DEPART_NETWORK)] = clk;
              packet.req.network[int(Request::NetworkStage::REQ_ARRIVE_LAST_SWITCH)] = clk;
              if(packet.req.req_hops == 1){
                //cpu request
                packet.req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)] = clk;
                packet.req.network[int(Request::NetworkStage::REQ_DEPART_FIRST_SWITCH)] = clk;
              }
              target_ctrl->receive(packet);
              link->slave.extracted_token_count += packet.total_flits;
              link->slave.buffer_occupied -= packet.total_flits;
              link->slave.input_buffer.pop_front();
              used_vaults.insert(vault_id);
            }
            continue;
          }
          // go to switch
        }
      }else{
        assert(packet.type == Packet::Type::RESPONSE);
        int source_cub = packet.header.SLID.value / configs.get_int_value("stack_links");
        if (source_cub == logic_layer->cub) {
          auto source_link = logic_layer->get_link_by_num(packet.header.SLID.value);
          if(source_link->type != Link<T>::Type::HOST_SOURCE_MODE){
            // response to local PIM 
            assert(packet.req.pu_id != -1);
            int vault_id = req.pu_id;
            if(link->belong_local_quadrant(vault_id)){
              if (used_vaults.find(vault_id) != used_vaults.end()) {
                continue;
              }
              packet.req.hops++;
              packet.req.resp_hops++;
              packet.req.network[int(Request::NetworkStage::RESP_DEPART_NETWORK)] = clk;
              packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)] = clk;
              auto target_ctrl = vault_ctrls[vault_id];
              target_ctrl->receive_pim_response(packet);
              link->slave.extracted_token_count += packet.total_flits;
              link->slave.buffer_occupied -= packet.total_flits;
              link->slave.input_buffer.pop_front();
              used_vaults.insert(vault_id);
              continue;
            }
            //go to switch
          }
        }
      }
    }

    int data_size = packet.total_flits * flit_size;
    int target_deviceIdx = -1;
    if(!link->switch_injection_buffer_full(data_size)){
      if(packet.type == Packet::Type::REQUEST){
        if (packet.header.CUB.value == logic_layer->cub) {
          int vault_id = req.addr_vec[int(HMC::Level::Vault)];
          auto target_ctrl = vault_ctrls[vault_id];
          target_deviceIdx = target_ctrl->deviceIdx;
          packet.req.network[int(Request::NetworkStage::REQ_ARRIVE_LAST_SWITCH)] = clk;
        }else{
          //To other hmc stack
          auto route_link = logic_layer->get_link_by_num(logic_layer->mem->get_route_link(logic_layer->cub,packet.header.CUB.value));
          target_deviceIdx = route_link->deviceIdx;
          inject_route_packets++;
          // printf("cub %d, inject_route_packets %d\n",logic_layer->cub,inject_route_packets);
        }
        packet.req.req_hops++;
        packet.req.hops++;
        if(packet.req.req_hops == 1){
            //cpu request
            packet.req.network[int(Request::NetworkStage::REQ_ARRIVE_NETWORK)] = clk;
        }
      }else{
        assert(packet.type == Packet::Type::RESPONSE);
        int source_cub = packet.header.SLID.value / configs.get_int_value("stack_links");
        if (source_cub == logic_layer->cub) {
          auto source_link = logic_layer->get_link_by_num(packet.header.SLID.value);
          if(source_link->type == Link<T>::Type::HOST_SOURCE_MODE){
            // response to CPU
            target_deviceIdx = source_link->deviceIdx;
            inject_route_packets++;
            // printf("cub %d, inject_route_packets %d\n",logic_layer->cub,inject_route_packets);
          }else{
            // response to local PIM 
            assert(packet.req.pu_id != -1);
            int vault_id = packet.req.pu_id;
            auto target_ctrl = vault_ctrls[vault_id];
            target_deviceIdx = target_ctrl->deviceIdx;
            packet.req.network[int(Request::NetworkStage::RESP_ARRIVE_LAST_SWITCH)] = clk;
          }
        }else{
          //to other stack
          auto route_link = logic_layer->get_link_by_num(logic_layer->mem->get_route_link(logic_layer->cub,source_cub));
          target_deviceIdx = route_link->deviceIdx;
          inject_route_packets++;
          // printf("cub %d, inject_route_packets %d\n",logic_layer->cub,inject_route_packets);
        }
        packet.req.resp_hops++;
        packet.req.hops++;
      }
      inject_packets++;
      packet.req.arrive_switch = clk;
      link->switch_inject_packet(packet, data_size, target_deviceIdx);
      link->slave.extracted_token_count += packet.total_flits;
      link->slave.buffer_occupied -= packet.total_flits;
      link->slave.input_buffer.pop_front();
    }
  }
}

template<typename T>
void Switch<T>::link_eject(std::set<int>& used_links) {
  for (auto link : logic_layer->links) {
    if(used_links.find(link->id) != used_links.end()){
      continue;
    }
    Packet* packet = link->switch_ejection_buffer_top();
    if(!packet)
      continue;
    // printf("Cub %d Link %d(%d) ejected a packet\n",logic_layer->cub, link->deviceIdx, link->id);
    // if(link->type == Link<T>::Type::HOST_SOURCE_MODE){
    //   assert(packet->type == Packet::Type::RESPONSE);
    // }
    if(packet->total_flits <= link->master.available_space()){
      if(packet->type == Packet::Type::REQUEST){
        if(packet->req.req_hops == 1){
          packet->req.network[int(Request::NetworkStage::REQ_DEPART_FIRST_SWITCH)] = clk;
        }
        if(packet->req.is_pim_inst){
          if(packet->req.req_hops > 1){
            eject_route_packets++;
            // printf("1cub %d, req_hops %d , eject_route_packets %d\n",logic_layer->cub,packet->req.req_hops,eject_route_packets);
            route_packet_service_latency_sum += (clk - packet->req.arrive_switch);
          }
        }else{
          eject_route_packets++;
          // printf("2cub %d, req_hops %d , eject_route_packets %d\n",logic_layer->cub,packet->req.req_hops,eject_route_packets);
          route_packet_service_latency_sum += (clk - packet->req.arrive_switch);
        }
      }else if(packet->type == Packet::Type::RESPONSE){
        if(packet->req.resp_hops == 1){
          packet->req.network[int(Request::NetworkStage::RESP_DEPART_FIRST_SWITCH)] = clk;
        }
        if(packet->req.resp_hops > 1){
          eject_route_packets++;
          // printf("3cub %d, resp_hops %d , eject_route_packets %d\n",logic_layer->cub,packet->req.resp_hops,eject_route_packets);
          route_packet_service_latency_sum += (clk - packet->req.arrive_switch);
        }
      }
      eject_packets++;
      packet_service_latency_sum += (clk - packet->req.arrive_switch);

      link->master.output_buffer.push_back(*packet);
      link->master.buffer_occupied += packet->total_flits;
      link->switch_ejection_buffer_pop();
      used_links.insert(link->id);
    }
  }
}

template<typename T>
void Switch<T>::tick() {
  clk++;
  debug_hmc("@ clk: %ld stack %d", clk, logic_layer->cub);
  std::set<int> used_vaults;
  std::set<int> used_links;

  //switch -> link
  link_eject(used_links);

  //switch -> vault
  vault_eject(used_vaults);

  //vault -> switch/link
  vault_inject(used_links);
  
  //link -> switch/vault
  link_inject(used_vaults);

}

template<typename T>
void LogicLayer<T>::tick() {
  for(auto link : links){
    (*link_input_buffer_sizes_sum)[cub] += link->slave.buffer_occupied;
    (*link_output_buffer_sizes_sum)[cub] += link->master.buffer_occupied;
  }
  (*vault_switch_queued_packets_sum)[cub] += xbar.get_queued_packets();
  (*vault_switch_queued_route_packets_sum)[cub] += xbar.get_queued_route_packets();
  // printf("logic layer %d\n",cub);
  for (auto link : source_mode_host_links) {
    link->master.tick();
  }
  for (auto link : host_links) {
    link->master.tick();
  }
  for (auto link : pass_thru_links) {
    link->master.tick();
  }
  xbar.tick();
}

} /* namespace ramulator */
#endif /*__LOGICLAYER_CPP*/
