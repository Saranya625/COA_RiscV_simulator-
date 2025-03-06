#include <iostream>
#include <vector>
#include <unordered_map>
#include "memory.cpp"
using namespace std;

#define CORE_COUNT 4
#define REGISTER_COUNT 32


struct PipelineStage {
    std::string instruction;
    std::vector<std::string> args;
    int rs1, rs2, rd;
    int rs1_value= 0,rs2_value= 0,rd_value=0;
    int offset,address;
    bool valid_instruction, valid_data;
    bool hazard_detected;
    bool stalled;
 
    PipelineStage() : valid_instruction(false),valid_data(false), rs1(0), rs2(0), rd(0),
    hazard_detected(false),stalled(false){}
};


class Pipeline {
public:
    PipelineStage IF_ID; 
    PipelineStage ID_EX;
    PipelineStage EX_MEM;
    PipelineStage MEM_WB;
    PipelineStage WB_Return;
    bool stall_flag=false;
    int stalled_stage;
    int stalls=0 ;
    bool enabled_forwarding;
    bool ex_dependency=false;
    bool mem_dependency=false;
    void shiftStages() {
        if(stall_flag== true){
            cout << "Stall flag is true"<<endl;
            if(stalled_stage==4){
                WB_Return.stalled= true;
                MEM_WB.stalled = true;
                EX_MEM.stalled = true;
                ID_EX.stalled = true;
                IF_ID.stalled = true;
                stall_flag=false;
                stalled_stage=0;
                
            }
            if(stalled_stage==3){
                WB_Return=MEM_WB;
                MEM_WB.stalled = true;
                EX_MEM.stalled = true;
                ID_EX.stalled = true;
                IF_ID.stalled = true;
                stall_flag=false;
                stalled_stage=0;
            }
            if(stalled_stage==2){
                if(enabled_forwarding && !ex_dependency){
                WB_Return=MEM_WB;
                MEM_WB=EX_MEM;
                EX_MEM.stalled = true;
                ID_EX.stalled = true;
                IF_ID.stalled = true;
                stall_flag=false;
                stalled_stage=0;
                }
                else if (!enabled_forwarding && ex_dependency){
                  WB_Return=MEM_WB;
                    MEM_WB=EX_MEM;
                    EX_MEM.stalled = true;
                    ID_EX.stalled = true;
                    IF_ID.stalled = true;
                    stalls++;
                    stall_flag=true;
                    stalled_stage=3;

                }
                else{
                    WB_Return=MEM_WB;
                    MEM_WB=EX_MEM;
                    EX_MEM.stalled = true;
                    ID_EX.stalled = true;
                    IF_ID.stalled = true;
                    stall_flag=false;
                    stalled_stage=0;

                }
            }
             if(stalled_stage==1){
                WB_Return=MEM_WB;
                MEM_WB=EX_MEM;
                EX_MEM=ID_EX;
                ID_EX.stalled = true;
                IF_ID.stalled = true;
                stall_flag=false;
                stalled_stage=0;
            }
        }

        else {
            cout << "Stall flag is flase"<<endl;
            WB_Return = MEM_WB;
            MEM_WB = EX_MEM;
            EX_MEM = ID_EX;
            ID_EX = IF_ID;
            IF_ID = PipelineStage();
            IF_ID.valid_instruction = false;
            IF_ID.valid_data = false;
        }
    }

    void Handle_stalls(int stage){
        stall_flag=true;
        stalled_stage=stage;   
    }
    
};


class Core {
public:
    const int core_id;
    int pc=0;
    int registers[REGISTER_COUNT]={0};
    std::unordered_map<int, int> visited_pcs;
    std::unordered_map<std::string, int> &label_map; 
    int base_memory_adress;
    Pipeline pipeline;
    int clock_cycles = 0;   
    bool enable_forwarding;  
    int latest_ex_result ;
    int hazard_in_id = 0;
  
