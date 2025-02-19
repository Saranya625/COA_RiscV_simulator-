#include <iostream>
#include <vector>
#include <set>
#include <stdexcept>
#include "memory.cpp"

#define REGISTERS 32  

class Core {
public:
    int registers[REGISTERS] = {0};  
    const int core_id;              
    int pc = 0;                      
    std::unordered_map<int,int> visited_pcs;       
    Core(int id) : core_id(id) {
        registers[0] = 0;  
        registers[31] = id; 
    }

    int getRegisterIndex(const std::string &reg) {
        if (reg[0] != 'x') throw std::invalid_argument("Invalid register: " + reg);
        int regIndex = std::stoi(reg.substr(1));
        if (regIndex < 0 || regIndex >= REGISTER_COUNT) throw std::out_of_range("Register index out of range: " + reg);
        return regIndex;
    }

    void printRegisters() {
        std::cout << "\nCore " << core_id << " Registers:\n";
        for (int r = 0; r < REGISTERS; r++) {
            std::cout << "x" << r << ": " << registers[r] << "\t";
            if ((r + 1) % 8 == 0) std::cout << std::endl;  
        }
        std::cout << std::endl;
    }
};