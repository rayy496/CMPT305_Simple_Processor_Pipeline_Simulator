#include "Simulator.h"
#include <cstdlib>
#include <cassert>
#include <iostream>

using std::exit;


PipelineSimulator::PipelineSimulator(const char* fp,
     int start, int count, int depth) : trace_data(fp, start, count)
{
    D = depth;
    inst_count = count;

    curr_cycle = 0;
    retire_count = 0;
    fetching_idx = 0;
    branch_stall = false;

    IF_slots = 2;
    ID_slots = 2;
    EX_slots = 2;
    MEM_slots = 2;
    WB_slots = 2;
    
    retire_int = 0;
    retire_fp = 0;
    retire_branch = 0;
    retire_load = 0;
    retire_store = 0;

    switch (D) {
        case 1:
            frequency = 1000000000;     // 1GHz = 1,000,000,000 Hz
            break;
        case 2:
            frequency = 1200000000;     // 1.2GHz
            break;
        case 3:
            frequency = 1700000000;     // 1.7GHz
            break;
        case 4:
            frequency = 1800000000;     // 1.8GHz
            break;
        default:
            std::cerr << "Invalid depth.\n";
            exit(EXIT_FAILURE);
    }
}

// Return true if find instruction with certain type in one stage, false otherwise
bool PipelineSimulator::FindInstType(int* stage_arr, int inst_type) const {
    for(int i = 0; i < 2; ++i) {
        int inst_idx = stage_arr[i];
        if(inst_idx == -1) {
            continue;
        }
        if(trace_data.GetInstType(inst_idx) == inst_type) {
            return true;
        }
    }
    return false;
}

// Update simulator state when upon retiring
void PipelineSimulator::UpdateRetire(int inst_type) {
    switch (inst_type) {
        case InstType::Int:
            retire_int++;
            break;
        case InstType::FP:
            retire_fp++;
            break;
        case InstType::Branch:
            retire_branch++;
            break;
        case InstType::Load:
            retire_load++;
            break;
        case InstType::Store:
            retire_store++;
            break;
        default:
            std::cerr << "Invalid instruction type.\n";
            exit(EXIT_FAILURE);
    }
    retire_count++;
}

// Plug instruction to available slot of destiny stage
// This function should only be called when there is empty slot
void PipelineSimulator::MoveToStage(int* stage_arr, int inst_idx) {
    if(stage_arr[0] == -1) {
        stage_arr[0] = inst_idx;
    }
    else if(stage_arr[1] == -1) {
        stage_arr[1] = inst_idx;
    }
    else {
        std::cerr << "available slots incorrect\n";
        exit(EXIT_FAILURE);
    }
}

// Tells which instruction enter the pipeline first
// (which is also reside at lower idx in trace data)
int PipelineSimulator::GivePriorInst(int* stage_arr) const {
    // if both slots empty, return -1
    if(stage_arr[0] == -1 && stage_arr[1] == -1) {
        return -1;
    }
    // if one slot empty, return another one
    if(stage_arr[0] == -1) {
        return 1;
    }
    else if(stage_arr[1] == -1) {
        return 0;
    }
    // if both slots occupied, return ther eariler one
    // i.e. I0 will be return rather than I1
    else {
        assert(stage_arr[0] != stage_arr[1]);
        return ((stage_arr[0] < stage_arr[1]) ? 0 : 1);
    }
}

// check if all dependences statisfied so that THIS instruction can proceed. Description as follow:
// A dependence on an integer or floating point instruction is satisfied after they finish the EX stage.
// A dependence on a load or store is satisfied after they finish the MEM stage.
bool PipelineSimulator::FreeFromDepend(int idx) const {
    for(int dependence_idx : trace_data.TraceDataAtIdx(idx).DependencesIdx) {
        const instruction& dependence = trace_data.TraceDataAtIdx(dependence_idx);
        if(dependence.InstructionType == InstType::Int || dependence.InstructionType == InstType::FP) {
            // if instruction stay in MEM or later stage, that means it done EX
            if(static_cast<int>(dependence.CurrentStage) <= static_cast<int>(StageType::EX)) {
                return false;
            }
        }
        else if(dependence.InstructionType == InstType::Load || dependence.InstructionType == InstType::Store) {
            if(static_cast<int>(dependence.CurrentStage) <= static_cast<int>(StageType::MEM)) {
                return false;
            }
        }
    }
    // if dependences are empty or all satisfied
    return true;
}

