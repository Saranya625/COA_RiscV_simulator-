#include "text_reporter.hpp"
#include "../memory/memory.hpp"
#include <iostream>

void printTextResults(std::vector<Core> &cores) {
    for (Core &core : cores) {
        std::cout << "\n====== Results for Core " << core.core_id << " ======\n";
        core.printRegisters();
        std::cout << "Clock Cycles: " << core.clock_cycles << std::endl;
        std::cout << "Stalls: " << core.pipeline.stalls << std::endl;
        std::cout << "Number of instructions executed: " << core.number_instructions << std::endl;

        double IPC = (double)core.number_instructions / core.clock_cycles;
        std::cout << "IPC: " << IPC << "\n";
        core.spm.sp_printMemory();
    }
    printMemory();
}
