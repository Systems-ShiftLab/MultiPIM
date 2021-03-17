#ifndef __MEMORY_FACTORY_H
#define __MEMORY_FACTORY_H

#include <map>
#include <string>
#include <cassert>

#include "Config.h"
#include "Memory.h"
#include "HMC_Controller.h"
#include "HMC_Memory.h"

#include "WideIO2.h"
#include "SALP.h"

using namespace std;

namespace ramulator
{

template <typename T>
class MemoryFactory {
public:
    static void extend_channel_width(T* spec, int cacheline)
    {
        int channel_unit = spec->prefetch_size * spec->channel_width / 8;
        int gang_number = cacheline / channel_unit;
        
        assert(gang_number >= 1 && 
            "cacheline size must be greater or equal to minimum channel width");
        
        assert(cacheline == gang_number * channel_unit &&
            "cacheline size must be a multiple of minimum channel width");
        
        spec->channel_width *= gang_number;
    }

    static Memory<T> *populate_memory(const Config& configs, T *spec, int channels, int ranks) {
        int& default_ranks = spec->org_entry.count[int(T::Level::Rank)];
        int& default_channels = spec->org_entry.count[int(T::Level::Channel)];

        if (default_channels == 0) default_channels = channels;
        if (default_ranks == 0) default_ranks = ranks;

        vector<Controller<T> *> ctrls;
        for (int c = 0; c < channels; c++){
            DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
            channel->id = c;
            channel->regStats("");
            ctrls.push_back(new Controller<T>(configs, channel));
        }
        return new Memory<T>(configs, ctrls);
    }

    static Memory<HMC,Controller> *populate_memory(const Config& configs, HMC *spec) {
        int V = spec->org_entry.count[int(HMC::Level::Vault)];
        int S = configs.get_stacks();
        int total_vault_number = V * S;
        debug_hmc("total_vault_number: %d\n", total_vault_number);
        std::vector<Controller<HMC>*> vault_ctrls;
        for (int c = 0 ; c < total_vault_number ; ++c) {
            DRAM<HMC>* vault = new DRAM<HMC>(spec, HMC::Level::Vault);
            vault->id = c;
            vault->regStats("");
            Controller<HMC>* ctrl = new Controller<HMC>(configs, vault);
            vault_ctrls.push_back(ctrl);
        }
        Config * tempConfig = const_cast<Config *>(&configs);
        if(configs.pim_mode_enabled())
            tempConfig->set_pu_core_num(spec->org_entry.count[int(HMC::Level::Vault)] * S);
        tempConfig->set_vaults_per_stack(spec->org_entry.count[int(HMC::Level::Vault)]);
        return new Memory<HMC, Controller>(configs, vault_ctrls);
    }

    static void validate(int channels, int ranks, const Config& configs) {
        assert(channels > 0 && ranks > 0);
    }

    static MemoryBase *create(Config& configs, int cacheline)
    {
        int channels = stoi(configs["channels"], NULL, 0);
        int ranks = stoi(configs["ranks"], NULL, 0);
        
        validate(channels, ranks, configs);

        const string& org_name = configs["org"];
        const string& speed_name = configs["speed"];

        T *spec = new T(org_name, speed_name);

        configs.set_cacheline_size(cacheline);

        if (configs.contains("extend_channel_width") &&
            configs["extend_channel_width"] == "true") {
          extend_channel_width(spec, cacheline);
        }

        return (MemoryBase *)populate_memory(configs, spec, channels, ranks);
    }
};

template <>
MemoryBase *MemoryFactory<WideIO2>::create(Config& configs, int cacheline);
template <>
MemoryBase *MemoryFactory<SALP>::create(Config& configs, int cacheline);
template <>
MemoryBase *MemoryFactory<HMC>::create(Config& configs, int cacheline);

} /*namespace ramulator*/

#endif /*__MEMORY_FACTORY_H*/
