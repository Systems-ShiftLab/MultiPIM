
#ifndef RANDOM_MEMORY_MAPPER_H_
#define RANDOM_MEMORY_MAPPER_H_

#include "g_std/g_vector.h"
#include "g_std/g_unordered_map.h"
#include "memory_hierarchy.h"
#include "rng.h"
#include "locks.h"

class RandomMemoryMapper : public MemoryMapper{
public:
    RandomMemoryMapper(uint64_t mem_capacity,uint32_t line_bits)
    {
        free_physical_pages_remaining = mem_capacity >> 12;
        free_physical_pages.resize(free_physical_pages_remaining, -1);
        memory_footprint = 0;
        physical_page_replacement = 0;
        lineBits = line_bits;
        futex_init(&addr_lock);
    }

    ~RandomMemoryMapper(){}

    void getPhyAddr(int procIdx, uint64_t vLineAddr, uint64_t &pLineAddr)
    {
        futex_lock(&addr_lock);
        bool page_exist = false;
        long virtual_page_number = vLineAddr >> (12 - lineBits);
        auto proc_map = page_translation.find(procIdx);
        if(page_translation.find(procIdx) != page_translation.end()){
            g_unordered_map<long, long>& vpn_ppn_map = page_translation[procIdx];
            if(vpn_ppn_map.find(virtual_page_number) != vpn_ppn_map.end())
                page_exist = true;
        }
        // page doesn't exist, so assign a new page
        // make sure there are physical pages left to be assigned
        if(!page_exist){
            memory_footprint += 1 << 12;
            uint64_t seed =  rng_seed((procIdx << 48) | vLineAddr);
            if (!free_physical_pages_remaining)
            {
                physical_page_replacement++;
                long phys_page_to_read = rng_next(seed) % free_physical_pages.size();
                assert(free_physical_pages[phys_page_to_read] != -1);
                page_translation[procIdx][virtual_page_number] = phys_page_to_read;
            }else{
                // assign a new page
                long phys_page_to_read = rng_next(seed) % free_physical_pages.size();
                // if the randomly-selected page was already assigned
                if (free_physical_pages[phys_page_to_read] != -1)
                {
                    long starting_page_of_search = phys_page_to_read;

                    do
                    {
                        // iterate through the list until we find a free page
                        // TODO: does this introduce serious non-randomness?
                        ++phys_page_to_read;
                        phys_page_to_read %= free_physical_pages.size();
                    } while ((phys_page_to_read != starting_page_of_search) && free_physical_pages[phys_page_to_read] != -1);
                }

                assert(free_physical_pages[phys_page_to_read] == -1);

                page_translation[procIdx][virtual_page_number] = phys_page_to_read;
                free_physical_pages[phys_page_to_read] = procIdx;
                --free_physical_pages_remaining;
            }
            // printf("Assign 0x%x <-> 0x%x\n",virtual_page_number,page_translation[procIdx][virtual_page_number]);
        }
        // SAUGATA TODO: page size should not always be fixed to 4KB
        pLineAddr = (page_translation[procIdx][virtual_page_number] << (12 - lineBits)) | (vLineAddr & ((1 << (12 - lineBits)) - 1));
        futex_unlock(&addr_lock);
    }

private:
    uint32_t lineBits; 
    uint64_t memory_footprint;
    uint64_t physical_page_replacement;
    long free_physical_pages_remaining;
    g_unordered_map<int, g_unordered_map<long, long>> page_translation;
    g_vector<long> free_physical_pages;
    lock_t addr_lock;

};

#endif //RANDOM_MEMORY_MAPPER_H_