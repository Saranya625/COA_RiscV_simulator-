#pragma once
#include <string>
#include <vector>

struct PipelineStage {
    std::string instruction;
    std::vector<std::string> args;
    int rs1, rs2, rd;
    int rs1_value= 0,rs2_value= 0,rd_value=0;
    int offset,address;
    int pc = 0; // instruction-index PC the instruction was fetched at (used by jal to compute the link/return address)
    bool valid_instruction, valid_data;
    bool hazard_detected;
    bool stalled;
    int remaining_cycles;// to track no of cycles left..
    int extra_cycles;
    bool high_latency;
    bool barrier_stall = false;
    int memory_latency =0;
    bool high_memory_latency = false;
    int memory_remaining_cycles=0 ;
    bool fetch_high = false;
    bool mem_done = false; // whether the MEM-stage access has already run
    PipelineStage() : valid_instruction(false),valid_data(false), rs1(0), rs2(0), rd(0),
    hazard_detected(false),stalled(false),remaining_cycles(0),extra_cycles(0){}
};
