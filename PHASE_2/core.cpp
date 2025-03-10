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
    int remaining_cycles;// to track no of cycles left..
    int extra_cycles;
 
    PipelineStage() : valid_instruction(false),valid_data(false), rs1(0), rs2(0), rd(0),
    hazard_detected(false),stalled(false),remaining_cycles(0),extra_cycles(0){}
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
    bool branch_taken =false;
    bool fetch_stall = false;
    void shiftStages() {
        if(stall_flag== true){
            cout << "Stall flag is true"<<endl;
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
            cout << "Stall flag is flase"<<endl;
            if(EX_MEM.remaining_cycles >1){
                EX_MEM.remaining_cycles--;
                cout << " stalling due to high latency ."<< endl ;
                WB_Return=MEM_WB;
                MEM_WB.valid_instruction = false;
                fetch_stall=true;
                ID_EX.stalled=true;
                stalls++;
                EX_MEM.extra_cycles ++;
                stall_flag=false;
              cout<<" EX_MEM.extra_cycles:"<< EX_MEM.extra_cycles<<endl;
            }
            else{
            
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

};


class Core {
public:
  int global_pc=0;
    const int core_id;
    int pc=0;
    int cid_val=-1;
    int registers[REGISTER_COUNT]={0};
    std::unordered_map<int, int> visited_pcs;
    std::unordered_map<std::string, vector<int>> &label_map; 
    unordered_map<string,int> instruction_latencies; 
    int base_memory_adress;
    Pipeline pipeline;
    int clock_cycles = 0;   
    bool enable_forwarding;  
    int latest_ex_result ;
    int hazard_in_id = 0;
    int number_instructions = 0;
    int base_adress;
    
  
    Core(int id, std::unordered_map<std::string,vector< int>> &labels, bool forwarding , unordered_map<string,int > latencies) : core_id(id), label_map(labels), enable_forwarding(forwarding) ,instruction_latencies(latencies){
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
            if (instr == "add" || instr == "sub" || instr == "and" || instr == "or" || instr == "xor"|| instr == "mul") {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.rs2 = getRegisterIndex(args[2]); 
               pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.rs2_value=registers[pipeline.ID_EX.rs2];
                //4.IF FORWARDING IS ENABLED THEN CHECK FOR FORWARDING
                if(enable_forwarding){
                //FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                
                 //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS1
                if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 ) {
                std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
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
                if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction !="lw") {
                    std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                              << pipeline.ID_EX.rs1 << ")value forwarded :" <<pipeline.EX_MEM.rd_value << endl;
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
                    std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS2: x" 
                              << pipeline.ID_EX.rs2 << ")\n";
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
                    cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl; 
                        pipeline.stall_flag=true; // stall flag is true
                        pipeline.stalled_stage=2; // stall stage ID
                        pipeline.mem_dependency= true ; // MEM dependency on prev instruction

                    }
                      //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                  if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1|| pipeline.EX_MEM.rd == pipeline.ID_EX.rs2)) {
                    cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                    pipeline.stall_flag=true;//stall flag is true 
                    pipeline.stalled_stage=2; // stall stage ID     
                    pipeline.ex_dependency=true; // EX dependency on prev instruction          
                   }
                }
            std::cout << "[Core " << core_id << "] Decoded instruction: " << instr
            << " | RD: " << pipeline.ID_EX.rd
            << " | RS1: " << pipeline.ID_EX.rs1
            << " | RS2: " << pipeline.ID_EX.rs2 << "\n";
         
        }
          
            else if (instr == "addi" || instr == "slli" || instr == "srli" ) {
                pipeline.ID_EX.rd = getRegisterIndex(args[0]);  
                pipeline.ID_EX.rs1 = getRegisterIndex(args[1]); 
                pipeline.ID_EX.offset = std::stoi(args[2]); 
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                cout<<"rs1_value"<<pipeline.ID_EX.rs1_value<<endl;
        //4.IF FORWARDING IS ENABLED THEN CHECK FOR DEPENDENCY
                if(enable_forwarding) {
       
                    
         //5.FORWARD DEPENDENCY ON MEM/WB TO ID/EX RS1
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                        std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")\n"<<"value forwarded :" <<pipeline.MEM_WB.rd_value;
                        pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                    }
                      //5.FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                      if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction!="lw") {
                        std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")\n"<<"value forwarded :" <<pipeline.EX_MEM.rd_value;
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
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // EX dependency on prev instruction 
                        
                    }
                     //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                     if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction 
                       
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
                }
                else{
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                    
                    //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.offset== pipeline.ID_EX.offset && pipeline.MEM_WB.instruction=="sw") ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // EX dependency on prev instruction         
                        cout << " mem dependency " << endl;       
                 
                    }
                    if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1 && pipeline.MEM_WB.instruction!="sw") ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.mem_dependency=true; // EX dependency on prev instruction         
                        cout << " mem dependency " << endl;       
                 
                    }
                    pipeline.ID_EX.address = pipeline.ID_EX.rs1_value + pipeline.ID_EX.offset;  
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.ID_EX.offset== pipeline.EX_MEM.offset && pipeline.EX_MEM.instruction=="sw" ) ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction  
                        cout << " ex dependency " << endl;       
                
                    }
                    if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction!="sw" ) ){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction  
                        cout << " ex dependency " << endl;       
                
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
                pipeline.ID_EX.rs1_value=registers[pipeline.ID_EX.rs1];
                pipeline.ID_EX.rs2 = baseRegIndex;
                cout<< "rs1:"<<pipeline.ID_EX.rs1 << endl;
                if(enable_forwarding){
                       //ex/mem to id/ex

                       //mem/wb to id/ex
                     if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1) {
                    std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
                              << pipeline.ID_EX.rs1 << ")\n";
                    pipeline.ID_EX.rs1_value = pipeline.MEM_WB.rd_value;
                    hazard_in_id++;
                    cout<<"Value forwarded:"<<pipeline.ID_EX.rs1_value<<endl;
                    }
                    if (pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs2) {
                        std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS2: x" 
                                  << pipeline.ID_EX.rs2 << ")\n";
                        pipeline.ID_EX.rd_value = pipeline.MEM_WB.rd_value;
                        hazard_in_id++;
                        cout<<"Value forwarded:"<<pipeline.ID_EX.rd_value<<endl;
                        }
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2) {
                        std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS2: x" 
                                  << pipeline.ID_EX.rs2 << ")\n";
                        pipeline.ID_EX.rd_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                        cout<<"Value forwarded:"<<pipeline.ID_EX.rd_value<<endl;
                        }
                        if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1) {
                            std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                                      << pipeline.ID_EX.rs1 << ")\n";
                            pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                            hazard_in_id++;
                            cout<<"Value forwarded:"<<pipeline.ID_EX.rs1_value<<endl;
                            }
                           //mem/wb to id/ex

                }
                else{
                    if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction        
                        cout << "ex dependency"<<endl;  
                    }
                    if(pipeline.MEM_WB.valid_instruction && pipeline.MEM_WB.rd == pipeline.ID_EX.rs1){
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
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
                if(pipeline.ID_EX.args[0] =="cid"){
                    cid_val= std::stoi(args[1]);
                    cout<<"core dependenct instruction "<<endl;
                    cout<<"CID value:"<<cid_val;
                    string branch_target =pipeline.ID_EX.args[2];
                    cout<<"branch target :"<< branch_target <<endl;
                    int new_pc=label_map[pipeline.ID_EX.args[2]][core_id];
                    //global_pc = new_pc;
                    //cout<<"global pc:"<<global_pc<<endl;
                    
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
                        std::cout << "[Core " << core_id << "] Forwarding from MEM/WB to ID/EX (RS1: x" 
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
                    //FORWARD DEPENDENCY ON EX/MEM TO ID/EX RS1
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction !="lw") {
                        std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS1: x" 
                                  << pipeline.ID_EX.rs1 << ")value forwarded :" <<pipeline.EX_MEM.rd_value << endl;
                        pipeline.ID_EX.rs1_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                    }
                    if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs1 && pipeline.EX_MEM.instruction=="lw"){
                        cout<<"stalling due to dependency on x " << pipeline.EX_MEM.rd << " goimg through lw "<<endl;
                        pipeline.stalls++;
                        pipeline.stall_flag=true;
                        pipeline.stalled_stage=2;
                        pipeline.ex_dependency=true;
                    }
                    //FORWARD DEPENDENCY ON  EX/MEM TO ID/EX RS 2 
                    if (pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2  && pipeline.EX_MEM.instruction!="lw") {
                        std::cout << "[Core " << core_id << "] Forwarding from EX/MEM to ID/EX (RS2: x" 
                                  << pipeline.ID_EX.rs2 << ")\n";
                        pipeline.ID_EX.rs2_value = pipeline.EX_MEM.rd_value;
                        hazard_in_id++;
                     }
                     if(pipeline.EX_MEM.valid_instruction && pipeline.EX_MEM.rd == pipeline.ID_EX.rs2 && pipeline.EX_MEM.instruction=="lw"){
                        cout<<"stalling due to dependency on x " << pipeline.EX_MEM.rd << " goimg through lw "<<endl;
                        pipeline.stalls++;
                        pipeline.stall_flag=true;
                        pipeline.stalled_stage=2;
                        pipeline.ex_dependency=true;
                    }
                     //FORWARD DEPENDCY FORM MEM/WB TO ID/EX RS1
                    
                }
                else{
                        //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN EX
                      if (pipeline.EX_MEM.valid_instruction && (pipeline.EX_MEM.rd == pipeline.ID_EX.rs1|| pipeline.EX_MEM.rd == pipeline.ID_EX.rs2)) {
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl;//dependency in EX stage 
                        pipeline.stall_flag=true;//stall flag is true 
                        pipeline.stalled_stage=2; // stall stage ID     
                        pipeline.ex_dependency=true; // EX dependency on prev instruction          
                       }
                       //5. IF FORWARDING IS DISABLED THEN CHECK FOR DEPENDENCY IN MEM
                       if (pipeline.MEM_WB.valid_instruction && (pipeline.MEM_WB.rd == pipeline.ID_EX.rs1||pipeline.MEM_WB.rd == pipeline.ID_EX.rs2)) {
                        cout<< " Forwarding is disabled stalling till wb of previous instruction"<<endl; 
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
                cout<<"rd:"<< pipeline.ID_EX.rd << "value:" << pipeline.ID_EX.offset;
            }
    pipeline.ID_EX.valid_data = true;
    if (instruction_latencies.count(instr)) {
        pipeline.ID_EX.remaining_cycles = instruction_latencies[instr];  // Set the latency
    } else {
        pipeline.ID_EX.remaining_cycles = 1; // Default latency
    }
   
}
    
    void executeStage() {
        //1.NORMAL CHECK
        if (!pipeline.EX_MEM.valid_instruction ) 
        {
            return;
        }
        //2.STALL CHECK
        if(pipeline.EX_MEM.stalled){
            pipeline.EX_MEM.stalled=false;
            pipeline.EX_MEM.valid_instruction=false ;
            return ;
        }
        //3.LATENCY CHECK 
        if (pipeline.EX_MEM.remaining_cycles > 1) {
            std::cout << "[Core " << core_id << "] Executing " << pipeline.EX_MEM.instruction 
                      << " (cycle:" << pipeline.EX_MEM.remaining_cycles << ") \n";
                      return ;
        }
            auto &instr = pipeline.EX_MEM.instruction;
            auto &args = pipeline.EX_MEM.args;
            pipeline.EX_MEM.valid_instruction = true;
            int rs1_val, rs2_val;
            //FORWARDING ENABLED AND NOT DEPENDENT ON MEMORY INSRUCTION FORWARD FROM ID_EX STAGE 
            if(enable_forwarding  ){
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
                    cout<<"rs1_value:"<<rs1_val<<endl;
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
            std::cout << "[Core " << core_id << "] Executing instruction: " << instr << "\n";
            if (instr == "add") {
                pipeline.EX_MEM.rd_value = rs1_val + rs2_val;
            } 
            else if (instr == "sub") {
                pipeline.EX_MEM.rd_value = rs1_val - rs2_val;
            }
            else if(instr == "mul"){
                pipeline.EX_MEM.rd_value = rs1_val*rs2_val;
                cout<<"Executed instruction :" <<pipeline.EX_MEM.instruction << " output :" <<  pipeline.EX_MEM.rd_value;
            }            
            else if (instr == "addi") {

                pipeline.EX_MEM.rd_value = rs1_val + pipeline.EX_MEM.offset;
            } else if (instr == "slli") {
                pipeline.EX_MEM.rd_value = rs1_val << imm;
            }
             else if (instr == "srli") {
            pipeline.EX_MEM.rd_value = imm >> rs1_val;
            }  
            else if (instr == "lw") {
            }
            else if (instr == "sw") {
               
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
                    std::cout << "Instruction is core-dependent\n";
                    std::cout << "Checking branch at Core " << core_id << ", CID value: " << cid_val << std::endl;
            
                    // **Only the correct core should take the branch**
                    if (core_id == cid_val) {  
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                      << target_label << " at index " << new_pc << std::endl;
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;  // Invalidate instruction after branch
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        std::cout << "[Core " << core_id << "] Ignoring core-dependent branch.\n";
                    }
                }
             else{
                cout<<"checking if "<< rs1_val << "=" << rs2_val <<endl;
                if (rs1_val == rs2_val) {
                    std::string target_label = pipeline.EX_MEM.args[2];
                    if (label_map.find(target_label) != label_map.end()) {
                        int new_pc = label_map[target_label][core_id];
                        std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                  << target_label << " at index " << new_pc << std::endl;
                        global_pc = new_pc ; // Adjust for fetch increment
                        pipeline.ID_EX.valid_instruction=false;

                    } else {
                        std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                        exit(EXIT_FAILURE);
                    }
                    
                }
                else{
                    cout<<"Branch not taken contiinuing with: "<< pipeline.ID_EX.instruction << endl;
                }

             }
              
             } 
             else if (instr == "bne") {
                std::string rs1_str = pipeline.EX_MEM.args[0];  // RS1 (could be "cid")
                std::string rs2_str = pipeline.EX_MEM.args[1];  // RS2
                std::string target_label = pipeline.EX_MEM.args[2];  // Branch label
                if (rs1_str == "cid") {  
                    std::cout << "Instruction is core-dependent\n";
                    std::cout << "Checking branch at Core " << core_id << ", CID value: " << cid_val << std::endl;
            
                    // **Only the correct core should take the branch**
                    if (core_id == cid_val) {  
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                      << target_label << " at index " << new_pc << std::endl;
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;  // Invalidate instruction after branch
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        std::cout << "[Core " << core_id << "] Ignoring core-dependent branch.\n";
                    }
                }
            
                // **Handle normal BNE X, Y, label**
                else {
                   
                    std::cout << "[Core " << core_id << "] Checking BNE: " << rs1_val << " != " << rs2_val << std::endl;
            
                    if (rs1_val != rs2_val) {
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                      << target_label << " at index " << new_pc << std::endl;
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        std::cout << "[Core " << core_id << "] Branch not taken, continuing execution.\n";
                    }
                }
            }
            else if (instr == "beq") {
                std::string rs1_str = pipeline.EX_MEM.args[0];  // RS1 (could be "cid")
                std::string rs2_str = pipeline.EX_MEM.args[1];  // RS2
                std::string target_label = pipeline.EX_MEM.args[2];  // Branch label
            
                int rs1_val, rs2_val;
            
                // **Handle CID-dependent branch**
                if (rs1_str == "cid") {  
                    std::cout << "Instruction is core-dependent\n";
                    std::cout << "Checking branch at Core " << core_id << ", CID value: " << cid_val << std::endl;
            
                    // **Only the correct core should take the branch**
                    if (core_id == cid_val) {  
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                      << target_label << " at index " << new_pc << std::endl;
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;  // Invalidate instruction after branch
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        std::cout << "[Core " << core_id << "] Ignoring core-dependent branch.\n";
                    }
                }
            
                // **Handle normal BNE X, Y, label**
                else {
                   
                    std::cout << "[Core " << core_id << "] Checking BNE: " << rs1_val << " != " << rs2_val << std::endl;
            
                    if (rs1_val == rs2_val) {
                        if (label_map.find(target_label) != label_map.end()) {
                            int new_pc = label_map[target_label][core_id];
                            std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                      << target_label << " at index " << new_pc << std::endl;
                            global_pc = new_pc;
                            pipeline.ID_EX.valid_instruction = false;
                        } else {
                            std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        std::cout << "[Core " << core_id << "] Branch not taken, continuing execution.\n";
                    }
                }
            }           
            else if (instr == "blt") {
                cout<<"checking if "<< rs1_val << "<" << rs2_val <<endl;
                if (rs1_val < rs2_val) {
                    std::string target_label = pipeline.EX_MEM.args[2];
                    if (label_map.find(target_label) != label_map.end()) {
                        int new_pc = label_map[target_label][core_id];
                        std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                  << target_label << " at index " << new_pc << std::endl;
                        global_pc = new_pc ; // Adjust for fetch increment
                        pipeline.ID_EX.valid_instruction=false;

                    } else {
                        std::cerr << "[ERROR] Label " << target_label << " not found!\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    cout<<"Branch not taken contiinuing with: "<< pipeline.ID_EX.instruction << endl;
                }
            } 
            else if(instr== "j"){
                std::string target_label = pipeline.EX_MEM.args[0];
                if (label_map.find(target_label) != label_map.end()) {
                    int new_pc = label_map[target_label][core_id];
                    std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                              << target_label << " at index " << new_pc << std::endl;
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
                        std::cout << "[Core " << core_id << "] Branch Taken! Jumping to " 
                                  << target_label << " at index " << new_pc << std::endl;
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
                cout<<"rd value:"<<pipeline.EX_MEM.rd_value<<endl;         
            }else if (instr == "ecall") {
                if (registers[17] == 10) {
                    std::cout << "[Core " << core_id << "] Exit syscall detected. Terminating program.\n";
                    exit(0);
                }
            }
            else if(instr == "nop"){

            }
        latest_ex_result = pipeline.EX_MEM.rd_value;
        cout << "Latest_ex_rd_value:"<<pipeline.EX_MEM.rd_value<<endl;
        pipeline.EX_MEM.valid_data = true;
    }

    void memoryStage() {
        
        if (!pipeline.MEM_WB.valid_instruction ) return;
        if(pipeline.MEM_WB.stalled){
            pipeline.MEM_WB.stalled=false;
            pipeline.MEM_WB.valid_instruction=false;
            return ;
        }
        if(enable_forwarding==0){
            pipeline.MEM_WB.rs1_value= registers[pipeline.MEM_WB.rs1];
            if(pipeline.MEM_WB.instruction == "lw" ){
                pipeline.MEM_WB.address=pipeline.MEM_WB.rs1_value + pipeline.MEM_WB.offset;
            }
            if(pipeline.MEM_WB.instruction == "sw"){
                pipeline.MEM_WB.rd_value = registers[pipeline.MEM_WB.rd];
                pipeline.MEM_WB.address=pipeline.MEM_WB.rd_value + pipeline.MEM_WB.offset;
            }
            
        }
        std::cout << "[Core " << core_id << "] Memory stage for instruction: " << pipeline.MEM_WB.instruction << "\n";
            auto &instr = pipeline.MEM_WB.instruction;

            if (instr == "lw") {
                cout<< "address: " << pipeline.MEM_WB.address<< " offset: " << pipeline.MEM_WB.offset << endl;
                pipeline.MEM_WB.rd_value = lw(pipeline.MEM_WB.address,core_id);
            } else if (instr == "sw") {
                sw(pipeline.MEM_WB.address, pipeline.MEM_WB.rs1_value,core_id);
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

            if (instr == "add" || instr == "sub" || instr == "addi" || instr == "slli" || instr == "lw" || instr == "li" || instr == "la"|| instr=="mul"||instr == "and"||instr=="srli"||instr == "and"||instr=="or"||instr=="xor") {
                registers[getRegisterIndex(args[0])] = pipeline.WB_Return.rd_value;
                std::cout << "[Core " << core_id << "]  Write Back: " << instr 
                << " | x" << pipeline.WB_Return.rd 
                << " = " << pipeline.WB_Return.rd_value << "\n";   
            }
           

            std::cout << "[Core " << core_id << "] Write back for instruction: " << instr << "\n";
        pipeline.WB_Return.valid_data = true;
        number_instructions++;
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
