#include "simulator_engine.hpp"
#include "fetch_unit.hpp"

using namespace std;

void executePipeline(vector<Core>& cores) {
    bool running = true;

    while (running) {
        for (Core &core : cores) {
            core.writeBack();
            core.memoryStage();
            core.executeStage();
            core.decodeInstruction();
        }
         fetchInstruction(cores);
        for (Core &core : cores) {
            core.pipeline.shiftStages();
        }

        for (Core &core : cores) {
            if(!core.no_instructions ){
                core.clock_cycles++;
            }
             else if(core.no_instructions && core.barrier_sync){
                core.clock_cycles++;
                core.pipeline.stalls++;
            }
            
        }

        running = false;
        for (Core &core : cores) {
            if (core.pipeline.IF_ID.valid_instruction || core.pipeline.ID_EX.valid_instruction || 
                core.pipeline.EX_MEM.valid_instruction || core.pipeline.MEM_WB.valid_instruction || 
                core.pipeline.WB_Return.valid_instruction) {
                running = true;
                break;
            }
         
        }
        
    }
}
