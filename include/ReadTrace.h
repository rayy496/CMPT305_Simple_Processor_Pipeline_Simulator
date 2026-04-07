#pragma once

#include "Instruction.h"


class Trace {
    private:
        vector<instruction> TraceData;
        void FillTraceData(const char* filepath, int start_inst, int inst_count);

    public:
        Trace(const char* filepath, int start_inst, int inst_count);
        const instruction& TraceDataAtIdx(int idx) const;
        int size() const;
        int GetInstType(int idx) const;
        void IncrementFP(int idx);
        void IncrementLoad(int idx);
        void SetCurrentStage(int idx, StageType stype);
};