
#include "coherence_directory.h"
#include "timing_event.h"
#include "bithacks.h"
#include "network.h"
#include "zsim.h"


uint32_t CDMESIBottomCC::getParentId(Address lineAddr) {
    //Hash things a bit
    uint32_t res = 0;
    uint64_t tmp = lineAddr;
    for (uint32_t i = 0; i < 4; i++) {
        res ^= (uint32_t) ( ((uint64_t)0xffff) & tmp);
        tmp = tmp >> 16;
    }
    return (res % parents.size());
}

uint32_t CDMESIBottomCC::getParentId(uint32_t srcId, Address lineAddr) {
    return srcId % parents.size();
}

void CDMESIBottomCC::init(const g_vector<MemObject*>& _parents, Network* network, const char* name) {
    parents.resize(_parents.size());
    parentRTTs.resize(_parents.size());
    for (uint32_t p = 0; p < parents.size(); p++) {
        parents[p] = _parents[p];
        parentRTTs[p] = (network)? network->getRTT(name, parents[p]->getName()) : 0;
    }
}

uint64_t CDMESIBottomCC::processWriteBack(Address lineAddr, AccessType type, uint64_t cycle, uint32_t srcId, uint32_t coreId, uint32_t threadId, bool isPIMInst){
    assert(type == PUTS || type == PUTX);
    uint64_t respCycle = cycle;
    uint32_t parentId = getParentId(srcId,lineAddr);
    MESIState state = MESIState::I;
    MemReq req = {lineAddr, type, selfId, &state, cycle, &ccLock, state, srcId, 0, coreId};
    req.threadId = threadId;
    req.isPIMInst = isPIMInst;
    uint32_t nextLevelLat = parents[parentId]->access(req) - cycle;
    uint32_t netLat = parentRTTs[parentId];
    profGETNextLevelLat.inc(nextLevelLat);
    profGETNetLat.inc(netLat);
    respCycle += nextLevelLat + netLat;
    return respCycle;
}

uint64_t CDMESIBottomCC::processAccess(Address lineAddr, uint32_t lineId, AccessType type, uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId, uint32_t threadId, bool isPIMInst) {
    uint64_t respCycle = cycle;
    switch (type) {
        case GETS:
            {
                uint32_t parentId = getParentId(srcId,lineAddr);
                MESIState state = MESIState::I;
                MemReq req = {lineAddr, GETS, selfId, &state, cycle, &ccLock, state, srcId, flags,  coreId};
                req.threadId = threadId;
                req.isPIMInst = isPIMInst;
                uint32_t nextLevelLat = parents[parentId]->access(req) - cycle;
                uint32_t netLat = parentRTTs[parentId];
                profGETNextLevelLat.inc(nextLevelLat);
                profGETNetLat.inc(netLat);
                respCycle += nextLevelLat + netLat;
            }
            break;
        case GETX:
            {
                uint32_t parentId = getParentId(srcId,lineAddr);
                MESIState state = MESIState::I;
                MemReq req = {lineAddr, GETX, selfId, &state, cycle, &ccLock, state, srcId, flags,  coreId};
                req.threadId = threadId;
                req.isPIMInst = isPIMInst;
                uint32_t nextLevelLat = parents[parentId]->access(req) - cycle;
                uint32_t netLat = parentRTTs[parentId];
                profGETNextLevelLat.inc(nextLevelLat);
                profGETNetLat.inc(netLat);
                respCycle += nextLevelLat + netLat;
            }
            break;

        default: panic("!?");
    }
    assert_msg(respCycle >= cycle, "XXX %ld %ld", respCycle, cycle);
    return respCycle;
}

void CDMESIBottomCC::processInval(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback) {
    //NOTE: BottomCC never calls up on an invalidate, so it adds no extra latency
}

/* CDMESITopCC implementation */

