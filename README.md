# RISC-V Simulator

### Phase 1: Multi-Core RISC-V Simulator
A RISC-V simulator was developed to execute assembly programs using a multi-core architecture. The simulator supports 4 cores, each assigned a dedicated 1KB memory segment. It reads assembly instructions from a file and executes them sequentially for each core. Each core executes independently, with no inter-core communication.

### Phase 2: Pipelined Execution
In this phase, instruction pipelining was introduced to improve performance. The pipeline supports:
* Variable latencies for arithmetic instructions.
* Core-dependent branch instructions, allowing cores to conditionally execute based on their identity.
* A shared instruction fetch unit with independent decode, execute, memory, and write-back stages for each core.
* Optional data forwarding and stall detection logic.
* Final statistics include stalls and Instructions Per Cycle (IPC).

### Phase 3: Cache Hierarchy
#### A multi-level cache system was implemented:
* Each core includes a private L1 instruction (L1I) and L1 data (L1D) cache.
* A shared unified L2 data cache and L2 instruction cache connects all cores.
* Separate instruction memory and data memory.
* Cache configuration (size, associativity, block size, latency) is provided via an external file- `Specifications.txt`.
* A complete write through Cache 
* Supports Least Recently Used (LRU) and Most recently used (MRU) policies .
* With variable memory latencies for the Instruction memory accesses and Data accesses depening on the hit or miss of the cache 
* Tracks cache hit/miss rates, stall cycles, and performance metrics.

#### SYNC Instruction
A special instruction SYNC was introduced, acting as a no-op (NOP) that enforces synchronization across all cores. Execution stalls at SYNC until all cores reach this instruction, ensuring consistent execution points in parallel programs.
HoThe SYNC instruction is implemented to simulate a barrier synchronization across multiple cores in hardware. This ensures that all cores reach the same program counter (PC) value before continuing execution. Here's how it works:

##### IMPLEMENTATION DETAILS:
* Each core maintains a 1-bit barrier flag (e.g., sync_bit[i] for Core i).
* A shared hardware mechanism (or a synchronization controller) constantly monitors the PC values of all cores.
* When a core encounters the SYNC instruction:
* It first checks the PC of the core which is different from others
* It sets its barrier bit to 1.
* It enters a stall state for the specific, waiting for other cores to reach the same pc .
* The synchronization controller checks whether all PC values are equal (i.e., all cores have reached the same SYNC instruction).
* Once all barrier bits are set (i.e., all cores reached the same PC):
* The controller resets all barrier bits to 0.
* All cores are unstalled and allowed to continue execution.

