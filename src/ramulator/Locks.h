#ifndef __RAMULATOR_LOCKS_H
#define __RAMULATOR_LOCKS_H

namespace ramulator {


#include <linux/futex.h>
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>
#include <assert.h>
#include <xmmintrin.h>  // NOLINT

typedef volatile uint32_t lock_t;

static inline void ramulator_futex_init(volatile uint32_t* lock) {
    *lock = 0;
    __sync_synchronize();
}

/* NOTE: The current implementation of this lock is quite unfair. Not that we care for its current use. */
static inline void ramulator_futex_lock(volatile uint32_t* lock) {
    uint32_t c;
    do {
        for (uint32_t i = 0; i < 5; i++) { //this should be tuned to balance syscall/context-switch and user-level spinning costs
            if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, 1)) {
                return;
            }

            // Do linear backoff instead of a single mm_pause; this reduces ping-ponging, and allows more time for the other hyperthread
            for (uint32_t j = 1; j < i+2; j++) _mm_pause();
        }

        //At this point, we will block
        c = __sync_lock_test_and_set(lock, 2); //this is not exactly T&S, but atomic exchange; see GCC docs
        if (c == 0) return;
        syscall(SYS_futex, lock, FUTEX_WAIT, 2, nullptr, nullptr, 0);
        c = __sync_lock_test_and_set(lock, 2); //atomic exchange
    } while (c != 0);
}

static inline void ramulator_futex_unlock(volatile uint32_t* lock) {
    if (__sync_fetch_and_add(lock, -1) != 1) {
        *lock = 0;
        /* This may result in additional wakeups, but avoids completely starving processes that are
         * sleeping on this. Still, if there is lots of contention in userland, this doesn't work
         * that well. But I don't care that much, as this only happens between phase locks.
         */
        syscall(SYS_futex, lock, FUTEX_WAKE, 1 /*wake next*/, nullptr, nullptr, 0);
    }
}

static inline void ramulator_futex_lock_nospin(volatile uint32_t* lock) {
    uint32_t c;
    do {
        if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, 1)) {
            return;
        }

        //At this point, we will block
        c = __sync_lock_test_and_set(lock, 2); //this is not exactly T&S, but atomic exchange; see GCC docs
        if (c == 0) return;
        syscall(SYS_futex, lock, FUTEX_WAIT, 2, nullptr, nullptr, 0);
        c = __sync_lock_test_and_set(lock, 2); //atomic exchange
    } while (c != 0);
}

} /* namespace ramulator */
#endif /*__LOCKS_H*/