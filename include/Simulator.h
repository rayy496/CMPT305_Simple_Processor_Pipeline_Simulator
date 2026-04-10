#pragma once

#include "ReadTrace.h"
#include <cstdbool>


class PipelineSimulator {
    private:
        Trace trace_data;
        int D;
        int inst_count;
        uint64_t frequency;

        //----------Instruction pipeline slot----------
        // -1 for empty/stall, otherwise fill in
        // with instructions index from trace data
        int IF[2] = {-1, -1};
        int ID[2] = {-1, -1};
        int EX[2] = {-1, -1};
        int MEM[2] = {-1, -1};
        int WB[2] = {-1, -1};

        //----------Simulator state----------
        uint64_t curr_cycle;
        int retire_count;
        int fetching_idx;           // index of next instruction to be fetch from trace data
        bool branch_stall;          // whether new fetch is stalled by a branch in the pipeline
        bool branch_finished_ex;    // branch finished EX this cycle; release stall next cycle
        // number of available slot for each stage, maximum 2
        int IF_slots;
        int ID_slots;
        int EX_slots;
        int MEM_slots;
        int WB_slots;

        //----------Retired instructions count----------
        int retire_int;
        int retire_fp;
        int retire_branch;
        int retire_load;
        int retire_store;

        //----------Simulation process----------
        // process the instruction in corresponding slot,
        // fetch new, move to next stage, or retire from pipeline
        void FetchNewInst();
        void ProcessIF();
        void ProcessID();
        void ProcessEX();
        void ProcessMEM();
        void ProcessWB();

        //----------Other helpers----------
        bool FreeFromDepend(int idx) const;
        int GivePriorInst(int* stage_arr) const;
        void MoveToStage(int* stage_arr, int idx);
        void UpdateRetire(int inst_type);
        bool FindInstType(int* stage_arr, int inst_type) const;

    public:
        PipelineSimulator(const char* trace_filepath, int start_inst, int inst_count, int depth);
        void RunSimulation();
        void OutputStatistics() const;
};