    Core(int id, std::unordered_map<std::string, int> &labels, bool forwarding) : core_id(id), label_map(labels), enable_forwarding(forwarding){
        registers[0]=0;
        registers[31]=core_id;
    } 

    int getRegisterIndex(const std::string &reg) {
        if (reg[0] == 'x') {
            int num = std::stoi(reg.substr(1));
            if (num >= 0 && num < REGISTER_COUNT) return num;
        }
        throw std::invalid_argument("Invalid register: " + reg);
    }

    void fetchInstruction(const std::vector<std::pair<std::string, std::vector<std::string>>> &instructions) {
        if(pipeline.IF_ID.stalled){
            pipeline.IF_ID.stalled=false;
            return ;
        }
        if (pc < instructions.size()) {
            pipeline.IF_ID.instruction = instructions[pc].first;
            pipeline.IF_ID.args = instructions[pc].second;
            pipeline.IF_ID.valid_instruction = true;
            pipeline.IF_ID.valid_data = false;
            pipeline.enabled_forwarding = enable_forwarding;
            std::cout << "[Core " << core_id << "] Fetched instruction: " << pipeline.IF_ID.instruction << "\n";
            pc++;
        } else {
            pipeline.IF_ID.valid_instruction = false;
            std::cout << "[Core " << core_id << "] No more instructions to fetch\n";
        }
    }
    

