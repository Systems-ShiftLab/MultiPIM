/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2014, TU Kaiserslautern
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Karthik Chandrasekar
 *
 */

#ifndef RAMULATOR_MEMORY_TOPOLOGY_H
#define RAMULATOR_MEMORY_TOPOLOGY_H

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>

namespace ramulator
{
	enum ROUTE_TYPE {
		STATIC_ROUTE,
		ROUTE_TYPE_NUM
	};
	class MemoryTopology
	{
	public:
		int getCPURouteLink(int dst_node){
			if(route_type == STATIC_ROUTE){
				assert(dst_node >= 0 && dst_node < nodenum);
				return cpuroutes[dst_node];
			}
			return -1;
		}
		int getRouteLink(int src_node, int dst_node){
			if(route_type == STATIC_ROUTE){
				assert((src_node >=0 && src_node < nodenum) && (dst_node >=0 && dst_node < nodenum));
				return routes[src_node][dst_node];
			}
			return -1;
		}
		int getTotalSourceModeLinkNum() const {
			int source_num=0;
			for(int i = 0; i < nodenum; i++){
				for(int j = 0; j < linkspernode; j++){
					if(nodes[i][j])
						source_num++;
				}
			}
			return source_num;
		}
		int getSourceModeLinkNum(int nodeId) const{
			int source_num=0;
			for(int i =0; i < linkspernode; i++){
				if(nodes[nodeId][i])
					source_num++;
			}
			return source_num;
		}
		int getHostModeLinkNum(int nodeId) const{
			int host_num=0;
			for(int i =0; i < linkspernode; i++){
				if(!nodes[nodeId][i])
					host_num++;
			}
			return host_num;
		}
		void print(){
			for(int i = 0; i < nodenum; i++){
				std::cout<<"Node: "<<i<<std::endl;
				for (int j = 0; j < linkspernode; j++){
					std::string typestr;
					if(nodes[i][j])
						typestr = "TOCPU";
					else
						typestr = "ToMem";
					std::cout<<"\t"<<j<<" "<<typestr<<std::endl;
				}
			}
		}
	public:
		int nodenum;
		int linkspernode;
		ROUTE_TYPE route_type;
		std::vector<std::vector<bool>> nodes; //<link_vector>, true: link to cpu (source mode link)
		std::vector<int> interconnections;	  //link->link
		std::vector<std::vector<int> > routes;//node-node-link
		std::vector<int> cpuroutes;//cpu-node-link
	};
} // namespace ramulator
#endif // ifndef RAMULATOR_MEMORY_TOPOLOGY_H