// -> IF
// Fetch new instruction from trace, stall if new instruction is branch,
// so that there should be only 1 branch without being execute in the pipeline.
void PipelineSimulator::FetchNewInst() {
    assert(IF_slots <= 2);
    // if all instruction fetched, waiting for remaining
    // instruction to retire, return right away
    if(fetching_idx >= trace_data.size()) {
        return;
    }
    for(int i = 0; !branch_stall && i < IF_slots; ++i) {
        if(IF[i] != -1) {
            continue;
        }
        IF[i] = fetching_idx;
        // if instruction type is branch, stall new fetch
        if(trace_data.TraceDataAtIdx(fetching_idx).InstructionType == InstType::Branch) {
            branch_stall = true;
        }
        trace_data.SetCurrentStage(fetching_idx, StageType::IF);
        fetching_idx++;
        assert(IF_slots > 0);
        IF_slots--;
    }
}

// IF -> ID
// Move instructions to next stage IN-ORDER if there is slot available.
void PipelineSimulator::ProcessIF() {
    assert(ID_slots <= 2);
    // if no ID slots available, return
    if(ID_slots == 0) {
        return;
    }

    while(ID_slots > 0) {
        int idx = GivePriorInst(IF);
        // if both IF slots empty, return
        if(idx == -1) {
            return;
        }
        MoveToStage(ID, IF[idx]);
        trace_data.SetCurrentStage(IF[idx], StageType::ID);
        IF[idx] = -1;

        IF_slots++;
        ID_slots--;
    }
}

// ID -> EX
// Move instructions to next stage IN-ORDER if there is slot available.
// Instruction must not proceed execute until all dependences satisfied.
// Check for all structural hazards below:
// An integer instruction cannot execute in the same cycle as another integer instruction.
// A floating point instruction cannot execute in the same cycle as another floating point instruction.
// A branch instruction cannot execute in the same cycle as another branch instruction.
void PipelineSimulator::ProcessID() {
    assert(EX_slots <= 2);
    if(EX_slots == 0) {
        return;
    }

    while(EX_slots > 0) {
        int idx = GivePriorInst(ID);
        if(idx == -1) {
            return;
        }
        // if the instruction with higher order cannot proceed to EX
        // due to its dependences, another instuction that has lower
        // order also need to be wait in ID until higher order inst done.
        if(!FreeFromDepend(ID[idx])) {
            return;
        }
        // if higher order instruction cannot proceed due to structural harzards
        if(trace_data.GetInstType(ID[idx]) == InstType::Int && FindInstType(EX, InstType::Int)) {
            return;
        }
        else if(trace_data.GetInstType(ID[idx]) == InstType::FP && FindInstType(EX, InstType::FP)) {
            return;
        }
        else if(trace_data.GetInstType(ID[idx]) == InstType::Branch && FindInstType(EX, InstType::Branch)) {
            return;
        }
        // if all dependences satisfied, and EX has room, proceed
        MoveToStage(EX, ID[idx]);
        trace_data.SetCurrentStage(ID[idx], StageType::EX);
        ID[idx] = -1;

        ID_slots++;
        EX_slots--;
    }
}

