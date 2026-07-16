#include "fetch_unit.hpp"
#include "../memory/memory.hpp"
#include <string>
#include <vector>

using namespace std;

bool check_sync_complete(vector<Core>& cores , int traget_address) {
  for(Core &core : cores) {
    if (core.global_pc != traget_address) {
      return false;
    }
  }
    return true;
  
}

bool parallel_sync(vector<Core>& cores){
    for(size_t i=0;i<cores.size();i++){
        if(cores[0].pc == cores[i].pc){
            cores[i].fetch_latency = cores[0].fetch_latency;
        }

    }
    return true ;
}

void fetchInstruction(vector<Core>& cores) {  
    bool branch_taken = false;

    string instr ;
    vector<string> args;
  
    for (Core &core : cores) {      
        if (core.pipeline.fetch_stall) {
            // Only this core's fetch is skipped; other cores have independent
            // pipelines/PCs and must still be able to fetch this cycle.
            continue;
        }
        if(core.barrier_sync == true){
            if(core.global_pc != 0) {
                if (core.cid_val == -1) {
                core.pc = core.global_pc;
                core.global_pc = 0;
                branch_taken = true;
                core.pipeline.ID_EX.valid_instruction=false;
            }
            if(core.core_id == core.cid_val) {
                core.pc = core.global_pc;
                if(check_sync_complete(cores, core.global_pc)){
                    core.pipeline.IF_ID.barrier_stall = false;
                    core.cid_val = -1;
                    core.global_pc = 0;

                }
                core.pipeline.ID_EX.barrier_stall = true;                       
            }

        }
    }        
        if (core.global_pc != 0) {
            if (core.cid_val == -1) {
                core.pc = core.global_pc;
                core.global_pc = 0;
                branch_taken = true;
                core.pipeline.ID_EX.valid_instruction=false;
            }
            if (core.core_id == core.cid_val) {
                core.pc = core.global_pc;
                core.global_pc = 0;
                branch_taken = true; 
                          
            }
            core.cid_val=-1;
    }  
        if (core.pc >= instruction_size() ) {
        if(core.pipeline.IF_ID.valid_instruction | core.pipeline.ID_EX.valid_instruction | core.pipeline.EX_MEM.valid_instruction | core.pipeline.MEM_WB.valid_instruction | core.pipeline.WB_Return.valid_instruction){
        core.no_instructions_fetch=true;
        }
        else{
            core.no_instructions=true;
        }
    }
        if(!core.no_instructions_fetch){
        core.no_instructions=false;
        args = fetch_args(core.pc);
        instr = fetch_instruction(core.pc,core.core_id);
        core.pipeline.IF_ID.instruction = instr ;
        core.pipeline.IF_ID.args = args;
        core.pipeline.IF_ID.pc = core.pc;
        core.pipeline.IF_ID.valid_instruction = true;
        core.pipeline.IF_ID.valid_data = false;
        core.pc++;
        core.fetch_latency=fetch_latency;
        parallel_sync(cores);
     }
          
    }
   
}
