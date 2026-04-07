#pragma once

#include <vector>
#include <cstdint>

using std::vector;


enum class StageType {
    before_fetch = 0,
    IF = 1,
    ID = 2,
    EX = 3,
    MEM = 4,
    WB = 5,
    retired = 6
};

namespace InstType {
    constexpr int Int = 1;
    constexpr int FP = 2;
    constexpr int Branch = 3;
    constexpr int Load = 4;
    constexpr int Store = 5;
}

struct instruction {
    uint64_t ProgramCounter;        // PC of THIS instruction (in hex)
    int InstructionType;            // value between 1 to 5, refer to namespace InstType
    vector<uint64_t> Dependences;   // list of (static) PCs value of instructions that THIS depends on
    vector<int> DependencesIdx;     // index of "last instance" of those (dynamic) instructions THIS depends on

    int FP_cycle_in_stage = 0;      // number of cycles that THIS instruction stayed in current stage
    int Load_cycle_in_stage = 0;    // Use for counting EX for float and MEM for load when D is not 1,
                                    // for comman case, keep it unchange for simplicity
                                    // For stage FP when D == 2, increment 1 by each cycle until 2
                                    // For stage Load when D == 3, increment 1 by each cycle until 3
                                    // Apply both when D == 4

    StageType CurrentStage = StageType::before_fetch;       // keep track of the state of THIS instruction
                                                            // starting from before_fetch before entering
                                                            // the pipeline and change to retired after
                                                            // leaving the pipeline.
};