#ifndef __ZSIM_HOOKS_H__
#define __ZSIM_HOOKS_H__

#include <stdint.h>
#include <stdio.h>

//Avoid optimizing compilers moving code around this barrier
#define COMPILER_BARRIER() { __asm__ __volatile__("" ::: "memory");}

//These need to be in sync with the simulator
#define ZSIM_MAGIC_OP_ROI_BEGIN         (1025)
#define ZSIM_MAGIC_OP_ROI_END           (1026)
#define ZSIM_MAGIC_OP_REGISTER_THREAD   (1027)
#define ZSIM_MAGIC_OP_HEARTBEAT         (1028)
#define ZSIM_MAGIC_OP_WORK_BEGIN        (1029) //ubik
#define ZSIM_MAGIC_OP_WORK_END          (1030) //ubik

#define ZSIM_MAGIC_OP_PIM_BLK_BEGIN    (1031)
#define ZSIM_MAGIC_OP_PIM_BLK_END      (1032)

#define ZSIM_MAGIC_OP_PIM_MP_BEGIN    (1033)
#define ZSIM_MAGIC_OP_PIM_MP_END      (1034)

#ifdef __x86_64__
#define HOOKS_STR  "HOOKS"
static inline void zsim_magic_op(uint64_t op) {
    COMPILER_BARRIER();
    __asm__ __volatile__("xchg %%rcx, %%rcx;" : : "c"(op));
    COMPILER_BARRIER();
}
#else
#define HOOKS_STR  "NOP-HOOKS"
static inline void zsim_magic_op(uint64_t op) {
    //NOP
}
#endif

static inline void zsim_roi_begin() {
    printf("[" HOOKS_STR "] ROI begin\n");
    zsim_magic_op(ZSIM_MAGIC_OP_ROI_BEGIN);
}

static inline void zsim_roi_end() {
    zsim_magic_op(ZSIM_MAGIC_OP_ROI_END);
    printf("[" HOOKS_STR  "] ROI end\n");
}

static inline void zsim_heartbeat() {
    zsim_magic_op(ZSIM_MAGIC_OP_HEARTBEAT);
}

static inline void zsim_work_begin() { zsim_magic_op(ZSIM_MAGIC_OP_WORK_BEGIN); }
static inline void zsim_work_end() { zsim_magic_op(ZSIM_MAGIC_OP_WORK_END); }

static inline void pim_blk_begin() {
    printf("[" HOOKS_STR "] PIM BLK begin\n");
    zsim_magic_op(ZSIM_MAGIC_OP_PIM_BLK_BEGIN);
}

static inline void pim_blk_end() {
    zsim_magic_op(ZSIM_MAGIC_OP_PIM_BLK_END);
    printf("[" HOOKS_STR  "] PIM BLK end\n");
}

static inline void pim_mp_begin() {
    printf("[" HOOKS_STR "] PIM MP begin\n");
    zsim_magic_op(ZSIM_MAGIC_OP_PIM_MP_BEGIN);
}

static inline void pim_mp_end() {
    zsim_magic_op(ZSIM_MAGIC_OP_PIM_MP_END);
    printf("[" HOOKS_STR "] PIM MP end\n");
}

#endif /*__ZSIM_HOOKS_H__*/
