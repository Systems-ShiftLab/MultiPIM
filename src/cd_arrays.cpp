#include "cd_arrays.h"
#include "hash.h"

CDSetAssocArray::CDSetAssocArray(uint32_t _numLines, uint32_t _assoc, HashFamily* _hf) : hf(_hf), numLines(_numLines), assoc(_assoc)  {
    array = gm_calloc<Address>(numLines);
    for (uint32_t i = 0; i < numLines; i++){
        array[i] = -1;
    }
    rp = new ProxyReplPolicy(this);
    numSets = numLines/assoc;
    setMask = numSets - 1;
    assert_msg(isPow2(numSets), "must have a power of 2 # sets, but you specified %d", numSets);
}

int32_t CDSetAssocArray::lookup(const Address lineAddr, const MemReq* req, bool updateReplacement) {
    uint32_t set = hf->hash(0, lineAddr) & setMask;
    uint32_t first = set*assoc;
    for (uint32_t id = first; id < first + assoc; id++) {
        if (array[id] ==  lineAddr) {
            // printf("lookup 0x%x, line %d\n",lineAddr, id);
            return id;
        }
    }
    // printf("lookup 0x%x, line -1\n",lineAddr);
    return -1;
}

uint32_t CDSetAssocArray::preinsert(const Address lineAddr, const MemReq* req, Address* wbLineAddr) { //TODO: Give out valid bit of wb cand?
    uint32_t set = hf->hash(0, lineAddr) & setMask;
    uint32_t first = set*assoc;

    uint32_t candidate = -1;
    for (uint32_t id = first; id < first + assoc; id++) {
        if (array[id] ==  -1) {
            candidate = id;
            break;
        }
    }
    assert(candidate != -1);
    *wbLineAddr = array[candidate];
    return candidate;
}

void CDSetAssocArray::postinsert(const Address lineAddr, const MemReq* req, uint32_t candidate) {
    array[candidate] = lineAddr;
    // printf("insert 0x%x, line %d\n",lineAddr, candidate);
}

void CDSetAssocArray::clear(uint32_t lineId){
    // printf("clear 0x%x, line %d\n",array[lineId], lineId);
    array[lineId] = -1;
}
