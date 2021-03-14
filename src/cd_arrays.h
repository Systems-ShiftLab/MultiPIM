#ifndef CD_ARRAYS_H_
#define CD_ARRAYS_H_

#include "cache_arrays.h"
#include "repl_policies.h"

/* Set-associative cache array */
class CDSetAssocArray : public CacheArray {
    private:
        //We need a fake replpolicy and just want the CC...
        class ProxyReplPolicy : public ReplPolicy {
            private:
                CDSetAssocArray* a;
            public:
                explicit ProxyReplPolicy(CDSetAssocArray* _a) : a(_a) {}
                void setCC(CC* _cc) {a->setCC(cc);}

                void update(uint32_t id, const MemReq* req) {panic("!")}
                void replaced(uint32_t id) {panic("!!");}
                template <typename C> uint32_t rank(const MemReq* req, C cands) {panic("!!!");}
                void initStats(AggregateStat* parent) {}
                DECL_RANK_BINDINGS
        };
    protected:
        Address* array;
        ProxyReplPolicy* rp;
        CC* cc;
        HashFamily* hf;
        uint32_t numLines;
        uint32_t numSets;
        uint32_t assoc;
        uint32_t setMask;

    public:
        CDSetAssocArray(uint32_t _numLines, uint32_t _assoc, HashFamily* _hf);

        int32_t lookup(const Address lineAddr, const MemReq* req, bool updateReplacement);
        uint32_t preinsert(const Address lineAddr, const MemReq* req, Address* wbLineAddr);
        void postinsert(const Address lineAddr, const MemReq* req, uint32_t candidate);
        void clear(uint32_t lineId);

        ReplPolicy* getRP() const {return rp;}
        void setCC(CC* _cc) {cc = _cc;}
};

#endif  // CD_ARRAYS_H_
