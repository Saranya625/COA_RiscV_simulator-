# RISC-V Simulator

This **RISC-V Simulator** is designed to execute assembly programs using a **multi-core architecture**. It supports **4 cores**, each with its own dedicated **1KB memory segment**. The simulator reads assembly instructions from a file, processes them, and executes them sequentially.  

## Implemented RISC-V Instructions
The following **RISC-V instructions** are implemented in our simulator:

### 1. Arithmetic Instructions
- `add` - Adds two registers.
- `sub` - Subtracts two registers.
- `addi` - Adds an immediate value to a register.
- `slli` - Shifts a register left by an immediate value.

### 2. Branch and Jump Instructions
- `bne` - Branch if registers are not equal.
- `blt` - Branch if one register is less than another.
- `beq` - Branch if registers are equal.
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

## Bubble Sort Implementation
To demonstrate the functionality of our simulator, we have provided an **assembly file (`assembly.asm`)** containing **Bubble Sort**. This program sorts an array using the **implemented RISC-V instructions** and executes it within the simulator using the intructions in it.

After execution, the simulator prints the final register values and memory contents. 🚀

## Minutes of Meeting 
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
# RISC-V Simulator Development (PHASE - 2)
---

## Development Timeline

## **28th February**
- **S:** Initially implemented registers for each stage in the pipeline, laying the foundation for efficient instruction execution.
- **B:** Implemented stalls to handle pipeline hazards, ensuring smooth execution flow.

## **1st March**
- **S:** Successfully integrated data forwarding to minimize stalls and improve performance.
- **B:** Debugging and refining the implementation to ensure correctness and stability.

## **2nd March**
- **S:** Realized the initial pipeline design had fundamental flaws and restructured it to have a dedicated pipeline for each stage.
- **B:** Conducted thorough checks on data dependencies to refine pipeline execution.

## **3rd March**
- **S:** Re-enabled data forwarding after design modifications to optimize instruction flow.
- **B:** Continued fixing bugs to ensure correct execution and data propagation.

## **5th - 6th March**
- **S:** Implemented variable latencies and introduced user input functionality, adding flexibility to execution. Due to design changes, stalls couldn't be implemented yet.
- **B:** Successfully implemented the shift stages function and enabled stalls within it, providing better hazard handling.

## **7th - 8th March**
- **S:** Attempted to implement Bubble Sort, but encountered errors in memory allocation. Began working on the single fetch unit to streamline instruction handling.
- **B:** Experimented with different `.asma` files and verified clock cycle correctness to ensure accurate instruction execution.

## **9th March**
- **S:** Handled ALU and memory instruction dependencies while implementing variable latencies for ALU operations.
- **B:** Adapted branch and jump instructions to accommodate the new single fetch unit, ensuring proper instruction fetching across all cores.

---

## Conclusion
Significant progress has been made in refining the RISC-V simulator, from pipeline structuring to advanced optimizations like data forwarding, variable latencies, and hazard handling. With ongoing debugging and refinements, the project is steadily progressing toward a more robust and efficient simulation model.

This **README** documents the progress and key decisions in developing the **RISC-V Simulator**. 🚀  
