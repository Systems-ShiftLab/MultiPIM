MultiPIM
====

MultiPIM is general purpose Processing-in-Memory (PIM) simulation framework that supports multiple memory stacks (e.g., multiple HMC cubes). MultiPIM consists of frontend and backend. The frontend mainly handles non-memory instructions and cache accesses, while the backend simulates the actual latency of memory requests issued from the frontend. Considering that a PIM system with multiple memory nodes can have hundreds of PIM cores, we implemented the frontend based on [ZSim](https://github.com/s5z/zsim) to support massive cores and achieve fast simulation. The backend is implemented based on [Ramulator](https://github.com/CMU-SAFARI/ramulator) and the HMC model from [ramulator-pim](https://github.com/CMU-SAFARI/ramulator-pim) to implement a 3D-stack memory.

If you use this simulator in your work, please cite:

[1] C. Yu, S. Liu, S. Khan, "MultiPIM: A Detailed and Configurable Multi-Stack Processing-In-Memory Simulator," in IEEE Computer Architecture Letters, doi: 10.1109/LCA.2021.3061905. [Link](https://ieeexplore.ieee.org/document/9362242)

Features
-----

MultiPIM supports following features:
* **Memory interconnection definition**
  
    MultiPIM generates memory interconnections from a user-defined XML configuration file. Users can define how each memory is connected with each other and CPU. For more details, please refer the MultiPIM paper.

* **Packet routing**

    To support multiple memory interconnections, there should be a scheme to route packets among memory nodes. Uers can either define their static packet routing rules in the XML configuration file or extend the "int getRouteLink(int src_node, int dst_node)" function in ramulator/MemoryTopology.cpp to support a dynamic routing algorithm.

* **Crossbar switch**

    The crossbar switch is implemented based on [Booksim2](https://github.com/booksim/booksim2), so that users can evaluate different crossbar NoCs flexiblely.

* **Virtual memory**
  
    MultiPIM supports virtual memory with different page sizes (e.g., 4KB page and 2MB page) following the Linux’s buddy memory management mechanism. The virtual memory system is implemented based on [HSCC](https://github.com/CGCL-codes/HSCC). Both the CPU and PIM cores in MultiPIM can be configured with translation lookaside buffers (TLBs), both instruction-TLB and data-TLB, and a page table walker (PTW). PTWs in the CPU cores are connected to the LLC to reduce the page-table-walk overhead; however, as there is no LLC in PIM cores, PTWs in PIM cores access memory directly. All PTW requests are sent to the backend and simulated as normal memory requests for better accuracy.

* **PIM-core coherence**
  
    The coherence problem arises as threads executing on different PIM cores may share data. As PIM cores are residing in different memory stacks, without a shared last-level cache (LLC), we implement a coherence directory (can be either shared or private) with the MESI protocol for PIM cores. MultiPIM supports both write-through and write-back cache policies. Users can have there specific coherence protocol by implementing the coherence interfaces.

* **PIM offloading interfaces**

    MultiPIM provides two kinds of offloading interfaces: PIM Multi-Processing interface and PIM Block interface. The PIM Multi-Processing interface is used to annotate the begin of multiple PIM tasks (e.g., OpenMP threads). The PIM Block interface is used to annotate a PIM code block in a single process/thread.
    
    1. PIM Multi-Processing interface
    ```
    pim_mp_begin()
    pim_mp_end()
    ```
    2. PIM Block interface
    ```
    pim_blk_begin()
    pim_blk_end()
    ```

* **CPU-PIM Co-simulation**

    THe current released version of MultiPIM only supports PIM-only and CPU-only simulation. We will release the CPU-PIM co-simulation in about a month.

Instructions
-----
Below instructions are based on Ubuntu18.04.

* To resolve all dependencies:
```
cd MultiPIM
sudo sh setup.sh
```
* Besides, MultiPIM requires a C++11 compiler (e.g., g++ >= 6), and boost should be installed. 

* To install MultiPIM:
```
cd MultiPIM
sh compile.sh opt
```

* Changing the `ZSIM_PATH` in env.sh to your actual path.

* To identify `PIM offload` regions, and wrap `PIM offload` regions with the following interfaces:
    1. Using the PIM Multi-Processing interface
    ```cpp
    #include "zsim-pim/misc/hooks/zsim_hooks.h"
    foo(){
        /*
        * zsim_roi_begin() and pim_mp_begin() must be included in a serial part of the code.
        */
    	zsim_roi_begin();//It must be included in a serial part of the code.
        pim_mp_begin();// Indicates the beginning of PIM offload region to simulate (hotspot).
        #pragma omp parallel for
        for(){
            ...
        }
    	pim_mp_end(); // Indicates the end of the PIM offload region to simulate.
        zsim_roi_end(); //zsim_roi_end() marks the end of the ROI. 
        /*
        * zsim_roi_end() and pim_mp_end()
        */
    	
    }
    ```
    1. Using the PIM Block interface
    ```cpp
    #include "zsim-pim/misc/hooks/zsim_hooks.h"
    foo(){
        /*
        * zsim_roi_begin() must be included in a serial part of the code.
        */
    	zsim_roi_begin(); //It must be included in a serial part of the code.
        pim_blk_begin();// Indicates the beginning of the PIM code block in a single process/thread to simulate (hotspot).
        ...
        for (i = 0; i < _PB_NX; i++)
        {
          tmp[i] = 0;
          for (j = 0; j < _PB_NY; j++){
            tmp[i] = tmp[i] + A[i][j] * x[j];
          }
        }
        ...
    	pim_blk_end(); // Indicates the end of the PIM code block to simulate.
        zsim_roi_end(); //zsim_roi_end() marks the end of the ROI. 
        /*
        * zsim_roi_end() and pim_blk_end()
        */
    	
    }
    ```
    These two kinds of interfaces cannot be nested. Please check the `tests/benchmarks` directory for more examples. Currently, the PIM Block interface is supported for single thread/process application but not fully supported for multithreading, which will be fully supported in the next release. 

* You may need to compile the test first:
```
cd tests/benchmarks/Polybench/
./compile.sh
```

* Setting configuration parameters:
1. Define memory network interconnections and packet routing rules, see the example in configs/ramulator/dragonfly_16mem_4link.xml. Users need to specify the actual path of the memory network configuration file to the parameter `topology_file` in ramulator's configuration file (e.g., configs/ramulator/MultiPIM-hmc-dragonfly.cfg).
   
2. Define ramulator configurations (e.g., configs/ramulator/MultiPIM-hmc-dragonfly.cfg). You may also need to define the crossbar swith NoC configurations (e.g., configs/ramulator/switch_network.cfg).
   
3. Define zsim configurations (e.g., configs/zsim/pim_normal_tlb_dragonfly.cfg). Make sure to set `sys.mem.ramulatorConfig` and `sys.mem.hmcSwitchConfig` to your actual paths.

4. Enable PIM mode or just host mode:
```
sys.enablePIMMode
```
To simulate a PIM architecture, the PIM core type should set to "PIM".

* To launch a test run:
```
cd tests
./run-pim.sh
```

Support or Contact
-----
MultiPIM is developed by Chao Yu. For any questions, please contact Chao Yu (yuchaocs@gmail.com).