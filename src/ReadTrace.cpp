#include "ReadTrace.h"
#include <fstream>
#include <cassert>
#include <string>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <cassert>

using std::string;
using std::getline;
using std::cerr;
using std::stringstream;
using std::vector;


Trace::Trace(const char* filepath, int start_inst, int inst_count) {
    FillTraceData(filepath, start_inst, inst_count);
}

void Trace::FillTraceData(const char* fp, int start, int count) {
    std::ifstream trace_input(fp);
    
    // error checking
    if(!trace_input.is_open()) {
        cerr << "Error reading file\n";
        return;
    }
    TraceData.clear();
    TraceData.reserve(count);
    
    // initialize dictionary of <key, value>
    // where key is the static PC of instruction,
    // value is the index of most recent instruction
    // with the key-PC in TraceData.
    std::unordered_map<uint64_t, int> last_instances;
    last_instances.reserve(count);

    string line;
    for(int i = 0; i < start - 1; ++i) {
        if(!getline(trace_input, line)) {
            cerr << "Start exceeds trace length\n";
            return;
        }
    }

    // read instructions
    for(int i = 0; i < count; ++i) {
        if(!getline(trace_input, line)) {
            cerr << "Trace ended before reading count instructions\n";
            break;
        }
        if(!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        stringstream ss(line);
        string token;
        instruction inst;
        if(!getline(ss, token, ',')) {
            cerr << "Malformed line: missing PC\n";
            continue;
        }
        inst.ProgramCounter = std::stoull(token, nullptr, 16);  // uint64, hex
        if(!getline(ss, token, ',')) {
            cerr << "Malformed line: missing instruction type\n";
            continue;
        }
        inst.InstructionType = std::stoi(token);    // int, 1 to 5

        // possible dependences
        while(getline(ss, token, ',')) {
            if(token.empty()) {
                continue;
            }
            uint64_t dependence_PC = std::stoull(token, nullptr, 16);
            inst.Dependences.push_back(dependence_PC);

            auto itr = last_instances.find(dependence_PC);
            if(itr == last_instances.end()) {
                // not found in current replication, ignore
                continue;
            }
            else {
                inst.DependencesIdx.push_back(itr->second);
            }
        }

        TraceData.push_back(std::move(inst));
        // link the PC and index, store/update to the unordered_map
        int idx = static_cast<int>(TraceData.size()) - 1;
        last_instances[TraceData[idx].ProgramCounter] = idx;
    }

    assert(static_cast<int>(TraceData.size()) == count);
    trace_input.close();
}

const instruction& Trace::TraceDataAtIdx(int idx) const {
    assert(idx >= 0 && idx < static_cast<int>(TraceData.size()));
    return TraceData.at(idx);
}

int Trace::size() const {
    return static_cast<int>(TraceData.size());
}

void Trace::IncrementFP(int idx) {
    assert(TraceData.at(idx).FP_cycle_in_stage < 2);
    TraceData.at(idx).FP_cycle_in_stage++;
}

void Trace::IncrementLoad(int idx) {
    assert(TraceData.at(idx).Load_cycle_in_stage < 3);
    TraceData.at(idx).Load_cycle_in_stage++;
}

void Trace::SetCurrentStage(int idx, StageType stype) {
    TraceData.at(idx).CurrentStage = stype;
}

int Trace::GetInstType(int idx) const {
    return TraceData.at(idx).InstructionType;
}