#### Scratch Pad Memory:
* This memory has the same access latency and size as the L1D cache.
* SPM is explicitly controlled by the programmer. The system does not automatically move data between main memory and SPM.
* Every core has a core specific Scratch Pad Memory 
* SPM is not coherent with main memory and must be managed manually if synchronization is needed.
* It can implement the instructions `lw_spm` which means load word from Scratch Pad Memory and `sw_spm` which means Store word to Scratch Pad Memory .
### Part 1:
##### Without SPM:
* Strided access: a[0], a[X], a[2X], ..., a[99X]
* X = 100 → so access pattern: a[0], a[100], a[200], ..., a[9900]
* In a direct-mapped cache, all of these addresses map to the same cache set, causing 100% conflict misses (only 1 block is used).
* Every access is a miss with an approx of 70k extra cycles.
##### With SPM:
* First 1000 cycles are misses rest of them are hit 
* with an approx increase of 10x in the IPC .
### Part 2:
* All accessed elements can fit into the cache simultaneously.
* Generally there's not much difference observed in the IPC with and without SPM.
* But my simulator shosws a bit of difference not sure if it's because of loop holes in the simulator.
### Part 3:
1. Scratchpad memories (SPMs) are more efficient and less complex than caches since they do not involve complex hardware mechanisms like tag comparison, cache check, or replacement policies. Caches need to check for hits or misses, decide which line to replace (in case of conflicts), and ensure coherence — all of which take time and power. SPMs are  addressed and fully managed by the programmer, which makes their access easy and quicker. Since no ther's no complex logic for data searching or replacing is present, SPMs can be built with lower access latency and much lowered power consumption. 
2. Let’s analyze why using SPM is advantageous in this specific scenario:When accessing 200 elements in a strided pattern (i.e., a[0], a[X], a[2X], ..., a[199X]), the L1D cache (400 bytes = 100 words) cannot hold all 200 strided elements at once. Due to the strided access,  direct-mapped or even fully associative caches, cache evictions will occur frequently as new strided elements replace older ones. This leads to repeated cache misses, reloading data from memory multiple times across the 100 outer iterations (count loop), which introduces performance overhead.
3. With SPM, the programmer  preloads the first 100 needed values into the scratchpad memory using sw_spm, and then repeatedly reuses them across 200 iterations using lw_spm. While the second 100 strided elements (indices 100–199) are still accessed from main memory, the first 100 accesses are served without any eviction or reloading overhead, because SPM access is predictable and under full programmer control.
4. All the cases have resulted in better performance of the SPM.
---
## KEY TAKEAWAYS FROM IMPLEMENTED PHASE 3
### 1.How It Works:
All the specifications must be listed in a file named `Specifictaions.txt` in the below manner.
```cpp==
Specifications{
    data_forwarding:1 
    replacement_policy:MRU
    line_size:16
    L1_cache_size:32
    L2_cache_size:1024
    L1_associativity:2
    L2_associativity:4
    L1_latency:1
    L2_latency : 4
    Main_memory:7
    [Arithmetic variable latencies:
    addi=3
    ]  
}

```
Inputs from this file are taken to intialise the Memory system of the simulator the TOTAL MEMORY SIZE and INSTRUCTION MEMORY SIZE will remain contant i.e 4096 and 1025B respecitively.

---
## KEY TAKEAWAYS FROM IMPLEMENTED PHASE 2
### 1.How It Works:
1. Provides an option to enable or disable data forwarding.
2. Parses the entire `assembly.asm` file, storing the contents of the .data section and labels.
3. Asks for the number of ALU instructions with variable latencies.
4. Instructions are **case-sensitive**.
5. Executes the program and displays:
          1. Clock cycles
          2.  Stalls
          3. IPC (Instructions Per Cycle) for each core
          4.  Final 4KB memory state

### 2. Assumptions:
1.  Due to the lack of certain details, the following assumptions are made:
#### Data Dependency Handling:
* If a data dependency occurs in memory instructions (lw or sw), the pipeline stalls.
* The dependent instruction receives data from the MEM/WB stage of the memory instruction.
#### Program Counter (PC) & Fetch Unit Behavior:
* The CID mechanism described in the specification suggests that each core may skip certain instructions.
* Each core maintains a separate PC, as a single fetch unit cannot fetch different instructions for different cores simultaneously.
* The fetch unit executes once per cycle but fetches for all four cores.
#### Array Sum Example Execution:
* Each core calculates a partial sum, storing it in its allocated memory segment.
* Core 1 executes sum_loop to compute the final sum.
* The sample assembly code used for testing is attached.
## Implemented RISC-V Instructions
The following **RISC-V instructions** are implemented in our simulator:
### 1. Arithmetic Instructions
- `add` - Adds two registers.
- `sub` - Subtracts two registers.
- `mul` - Multiplies two registers.
- `addi` - Adds an immediate value to a register.
- `slli` - Shifts a register left by an immediate value.
- `and` - r1&r2
- `or` - r1 || r2
- `xor` - r1 ^r2

### 2. Branch and Jump Instructions
- `bne` - Branch if registers are not equal along with the core specific branch conditions (i.e CID).
- `blt` - Branch if one register is less than another.
- `beq` - Branch if registers are equal along with the core specific branch conditions (i.e CID).
- `jal` - Jump and link (function call).
- `j` - Unconditional jump.

