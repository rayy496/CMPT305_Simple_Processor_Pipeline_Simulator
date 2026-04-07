#include "Simulator.h"
#include <cstdio>
#include <iostream>
#include <cstdlib>

using std::cout;


int main(int argc, char *argv[]) {
    if(argc < 5) {
        cout << "Insufficient number of arguments provided.\n";
        return 1;
    }
    char* trace_filepath = argv[1];
    int start_inst = atoi(argv[2]);
    int inst_count = atoi(argv[3]);
    int pipeline_depth = atoi(argv[4]);
    
    if(pipeline_depth < 1 || pipeline_depth > 4) {
        cout << "Invalid pipeline depth configuration.\n";
        return 1;
    }

    PipelineSimulator ps = PipelineSimulator(trace_filepath, start_inst, inst_count, pipeline_depth);
    ps.RunSimulation();

    return 0;
}