void CDMESITopCC::init(const g_vector<BaseCache*>& _children, Network* network, const char* name) {
    if (_children.size() > MAX_CACHE_CHILDREN) {
        panic("[%s] Children size (%d) > MAX_CACHE_CHILDREN (%d)", name, (uint32_t)_children.size(), MAX_CACHE_CHILDREN);
    }
    children.resize(_children.size());
    childrenRTTs.resize(_children.size());
    for (uint32_t c = 0; c < children.size(); c++) {
        children[c] = _children[c];
        childrenRTTs[c] = (network)? network->getRTT(name, children[c]->getName()) : 0;
    }
    cache_children_num = children.size();
}

uint64_t CDMESITopCC::sendAllInvalidates(Address lineAddr, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId){
    uint64_t maxCycle = cycle; //keep maximum cycle only, we assume all invals are sent in parallel
    uint32_t numChildren = children.size();
    for (uint32_t c = 0; c < numChildren; c++) {
        if(srcId == c)
            continue;
        InvReq req = {lineAddr, type, reqWriteback, cycle, srcId};
        uint64_t respCycle = children[c]->invalidate(req);
        respCycle += childrenRTTs[c];
        maxCycle = MAX(respCycle, maxCycle);
        sentInvs.inc();
    }
    return maxCycle;
}

uint64_t CDMESITopCC::sendInvalidates(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId, bool is_clflush) {
    //Send down downgrades/invalidates
    if(lineId == -1){
        // assert(is_clflush);
        // This level don't have the dirty line, but lower levels may have
        uint64_t maxCycle = cycle; //keep maximum cycle only, we assume all invals are sent in parallel
        for (uint32_t c = 0; c < cache_children_num; c++) {
            InvReq req = {lineAddr, type, reqWriteback, cycle, srcId, is_clflush};
            uint64_t respCycle = children[c]->invalidate(req);
            respCycle += childrenRTTs[c];
            maxCycle = MAX(respCycle, maxCycle);
        }
        return maxCycle;
    }
    Entry* e = &array[lineId];

    //Don't propagate downgrades if sharers are not exclusive.
    if (type == INVX && !e->isExclusive()) {
        return cycle;
    }

    uint64_t maxCycle = cycle; //keep maximum cycle only, we assume all invals are sent in parallel
    if (!e->isEmpty()) {
        // uint32_t numChildren = children.size();
        uint32_t numSentInvs = 0;
        // for (uint32_t c = 0; c < numChildren; c++) {
        for (uint32_t c = 0; c < cache_children_num; c++) {
            if (e->sharers[c]) {
                InvReq req = {lineAddr, type, reqWriteback, cycle, srcId, is_clflush};
                uint64_t respCycle = children[c]->invalidate(req);
                respCycle += childrenRTTs[c];
                maxCycle = MAX(respCycle, maxCycle);
                if (type == INV) e->sharers[c] = false;
                numSentInvs++;
                sentInvs.inc();
            }
        }
        assert(numSentInvs == e->numSharers);
        if (type == INV) {
            e->numSharers = 0;
        } else {
            //TODO: This is kludgy -- once the sharers format is more sophisticated, handle downgrades with a different codepath
            assert(e->exclusive);
            assert(e->numSharers == 1);
            e->exclusive = false;
        }
    }
    return maxCycle;
}