### 3. Memory Access Instructions
- `lw` - Load word (loads from memory into a register).
- `sw` - Store word (stores a register value into memory).

### 4. Data Transfer Instructions
- `li` - Load an immediate value into a register.
- `la` - Load the address of a label into a register.

### 5. System Call Instruction
- `ecall` - Used for system exit (if register `x17 == 10`, program terminates).

### 6. No-Operation Instruction
- `nop` - Does nothing (used for alignment or delays).

---
## Minutes of Meeting 
# RISC-V Simulator Development (PHASE - 3)
---
## 12th May
 **Saranya:** Implemented the Scratch Pad Memory (SPM) system, including support for variable access latencies.
 
 **Binnu:** Assisted in preparing the README, report documentation, and test cases for the Scratch Pad Memory module.

## 11th May
 **Saranya:** Designed, implemented, and rigorously tested the `SYNC` function, which synchronizes all cores at a defined execution point.
 
**Binnu:** Contributed to the design and refinement of the SYNC function.

## 10th May 
 **Saranya:**
* Implemented the Most Recently Used (LRU) cache replacement policy.
* Added support for configurable data cache sizes based on user input.
* Completed the instruction and data caches.
  
**Binnu:** Completed the implementation of data caches using the Least Recently Used (MRU) replacement policy , tried implementing varaible fetch latencies .
## 9th May
**Saranya:** Implemented the designs for instruction and data caches .

**Binnu:** Implmented varaible cache latencies for data caches and helped with instruction cache.
## 8th May
**Saranya & Binnu**: Finalized the design for the complete simulator, including the memory systems. All previously identified flaws were addressed and corrected.
## 7th May
**Saranya:** Designed the initial architecture for the memory system.
## Minutes of Meeting 
# RISC-V Simulator Development (PHASE - 2)
---
## **10th March**
**DESIGN DECISION**:Realised the single fetch unit would not work , reason is written in the key take away.
- **Saranya:** Implemented the fetch in a different way handling the CID condition , tested the array sum and fixed the bugs .

- **Binnu:** Adapted branch and jump instructions to accommodate the new single fetch unit, ensuring proper instruction fetching across all cores.

## **9th March**
- **Saranya:** Started implementing the single fetch unit for all the compute units along with the given CID condition .
- **Binnu:** Adapted branch and jump instructions to accommodate the new single fetch unit, ensuring proper instruction fetching across all cores.

## **7th - 8th March**
- **Saranya:** Attempted to implement Bubble Sort, but encountered errors in memory dependencies and corrected the data dependencies on memory instructions.
  - Fixed the errors in varaible latenices like : values getting lost after the nth execution of the the instruction.
- **Binnu:** Experimented with different `.asm` files and verified clock cycle correctness to ensure accurate instruction execution.

 ## **5th March**
- **Saranya:** Implemented variable latencies and introduced user input functionality. Due to design changes, stalls couldn't be implemented yet.
- **Binnu:**  Implemented the shift stages function and enabled stalls within it, providing better hazard handling.

## **3rd March**
- **Saranya:** Re-enabled data forwarding after design modifications to optimize instruction flow.
- **Binnu:** Continued fixing bugs to ensure correct execution and data propagation.

## **2nd March**
**DESIGN DECISION**:The initial design assigned registers to each stage, which did not allow data forwarding to function smoothly. To address this, the registers were changed to pipeline registers that are shared between two stages, enabling efficient data forwarding.
- **Saranya:** Realized the initial pipeline design had fundamental flaws and restructured it to have a dedicated pipeline for each stage.
- **Binnu:** Conducted thorough checks on data dependencies to refine pipeline execution.

## **1st March**
- **Saranya:** Integrated data forwarding to minimize stalls and improve performance.
- **Binnu:** Debugging and refining the implementation to ensure correctness and stability.

