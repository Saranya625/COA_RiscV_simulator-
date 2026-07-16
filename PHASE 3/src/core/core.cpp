#include "core.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

using namespace std;

Core::Core(int id, std::unordered_map<std::string, std::vector<int>> &labels, bool forwarding,
           std::unordered_map<std::string, int> latencies, const SP_memory& spm_config, int main_mem_lat)
    : core_id(id), label_map(labels),
    enable_forwarding(forwarding) ,instruction_latencies(latencies),spm(spm_config), main_memory_latency(main_mem_lat) {
        registers[0]=0;
        registers[31]=core_id;
        
    } 

int Core::getRegisterIndex(const std::string &reg) {
        if (reg[0] == 'x') {
            int num = std::stoi(reg.substr(1));
            if (num >= 0 && num < REGISTER_COUNT) return num;
        }
        throw std::invalid_argument("Invalid register: " + reg);
}

    void Core::decodeInstruction() {
        //1.CHECK IF THE INSTRUCTION IS VALID OR NOT 
        if(!pipeline.ID_EX.valid_instruction){
            return;
        } 
        if(pipeline.ID_EX.barrier_stall){
            return ;
        }
        //2. CHECK IF THE INSTRUCTION IS STALLED OR NOT
        if(pipeline.ID_EX.stalled){
            pipeline.ID_EX.stalled=false;
            return ;
        }
        hazard_in_id=0;
        pipeline.ID_EX.valid_instruction = true;
            auto &instr = pipeline.ID_EX.instruction;
            auto &args = pipeline.ID_EX.args;
        //3.CHECK AND DECODE ACCORDINGLY 
            if (instr == "add" || instr == "sub" || instr == "and" || instr == "or" || instr == "xor"|| instr == "mul") {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.rs2 = getRegisterIndex(args[2]); 
               pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.rs2_value=registers[pipeline.ID_EX.rs2];
                //4.IF FORWARDING IS ENABLED THEN CHECK FOR FORWARDING
                if(enable_forwarding){
                 //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS1
                if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 ) {
                pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                hazard_in_id++;
                }
                //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS2
                 if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs2) {
                pipeline.ID_EX.rs2_value = pipeline.MEM_WB.rd_value;
                hazard_in_id++;
                }
                if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction !="lw") {
                    pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                    hazard_in_id++;
                }
                if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction=="lw"){
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.ex_dependency=true;
                }
                //FORWARD DEPENDENCY ON  EX/MEM TO ID/EX RS 2 
                if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2  && pipeline.EX_MEM.instruction!="lw") {
                    pipeline.ID_EX.rs2_value = pipeline.EX_MEM.rd_value;
                    hazard_in_id++;
                 }
                 if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2 && pipeline.EX_MEM.instruction=="lw"){
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.ex_dependency=true;
                }
            }
            else{
                   //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                   if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1||pipeline.MEM_WB.rd == pipeline.ID_EX.rs2)) {
                        pipeline.stall_flag=true; // stall flag is true
                        pipeline.stalled_stage=2; // stall stage ID
                        pipeline.mem_dependency= true ; // MEM dependency on prev instruction

                    }
                      //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                  if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1|| pipeline.EX_MEM.rd == pipeline.ID_EX.rs2)) {
                    pipeline.stall_flag=true;//stall flag is true 
                    pipeline.stalled_stage=2; // stall stage ID     
                    pipeline.ex_dependency=true; // EX dependency on prev instruction          
                   }
                }
        }
          
            else if (instr == "addi" || instr == "slli" || instr == "srli" ) {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.offset = std::stoi(args[2]); 
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
        //4.IF FORWARDING IS ENABLED THEN CHECK FOR DEPENDENCY
                if(enable_forwarding) {
         //5.FORWARD DEPENDENCY ON MEM/WB TO ID/EX RS1
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                        pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                    }
                      //5.FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                      if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction!="lw") {
                        pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                    }
        //5.MEMORY INSTRUCTION DEPENDENCY SO STALL TILL END OF MEM 
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction=="lw"){
                        pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.ex_dependency=true;
                    }
                }

         //4.IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY
                else{
          //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // MEM dependency on prev instruction 
                    }
                     //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                     if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction 
                    }
                }
            } 

            else if (instr == "lw"||instr == "lw_spm") {
                int baseRegIndex = getRegisterIndex(args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1));
                pipeline.ID_EX.offset = std::stoi(args[1].substr(0, args[1].find('(')));
                pipeline.ID_EX.rs1 = baseRegIndex;
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset; 
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                //4.IF FORWARDING IS ENABLED THEN CHECK FOR DEPENDENCY      
                if(enable_forwarding){
                    //5.FORWARD DEPENDENCY ON MEM/WB TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS NOT SW
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction != "sw") ){
                        pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;
                    //5.DEPENDENCY ON MEM/WB TO ID/EX AND PREVIOUS INSTRUCTION IS SW SO STALL TILL MEM OF SW IS FINISHED  
                    if(pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.address == pipeline.ID_EX.address && pipeline.MEM_WB.instruction == "sw")){
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.mem_dependency=true;
                    }
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction != "sw") ){
                        pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                    //5.DEPENDENCY ON EX/MEM TO ID/EX AND PREVIOUS INSTRUCTION IS SW SO STALL TILL MEM OF SW IS FINISHED
                    if(pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.address == pipeline.ID_EX.address && pipeline.EX_MEM.instruction == "sw")){
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.ex_dependency=true;
                    }
                }
                else{
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.offset== pipeline.ID_EX.offset && pipeline.MEM_WB.instruction=="sw") ){
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // MEM dependency on prev instruction         
                    }
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction!="sw") ){
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // MEM dependency on prev instruction         
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.ID_EX.offset== pipeline.EX_MEM.offset && pipeline.EX_MEM.instruction=="sw" ) ){
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction  
                    }
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction!="sw" ) ){
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction  
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                }

                pipeline.ID_EX.address = pipeline.ID_EX.rs1_value+ pipeline.ID_EX.offset;
            } 

            else if (instr == "sw"||instr == "sw_spm") {
                int baseRegIndex = getRegisterIndex(args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1));
                pipeline.ID_EX.offset = std::stoi(args[1].substr(0, args[1].find('(')));            
                pipeline.ID_EX.rd = baseRegIndex;  
                pipeline.ID_EX.rd_value=registers[baseRegIndex];
                pipeline.ID_EX.address = pipeline.ID_EX.rd_value + pipeline.ID_EX.offset; 
                pipeline.ID_EX.rs1 = getRegisterIndex(args[0]);
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.rs2 = baseRegIndex;
                if(enable_forwarding){
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction=="lw") {
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.mem_dependency=true;
                    pipeline.forwarding =1;
                    }
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs2&& pipeline.MEM_WB.instruction=="lw") {
                        pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.mem_dependency=true;
                     pipeline.forwarding= 2;
                        }
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2 && pipeline.EX_MEM.instruction=="lw") {
                        pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.mem_dependency=true;
                     pipeline.forwarding = 2;
                        }
                     if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction=="lw") {
                     pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.mem_dependency=true;
                    pipeline.forwarding = 1;
                    }

                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction!="lw") {
                    pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                    hazard_in_id++;
                    }
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs2 && pipeline.MEM_WB.instruction!="lw") {
                        pipeline.ID_EX.rd_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                        }
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2 && pipeline.EX_MEM.instruction!="lw") {
                        pipeline.ID_EX.rd_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                        }
                        if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction!="lw") {
                            pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                            hazard_in_id++;
                            }

                }
                else{
                    if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1){
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction        
                    }
                    if(pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1){
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // MEM dependency on prev instruction 
                    }
                }
             
                pipeline.ID_EX.address = pipeline.ID_EX.rd_value + pipeline.ID_EX.offset;  
                
            } 

            else if (instr == "beq" || instr == "bne" || instr == "blt" || instr == "bge") {
                if(pipeline.ID_EX.args[0] =="cid"){
                    cid_val= std::stoi(args[1]);
                    string branch_target =pipeline.ID_EX.args[2];
                    int new_pc=label_map[pipeline.ID_EX.args[2]][core_id];
                }
                else{
                pipeline.ID_EX.rs1 = getRegisterIndex(args[0]); 
                pipeline.ID_EX.rs2 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.rs2_value=registers[pipeline.ID_EX.rs2];
               if (label_map.find(args[2]) != label_map.end()) {
                    pipeline.ID_EX.rd = label_map[args[2]][core_id];
                } else {
                    std::cerr << "[Core " << core_id << "] ERROR: Label '" << args[2] << "' not found in label_map!\n";
                    exit(EXIT_FAILURE);
                }
                if(enable_forwarding){
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 ) {
                        pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                        }
                        //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS2
                         if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs2) {
                        pipeline.ID_EX.rs2_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                        }
                    //FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction !="lw") {
                        pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                    }
                    if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction=="lw"){
                        pipeline.stalls++;
                        pipeline.stall_flag=true;
                        pipeline.stalled_stage=2;
                        pipeline.ex_dependency=true;
                    }
                    //FORWARD DEPENDENCY ON  EX/MEM TO ID/EX RS 2 
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2  && pipeline.EX_MEM.instruction!="lw") {
                        pipeline.ID_EX.rs2_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                     }
                     if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2 && pipeline.EX_MEM.instruction=="lw"){
                        pipeline.stalls++;
                        pipeline.stall_flag=true;
                        pipeline.stalled_stage=2;
                        pipeline.ex_dependency=true;
                    }
                }
                else{
                        //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                      if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1|| pipeline.EX_MEM.rd == pipeline.ID_EX.rs2)) {
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction          
                       }
                       //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                       if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1||pipeline.MEM_WB.rd == pipeline.ID_EX.rs2)) {
                            pipeline.stall_flag=true; // stall flag is true
                            pipeline.stalled_stage=2; // stall stage ID
                            pipeline.mem_dependency= true ; // MEM dependency on prev instruction
                        }
                    }
                }

            }

            else if (instr == "jal") {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  // Destination register
                if (label_map.find(args[1]) != label_map.end()) {
                    pipeline.ID_EX.offset = label_map[args[1]][core_id];  // Target label address
                } else {
                    std::cerr << "[Core " << core_id << "] ERROR: Label '" << args[1] << "' not found in label_map!\n";
                    exit(EXIT_FAILURE);
                }
            } else if (instr == "j") {
                if (label_map.find(args[0]) != label_map.end()) {
                    pipeline.branch_taken=true; // Target label address
                    
                } else {
                    std::cerr << "[Core " << core_id << "] ERROR: Label '" << args[0] << "' not found in label_map!\n";
                    exit(EXIT_FAILURE);
                }
            }
            else if (instr == "la")
            {
               pipeline.ID_EX.rd= getRegisterIndex(args[0]);
               pipeline.ID_EX.rs1=-1;
               pipeline.ID_EX.rs2=-1;
               if(label_map.find(args[1]) != label_map.end()){
                pipeline.ID_EX.rd_value = label_map[args[1]][core_id];
            }  
               pipeline.ID_EX.rd_value = label_map[args[1]][core_id];

            }
            else if(instr == "li"){
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);
                pipeline.ID_EX.offset = stoi(args[1]);
            }
            else if(instr == "SYNC"){
               
            }
            pipeline.ID_EX.valid_data = true;
    if (instruction_latencies.count(instr)) {
        pipeline.ID_EX.remaining_cycles = instruction_latencies[instr];  // Set the latency
        pipeline.ID_EX.high_latency=true;
    }
    else {
        pipeline.ID_EX.remaining_cycles = 1; // Default latency
        pipeline.ID_EX.high_latency=false;
    }
    // Memory latency is resolved later, in the MEM stage, once we know whether
    // the access is an L1 hit, an L2 hit or a main-memory miss.
}
    
    void Core::executeStage() {
        if (!pipeline.EX_MEM.valid_instruction ) 
        {
            return;
        }
         if(pipeline.EX_MEM.barrier_stall){
            return ;
        }
        //2.STALL CHECK
        if(pipeline.EX_MEM.stalled){
            pipeline.EX_MEM.stalled=false;
            pipeline.EX_MEM.valid_instruction=true ;
            return ;
        }
        //3.LATENCY CHECK 
        if(pipeline.EX_MEM.high_latency == true){
            if (pipeline.EX_MEM.remaining_cycles == instruction_latencies[pipeline.EX_MEM.instruction]) {
                // first EX cycle for a multi-cycle ALU op: compute the result now
            }
            else{
                return ;
            }

        }
     
            auto &instr = pipeline.EX_MEM.instruction;
            auto &args = pipeline.EX_MEM.args;
            pipeline.EX_MEM.valid_instruction = true;
            int rs1_val, rs2_val;
            //FORWARDING ENABLED AND NOT DEPENDENT ON MEMORY INSRUCTION FORWARD FROM ID_EX STAGE 
            if(enable_forwarding ){
                 rs1_val = pipeline.EX_MEM.rs1_value;
                rs2_val = pipeline.EX_MEM.rs2_value; 
            }
            else{
                 rs1_val = registers[pipeline.EX_MEM.rs1];
                 rs2_val = registers[pipeline.EX_MEM.rs2];
            }
             //FORWARDING ENABLED AND  DEPENDENT ON MEMORY INSRUCTION FORWARD FROM MEMORY OF PREV INSTR 
             if (enable_forwarding && (pipeline.WB_Return.instruction == "lw")){
                if(pipeline.EX_MEM.rs1 == pipeline.WB_Return.rd){
                   rs1_val=pipeline.WB_Return.rd_value;
                    rs2_val=pipeline.EX_MEM.rs2_value;
                }
                if(pipeline.EX_MEM.rs2 == pipeline.WB_Return.rd){
                   rs2_val=pipeline.WB_Return.rd_value;
                   rs1_val=pipeline.EX_MEM.rs1_value;
                }
            }
            //NO FORWARDING SO TAKE AFTER WB OF PREV INSTR I.E FROM REGISTERS 
                   
            int imm = pipeline.EX_MEM.offset;     
            int adress = pipeline.EX_MEM.address;  
            int rd = pipeline.EX_MEM.rd;
            if (instr == "add") {
                pipeline.EX_MEM.rd_value = rs1_val + rs2_val;
            } 
            else if (instr == "sub") {
                pipeline.EX_MEM.rd_value = rs1_val - rs2_val;
            }
            else if(instr == "mul"){
                pipeline.EX_MEM.rd_value = rs1_val*rs2_val;
            }            
            else if (instr == "addi") {
                pipeline.EX_MEM.rd_value = rs1_val + pipeline.EX_MEM.offset;
            } else if (instr == "slli") {
                pipeline.EX_MEM.rd_value = rs1_val << imm;
            }
             else if (instr == "srli") {
            pipeline.EX_MEM.rd_value = imm >> rs1_val;
            }  
            else if (instr == "lw"|| instr == "lw_spm") {
            }
            else if (instr == "sw"||instr == "sw_spm") {
               
            }
            else if( instr == "and" ){
                pipeline.EX_MEM.rd_value = rs1_val & rs2_val;
            }      
            else if( instr == "or" ){
                pipeline.EX_MEM.rd_value = rs1_val || rs2_val;
            }    
            else if( instr == "xor" ){
                pipeline.EX_MEM.rd_value = rs1_val ^ rs2_val;
            }   
             else if (instr == "beq") {
                std::string rs1_str = pipeline.EX_MEM.args[0];  // RS1 (could be "cid")
                std::string rs2_str = pipeline.EX_MEM.args[1];  // RS2
                std::string target_label = pipeline.EX_MEM.args[2];  // Branch label
                if (rs1_str == "cid") {  
                    // **Only the correct core should take the branch**
                    if (core_id == cid_val) {  
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;  // Invalidate instruction after branch
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    }
                }
             else{
                if (rs1_val == rs2_val) {
                    std::string target_label = pipeline.EX_MEM.args[2];
                    if (label_map.find(target_label) != label_map.end()) {
                        int new_pc = label_map[target_label][core_id];
                        global_pc = new_pc ; // Adjust for fetch increment
                        pipeline.ID_EX.valid_instruction=false;

                    } else {
                        std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                        exit(EXIT_FAILURE);
                    }
                    
                }
             }
              
             } 
             else if (instr == "bne") {
                std::string rs1_str = pipeline.EX_MEM.args[0];  // RS1 (could be "cid")
                std::string rs2_str = pipeline.EX_MEM.args[1];  // RS2
                std::string target_label = pipeline.EX_MEM.args[2];  // Branch label
                if (rs1_str == "cid") {  
                    // **Only the correct core should take the branch**
                    if (core_id == cid_val) {  
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;  // Invalidate instruction after branch
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            
                // **Handle normal BNE X, Y, label**
                else {
                    if (rs1_val != rs2_val) {
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            else if (instr == "blt") {
                if (rs1_val < rs2_val) {
                    std::string target_label = pipeline.EX_MEM.args[2];
                    if (label_map.find(target_label) != label_map.end()) {
                        int new_pc = label_map[target_label][core_id];
                        global_pc = new_pc ; // Adjust for fetch increment
                        pipeline.ID_EX.valid_instruction=false;

                    } else {
                        std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                        exit(EXIT_FAILURE);
                    }
                }
            } 
            else if(instr== "j"){
                std::string target_label = pipeline.EX_MEM.args[0];
                if (label_map.find(target_label) != label_map.end()) {
                    int new_pc = label_map[target_label][core_id];
                    global_pc = new_pc ; // Adjust for fetch increment
                    pipeline.ID_EX.valid_instruction=false;

                } else {
                    std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                    exit(EXIT_FAILURE);
                }
            }          
            else if (instr == "jal") {
                std::string target_label = pipeline.EX_MEM.args[2];
                    if (label_map.find(target_label) != label_map.end()) {
                        int new_pc = label_map[target_label][core_id];
                        global_pc = new_pc ; // Adjust for fetch increment
                        pipeline.ID_EX.valid_instruction=false;

                    } else {
                        std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                        exit(EXIT_FAILURE);
                    }
            } else if (instr == "li") {
                pipeline.EX_MEM.rd= getRegisterIndex(args[0]);
                pipeline.EX_MEM.rd_value = pipeline.EX_MEM.offset;
            } else if (instr == "la") {  
                pipeline.EX_MEM.rd= getRegisterIndex(args[0]);
                pipeline.EX_MEM.rd_value= label_map[args[1]][core_id];  
            }else if (instr == "ecall") {
                if (registers[17] == 10) {
                    exit(0);
                }
            }
            else if(instr == "nop"){

            }
            else if(instr == "SYNC"){
                barrier_sync=true;
            }      
        
        
        latest_ex_result = pipeline.EX_MEM.rd_value;
        pipeline.EX_MEM.valid_data = true;
    }

    void Core::memoryStage() {
        if (!pipeline.MEM_WB.valid_instruction ) return;
         if(pipeline.MEM_WB.barrier_stall){
            return ;
        }
        if(pipeline.MEM_WB.stalled){
            pipeline.MEM_WB.stalled=false;
            pipeline.MEM_WB.valid_instruction=false;
            return ;
        }

        auto &instr = pipeline.MEM_WB.instruction;

        // Perform the memory access exactly once, on the first cycle the
        // instruction spends in the MEM stage. The access resolves the real
        // latency (L1 hit / L2 hit / main-memory miss), which then drives the
        // stall countdown in Pipeline::shiftStages().
        if (!pipeline.MEM_WB.mem_done) {
            if (enable_forwarding == 0) {
                pipeline.MEM_WB.rs1_value = registers[pipeline.MEM_WB.rs1];
                if (instr == "lw" || instr == "lw_spm") {
                    pipeline.MEM_WB.address = pipeline.MEM_WB.rs1_value + pipeline.MEM_WB.offset;
                }
                if (instr == "sw" || instr == "sw_spm") {
                    pipeline.MEM_WB.rd_value = registers[pipeline.MEM_WB.rd];
                    pipeline.MEM_WB.address = pipeline.MEM_WB.rd_value + pipeline.MEM_WB.offset;
                }
            }

            if (instr == "lw") {
                memory_accesses++;
                pipeline.MEM_WB.rd_value = lw(pipeline.MEM_WB.address, core_id);
                pipeline.MEM_WB.memory_latency = memory_latency;
            } else if (instr == "sw") {
                memory_accesses++;
                if (pipeline.forwarding == 1) {
                    pipeline.MEM_WB.rs1_value = registers[pipeline.MEM_WB.rs1];
                    pipeline.forwarding = 0;
                }
                if (pipeline.forwarding == 2) {
                    pipeline.MEM_WB.rd_value = registers[pipeline.MEM_WB.rd];
                    pipeline.MEM_WB.address = pipeline.MEM_WB.rd_value + pipeline.MEM_WB.offset;
                    pipeline.forwarding = 0;
                }
                sw(pipeline.MEM_WB.address, pipeline.MEM_WB.rs1_value, core_id);
                pipeline.MEM_WB.memory_latency = memory_latency;
            } else if (instr == "lw_spm") {
                pipeline.MEM_WB.rd_value = spm.lw_spm(pipeline.MEM_WB.address);
                pipeline.MEM_WB.memory_latency = spm.latency_sp;
            } else if (instr == "sw_spm") {
                if (pipeline.forwarding == 1) {
                    pipeline.MEM_WB.rs1_value = registers[pipeline.MEM_WB.rs1];
                    pipeline.forwarding = 0;
                }
                if (pipeline.forwarding == 2) {
                    pipeline.MEM_WB.rd_value = registers[pipeline.MEM_WB.rd];
                    pipeline.MEM_WB.address = pipeline.MEM_WB.rd_value + pipeline.MEM_WB.offset;
                    pipeline.forwarding = 0;
                }
                spm.sw_spm(pipeline.MEM_WB.address, pipeline.MEM_WB.rs1_value);
                pipeline.MEM_WB.memory_latency = spm.latency_sp;
            } else {
                // Non-memory instructions do not stall the MEM stage.
                pipeline.MEM_WB.memory_latency = 1;
            }

            pipeline.MEM_WB.high_memory_latency = (pipeline.MEM_WB.memory_latency > 1);
            pipeline.MEM_WB.mem_done = true;
        }

        pipeline.MEM_WB.valid_data = true;
    }

    void Core::writeBack() {
        if (!pipeline.WB_Return.valid_instruction) return;
         if(pipeline.WB_Return.barrier_stall){
            return ;
        }
        if(pipeline.WB_Return.stalled){
            pipeline.WB_Return.stalled=false;
            return ;
        }
            auto &instr = pipeline.WB_Return.instruction;
            auto &args= pipeline.WB_Return.args;

            if (instr == "add" || instr == "sub" || instr == "addi" || instr == "slli" || instr == "lw" || instr == "li" || instr == "la"|| instr=="mul"||instr == "and"||instr=="srli"||instr == "and"||instr=="or"||instr=="xor") {
                registers[getRegisterIndex(args[0])] = pipeline.WB_Return.rd_value;
            }
        pipeline.WB_Return.valid_data = true;
        number_instructions++;
    }

    void Core::printRegisters() {
        std::cout << "Core " << core_id << " Registers:\n";
        for (int i = 0; i < REGISTER_COUNT; i++) {
            std::cout << "x" << i << ": " << registers[i] << "\t";
            if ((i + 1) % 8 == 0) {
                std::cout << std::endl;
            }
        }
    }
