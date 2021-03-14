// Copyright (c) 2009-2011, Tor M. Aamodt, Wilson W.L. Fung, Ali Bakhoda
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Icnt_Wrapper.h"
#include <assert.h>
#include "globals.hpp"
#include "interconnect_interface.hpp"

icnt_create_p                icnt_create;
icnt_init_p                  icnt_init;
icnt_has_buffer_p            icnt_has_buffer;
icnt_push_p                  icnt_push;
icnt_pop_p                   icnt_pop;
icnt_top_p                   icnt_top;
icnt_transfer_p              icnt_transfer;
icnt_busy_p                  icnt_busy;
icnt_display_stats_p         icnt_display_stats;
icnt_display_overall_stats_p icnt_display_overall_stats;
icnt_display_state_p         icnt_display_state;
icnt_get_flit_size_p         icnt_get_flit_size;

// Wrapper to intersim2 to accompany old icnt_wrapper
// TODO: use delegate/boost/c++11<funtion> instead

static unsigned int links_per_cube;
static unsigned int vaults_per_cube;
static unsigned int num_subnets;

static void booksim2_create(unsigned int n_link, unsigned int n_vault)
{
   links_per_cube = n_link;
   vaults_per_cube = n_vault;
   g_icnt_interface->CreateInterconnect(n_link, n_vault);
   num_subnets = g_icnt_interface->GetSubNets();
}

static void booksim2_init()
{
   g_icnt_interface->Init();
}

static bool booksim2_has_buffer(unsigned subnet, unsigned input, unsigned int size)
{
   // if(num_subnets != num_cubes){
   //    subnet = 2*subnet;
   //    if(input >= links_per_cube)
   //       subnet = subnet + 1;
   // }
   return g_icnt_interface->HasBuffer(subnet, input, size);
}

static void booksim2_push(unsigned subnet, unsigned input, unsigned output, void* data, unsigned int size, bool is_request, bool is_read)
{
   // if(num_subnets != num_cubes){
   //    subnet = 2*subnet;
   //    if(input >= links_per_cube)
   //       subnet = subnet + 1;
   // }
   g_icnt_interface->Push(subnet, input, output, data, size, is_request, is_read);
}

static void* booksim2_pop(unsigned subnet, unsigned output)
{
   // if(num_subnets != num_cubes){
   //    subnet = 2*subnet;
   //    if(output < links_per_cube)
   //       subnet = subnet + 1;
   // }
   return g_icnt_interface->Pop(subnet, output);
}

static void* booksim2_top(unsigned subnet, unsigned output)
{
   // if(num_subnets != num_cubes){
   //    subnet = 2*subnet;
   //    if(output < links_per_cube)
   //       subnet = subnet + 1;
   // }
   return g_icnt_interface->Top(subnet, output);
}

static void booksim2_transfer()
{
   g_icnt_interface->Advance();
}

static bool booksim2_busy()
{
   return g_icnt_interface->Busy();
}

static void booksim2_display_stats()
{
   g_icnt_interface->DisplayStats();
}

static void booksim2_display_overall_stats()
{
   g_icnt_interface->DisplayOverallStats();
}

static void booksim2_display_state(FILE *fp)
{
   g_icnt_interface->DisplayState(fp);
}

static unsigned booksim2_get_flit_size()
{
   return g_icnt_interface->GetFlitSize();
}

void icnt_wrapper_init(unsigned n_subnet, const char* g_network_config_filename)
{
   //FIXME: delete the object: may add icnt_done wrapper
    g_icnt_interface = InterconnectInterface::New(n_subnet, g_network_config_filename);
    icnt_create     = booksim2_create;
    icnt_init       = booksim2_init;
    icnt_has_buffer = booksim2_has_buffer;
    icnt_push       = booksim2_push;
    icnt_pop        = booksim2_pop;
    icnt_top        = booksim2_top;
    icnt_transfer   = booksim2_transfer;
    icnt_busy       = booksim2_busy;
    icnt_display_stats = booksim2_display_stats;
    icnt_display_overall_stats = booksim2_display_overall_stats;
    icnt_display_state = booksim2_display_state;
    icnt_get_flit_size = booksim2_get_flit_size;
}