## **28th February**
- **Saranya:** Initially implemented registers for each stage in the pipeline.
- **Binnu:** Implemented stalls to handle pipeline hazards, ensuring smooth execution flow.

---

## Conclusion
Significant progress has been made in refining the RISC-V simulator, from pipeline structuring to advanced optimizations like data forwarding, variable latencies, and hazard handling. With ongoing debugging and refinements, the project is steadily progressing toward a more robust and efficient simulation model.

---

# RISC-V Simulator Development (PHASE - 1) 

## **February 20**  
### **Final Fixes:**  
✅ Implemented all required instructions.  
✅ Loops executed correctly.  

#### **Saranya (Final Fixes):**  
- **Problem in memory allocation of cores:** All cores accessed the same memory.  
- A basic swap function swapped the elements in the same address 4 times (took time to debug; initially thought it was an `sw` function error).
- **Major Issue Fixed:**  
- Memory allocation for cores was incorrect—each core accessed the same memory, causing `sw` to overwrite the same address multiple times.  
- Initially assumed this was an `sw` function error but later discovered the issue was due to improper memory segmentation.  
- **Solution:** Implemented another structure called core memory and allocated distinct memory sections for each core.  
#### **Binnu:**  
- Compiled all the progress and key fixes into a structured **README file** for documentation.  

- ✅ **Final Outcome:** The bubble sort program executed successfully!  

## **February 19**  
### **Challenges Faced:**  

#### **Saranya (Simulator - Branch Instruction & PC Interaction):**  
- `.data` was successfully stored, but the code didn’t execute correctly.  
- After extensive debugging, found that **branch instructions** were not working properly.  
- Adjusted **instruction execution** and **PC increments** accordingly.  

#### **Binnu (Simulator):**  
- **Jump instructions** were not working properly, causing infinite loops.  
- Fixed the issue and successfully executed basic loops.  

**Conclusion:** Bubble sort still did not work.  

## **February 18**  
### **Decisions:**  

#### **Saranya (Simulator & Memory-Simulator Interaction):**  
- Updated parsing logic to read `.data`, `.text`, and `.word` sections from an assembly file.  
- Faced an issue where `.data` parsing failed because values were **not separated by commas**, leading to debugging delays.  
- Changed a data structure from `set` to `unordered_map`, which **broke the code**, requiring extensive debugging and partial rewrites.  

#### **Binnu (Simulator):**  
- Implemented additional instructions (`la`, `li`, `slli`, etc.) required for the **bubble sort program**.  

## **February 17**  
### **Decisions:**  

#### **Saranya (Simulator):**  
- Modified input handling to **read from a file** instead of terminal commands (`parseAssembly` function).  
- Commands did not work initially; **adjusted input structure** for each command.  

#### **Binnu (Simulator):**  
- Implemented **`lw` and `sw`** instructions and integrated memory access functions.  

#### **Binnu (Core & Memory):**  
- Added debugging commands (`print registers`, `print memory`) to **verify `lw` and `sw` operations**.  

## **February 15**  
### **Decisions:**  

- **Split** `main.cpp` into three separate files:  
  - `core.cpp` - Handles core attributes like `core_id`, **program counter (PC)**, and **registers**.  
  - `memory.cpp` - Manages **memory allocation per core**, including `lw` and `sw` functions.  
  - `simulator.cpp` - Implements **basic RISC-V instructions** (`add`, `sub`, `bne`, `jal`) and reads instructions from **terminal input**.  

#### **Responsibilities:**  
- **Binnu:** Worked on `core.cpp` and `memory.cpp`.  
- **Saranya:** Worked on the **simulator's basic functionality**.  


## **February 14**  
### **Decisions:**  
- **Finalized** C++ as the programming language for development.  
- Wrote a **basic C++ program** combining **core and memory functionality** in a single file.  
- **Saranya** worked on this initial structure.  

---