// EX -> MEM
// Move instructions to next stage IN-ORDER if there is slot available.
// Check for all structural hazards below:
// A load instruction cannot go to the MEM stage in the same cycle as another load instruction.
// A store instruction cannot go to the MEM stage in the same cycle as another store instruction.
// When D == 2 or 4, floating point instructions spend 2 cycles in the EX stage. IN-ORDER execution needed,
// i.e. if I0 == float, I1 = others, even if I1 done, I1 still need to wait until I0 done and leave first
void PipelineSimulator::ProcessEX() {
    assert(MEM_slots <= 2);
    if(D == 2 || D == 4) {
        for(int i = 0; i < 2; ++i) {
            if(EX[i] == -1) {
                continue;
            }
            else if(trace_data.GetInstType(EX[i]) == InstType::FP) {
                // Increment the cycle time that FP type instruction stayed in EX stage
                // If cycle time == 2, then it is ready to leave and proceed to MEM stage
                if(trace_data.TraceDataAtIdx(EX[i]).FP_cycle_in_stage < 2) {
                    trace_data.IncrementFP(EX[i]);
                }
            }
        }
    }
    if(MEM_slots == 0) {
        return;
    }
    while(MEM_slots > 0) {
        int idx = GivePriorInst(EX);
        if(idx == -1) {
            return;
        }
        // If FP cycle time of this instruction is not 0, that means it is float type and D == 2 or 4 according to above code
        // In addition, if FP cycle time is less than 2, that means it has not finish spending 2 cycle in FP stage yet.
        if(trace_data.TraceDataAtIdx(EX[idx]).FP_cycle_in_stage > 0 && trace_data.TraceDataAtIdx(EX[idx]).FP_cycle_in_stage < 2) {
            return;
        }
        // Since we check FreeFromDependences in ID -> EX, so it must satisfied and no need to check
        // check for structural harzards
        if(trace_data.GetInstType(EX[idx]) == InstType::Load && FindInstType(MEM, InstType::Load)) {
            return;
        }
        else if(trace_data.GetInstType(EX[idx]) == InstType::Store && FindInstType(MEM, InstType::Store)) {
            return;
        }
        // proceed if all dependences satisfied
        MoveToStage(MEM, EX[idx]);
        if(trace_data.GetInstType(EX[idx]) == InstType::Branch) {
            branch_stall = false;
        }
        trace_data.SetCurrentStage(EX[idx], StageType::MEM);
        EX[idx] = -1;

        EX_slots++;
        MEM_slots--;
    }
}

// MEM -> WB
// When D == 3 or 4, Load instructions spend 3 cycles in the MEM stage. IN-ORDER memory access needed
// i.e. same as EX -> MEM
void PipelineSimulator::ProcessMEM() {
    assert(WB_slots <= 2);
    if(D == 3 || D == 4) {
        for(int i = 0; i < 2; ++i) {
            if(MEM[i] == -1) {
                continue;
            }
            else if(trace_data.GetInstType(MEM[i]) == InstType::Load) {
                if(trace_data.TraceDataAtIdx(MEM[i]).Load_cycle_in_stage < 3) {
                    trace_data.IncrementLoad(MEM[i]);                    
                }
            }
        }
    }
    if(WB_slots == 0) {
        return;
    }
    while(WB_slots > 0) {
        int idx = GivePriorInst(MEM);
        if(idx == -1) {
            return;
        }
        if(trace_data.TraceDataAtIdx(MEM[idx]).Load_cycle_in_stage > 0 && trace_data.TraceDataAtIdx(MEM[idx]).Load_cycle_in_stage < 3) {
            return;
        }
        MoveToStage(WB, MEM[idx]);
        trace_data.SetCurrentStage(MEM[idx], StageType::WB);
        MEM[idx] = -1;

        MEM_slots++;
        WB_slots--;
    }
}

// WB ->
// Retire
void PipelineSimulator::ProcessWB() {
    assert(WB_slots <= 2);
    while(WB_slots < 2) {
        int idx = GivePriorInst(WB);
        if(idx == -1) {
            std::cerr << "available slots incorrect\n";
            exit(EXIT_FAILURE);
        }
        UpdateRetire(trace_data.GetInstType(WB[idx]));
        trace_data.SetCurrentStage(idx, StageType::retired);
        WB[idx] = -1;

        WB_slots++;
    }
}

void PipelineSimulator::RunSimulation() {
    while (retire_count < inst_count) {
        ProcessWB();
        ProcessMEM();
        ProcessEX();
        ProcessID();
        ProcessIF();
        FetchNewInst();

        curr_cycle++;
    }

    OutputStatistics();
}

void PipelineSimulator::OutputStatistics() const {
    printf("Simulation clock: %ld cycles.\n", curr_cycle);
    double exec_time = (static_cast<double>(curr_cycle) / static_cast<double>(frequency)) * 1000.0;
    printf("Total execution time: %.4f miliseconds.\n", exec_time);
    printf("Total num of instructions: %d\n", retire_count);
    printf("Total num of integer instructions: %d\n", retire_int);
    printf("Total num of floating point instructions: %d\n", retire_fp);
    printf("Total num of branch instructions: %d\n", retire_branch);
    printf("Total num of load instructions: %d\n", retire_load);
    printf("Total num of store instructions: %d\n", retire_store);
}