    void decodeInstruction() {
        //1.CHECK IF THE INSTRUCTION IS VALID OR NOT 
        if(!pipeline.ID_EX.valid_instruction){
            return;
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
            cout<<"[core "<<core_id<<"] Decoding instruction: "<<instr<<endl;
            //3.CHECK AND DECODE ACCORDINGLY 
            if (instr == "add" || instr == "sub" || instr == "and" || instr == "or" || instr == "xor") {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.rs2 = getRegisterIndex(args[2]); 
               pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.rs2_value=registers[pipeline.ID_EX.rs2];
                //4.IF FORWARDING IS ENABLED THEN CHECK FOR FORWARDING
                if(enable_forwarding){
                //FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                    std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                              << pipeline.ID_EX.rs1 << ")value forwarded :" <<pipeline.EX_MEM.rd_value << endl;
                    pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                    hazard_in_id++;
                }
                //FORWARD DEPENDENCY ON  EX/MEM TO ID/EX RS 2 
                if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2) {
                    std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS2: x" 
                              << pipeline.ID_EX.rs2 << ")\n";
                    pipeline.ID_EX.rs2_value = pipeline.EX_MEM.rd_value;
                    hazard_in_id++;
                 }
                 //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS1
                if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                          << pipeline.ID_EX.rs1 << ")value forwarded :" <<pipeline.MEM_WB.rd_value << endl;
                pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                hazard_in_id++;
                }
                //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS1
                 if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs2) {
                std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS2: x" 
                          << pipeline.ID_EX.rs2 << ")\n";
                pipeline.ID_EX.rs2_value = pipeline.MEM_WB.rd_value;
                hazard_in_id++;
                }
            }
                else{
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                  if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1|| pipeline.EX_MEM.rd == pipeline.ID_EX.rs2)) {
                    cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                    pipeline.stalls++;//Incrementing stalls
                    pipeline.stall_flag=true;//stall flag is true 
                    pipeline.stalled_stage=2; // stall stage ID     
                    pipeline.ex_dependency=true; // EX dependency on prev instruction          
                   }
                   //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                   if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1||pipeline.MEM_WB.rd == pipeline.ID_EX.rs2)) {
                    cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;
                        pipeline.stalls++;//incrementing stalls 
                        pipeline.stall_flag=true; // stall flag is true
                        pipeline.stalled_stage=2; // stall stage ID
                        pipeline.mem_dependency= true ; // MEM dependency on prev instruction

                    }
            
                }
            std::cout << "[Core " << core_id << "] Decoded instruction: " << instr
            << " | RD: " << pipeline.ID_EX.rd
            << " | RS1: " << pipeline.ID_EX.rs1
            << " | RS2: " << pipeline.ID_EX.rs2 << "\n";
         
        }
          
            else if (instr == "addi" || instr == "slli" || instr == "srli" || instr == "andi" || instr == "ori" || instr == "xori") {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.offset = std::stoi(args[2]); 
                //4.IF FORWARDING IS ENABLED THEN CHECK FOR DEPENDENCY
                if(enable_forwarding) {
                    //5.FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                        std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")\n"<<"value forwarded :" <<pipeline.EX_MEM.rd_value;
                        pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                    }
                    //5.FORWARD DEPENDENCY ON MEM/WB TO ID/EX RS1
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                        std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")\n"<<"value forwarded :" <<pipeline.MEM_WB.rd_value;
                        pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                    }
                }
                //4.IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY
                else{
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction 
                       
                    }
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // EX dependency on prev instruction 
                        
                    }

                }
               
                std::cout << "[Core " << core_id << "] Decoded instruction: " << instr
                << " | RD: " << pipeline.ID_EX.rd
                << " | RS1: " << pipeline.ID_EX.rs1
                << " | Offset: " << pipeline.ID_EX.offset << "\n";
            } 

            else if (instr == "lw") {
                int baseRegIndex = getRegisterIndex(args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1));
                pipeline.ID_EX.offset = std::stoi(args[1].substr(0, args[1].find('(')));
                pipeline.ID_EX.rs1 = baseRegIndex;
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset; 
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                cout<< "rd " << pipeline.ID_EX.rd<< " offset: " << pipeline.ID_EX.offset<<" rs1: "<<pipeline.ID_EX.rs1 << endl;         
                //4.IF FORWARDING IS ENABLED THEN CHECK FOR DEPENDENCY      
                if(enable_forwarding){
                    //5.FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS NOT SW
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction != "sw") ){
                        std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")\n";
                        pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                    //5.DEPENDENCY ON EX/MEM TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS SW SO STALLING TILL MEM OF SW IS FINISHED
                    if(pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.address == pipeline.ID_EX.address && pipeline.EX_MEM.instruction == "sw")){
                        cout << "DEPENDENCY ON EX/MEM TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS SW SO STALLING TILL MEM OF SW IS FINISHED"<<endl;
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.ex_dependency=true;
                    }
                    //5.FORWARD DEPENDENCY ON MEM/WB TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS NOT SW
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction != "sw") ){
                        std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")\n";
                        pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;
                    //5.DEPENDENCY ON EX/MEM TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS SW SO STALLING TILL MEM OF SW IS FINISHED  
                    if(pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.address == pipeline.ID_EX.address && pipeline.MEM_WB.instruction == "sw")){
                        cout << "DEPENDENCY ON MEM/WB TO ID/EX RS1 AND PREVIOUS INSTRUCTION IS SW SO STALLING TILL MEM OF SW IS FINISHED"<<endl;
                    pipeline.stalls++;
                    pipeline.stall_flag=true;
                    pipeline.stalled_stage=2;
                    pipeline.mem_dependency=true;
                    }
                }
                else{
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.ID_EX.offset== pipeline.EX_MEM.offset && pipeline.EX_MEM.instruction=="sw" ) ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction  
                        cout << " ex dependency " << endl;       
                
                    }
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction!="sw" ) ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction  
                        cout << " ex dependency " << endl;       
                
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.offset== pipeline.ID_EX.offset && pipeline.MEM_WB.instruction=="sw") ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // EX dependency on prev instruction         
                        cout << " mem dependency " << endl;       
                 
                    }
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction!="sw") ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // EX dependency on prev instruction         
                        cout << " mem dependency " << endl;       
                 
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                }

                pipeline.ID_EX.address = pipeline.ID_EX.rs1_value+ pipeline.ID_EX.offset;
            } 

            else if (instr == "sw") {
                int baseRegIndex = getRegisterIndex(args[1].substr(args[1].find('(') + 1, args[1].find(')') - args[1].find('(') - 1));
                pipeline.ID_EX.offset = std::stoi(args[1].substr(0, args[1].find('(')));            
                pipeline.ID_EX.rd = baseRegIndex;  
                pipeline.ID_EX.rd_value=registers[baseRegIndex];
                cout<< "rd: " << pipeline.ID_EX.rd << " offset: " << pipeline.ID_EX.offset << endl;
                pipeline.ID_EX.address = pipeline.ID_EX.rd_value + pipeline.ID_EX.offset; 
                pipeline.ID_EX.rs1 = getRegisterIndex(args[0]); 
                cout<< "rs1:"<<pipeline.ID_EX.rs1 << endl;
                if(enable_forwarding){
                       //ex/mem to id/ex
                   if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                    std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                              << pipeline.ID_EX.rs1 << ")\n";
                    pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                    hazard_in_id++;
                    cout<<"Value forwarded:"<<pipeline.ID_EX.rs1_value<<endl;
                    }
                       //mem/wb to id/ex
                     if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                    std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
                              << pipeline.ID_EX.rs1 << ")\n";
                    pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                    hazard_in_id++;
                    cout<<"Value forwarded:"<<pipeline.ID_EX.rs1_value<<endl;
                    }
                }
                else{
                    if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction        
                        cout << "ex dependency"<<endl;  
                    }
                    if(pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stalls++;//Incrementing stalls
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // MEM dependency on prev instruction 
                        cout << "mem dependency"<<endl;           
                    }
                }
             
                pipeline.ID_EX.address = pipeline.ID_EX.rd_value + pipeline.ID_EX.offset;  
                cout<<"address"<< pipeline.ID_EX.address<<endl;
                
            } 

            else if (instr == "beq" || instr == "bne" || instr == "blt" || instr == "bge") {
                pipeline.ID_EX.rs1 = getRegisterIndex(args[0]); 
                pipeline.ID_EX.rs2 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.rd = std::stoi(args[2]);
            }

            else if (instr == "jal" || instr == "j") {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = -1;
                pipeline.ID_EX.rs2 = -1;
            }

    pipeline.ID_EX.valid_data = true;
}
    

    void executeStage() {
        if (!pipeline.EX_MEM.valid_instruction ) 
        {
            return;
        }
        if(pipeline.EX_MEM.stalled){
            pipeline.EX_MEM.stalled=false;
            pipeline.EX_MEM.valid_instruction=false ;
            return ;
        }
            auto &instr = pipeline.EX_MEM.instruction;
            auto &args = pipeline.EX_MEM.args;
            pipeline.EX_MEM.valid_instruction = true;
            int rs1_val, rs2_val;
            if(enable_forwarding){
                 rs1_val = pipeline.EX_MEM.rs1_value;
                rs2_val = pipeline.EX_MEM.rs2_value; 
            }
            else{
                 rs1_val = registers[pipeline.EX_MEM.rs1];
                 rs2_val = registers[pipeline.EX_MEM.rs2];
            }
        
            int imm = pipeline.EX_MEM.offset;     
            int adress = pipeline.EX_MEM.address;  
            int rd = pipeline.EX_MEM.rd;
            std::cout << "[Core " << core_id << "] Executing instruction: " << instr << "\n";
            if (instr == "add") {
                pipeline.EX_MEM.rd_value = rs1_val + rs2_val;
            } else if (instr == "sub") {
                pipeline.EX_MEM.rd_value = rs1_val - rs2_val;
            } else if (instr == "addi") {
                pipeline.EX_MEM.rd_value = rs1_val + imm;
            } else if (instr == "slli") {
                pipeline.EX_MEM.rd_value = rs1_val << imm;
            } else if (instr == "lw") {
              if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.address == pipeline.EX_MEM.address && pipeline.MEM_WB.instruction=="sw")) {
                std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to EX/MEM (RS1: x" 
                          << pipeline.EX_MEM.rs1 << "value forwarded:"<< pipeline.MEM_WB.rd_value<<")\n";
                pipeline.EX_MEM.rd_value = pipeline.MEM_WB.rs1_value;
                hazard_in_id++;
            }
            pipeline.EX_MEM.address = pipeline.EX_MEM.rs1_value + pipeline.EX_MEM.offset;  
            }
            else if (instr == "sw") {
               
            }
            
            
             else if (instr == "beq") {
                if (rs1_val == rs2_val) {
                    pc = label_map[args[2]];
                }
            } else if (instr == "bne") {
                if (rs1_val != rs2_val) {
                    pc = label_map[args[2]];
                }
            } else if (instr == "blt") {
                if (rs1_val < rs2_val) {
                    pc = label_map[args[2]];
                }
            } else if (instr == "j") {
                pc = label_map[args[0]];
            } else if (instr == "jal") {
                registers[getRegisterIndex(args[0])] = pc;
                pc = label_map[args[1]];
            } else if (instr == "li") {
                pipeline.EX_MEM.rd_value = std::stoi(args[1]);
            } else if (instr == "la") {
                pipeline.EX_MEM.rd_value = label_map[args[1]];
            }else if (instr == "ecall") {
                if (registers[17] == 10) {
                    std::cout << "[Core " << core_id << "] Exit syscall detected. Terminating program.\n";
                    exit(0);
                }
            }
        latest_ex_result = pipeline.EX_MEM.rd_value;
        cout << "Latest_ex_rd_value:"<<latest_ex_result<<endl;
        pipeline.EX_MEM.valid_data = true;
    }

    void memoryStage() {
        
        if (!pipeline.MEM_WB.valid_instruction ) return;
        if(pipeline.MEM_WB.stalled){
            pipeline.MEM_WB.stalled=false;
            return ;
        }
        if(enable_forwarding==0){
            pipeline.MEM_WB.rs1_value= registers[pipeline.MEM_WB.rs1];
        }
        std::cout << "[Core " << core_id << "] Memory stage for instruction: " << pipeline.MEM_WB.instruction << "\n";
            auto &instr = pipeline.MEM_WB.instruction;

            if (instr == "lw") {
                cout<< "rs1_val: " << pipeline.MEM_WB.rd_value << " offset: " << pipeline.MEM_WB.offset << endl;
                pipeline.MEM_WB.rd_value = lw(pipeline.MEM_WB.address);
            } else if (instr == "sw") {
                sw(pipeline.MEM_WB.address, pipeline.MEM_WB.rs1_value);
            }
     pipeline.MEM_WB.valid_data = true;
            
        
    }

    void writeBack() {
        if (!pipeline.WB_Return.valid_instruction) return;
        if(pipeline.WB_Return.stalled){
            pipeline.WB_Return.stalled=false;
            return ;
        }
            auto &instr = pipeline.WB_Return.instruction;
            auto &args= pipeline.WB_Return.args;

            if (instr == "add" || instr == "sub" || instr == "addi" || instr == "slli" || instr == "lw" || instr == "li" || instr == "la") {
                registers[getRegisterIndex(args[0])] = pipeline.WB_Return.rd_value;
                std::cout << "[Core " << core_id << "]  Write Back: " << instr 
                << " | x" << pipeline.WB_Return.rd 
                << " = " << pipeline.WB_Return.rd_value << "\n";   
            }
           

            std::cout << "[Core " << core_id << "] Write back for instruction: " << instr << "\n";
        pipeline.WB_Return.valid_data = true;
    }

    void printRegisters() {
        std::cout << "Core " << core_id << " Registers:\n";
        for (int i = 0; i < REGISTER_COUNT; i++) {
            std::cout << "x" << i << ": " << registers[i] << "\t";
            if ((i + 1) % 8 == 0) {
                std::cout << std::endl;
            }
        }
    }


    
};    