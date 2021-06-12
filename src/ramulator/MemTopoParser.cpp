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

#include <iostream>
#include <cstring>
#include <string>
#include "MemTopoParser.h"
#include "XMLParser.h"

XERCES_CPP_NAMESPACE_USE

using namespace std;
using namespace ramulator;

MemTopoParser::MemTopoParser() : curNode(-1)
{
}

void MemTopoParser::startElement(const string &name, const Attributes &attrs)
{
	// cout<<"start "<<name<<endl;
	if (name == "memnodes")
	{
		memoryTopology.nodenum = atoi(getValue("num", attrs).c_str());
		memoryTopology.linkspernode = atoi(getValue("linkspernode", attrs).c_str());
		assert(memoryTopology.nodenum > 0);
		assert(memoryTopology.linkspernode > 0);
		memoryTopology.nodes.resize(memoryTopology.nodenum);
		for (int i = 0; i < memoryTopology.nodenum; i++)
		{
			memoryTopology.nodes[i].resize(memoryTopology.linkspernode);
		}
		memoryTopology.interconnections.resize(memoryTopology.nodenum * memoryTopology.linkspernode);
		for (int i = 0; i < memoryTopology.nodenum * memoryTopology.linkspernode; i++)
			memoryTopology.interconnections[i] = -1;
	}
	else if (name == "node")
	{
		curNode = atoi(getValue("id", attrs).c_str());
	}
	else if (name == "link")
	{
		if (getValue("tocpu", attrs) == "true")
		{
			int link_id = atoi(getValue("id", attrs).c_str());
			assert(link_id < memoryTopology.nodenum * memoryTopology.linkspernode);
			int inner_link_id = link_id % memoryTopology.linkspernode;
			memoryTopology.nodes[curNode][inner_link_id] = true;
		}
	}
	else if (name == "interconnection")
	{
		int from = atoi(getValue("from", attrs).c_str());
		int to = atoi(getValue("to", attrs).c_str());
		assert(from < memoryTopology.nodenum * memoryTopology.linkspernode);
		assert(to < memoryTopology.nodenum * memoryTopology.linkspernode);
		memoryTopology.interconnections[from] = to;
		if(!hasValue("type", attrs) || getValue("type", attrs) == "undirected")
			memoryTopology.interconnections[to] = from;
	}else if (name == "memroutes"){
		std::string routetype = getValue("type", attrs);
		if(routetype == "static"){
			memoryTopology.route_type = STATIC_ROUTE;
			memoryTopology.routes.resize(memoryTopology.nodenum);
			memoryTopology.cpuroutes.resize(memoryTopology.nodenum);
			for (int i = 0; i < memoryTopology.nodenum; i++){
				memoryTopology.routes[i].resize(memoryTopology.nodenum);
				// for (int j = 0; j < memoryTopology.nodenum; j++){
				// 	memoryTopology.routes[i][j] = -1;
				// }
				// memoryTopology.cpuroutes[i] = -1;
			}
		}else{
			std::cout<<"Unsupported route type"<<std::endl;
			assert(0);
		}
	}else if (name == "route"){
		assert(memoryTopology.route_type == STATIC_ROUTE);
		int src = atoi(getValue("src", attrs).c_str());
		int dst = atoi(getValue("dst", attrs).c_str());
		assert(src >=0 && src < memoryTopology.nodenum);
		assert(dst >=0 && dst < memoryTopology.nodenum);
		std::string next_str = getValue("next", attrs);
		char * cstr = new char[next_str.length()+1];
		std::strcpy(cstr,next_str.c_str());
		char*p = std::strtok(cstr,",");
		while(p!=0){
			int next = atoi(p);
			assert(next >=src*memoryTopology.linkspernode && next < ((src+1)*memoryTopology.linkspernode));
			memoryTopology.routes[src][dst].push_back(next);
			p = std::strtok(NULL, ",");
		}
		delete[] cstr;
	}else if(name == "cpuroute"){
		assert(memoryTopology.route_type == STATIC_ROUTE);
		int dst = atoi(getValue("dst", attrs).c_str());
		assert(dst >=0 && dst < memoryTopology.nodenum);
		std::string next_str = getValue("next", attrs);
		char * cstr = new char[next_str.length()+1];
		std::strcpy(cstr,next_str.c_str());
		char*p = std::strtok(cstr,",");
		while(p!=0){
			int next = atoi(p);
			assert(next >=0 && next < (memoryTopology.nodenum * memoryTopology.linkspernode));
			assert(memoryTopology.nodes[next/memoryTopology.linkspernode][next%memoryTopology.linkspernode]);
			memoryTopology.cpuroutes[dst].push_back(next);
			p = std::strtok(NULL, ",");
		}
		delete[] cstr;
	}
} // MemTopoParser::startElement

void MemTopoParser::endElement(const string &name)
{
	// cout<<"end "<<name<<endl;
} // MemSpecParser::endElement

MemoryTopology MemTopoParser::getMemoryTopology()
{
	return memoryTopology;
}

// Get memTopo from XML
MemoryTopology MemTopoParser::getMemTopoFromXML(const string &xml_file)
{
	MemTopoParser memTopoParser;

	cout << "* Parsing " << xml_file << endl;
	XMLParser::parse(xml_file, &memTopoParser);

	return memTopoParser.getMemoryTopology();
}