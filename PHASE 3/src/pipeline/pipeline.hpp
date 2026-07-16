#pragma once
#include "pipeline_stage.hpp"

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
    int forwarding =0;

    void shiftStages();
};
