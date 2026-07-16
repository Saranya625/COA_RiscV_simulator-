#include "pipeline.hpp"

void Pipeline::shiftStages() {
        if(stall_flag== true){
            if(stalled_stage==4){
                WB_Return.stalled= true;
                MEM_WB.stalled = true;
                EX_MEM.stalled = true;
                ID_EX.stalled = true;
                fetch_stall = true;
                stall_flag=false;
                stalled_stage=0;
                stalls++;
                return ;
                
            }
            if(stalled_stage==3){
                WB_Return=MEM_WB;
                MEM_WB.stalled = true;
                EX_MEM.stalled = true;
                ID_EX.stalled = true;
                fetch_stall = true;
                stall_flag=false;
                stalled_stage=0;
                stalls++;
                return ;
            }
            if(stalled_stage==2){
                if(enabled_forwarding ){
                WB_Return=MEM_WB;
                MEM_WB=EX_MEM;
                EX_MEM.stalled = true;
                ID_EX.stalled = true;
                fetch_stall = true;
                stalls++;
                stall_flag=false;
                stalled_stage=0;
                return ;
                }
                else if (!enabled_forwarding && ex_dependency){
                  WB_Return=MEM_WB;
                    MEM_WB=EX_MEM;
                    EX_MEM.stalled = true;
                    ID_EX.stalled = true;
                    fetch_stall = true;
                    stalls++;
                    stall_flag=true;
                    stalled_stage=3;
                    return;

                }
                else{
                    WB_Return=MEM_WB;
                    MEM_WB=EX_MEM;
                    EX_MEM.stalled = true;
                    ID_EX.stalled = true;
                    fetch_stall = true;
                    stalls++;
                    stall_flag=false;
                    stalled_stage=0;
                    return ;

                }
                if(stalled_stage==1){
                    WB_Return=MEM_WB;
                    MEM_WB=EX_MEM;
                    EX_MEM=ID_EX;
                    ID_EX.stalled = true;
                    fetch_stall = true;
                    stalls++;
                    stall_flag=false;
                    stalled_stage=0;
                    return ;
                }
            }
           
        }

        else {
            if(EX_MEM.remaining_cycles >1){
                EX_MEM.remaining_cycles--;
                WB_Return=MEM_WB;
                MEM_WB.valid_instruction = false;
                fetch_stall=true;
                EX_MEM=EX_MEM;
                ID_EX.stalled=true;
                stalls++;
                EX_MEM.extra_cycles ++;
                stall_flag=false;
            }
            else if(MEM_WB.memory_latency>1 && MEM_WB.valid_instruction){
                MEM_WB.memory_latency=MEM_WB.memory_latency-1;
                WB_Return.valid_instruction = false;
                fetch_stall=true;
                MEM_WB=MEM_WB;
                IF_ID=IF_ID;
                EX_MEM=EX_MEM;
                ID_EX=ID_EX;
                EX_MEM.stalled=true;               
                ID_EX.stalled=true;               
                stalls++;               
                MEM_WB.extra_cycles++;
                stall_flag=false;
            }
            else{
                    EX_MEM.stalled=false;               
                    ID_EX.stalled=false;   
                    WB_Return = MEM_WB;
                    MEM_WB = EX_MEM;
                    EX_MEM = ID_EX;
                    ID_EX = IF_ID;
                    IF_ID = PipelineStage();
                    fetch_stall=false;
                    IF_ID.valid_instruction = false;
                    stall_flag=false;
                    IF_ID.valid_data = false;
                    return ;
                }
               
            }
 
        
}
