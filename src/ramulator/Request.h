#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>
#include <assert.h>
#include "common/common_functions.h"

using namespace std;

namespace ramulator
{
class PUInstr;
class Request
{
public:
    bool is_first_command;
    long addr;
    // long addr_row;
    vector<int> addr_vec;
    long reqid = -1;
    // specify which core this request sent from, for virtual address translation
    int coreid = -1;
    int taskid = -1;
    long initial_addr = 0;
    bool instruction_request = false;
    bool is_pim_inst = false;
    bool is_ptw = false;
    bool ideal_memnet = false;
    int acc_type = 0;
    // pu_id : processor unit id, which issues the request packet; -1 represent CPU
    // the response packet uses the same source_uid as request packet
    int pu_id = -1;

    enum class Type
    {
        READ,
        WRITE,
        REFRESH,
        POWERDOWN,
        SELFREFRESH,
        EXTENSION,
        INSTRUCTION,
        MAX
    } type;

    enum class NetworkStage
    {
        REQ_WAIT_NETWORK = 0,
        REQ_ARRIVE_NETWORK = 1,
        REQ_DEPART_FIRST_SWITCH = 2,
        REQ_ARRIVE_LAST_SWITCH = 3,
        REQ_DEPART_NETWORK = 4,
        RESP_WAIT_NETWORK = 5,
        RESP_ARRIVE_NETWORK = 6,
        RESP_DEPART_FIRST_SWITCH = 7,
        RESP_ARRIVE_LAST_SWITCH = 8,
        RESP_DEPART_NETWORK = 9,
        MAX = 10
    };

    long arrive = -1;
    long depart = -1;
    long arrive_hmc = -1;
    long depart_hmc = -1;
    long depart_pu = -1;
    long send = -1;//the initial issue cycle of PU
    long arrive_switch = -1; // the cycle injected to each vault switch
    long network[10] = {0,0,0,0,0,0,0,0,0,0};
    
    int hops = 0;
    int req_hops = 0;
    int resp_hops = 0;


    int burst_count = 0;
    int transaction_bytes = 0;
    function<void(Request&)> callback; // call back with more info
    bool clflush = false;
    bool is_write = false;// save the WRITE type, as cache may changes the WRITE type to READ
    PUInstr* instr = NULL;

    Request(long addr, Type type, int coreid)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), is_write(type==Type::WRITE?true:false),
      callback([](Request& req){}) {initial_addr = addr;}

    Request(long addr, Type type, int pu_id, bool clflush)
        : is_first_command(true), addr(addr), pu_id(pu_id), type(type), is_write(type==Type::WRITE?true:false),clflush(clflush),is_pim_inst(true),
      callback([](Request& req){}) {initial_addr = addr;}

    Request(long addr, Type type, function<void(Request&)> callback, int coreid)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), is_write(type==Type::WRITE?true:false), callback(callback) {initial_addr = addr;}

    Request(long addr, Type type, function<void(Request&)> callback, int _coreid, int _pu_id, bool _is_pim_inst)
        : is_first_command(true), addr(addr), coreid(_coreid),pu_id(_pu_id), type(type),is_pim_inst(_is_pim_inst), is_write(type==Type::WRITE?true:false), callback(callback) {initial_addr = addr;}

    Request(long addr, Type type, function<void(Request&)> callback, int _coreid, int _pu_id, bool _is_pim_inst, bool _is_ptw)
        : is_first_command(true), addr(addr), coreid(_coreid),pu_id(_pu_id), type(type),is_pim_inst(_is_pim_inst), is_ptw(_is_ptw), is_write(type==Type::WRITE?true:false), callback(callback) {initial_addr = addr;}

    Request(long addr, Type type, function<void(Request&)> callback, int pu_id, int taskid)
        : is_first_command(true), addr(addr), pu_id(pu_id), type(type), taskid(taskid), is_pim_inst(true),is_write(type==Type::WRITE?true:false),callback(callback) {initial_addr = addr;}

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid)
        : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), is_write(type==Type::WRITE?true:false), callback(callback) {initial_addr = addr;}

    Request() {}

};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/