uint64_t CDMESITopCC::processWriteBack(Address lineAddr, uint32_t lineId, AccessType type, uint32_t childId, MESIState *childState,  bool* evictEntry, uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId){
    Entry *e = &array[lineId];
    uint64_t respCycle = cycle;
    switch (type)
    {
        case PUTX:
            assert(e->isExclusive());
            if (flags & MemReq::PUTX_KEEPEXCL)
            {
                assert(e->sharers[childId]);
                assert(*childState == M);
                *childState = E; //they don't hold dirty data anymore
                break;           //don't remove from sharer set. It'll keep exclusive perms.
            }
            //note NO break in general
        case PUTS:
            assert(e->sharers[childId]);
            e->sharers[childId] = false;
            e->numSharers--;
            *childState = I;
            break;
        default:
            panic("!?");
    }
    if(e->isEmpty()){
        e->clear();
        *evictEntry = true;
    }
    return respCycle;
}
uint64_t CDMESITopCC::processAccess(Address lineAddr, uint32_t lineId, AccessType type, uint32_t childId, 
                                  MESIState* childState, bool* inducedWriteback, uint64_t cycle, uint32_t srcId, uint32_t flags, uint32_t coreId) {
    Entry* e = &array[lineId];
    uint64_t respCycle = cycle;
    switch (type) {
        case GETS:
            if (e->isEmpty()  && !(flags & MemReq::NOEXCL)) {
                //Give in E state
                e->exclusive = true;
                e->sharers[childId] = true;
                e->numSharers = 1;
                *childState = E;
            } else {
                //Give in S state
                assert(e->sharers[childId] == false);

                if (e->isExclusive()) {
                    //Downgrade the exclusive sharer
                    respCycle = sendInvalidates(lineAddr, lineId, INVX, inducedWriteback, cycle, srcId);
                    profINVX.inc();
                }

                assert_msg(!e->isExclusive(), "Can't have exclusivity here. isExcl=%d excl=%d numSharers=%d", e->isExclusive(), e->exclusive, e->numSharers);

                e->sharers[childId] = true;
                e->numSharers++;
                e->exclusive = false; //dsm: Must set, we're explicitly non-exclusive
                *childState = S;

                profGETSCache.inc();
            }
            break;
        case GETX:
            // If child is in sharers list (this is an upgrade miss), take it out
            if (e->sharers[childId]) {
                assert_msg(!e->isExclusive(), "Spurious GETX, childId=%d numSharers=%d isExcl=%d excl=%d", childId, e->numSharers, e->isExclusive(), e->exclusive);
                e->sharers[childId] = false;
                e->numSharers--;
            }

            // Invalidate all other copies
            respCycle = sendInvalidates(lineAddr, lineId, INV, inducedWriteback, cycle, srcId);

            // Set current sharer, mark exclusive
            e->sharers[childId] = true;
            e->numSharers++;
            e->exclusive = true;

            assert(e->numSharers == 1);

            *childState = M; //give in M directly

            profINV.inc();
            break;

        default: panic("!?");
    }

    return respCycle;
}

uint64_t CDMESITopCC::processInval(Address lineAddr, uint32_t lineId, InvType type, bool* reqWriteback, uint64_t cycle, uint32_t srcId) {
    if (type != FWD) {//if it's a FWD, we should be inclusive for now, so we must have the line, just invLat works
        assert(!nonInclusiveHack); //dsm: ask me if you see this failing and don't know why
        return cycle;
    } else {
        //Just invalidate or downgrade down to children as needed
        return sendInvalidates(lineAddr, lineId, type, reqWriteback, cycle, srcId);
    }
}

bool CDMESICC::startAccess(MemReq &req){
    assert((req.type == GETS) || (req.type == GETX) || (req.type == PUTS) || (req.type == PUTX));

    /* Child should be locked when called. We do hand-over-hand locking when going
             * down (which is why we require the lock), but not when going up, opening the
             * child to invalidation races here to avoid deadlocks.
             */
    if (req.childLock)
    {
        futex_unlock(req.childLock);
    }

    tcc->lock(); //must lock tcc FIRST
    bcc->lock();

    /* The situation is now stable, true race-wise. No one can touch the child state, because we hold
             * both parent's locks. So, we first handle races, which may cause us to skip the access.
             */
    bool skipAccess;
    if (zinfo->cacheWritePolicy == WritePolicy::WRITETHROUGH)
        skipAccess = false;
    else
        skipAccess = CheckForMESIRace(req.type /*may change*/, req.state, req.initialState);
    return skipAccess;
}

CoherenceDirectory::CoherenceDirectory(uint32_t _numLines, CC* _cc, CacheArray* _array, ReplPolicy* _rp, bool _idealCoherence, uint32_t _accLat, uint32_t _invLat, const g_string& _name):
    Cache(_numLines, _cc, _array, _rp, _accLat, _invLat, _name)
{
    idealCoherence = _idealCoherence;
}


