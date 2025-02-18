#include <iostream>
#include <vector>
#include <set>
#include <stdexcept>
#include "memory.cpp"

#define REGISTERS 32  // 32 registers

class Core {
public:
    int registers[REGISTERS] = {0};  // Registers
    const int core_id;               // Core ID
    int pc = 0;                      // Program counter
    std::set<int> visited_pcs;       // Track visited instructions

    Core(int id) : core_id(id) {
        registers[0] = 0;  // x0 is hardwired to 0
        registers[31] = id; // Special purpose register for core ID
    }

    // Convert "x1" -> 1, "x2" -> 2, ..., "x31" -> 31
    int getRegisterIndex(const std::string &reg) {
        if (reg[0] == 'x') {
            int num = std::stoi(reg.substr(1));
            if (num >= 0 && num < REGISTERS) return num;
        }
        throw std::invalid_argument("Invalid register: " + reg);
    }

    // Print registers
    void printRegisters() {
        std::cout << "\nCore " << core_id << " Registers:\n";
        for (int r = 0; r < REGISTERS; r++) {
            std::cout << "x" << r << ": " << registers[r] << "\t";
            if ((r + 1) % 8 == 0) std::cout << std::endl;  // Print 8 registers per line
        }
        std::cout << std::endl;
    }
};