uint64_t CoherenceDirectory::access(MemReq &req){
    uint64_t respCycle = req.cycle;
    if(zinfo->nonCacheable || req.nonCacheable){
        respCycle = cc->bypassAccess(req);
        return respCycle;
    }
    bool skipAccess = cc->startAccess(req); //may need to skip access due to races (NOTE: may change req.type!)
    if (likely(!skipAccess)) {
        if(req.is(MemReq::Flag::IFETCH) || req.is(MemReq::Flag::PTW)){
            //Just forward requests
            respCycle = cc->bypassAccess(req);
        }else if(zinfo->cacheWritePolicy == WritePolicy::WRITETHROUGH){
            assert((req.type == GETS) || (req.type == PUTX));
            respCycle = cc->processWriteThroughAccess(req);
        }else if(!idealCoherence){
            // printf("CoherenceDirectory addrline 0x%x\n",req.lineAddr);
            bool updateReplacement = (req.type == GETS) || (req.type == GETX);
            int32_t lineId = array->lookup(req.lineAddr, &req, updateReplacement);
            respCycle += accLat;

            if((req.type == PUTS) || (req.type == PUTX)){
                if(lineId == -1){
                    //instruction eviction
                    respCycle = cc->bypassAccess(req);
                }else{
                    bool evictEntry = false;
                    respCycle = cc->processWriteBack(req, lineId, respCycle, &evictEntry);
                    if(evictEntry){
                        array->clear(lineId);
                    }
                }
            }else{
                if (lineId == -1 && cc->shouldAllocate(req)){
                    //Make space for new line
                    Address wbLineAddr;
                    lineId = array->preinsert(req.lineAddr, &req, &wbLineAddr); //find the lineId to replace
                    trace(Cache, "[%s] Evicting 0x%lx", name.c_str(), wbLineAddr);

                    array->postinsert(req.lineAddr, &req, lineId); //do the actual insertion. NOTE: Now we must split insert into a 2-phase thing because cc unlocks us.
                }

                respCycle = cc->processAccess(req, lineId, respCycle);
            }
        }else{
            respCycle = cc->bypassAccess(req);
        }
        switch (req.type) {
            case PUTX:
                profPUTX.inc();
                break;
            case PUTS:
                profPUTS.inc();
                break;
            case GETX:
                profGETX.inc();
                break;
            case GETS:
                profGETS.inc();
                break;
            default: panic("!?");
        }
    }

    cc->endAccess(req);

    assert_msg(respCycle >= req.cycle, "[%s] resp < req? 0x%lx type %s childState %s, respCycle %ld reqCycle %ld",
            name.c_str(), req.lineAddr, AccessTypeName(req.type), MESIStateName(*req.state), respCycle, req.cycle);
    return respCycle;
}

void CoherenceDirectory::startInvalidate() {
    cc->startInv(); //note we don't grab tcc; tcc serializes multiple up accesses, down accesses don't see it
}

uint64_t CoherenceDirectory::finishInvalidate(const InvReq& req) {
    int32_t lineId = array->lookup(req.lineAddr, nullptr, false);
    // if(!req.clflush && zinfo->cacheWritePolicy == WritePolicy::WRITEBACK){
    //     assert_msg(lineId != -1, "[%s] Invalidate on non-existing address 0x%lx type %s lineId %d, reqWriteback %d", name.c_str(), req.lineAddr, InvTypeName(req.type), lineId, *req.writeback);
    // }
    uint64_t respCycle = req.cycle + invLat;
    if(lineId != -1)
        trace(Cache, "[%s] Invalidate start 0x%lx type %s lineId %d, reqWriteback %d", name.c_str(), req.lineAddr, InvTypeName(req.type), lineId, *req.writeback);
    // printf("Coming here3\n");
    respCycle = cc->processInv(req, lineId, respCycle); //send invalidates or downgrades to children, and adjust our own state
    if(lineId != -1)
        trace(Cache, "[%s] Invalidate end 0x%lx type %s lineId %d, reqWriteback %d, latency %ld", name.c_str(), req.lineAddr, InvTypeName(req.type), lineId, *req.writeback, respCycle - req.cycle);
    // printf("Coming here4\n");
    return respCycle;
}

uint64_t CoherenceDirectory::invalidate(const InvReq &req)
{
    // printf("Coming here1\n");
    startInvalidate();
    // printf("Coming here2\n");
    return finishInvalidate(req);
}